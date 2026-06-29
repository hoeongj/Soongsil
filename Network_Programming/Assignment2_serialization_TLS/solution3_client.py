import socket
import ssl

# Client configuration — must match server HOST and PORT
HOST     = '127.0.0.1'
PORT     = 9443
BUFFER   = 1024
CERTFILE = 'server.crt'   # Used as trusted CA on the client side


# TODO: Write the TLS echo client from here.
#
# Your client should:
#   - Connect to the server at HOST:PORT over TLS.
#   - Verify the server's certificate using CERTFILE.
#     Do NOT disable certificate verification.
#   - After connecting, print the server's certificate
#     details.
#   - In a loop: prompt the user for input, send it, and
#     display the server's reply.  If the user typed "exit",
#     display the final reply and disconnect.
#   - Handle exceptions (TLS errors, disconnections, etc.)
#     gracefully.


# ----------------------------------------------------------
# TODO: Verify your output matches the following format:
#
#   Connected to TLS server at 127.0.0.1:9443
#   Certificate: {'subject': ... , 'issuer': ... }
#   > Hello
#   Server: Echo: Hello
#   > exit
#   Server: Goodbye!
#   Connection closed.
# ----------------------------------------------------------

def main():
    # Build a client-side TLS context. create_default_context() already
    # enables hostname checking and CERT_REQUIRED verification, so we only
    # register our self-signed cert as a trusted CA — verification stays on.
    ctx = ssl.create_default_context(ssl.Purpose.SERVER_AUTH,
                                     cafile=CERTFILE)

    raw = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    tls = None                             # defined before wrap_socket

    try:
        # Finish the TCP 3-way handshake first, then wrap with TLS so
        # the handshake happens right here.
        raw.connect((HOST, PORT))
        tls = ctx.wrap_socket(raw, server_hostname=HOST)

        print(f"Connected to TLS server at {HOST}:{PORT}")
        # getpeercert() proves TLS is in use and verification succeeded.
        print(f"Certificate: {tls.getpeercert()}")

        # Per-client message loop
        while True:
            try:
                msg = input("> ")
            except (EOFError, KeyboardInterrupt):
                print()
                break

            if msg == "":
                continue                  # skip empty input

            try:
                tls.sendall(msg.encode("utf-8"))
                data = tls.recv(BUFFER)
            except (ssl.SSLError, OSError) as e:
                print(f"Connection error: {e}")
                break

            if not data:
                print("Server closed the connection.")
                break

            print(f"Server: {data.decode('utf-8')}")

            # Exit after showing the server's "Goodbye!" reply.
            if msg == "exit":
                break

    # Specific handlers so the user sees a clear message instead of a trace.
    except ConnectionRefusedError:
        print(f"Server not reachable at {HOST}:{PORT}")
        return
    except ssl.SSLError as e:
        print(f"TLS handshake failed: {e}")
        return
    except OSError as e:
        print(f"Socket error: {e}")
        return

    finally:
        # Only announce close when a TLS session actually existed.
        if tls is not None:
            try:
                tls.shutdown(socket.SHUT_RDWR)
            except OSError:
                pass
            tls.close()
            print("Connection closed.")
        else:
            raw.close()


if __name__ == "__main__":
    main()