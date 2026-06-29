import socket
import random

# Server configuration
HOST         = '127.0.0.1'
PORT         = 9999
BUFFER_SIZE  = 1024
MAX_ATTEMPTS = 3

def play_game(conn: socket.socket, addr: tuple) -> None:
    """
    Args:
        conn: Accepted TCP client socket.
        addr: Client's (host, port).
    """

    # play one full math quiz game
    try:
        # expect "start" handshake from the client
        first_message = conn.recv(BUFFER_SIZE).decode('UTF-8').strip()
        if first_message != "start":
            conn.sendall("Error 400: Bad Request. Send 'start' to play.".encode('UTF-8'))
            return

        # generate a random addition problem
        x = random.randint(1, 20)
        y = random.randint(1, 20)

        send_message = "What is " + str(x) + " + " + str(y) + "?"
        conn.sendall(send_message.encode('UTF-8'))

        #handle up to 3 attempts
        for i in range(1, MAX_ATTEMPTS + 1):
            real_message = conn.recv(BUFFER_SIZE).decode('UTF-8').strip()
            if not real_message: break

            # validate input without crashing
            try:
                ans = int(real_message)
            except ValueError:
                ans = None

            # check the answer and reply
            if ans == x+y:
                conn.sendall("Correct! You win.".encode('UTF-8'))
                return
            else:
                if i < MAX_ATTEMPTS:
                    conn.sendall("Incorrect. Try again!".encode('UTF-8'))
                else:
                    mem = "Game Over. Out of attempts. The correct answer was " + str(x+y) + "."
                    conn.sendall(mem.encode('UTF-8'))
                    return
    except socket.error as e:
        print(addr,"Connection error:",e)
    finally:
        # always close the connection
        conn.close()


try:
    # initialize and start the TCP server
    tcp_server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    tcp_server.bind((HOST, PORT))
    tcp_server.listen(5)

    # accept incoming client connections continuously
    while True:
        conn, addr = tcp_server.accept()
        play_game(conn, addr)

# exception handling
except socket.error as e:
    print("Server error",e)
finally:
    tcp_server.close()