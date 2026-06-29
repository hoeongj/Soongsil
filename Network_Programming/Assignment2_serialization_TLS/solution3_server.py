import socket
import ssl

# Server configuration
HOST     = '127.0.0.1'
PORT     = 9443
BUFFER   = 1024
CERTFILE = 'server.crt'
KEYFILE  = 'server.key'


# TODO: Write the TLS echo server from here.
#
# Your server should:
#   - Listen for TCP connections on HOST:PORT.
#   - Use the ssl module to secure the connection, loading
#     CERTFILE and KEYFILE so clients can verify the server.
#   - For each client, echo received messages back with an
#     "Echo: " prefix.  If the client sends "exit", reply
#     with "Goodbye!" and close that connection.
#   - Support multiple sequential clients.
#   - Handle exceptions (TLS errors, disconnections, etc.)
#     gracefully.


# ----------------------------------------------------------
# TODO: Verify your output matches the following format:
#
#   TLS Echo Server listening on 127.0.0.1:9443
#   Client connected: ('127.0.0.1', xxxxx)
#   Received: Hello
#   Client sent exit. Closing connection.
# ----------------------------------------------------------

# Generate server.key / server.crt once before running this server:
#
#   openssl req -x509 -newkey rsa:2048 -keyout server.key -out server.crt \
#           -days 365 -nodes \
#           -subj "/CN=localhost" \
#           -addext "subjectAltName=IP:127.0.0.1,DNS:localhost"
#
# The SAN entry for IP:127.0.0.1 is required because Python 3.7+ no longer
# falls back to the Common Name for hostname verification.
def main():
    # Build a server-side TLS context and load our certificate + private key.
    try:
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        ctx.load_cert_chain(certfile=CERTFILE, keyfile=KEYFILE)
    except FileNotFoundError as e:
        print(f"Certificate/key not found: {e}")
        print("Generate them first with the openssl command in the header.")
        return
    except ssl.SSLError as e:
        print(f"Failed to load certificate/key: {e}")
        return

    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        srv.bind((HOST, PORT))
        srv.listen(5)
        print(f"TLS Echo Server listening on {HOST}:{PORT}")

        # Accept clients one at a time.
        while True:
            try:
                raw, addr = srv.accept()
            except KeyboardInterrupt:
                print("\nServer shutting down.")
                break

            # Wrap the raw TCP socket with TLS. A failed handshake should
            # not kill the whole server — just drop this client.
            try:
                tls = ctx.wrap_socket(raw, server_side=True)
            except ssl.SSLError as e:
                print(f"TLS handshake error with {addr}: {e}")
                raw.close()
                continue
            except OSError as e:
                print(f"Socket error during handshake with {addr}: {e}")
                raw.close()
                continue

            print(f"Client connected: {addr}")

            # Per-client message loop
            try:
                while True:
                    try:
                        data = tls.recv(BUFFER)
                    except ssl.SSLError as e:
                        print(f"SSL recv error: {e}")
                        break
                    except ConnectionResetError:
                        print(f"Client {addr} reset the connection.")
                        break

                    if not data:
                        print(f"Client {addr} disconnected.")
                        break

                    msg = data.decode("utf-8").strip()

                    if msg == "exit":
                        # Spec wording must match exactly.
                        print("Client sent exit. Closing connection.")
                        try:
                            tls.sendall(b"Goodbye!")
                        except (ssl.SSLError, BrokenPipeError,
                                ConnectionResetError):
                            pass
                        break

                    print(f"Received: {msg}")
                    try:
                        tls.sendall(f"Echo: {msg}".encode("utf-8"))
                    except (ssl.SSLError, BrokenPipeError,
                            ConnectionResetError) as e:
                        print(f"Error sending reply: {e}")
                        break
            except Exception as e:
                # Keep the server up on unexpected per-client errors.
                print(f"Unexpected error with {addr}: {e}")
            finally:
                # Graceful TLS close, then underlying socket close.
                try:
                    tls.shutdown(socket.SHUT_RDWR)
                except (OSError, ssl.SSLError):
                    pass
                tls.close()

    except OSError as e:
        print(f"Server socket error: {e}")
    finally:
        srv.close()
        print("Server socket closed.")


if __name__ == "__main__":
    main()
