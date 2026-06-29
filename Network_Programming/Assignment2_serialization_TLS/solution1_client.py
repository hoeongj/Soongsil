import socket
import pickle
import struct

# Client configuration — must match server HOST and PORT
HOST        = '127.0.0.1'
PORT        = 7777
BUFFER_SIZE = 4096


def recv_message(sock: socket.socket) -> object:
    """
    TODO: Receive a length-prefixed pickled message.

    Protocol:
      1. Read exactly 4 bytes → big-endian unsigned int (the length).
      2. Read exactly `length` bytes → the pickled payload.
      3. Unpickle and return the Python object.

    Args:
        sock: Connected TCP socket.

    Returns:
        The deserialized Python object sent by the server.
    """
    # ← implement here
    # Read the 4-byte length header first (loop in case of partial recv).
    head = b''
    while len(head) < 4:
        datac = sock.recv(4 - len(head))
        if not datac:
            return None                  # server closed connection
        head += datac

    length = struct.unpack('>I', head)[0]

    # Read exactly `length` bytes of the pickled body.
    payload = b''
    while len(payload) < length:
        datac = sock.recv(min(BUFFER_SIZE, length - len(payload)))
        if not datac:
            return None
        payload += datac

    return pickle.loads(payload)


def send_message(sock: socket.socket, obj: object) -> None:
    """
    TODO: Send a Python object as a length-prefixed pickled message.

    Protocol:
      1. Pickle the object into bytes.
      2. Pack the length as a 4-byte big-endian unsigned int.
      3. Send the length header followed by the pickled bytes.

    Args:
        sock: Connected TCP socket.
        obj:  Any picklable Python object.
    """
    # ← implement here
    data = pickle.dumps(obj)
    header = struct.pack('>I', len(data))
    sock.sendall(header + data)          # sendall ensures full transmission


def parse_input(user_input: str) -> dict:
    """
    TODO: Parse the user's command string into a command dict.

    Formats:
      ADD <name> <phone> <email>
        → {"action": "ADD", "contact": {"name":..., "phone":..., "email":...}}
      SEARCH <keyword>
        → {"action": "SEARCH", "keyword": ...}
      LIST
        → {"action": "LIST"}

    Args:
        user_input: Raw string from input().

    Returns:
        dict: Command dictionary to send to the server.
    """
    # ← implement here
    # Split on whitespace; first token is the action (case-insensitive).
    word = user_input.strip().split()
    if not word:
        return None
    action = word[0].upper()

    if action == "ADD":
        # Need exactly action + name + phone + email
        if len(word) < 4:
            return None
        return {
            "action": "ADD",
            "contact": {
                "name":  word[1],
                "phone": word[2],
                "email": word[3],
            },
        }
    elif action == "SEARCH":
        if len(word) < 2:
            return None
        return {"action": "SEARCH", "keyword": word[1]}
    elif action == "LIST":
        return {"action": "LIST"}

    # Unrecognised command
    return None


# TODO: Write the client logic from here.
# - Connect to the server.
# - In a loop: prompt user, parse input, send command, receive reply.
def main() -> None:
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    try:
        sock.connect((HOST, PORT))
        print(f"Connected to Contact Book Server at {HOST}:{PORT}")
        print("Commands:")
        print("  ADD <name> <phone> <email>")
        print("  SEARCH <keyword>")
        print("  LIST")
        print("  QUIT  (disconnect)")

        while True:
            try:
                line = input("> ")
            except EOFError:
                # Ctrl+D / Ctrl+Z ends the session cleanly.
                print()
                break

            if not line.strip():
                continue                 # skip empty input

            # QUIT/EXIT is purely a client-side command.
            if line.strip().upper() in ("QUIT", "EXIT"):
                print("Disconnecting from server.")
                break

            cmd = parse_input(line)
            if cmd is None:
                print("Invalid command. Usage:")
                print("  ADD <name> <phone> <email>")
                print("  SEARCH <keyword>")
                print("  LIST")
                continue

            try:
                send_message(sock, cmd)
                reply = recv_message(sock)
                if reply is None:
                    print("Server closed the connection.")
                    break
                print(f"Server: {reply}")
            except (ConnectionResetError, BrokenPipeError) as e:
                print(f"Connection error: {e}")
                break

    except ConnectionRefusedError:
        print(f"Cannot connect to server at {HOST}:{PORT}. Is the server running?")
    except KeyboardInterrupt:
        print("\nClient interrupted by user.")
    except Exception as e:
        print(f"Unexpected error: {e}")
    finally:
        sock.close()
        print("Client socket closed.")


if __name__ == "__main__":
    main()
