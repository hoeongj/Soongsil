import socket 

# Client configuration — must match server HOST and PORT
HOST        = '127.0.0.1'   # Server address
PORT        = 12345          # Server UDP port
BUFFER_SIZE = 1024           # Max datagram size
TIMEOUT_SEC = 5.0            # Seconds to wait for reply

try:
    # create a UDP socket with a timeout for receiving data
    udp_client = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_client.settimeout(TIMEOUT_SEC)

    # prompt the user to enter a sentence
    enter = input("Enter a sentence : ")

    # send the encoded string to the UDP server
    udp_client.sendto(enter.encode('UTF-8'), (HOST, PORT))

    # receive the response from the server and decode it
    text, addr = udp_client.recvfrom(BUFFER_SIZE)
    text = text.decode('UTF-8')

    # display the server's reply
    print(text)

    #close the client socket after successful execution
    udp_client.close()

# exception handling
# handling socket errors
except socket.error as e:
    print("Socket error:", e)