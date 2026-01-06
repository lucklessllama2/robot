import socket
import json
import threading
import os
f = os.getcwd().split("robot")[0]+"robot"
os.chdir(f)
class udpServerClass:
    def __init__(self):
        self.PORT = 8000
        self.IP = "127.0.0.1"
        self.cState = None
        try: # load the json with the server data and update vars
            with open('Server/SeverData.json', "r") as jFile:
                jData = json.load(jFile)
                self.PORT = jData['port']
                self.IP = jData['ip']
        except FileNotFoundError:
            print("when starting UDP sever \"SeverData.json\" not found")
        except json.JSONDecodeError:
            print("error in the file \"SeverData.json\"")

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        # Bind the socket to the address and port
        self.sock.bind((self.IP, self.PORT))
        self.running = True
        self.thread = threading.Thread(target= self.UDPServerLoop, args=())
    def runUDPServer(self):
         self.thread.start()
    def UDPServerLoop(self):
        while self.running:
            # Receive data and the client address
            # 1024 is the buffer size (in bytes)
            data, client_address = self.sock.recvfrom(1024)

            # Decode the received data
            decoded_data = data.decode('utf-8')

            #print(f"Received message from {client_address}: {decoded_data!r}")

            # Send a response back to the client
            if decoded_data.startswith("con_update"):
                decoded_data = decoded_data.replace("con_update","")
                self.cState=json.loads(decoded_data)
            #response_message = f"Server received your data: {decoded_data!r}"
            #sock.sendto(response_message.encode('utf-8'), client_address)
        print("UdpServer Closing")
        self.sock.close()
    def getControllerData(self):
        if self.cState is not None:
            return self.cState
        else:
            print("no controller data yet")
            return None
udpServer = udpServerClass()