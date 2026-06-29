import socket

# Client configuration — must match server
HOST = '127.0.0.1'
PORT = 8080


# TODO: Implement the HTTP dictionary lookup client.
#
# - Prompt the user for a keyword.
# - Send a raw HTTP GET request to the server using only
#   the socket module.
# - Parse and display the response body.
# - Allow repeated lookups until the user types "quit".
#
# Hint: Reference Chapter 9 for HTTP request formatting.


def url_encode(s: str) -> str:
    """Percent-encode characters that are unsafe inside a query string value."""
    safe = set('ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~')
    encoded = ''
    for ch in s:
        if ch in safe:
            encoded += ch
        elif ch == ' ':
            encoded += '%20'
        else:
            encoded += f'%{ord(ch):02X}'
    return encoded


def lookup(keyword: str) -> str:
    """
    Open a fresh TCP socket, send one HTTP GET request, and return the body.

    A new connection per request is used because the server sends
    Connection: close, so we cannot reuse the same socket.
    """
    encoded = url_encode(keyword)
    request = (
        f"GET /define?keyword={encoded} HTTP/1.1\r\n"
        f"Host: {HOST}:{PORT}\r\n"
        f"Connection: close\r\n"
        f"\r\n"
    ).encode('utf-8')

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.connect((HOST, PORT))
        sock.sendall(request)

        # Read until the server closes the connection (Connection: close).
        raw = b''
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            raw += chunk

    except ConnectionRefusedError:
        return f"Cannot connect to {HOST}:{PORT}. Is the server running?"
    except OSError as e:
        return f"Socket error: {e}"
    finally:
        sock.close()

    # Discard HTTP headers; show only the body to the user.
    if b'\r\n\r\n' in raw:
        body = raw.split(b'\r\n\r\n', 1)[1]
        return body.decode('utf-8', errors='replace')
    return raw.decode('utf-8', errors='replace')


def main() -> None:
    print(f"HTTP Dictionary Client - server at {HOST}:{PORT}")
    print("Type a keyword to look up, or 'quit' to exit.\n")

    while True:
        try:
            keyword = input("> ").strip()
        except (EOFError, KeyboardInterrupt):
            print()
            break

        if not keyword:
            continue

        if keyword.lower() == 'quit':
            print("Disconnected.")
            break

        print(lookup(keyword))


if __name__ == "__main__":
    main()
