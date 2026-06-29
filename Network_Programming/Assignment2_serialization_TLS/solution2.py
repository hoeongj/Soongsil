import socket

HOST = "127.0.0.1"
PORT = 65432

def run_server():
    # FIX 1: SOCK_DGRAM is for UDP, which does not support listen()/accept().
    # We need SOCK_STREAM for TCP.
    server_sock = socket.socket(socket.AF_INET,
                                socket.SOCK_STREAM)  # FIX 1: UDP -> TCP
    server_sock.bind((HOST, PORT))
    server_sock.listen(1)
    print(f"Server listening on {HOST}:{PORT}")
    conn, addr = server_sock.accept()
    print(f"Connected by {addr}")

    while True:
        data = conn.recv(1024)
        if not data:
            break
        message = data.decode("utf-8")
        print(f"Client: {message}")
        if message.lower() == "quit":
            break

        reply = input("Server> ")
        # FIX 2: socket.send() only accepts bytes, so we must encode the
        # string reply first (otherwise TypeError).
        conn.send(reply.encode("utf-8"))  # FIX 2: encode str -> bytes
        # FIX 3: the original code never breaks when the server itself sends
        # "quit" — it just loops back and blocks on recv(). Check the reply
        # and break so either side can end the chat.
        if reply.lower() == "quit":       # FIX 3: exit on server-side quit
            break

    conn.close()
    server_sock.close()
    print("Server shut down.")

def run_client():
    client_sock = socket.socket(socket.AF_INET,
                                socket.SOCK_STREAM)
    client_sock.connect((HOST, PORT))
    print("Connected to server.")

    while True:
        msg = input("Client> ")
        client_sock.send(msg.encode("utf-8"))
        if msg.lower() == "quit":
            break

        data = client_sock.recv(1024)
        if not data:
            break
        # FIX 4: `data` is bytes; printing it directly displays as b'...'.
        # Decode to UTF-8 so the output is a readable string.
        message = data.decode("utf-8")    # FIX 4: decode bytes for display
        print(f"Server: {message}")
        # FIX 5: client also needs to stop when the server sends "quit",
        # so the loop doesn't prompt for another message after the server
        # has already closed.
        if message.lower() == "quit":     # FIX 5: exit on received quit
            break

    client_sock.close()

if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1 and sys.argv[1] == "client":
        run_client()
    else:
        run_server()
