from socket import *
import json
import time

PORT = 8005
IP = "127.0.0.1"

try: # load the json with the server data and update vars
    with open('Sever/SeverData.json') as jFile:
        jData = json.load(jFile)
        PORT = jData['port']
        IP = jData['ip']
except FileNotFoundError:
    print("when starting UDP sever \"SeverData.json\" not found")
except json.JSONDecodeError:
    print("error in the file \"SeverData.json\"")

clientSocket = socket(AF_INET, SOCK_DGRAM)
pings =1
while pings < 11:
    # Create a UDP socket
    clientSocket = socket(AF_INET, SOCK_DGRAM)

    # Set a timeout value of 1 second
    clientSocket.settimeout(1)

    # Ping to server
    message = 'test'

    addr = (IP, PORT)

    # Send ping
    start = time.time()
    clientSocket.sendto(message.encode(), addr)

    # If data is received back from server, print
    try:
        data, server = clientSocket.recvfrom(1024)
        end = time.time()
        elapsed = end - start
        print(data , " " , pings , " " , elapsed)

        # If data is not received back from server, print it has timed out
    except timeout:
        print
        'REQUEST TIMED OUT'

    pings = pings - 1