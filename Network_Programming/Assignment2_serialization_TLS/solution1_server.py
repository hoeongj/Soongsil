import socket
import pickle
import struct

# Server configuration
HOST        = '127.0.0.1'
PORT        = 7777
BUFFER_SIZE = 4096

# In-memory contact storage
contacts = []


def recv_message(conn: socket.socket) -> object:
    """
    TODO: Receive a length-prefixed pickled message.

    Protocol:
      1. Read exactly 4 bytes → big-endian unsigned int (the length).
      2. Read exactly `length` bytes → the pickled payload.
      3. Unpickle and return the Python object.

    Args:
        conn: Connected TCP socket.

    Returns:
        The deserialized Python object sent by the client.
    """
    # TCP may return fewer bytes than asked, so keep looping until
    # we have the full 4-byte length header.
    header = b''
    while len(header) < 4:
        chunk = conn.recv(4 - len(header))
        if not chunk:
            return None          # peer closed the connection
        header += chunk

    length = struct.unpack('>I', header)[0]

    # Read exactly `length` bytes of the pickled payload.
    body = b''
    while len(body) < length:
        chunk = conn.recv(min(BUFFER_SIZE, length - len(body)))
        if not chunk:
            return None
        body += chunk

    return pickle.loads(body)


def send_message(conn: socket.socket, obj: object) -> None:
    """
    TODO: Send a Python object as a length-prefixed pickled message.

    Protocol:
      1. Pickle the object into bytes.
      2. Pack the length as a 4-byte big-endian unsigned int.
      3. Send the length header followed by the pickled bytes.

    Args:
        conn: Connected TCP socket.
        obj:  Any picklable Python object.
    """
    # ← implement here
    data = pickle.dumps(obj)
    # 4-byte big-endian length prefix, then the data. sendall() guarantees
    # every byte is transmitted.
    conn.sendall(struct.pack('>I', len(data)) + data)


def handle_client(conn: socket.socket, addr: tuple) -> None:
    """
    TODO: Process commands from a single client connection.

    Supported commands (received as pickled dicts):
      {"action": "ADD",    "contact": {"name":..., "phone":..., "email":...}}
      {"action": "SEARCH", "keyword": str}
      {"action": "LIST"}

    Args:
        conn: Accepted TCP client socket.
        addr: Client's (host, port).
    """
    # ← implement here
    print(f"Client connected: {addr}")
    try:
        while True:
            cmd = recv_message(conn)
            if cmd is None:
                break                                # client disconnected

            # Be defensive: only proceed if we got a dict.
            if not isinstance(cmd, dict):
                send_message(conn, "Error: command must be a dict")
                continue

            action = cmd.get("action")

            if action == "ADD":
                contact = cmd.get("contact", {})
                contacts.append(contact)
                name = contact.get("name", "")
                send_message(conn, f"Contact added: {name}")
                print(f"[ADD] {contact}")

            elif action == "SEARCH":
                kw = cmd.get("keyword", "").lower()
                # Case-insensitive substring match on the name field.
                hits = [c for c in contacts
                        if kw in c.get("name", "").lower()]
                send_message(conn, hits)             # empty list [] if none
                print(f"[SEARCH] '{kw}' -> {len(hits)} match(es)")

            elif action == "LIST":
                send_message(conn, list(contacts))   # send a copy
                print(f"[LIST] {len(contacts)} contact(s) returned")

            else:
                send_message(conn, f"Unknown action: {action}")

    except (ConnectionResetError, BrokenPipeError, EOFError) as e:
        print(f"Client {addr} left abruptly: {e}")
    except pickle.UnpicklingError as e:
        print(f"Bad pickle data from {addr}: {e}")
    except Exception as e:
        # Keep the server alive even on unexpected errors.
        print(f"Unexpected error with {addr}: {e}")
    finally:
        conn.close()
        print(f"Client disconnected: {addr}")


# TODO: Write the main server loop from here.
# - Create a TCP socket, bind, listen.
# - Accept clients in a loop and call handle_client().
def main() -> None:
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # Avoid "Address already in use" when restarting quickly.
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        srv.bind((HOST, PORT))
        srv.listen(5)
        print(f"Contact Book Server listening on {HOST}:{PORT}")

        # Serve one client at a time, sequentially.
        while True:
            try:
                conn, addr = srv.accept()
            except KeyboardInterrupt:
                print("\nServer shutting down (Ctrl+C).")
                break
            handle_client(conn, addr)

    except OSError as e:
        print(f"Server socket error: {e}")
    finally:
        srv.close()
        print("Server socket closed.")


if __name__ == "__main__":
    main()
