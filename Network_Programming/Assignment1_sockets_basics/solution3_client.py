import socket

# Client configuration — must match server HOST and PORT
HOST         = '127.0.0.1'
PORT         = 9999
BUFFER_SIZE  = 1024
MAX_ATTEMPTS = 3   # Must match server's limit


def get_integer_answer(attempt: int) -> str:
    """
    asking until valid numeric input is provided.

    Return the validated string.

    Args:
        attempt (int): Current attempt number for the prompt.

    Returns:
        str: Validated integer string to send to the server.
    """

    # continuously prompt until valid integer input is provided
    while True:
        ans = input("Attempt " + str(attempt) +" / " + str(MAX_ATTEMPTS) + ": ")
        try:
            int(ans)
            return ans
        except ValueError:
            print("please enter an integer")


try:
    # create a TCP socket and connect to the server
    tcp_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    tcp_client.connect((HOST, PORT))

    # send the "start" message to initiate the game
    tcp_client.sendall("start".encode('UTF-8'))

    # receive the math problem from the server
    text = tcp_client.recv(BUFFER_SIZE).decode('UTF-8')
    print("Server:", text)

    # enter the game loop for a maximum of 3 attempts
    for i in range(1, MAX_ATTEMPTS + 1):
        # get validated integer input from the user
        ans = get_integer_answer(i)
        tcp_client.sendall(ans.encode('UTF-8'))

        # receive the result for the attempt
        result = tcp_client.recv(BUFFER_SIZE).decode('UTF-8')
        print("Server:", result)

        # break the loop cleanly on game end conditions
        if "Correct" in result or "Game Over" in result:
            break

# exception handling
except socket.error as e:
    print("Socket error:", e)
finally:
    tcp_client.close()