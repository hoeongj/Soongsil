from flask import Flask, request, jsonify
import csv
import os
import re
import requests
from bs4 import BeautifulSoup

app = Flask(__name__)

PUBLICATIONS_CSV = 'publications.csv'
SCRAPE_URL = 'https://sites.google.com/view/davidchoi/home/publications'


# TODO: Implement the Flask web application.
#
# - Write a scrape function that extracts publication data
#   from SCRAPE_URL and saves them to PUBLICATIONS_CSV.
# - Call the scraper when this script starts.
# - Load PUBLICATIONS_CSV into memory.
# - Serve GET / with an HTML search page.
# - Serve GET /search?q=<query> returning matching
#   publications as JSON.
#
# Hint: Reference Chapter 10 for Flask and web scraping
#       patterns.


CSV_FIELDS = ['research_area', 'pub_type', 'title', 'authors', 'venue', 'year']

# In-memory publication records; populated at startup from the scraped CSV.
publications = []

# Research area keywords used to identify section headings on the page.
AREA_KEYWORDS = [
    'internet of things', 'distributed intelligent', 'federated learning',
    'machine learning', 'deep learning', 'vehicular', 'smart grid',
    'wireless', 'edge computing', 'cloud computing', 'energy grid',
    'cybersecurity', 'data analytics', 'business intelligence',
]


def scrape_publications(url: str) -> list:
    """
    Fetch the DILab publications page and extract structured records.

    Strategy: strip all non-visible tags, dump the page to plain text with
    soup.get_text(), then scan line by line for area headings,
    Journals/Conferences sub-headings, and publication entries.  This avoids
    the HTML-nesting issues that caused the element-traversal approach to skip
    headings whose immediate children happened to be block-level tags.

    Falls back to scanning script-tag JSON blobs if the text scan finds nothing
    (handles fully client-side-rendered Google Sites pages).

    Returns a list of dicts with keys: research_area, pub_type, title, authors,
    venue, year.  Returns an empty list on any network or parse failure.
    """
    records = []
    headers = {
        'User-Agent': (
            'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 '
            '(KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36'
        )
    }

    # Fetch the page; handle each failure type with a clear message.
    try:
        resp = requests.get(url, headers=headers, timeout=15)
        resp.raise_for_status()
    except requests.exceptions.ConnectionError:
        print("Scraper: network error - cannot reach the publications page.")
        return records
    except requests.exceptions.Timeout:
        print("Scraper: request timed out after 15 s.")
        return records
    except requests.exceptions.HTTPError as e:
        print(f"Scraper: HTTP error - {e}")
        return records
    except requests.exceptions.RequestException as e:
        print(f"Scraper: unexpected request error - {e}")
        return records

    soup = BeautifulSoup(resp.text, 'html.parser')

    # Save script content now for the fallback before we remove script tags.
    script_texts = [s.string or '' for s in soup.find_all('script')]

    # Remove non-visible tags so their text does not pollute get_text() output.
    for tag in soup(['script', 'style', 'meta', 'link', 'noscript', 'head']):
        tag.decompose()

    # Strategy 1: plain-text line scan.
    # get_text() with separator='\n' gives one logical chunk per element, which
    # maps naturally to the page's visual structure regardless of HTML nesting.
    raw_text = soup.get_text(separator='\n')
    lines    = [ln.strip() for ln in raw_text.split('\n') if ln.strip()]

    current_area = 'NA'
    current_type = 'NA'
    entry_buf    = []

    def flush():
        """Flush buffered lines as one publication record.

        The page uses a consistent 3-part structure per publication:
          1. Authors line  — ends with   ,"  (opens the title)
          2. Title line(s) — free text
          3. Venue line    — starts with  ,"  (year embedded at the end)
        """
        nonlocal entry_buf
        if not entry_buf:
            return
        buf       = entry_buf[:]
        entry_buf = []
        if not buf:
            return

        # Separate the venue/year line from the authors+title lines.
        venue_line   = ''
        content_lines = []
        # Quote characters used by the page (typographic and ASCII).
        QUOTES = (34, 0x201C, 0x201D)

        for l in buf:
            # Venue line: starts with comma then any quote character.
            if len(l) > 1 and l[0] == ',' and ord(l[1]) in QUOTES:
                venue_line = l[1:].strip().strip('"').strip('“').strip('”')
            else:
                content_lines.append(l)

        if not content_lines:
            return

        # The first content line is the authors line.
        # It ends with  , "  (comma space opening-quote, either ASCII or U+201C)
        # which introduces the paper title on the next line.
        first = content_lines[0]
        last_char = first.rstrip()[-1:] if first.rstrip() else ''
        is_authors = (last_char and ord(last_char) in QUOTES
                      and not (len(first) > 1 and first[0] == ','
                               and ord(first[1]) in QUOTES))
        if is_authors:
            authors = first.rstrip().rstrip('"').rstrip('"').rstrip('"').rstrip(', ').strip()
            title   = ' '.join(content_lines[1:]).strip() if len(content_lines) > 1 else 'NA'
        else:
            authors = 'NA'
            title   = ' '.join(content_lines).strip()[:200]

        if not title or title == 'NA':
            title = ' '.join(content_lines).strip()[:200]

        # Year from the venue line.
        search_text = venue_line or ' '.join(buf)
        year_m      = re.search(r'\b(19|20)\d{2}\b', search_text)
        year        = year_m.group(0) if year_m else 'NA'

        # Venue: look for common academic publisher / series patterns.
        venue_m = re.search(
            r'(IEEE[^,\.]{0,80}|ACM[^,\.]{0,80}|[A-Z][A-Za-z\s]+'
            r'(?:Journal|Transactions|Letters|Magazine|Conference|Symposium'
            r'|Workshop|Proceedings|Sensors|Electronics|Access)[^,\.]{0,50})',
            venue_line or search_text
        )
        venue = venue_m.group(1).strip() if venue_m else 'NA'

        if len(title) < 5:
            return

        records.append({
            'research_area': current_area,
            'pub_type':      current_type,
            'title':         title[:300],
            'authors':       authors[:250],
            'venue':         venue[:200],
            'year':          year,
        })

    for line in lines:
        lower = line.lower()

        # Research area heading — two formats appear on the page:
        #   (a) Numbered:  "1. Internet of Things:"  or  "3 . Federated Learning:"
        #       (a space before the period sometimes appears)
        #   (b) Plain:     "Federated Learning:"
        #       (no leading number; line is short and matches a known keyword
        #        exactly after stripping the trailing colon/space)
        # Leading parenthetical award notes like "(Best Paper Award Finalist)"
        # are stripped before the number check.
        stripped    = re.sub(r'^\([^)]+\)\s*', '', line)
        clean_lower = stripped.lower().rstrip(': ')
        is_numbered = (bool(re.match(r'^\d+\s*\.', stripped))
                       and any(kw in lower for kw in AREA_KEYWORDS)
                       and len(line) < 150)
        is_plain    = (clean_lower in AREA_KEYWORDS and len(line) < 60)
        if is_numbered or is_plain:
            flush()
            # Store just the area name (drop leading number+period and colon).
            current_area = re.sub(r'^\d+\s*\.\s*', '', stripped).rstrip(':').strip()
            current_type = 'NA'
            continue

        # Journals / Conferences sub-heading.
        # Strip both periods AND colons because the page writes "Journals:"
        clean = lower.strip('.: ')
        if clean in ('journals', 'journal'):
            flush()
            current_type = 'Journal'
            continue
        if clean in ('conferences', 'conference', 'proceedings'):
            flush()
            current_type = 'Conference'
            continue

        # Accumulate publication body text under the current section type.
        if current_type != 'NA':
            entry_buf.append(line)
            # The venue/year line starts with , followed by a quote character.
            # The page uses the typographic right double quote U+201D (8221),
            # not plain ASCII “ (34).  Check both to be safe.
            if (len(line) > 1 and line[0] == ','
                    and ord(line[1]) in (34, 0x201C, 0x201D)):
                flush()

    flush()

    # Strategy 2: Scan script-tag JSON blobs saved before decompose().
    # Google Sites buries rendered content in AF_initDataCallback payloads.
    if not records:
        print("Scraper: text scan found nothing; scanning script tags...")
        for src in script_texts:
            candidates = re.findall(
                r'"([^"]{40,400}(?:IEEE|ACM|Journal|Conference|Sensors|'
                r'Electronics|Wireless|Energy|IoT|Federated)[^"]{0,200})"',
                src
            )
            for c in candidates:
                year_m = re.search(r'\b(19|20)\d{2}\b', c)
                records.append({
                    'research_area': 'NA',
                    'pub_type':      'NA',
                    'title':         c[:300],
                    'authors':       'NA',
                    'venue':         'NA',
                    'year':          year_m.group(0) if year_m else 'NA',
                })
            if records:
                print(f"Scraper: found {len(records)} record(s) in script JSON.")
                break

    print(f"Scraper: total records extracted - {len(records)}")
    return records


def save_csv(records: list, filepath: str) -> None:
    """Write publication records to a CSV file, overwriting any existing one."""
    try:
        with open(filepath, 'w', newline='', encoding='utf-8') as f:
            writer = csv.DictWriter(f, fieldnames=CSV_FIELDS)
            writer.writeheader()
            writer.writerows(records)
        print(f"Scraper: saved {len(records)} record(s) to '{filepath}'.")
    except OSError as e:
        print(f"Scraper: could not write '{filepath}' - {e}")


def load_csv(filepath: str) -> list:
    """Load publication records from CSV into a list of dicts."""
    rows = []
    try:
        with open(filepath, newline='', encoding='utf-8') as f:
            reader = csv.DictReader(f)
            for row in reader:
                rows.append(dict(row))
        print(f"Loaded {len(rows)} publication(s) from '{filepath}'.")
    except FileNotFoundError:
        print(f"'{filepath}' not found - starting with an empty list.")
    except Exception as e:
        print(f"Error reading '{filepath}': {e}")
    return rows


@app.route('/')
def index():
    """Serve the publication search page with a text input and search button."""
    html = """<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>DILab Publication Search</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      max-width: 860px;
      margin: 48px auto;
      padding: 0 24px;
      color: #222;
    }
    h1 { margin-bottom: 24px; }
    .search-bar { display: flex; gap: 8px; }
    input[type=text] {
      flex: 1;
      padding: 10px 14px;
      font-size: 1rem;
      border: 1px solid #ccc;
      border-radius: 4px;
    }
    button {
      padding: 10px 20px;
      font-size: 1rem;
      background: #1a73e8;
      color: #fff;
      border: none;
      border-radius: 4px;
      cursor: pointer;
    }
    button:hover { background: #1558b0; }
    #results {
      margin-top: 28px;
      white-space: pre-wrap;
      font-family: monospace;
      font-size: 0.9rem;
      background: #f8f8f8;
      padding: 16px;
      border-radius: 4px;
      border: 1px solid #ddd;
    }
  </style>
</head>
<body>
  <h1>DILab Publication Search</h1>
  <div class="search-bar">
    <input type="text" id="query"
           placeholder="Keyword, author, research area..." autofocus>
    <button onclick="doSearch()">Search</button>
  </div>
  <div id="results"></div>
  <script>
    document.getElementById('query').addEventListener('keydown', function(e) {
      if (e.key === 'Enter') doSearch();
    });
    async function doSearch() {
      const q = document.getElementById('query').value.trim();
      if (!q) return;
      const res  = await fetch('/search?q=' + encodeURIComponent(q));
      const data = await res.json();
      document.getElementById('results').textContent =
        JSON.stringify(data, null, 2);
    }
  </script>
</body>
</html>"""
    return html


@app.route('/search')
def search():
    """Return all publications whose title or authors contain the query (JSON)."""
    query = request.args.get('q', '').strip().lower()

    if not query:
        return jsonify({'error': 'No search query provided.'}), 400

    matches = [
        p for p in publications
        if query in p.get('title', '').lower()
        or query in p.get('authors', '').lower()
    ]

    if not matches:
        return jsonify({'message': f"No publications found for '{query}'."}), 200

    return jsonify(matches), 200


# Startup: scrape -> save -> load
# WERKZEUG_RUN_MAIN prevents the scraper from running a second time when
# Flask's reloader forks a child process; the child inherits the saved CSV.
if os.environ.get('WERKZEUG_RUN_MAIN') != 'true':
    print("Scraping publications from DILab website...")
    _scraped = scrape_publications(SCRAPE_URL)
    if _scraped:
        save_csv(_scraped, PUBLICATIONS_CSV)
    else:
        print("Scraper returned no records; loading existing CSV if present.")

publications = load_csv(PUBLICATIONS_CSV)

if __name__ == '__main__':
    app.run(host='127.0.0.1', port=5000, debug=True)
