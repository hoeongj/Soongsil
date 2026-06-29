from urllib.parse import quote
import threading
import time
import logging
import sys

# Vulnerable server configuration
HOST = '127.0.0.1'
PORT = 8888

sys.stdout.reconfigure(encoding='utf-8')


# TODO: Implement the XSS URL generator.
#
# - Optionally include a minimal vulnerable web server
#   for demonstration.
# - Generate a URL that:
#     • Targets a vulnerable endpoint's query parameter
#     • Embeds a harmless <script>alert('XSS')</script>
#     • Displays visible, innocent-looking text
# - Print the URL and an explanation of why the attack
#   works and how to prevent it.
#
# Hint: Reference Chapter 11 for XSS attack vectors.


# --- Vulnerable Flask endpoint ------------------------------------------------
# This server is intentionally insecure: it reflects the 'name' query parameter
# directly into an HTML response without any escaping.  That is the exact
# behaviour that makes reflected (non-persistent) XSS possible.

try:
    from flask import Flask, request as flask_request
    _FLASK_AVAILABLE = True
except ImportError:
    _FLASK_AVAILABLE = False

if _FLASK_AVAILABLE:
    vuln_app = Flask(__name__)

    @vuln_app.route('/greet')
    def greet():
        # VULNERABILITY: user-controlled input is embedded raw into HTML.
        # Any <script> tag the attacker places in 'name' will be executed
        # by the victim's browser when the page loads.
        name = flask_request.args.get('name', 'Guest')
        html = (
            "<!DOCTYPE html>\n"
            "<html>\n"
            "<head><meta charset=\"UTF-8\"><title>Welcome</title></head>\n"
            "<body>\n"
            f"  <h2>Hello, {name}!</h2>\n"
            "  <p>Thanks for visiting our page.</p>\n"
            "</body>\n"
            "</html>"
        )
        return html, 200, {'Content-Type': 'text/html; charset=utf-8'}

    def _run_server():
        """Start the vulnerable Flask server; suppress Werkzeug request logs."""
        log = logging.getLogger('werkzeug')
        log.setLevel(logging.ERROR)
        vuln_app.run(host=HOST, port=PORT, debug=False, use_reloader=False)


def build_xss_url() -> str:
    """
    Construct a URL that embeds a reflected XSS payload in the 'name' parameter.

    The visible text ("Welcome Back") is placed before the payload so that a
    hover tooltip or link preview shows something innocuous; the injected
    script tag is only revealed after the browser URL-decodes the full string.
    """
    visible_text = "Welcome Back"
    # Harmless alert() payload - demonstrates script execution without damage.
    payload  = "<script>alert('XSS')</script>"
    combined = visible_text + payload

    # quote() percent-encodes <, >, /, spaces, and quotes so the string is
    # safe to embed in a URL while remaining executable when decoded.
    encoded = quote(combined, safe='')
    return f"http://{HOST}:{PORT}/greet?name={encoded}"


def main() -> None:
    # Launch the vulnerable server as a daemon thread so the script stays
    # alive while the URL can be opened and tested in a browser.
    if _FLASK_AVAILABLE:
        t = threading.Thread(target=_run_server, daemon=True)
        t.start()
        # Wait briefly for Flask to bind the port before printing the URL.
        time.sleep(1)
        server_note = f"Vulnerable server is running at http://{HOST}:{PORT}"
    else:
        server_note = (
            "Flask is not installed; the server could not be started.\n"
            "Install it with: pip install flask"
        )

    xss_url = build_xss_url()

    print("=" * 60)
    print("Generated XSS URL")
    print("=" * 60)
    print(xss_url)
    print()
    print("=" * 60)
    print("Explanation")
    print("=" * 60)
    print(
        "Attack mechanism:\n"
        "  The /greet endpoint inserts the 'name' query parameter directly\n"
        "  into the HTML body without sanitization. When a victim clicks\n"
        "  this link, the browser URL-decodes the value, sees a <script>\n"
        "  tag, and executes it - this is reflected (non-persistent) XSS.\n"
        "\n"
        "Why the link looks legitimate:\n"
        "  The encoded value starts with 'Welcome%20Back', so a hover\n"
        "  tooltip or link preview shows innocent text. The injected script\n"
        "  tag only becomes visible after the browser decodes the full\n"
        "  query string.\n"
        "\n"
        "Prevention techniques:\n"
        "  1. Escape output - use html.escape() or a template engine with\n"
        "     auto-escaping (e.g. Jinja2 {{ value }}) before inserting any\n"
        "     user-controlled value into HTML.\n"
        "  2. Content Security Policy - add the response header:\n"
        "       Content-Security-Policy: script-src 'self'\n"
        "     to block inline scripts even if they are injected.\n"
        "  3. Input validation - reject or strip angle brackets on the\n"
        "     server before the value reaches any template or response.\n"
    )
    print(server_note)

    if _FLASK_AVAILABLE:
        print("\nOpen the URL above in a browser, then press Ctrl+C to quit.")
        try:
            # Keep the main thread alive so the daemon server stays up.
            while True:
                time.sleep(1)
        except KeyboardInterrupt:
            print("\nShutting down.")


if __name__ == "__main__":
    main()
