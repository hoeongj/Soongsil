import socket
import csv

# Server configuration
HOST = '127.0.0.1'
PORT = 8080
KEYWORDS_FILE = 'keywords.csv'


# TODO: Implement the HTTP dictionary lookup server.
#
# - Load keywords and definitions from KEYWORDS_FILE.
# - Listen for HTTP connections on HOST:PORT.
# - Parse incoming GET requests, extract the keyword from
#   the query string, and respond with the definition
#   (HTTP 200) or a not-found message (HTTP 404).
# - Construct raw HTTP/1.1 responses manually.
# - Handle multiple sequential requests.
#
# Hint: Reference Chapter 9 for HTTP request/response
#       structure.


def load_keywords(filepath: str) -> dict:
    """Load keyword-definition pairs from a CSV into a case-insensitive dict."""
    table = {}
    try:
        with open(filepath, newline='', encoding='utf-8') as f:
            reader = csv.DictReader(f)
            for row in reader:
                key = row['keyword'].strip().lower()
                table[key] = row['definition'].strip()
    except FileNotFoundError:
        print(f"Error: '{filepath}' not found.")
    except KeyError as e:
        print(f"CSV is missing expected column: {e}")
    return table


def make_response(status_code: int, status_text: str, body: str) -> bytes:
    """Assemble a raw HTTP/1.1 response with status line, headers, and body."""
    body_bytes = body.encode('utf-8')
    # Content-Length is required so the client knows when the body ends.
    headers = (
        f"HTTP/1.1 {status_code} {status_text}\r\n"
        f"Content-Type: text/plain; charset=utf-8\r\n"
        f"Content-Length: {len(body_bytes)}\r\n"
        f"Connection: close\r\n"
        f"\r\n"
    )
    return headers.encode('utf-8') + body_bytes


def url_decode(s: str) -> str:
    """Decode %XX percent-encoded sequences using only built-in string ops."""
    result = []
    i = 0
    s = s.replace('+', ' ')
    while i < len(s):
        # A valid percent-sequence is % followed by exactly two hex digits.
        if s[i] == '%' and i + 2 < len(s):
            try:
                result.append(chr(int(s[i + 1:i + 3], 16)))
                i += 3
            except ValueError:
                result.append(s[i])
                i += 1
        else:
            result.append(s[i])
            i += 1
    return ''.join(result)


def handle_request(conn: socket.socket, keywords: dict) -> None:
    """
    Read one HTTP request from conn, look up the keyword, and send a response.

    Expected request format: GET /define?keyword=<word> HTTP/1.1
    """
    try:
        # Accumulate bytes until the blank line that ends HTTP headers.
        raw = b''
        while b'\r\n\r\n' not in raw:
            chunk = conn.recv(4096)
            if not chunk:
                return          # client disconnected before sending a request
            raw += chunk

        # Parse only the request line - headers are not needed for this service.
        request_line = raw.decode('utf-8', errors='replace').split('\r\n')[0]
        parts = request_line.split(' ')
        if len(parts) < 2:
            conn.sendall(make_response(400, 'Bad Request', 'Malformed request line.'))
            return

        method = parts[0]
        path   = parts[1]   # e.g. /define?keyword=HTTP

        if method != 'GET':
            conn.sendall(make_response(405, 'Method Not Allowed', 'Only GET is supported.'))
            return

        # Extract the keyword value from the query string.
        keyword = ''
        if '?' in path:
            query_string = path.split('?', 1)[1]
            for param in query_string.split('&'):
                if param.startswith('keyword='):
                    keyword = url_decode(param[len('keyword='):])
                    break

        if not keyword:
            conn.sendall(make_response(400, 'Bad Request', 'Missing keyword parameter.'))
            return

        # Case-insensitive dictionary lookup.
        definition = keywords.get(keyword.lower())
        if definition:
            print(f"[GET] /define?keyword={keyword} -> 200 OK")
            conn.sendall(make_response(200, 'OK', f"Definition: {definition}"))
        else:
            print(f"[GET] /define?keyword={keyword} -> 404 Not Found")
            conn.sendall(make_response(404, 'Not Found', 'Error 404: Keyword not found.'))

    except Exception as e:
        print(f"Error handling request: {e}")


def main() -> None:
    keywords = load_keywords(KEYWORDS_FILE)
    if not keywords:
        print("Warning: keyword table is empty. Check keywords.csv.")

    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # Avoid "Address already in use" when restarting the server quickly.
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        srv.bind((HOST, PORT))
        srv.listen(5)
        print(f"HTTP Dictionary Server listening on {HOST}:{PORT}")

        # Serve one request at a time, sequentially.
        while True:
            try:
                conn, addr = srv.accept()
            except KeyboardInterrupt:
                print("\nServer shutting down (Ctrl+C).")
                break
            try:
                handle_request(conn, keywords)
            finally:
                conn.close()

    except OSError as e:
        print(f"Server socket error: {e}")
    finally:
        srv.close()
        print("Server socket closed.")


if __name__ == "__main__":
    main()
