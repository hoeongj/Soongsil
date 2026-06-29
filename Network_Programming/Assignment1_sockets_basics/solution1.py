import socket
import ssl
import json

# API configuration
HOST = 'timeapi.io'
PORT = 443                                        
PATH = '/api/v1/timezone/zone?timeZone=Asia/Seoul'

try:
    # create a TCP socket and connect to the server
    tcp_server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    tcp_server.connect((HOST, PORT))

    # wrap the socket with SSL context since port 443 requires HTTPS
    ssl_context = ssl.create_default_context()
    http_req = ssl_context.wrap_socket(tcp_server, server_hostname=HOST)

    # construct the raw HTTP/1.1 GET request header
    get_header = (
        f"GET {PATH} HTTP/1.1\r\n"
        f"HOST: {HOST}\r\n"
        "Connection: close\r\n"
        "Accept: application/json\r\n\r\n"
    )

    # send the encoded HTTP request to the server
    http_req.sendall(get_header.encode('UTF-8'))

    # receive the response data in chunks
    data = ""
    while True:
        a = http_req.recv(4096)
        if not a: break
        data += a.decode('UTF-8')

    # close the socket after receiving all data
    http_req.close()

    # separate the HTTP header from the JSON payload
    message = data.split("\r\n\r\n", 1)[1]

    # extract the exact JSON string and parse it
    head = message.find('{')
    tail = message.rfind('}') + 1
    real_data = json.loads(message[head:tail])

    # print the required parsed data
    print("local_time  =", real_data['local_time'])
    print("timezone    =", real_data['timezone'])
    print("utc_time    =", real_data['utc_time'])

# exception handling

# handle network error
except socket.error as e:
    print("네트워크 오류가 발생했습니다 :",e)
# handling json error
except json.JSONDecodeError:
    print("서버에서 온 JSON을 분석하는데 실패하였습니다")
# handling other errors
except Exception as e:
    print("다음과 같은 오류가 발생했습니다 : ",e)

# ----------------------------------------------------------
#   local_time  = 2026-04-03T18:30:45.123456+09:00
#   timezone    = Asia/Seoul
#   utc_time    = 2026-04-03T09:30:45.123456+00:00
# ----------------------------------------------------------