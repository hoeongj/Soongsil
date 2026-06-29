import socket

# Server configuration
HOST        = '127.0.0.1'
PORT        = 12345
BUFFER_SIZE = 1024

def count_vowels(message: str) -> int:
    # count all vowel chacters
    vowels = 0
    for i in message:
        if(i in 'aeiouAEIOU'):
            vowels += 1
    return vowels

try:
    # create a UDP socket and bind it to the host and port
    udp_server = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_server.bind((HOST, PORT))

    # run continuously to handle multiple clients sequentially
    while True:
        # receive data from the client
        text, addr = udp_server.recvfrom(BUFFER_SIZE)
        text = text.decode('UTF-8')
        # calculate the number of vowels
        vowels = count_vowels(text)
        # construct the exact required reply format
        message = "Vowel count: " + str(vowels)
        # send the result back to the client
        udp_server.sendto(message.encode('UTF-8'), addr)

# exception handling
except socket.error as e:
    # handling socket errors
    print("Socket error:", e)
# ensure the socket is closed when the server shuts down
finally : udp_server.close()