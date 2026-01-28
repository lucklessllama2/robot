import CMD_Servers
import socket
import threading
import json
class UDP_Server(CMD_Servers.CMD_Server):
    def __init__(self,s):
        super().__init__(s)

        self.client_address = None

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        # Bind the socket to the address and port
        self.sock.bind((self.get_local_ip(), 8000))
        self.running = True
        self.thread = threading.Thread(target=self.Loop, args=())
    def Loop(self):
        while self.running:
            # Receive data and the client address
            # 1024 is the buffer size (in bytes)
            data, client_address = self.sock.recvfrom(1024)
            self.client_address = client_address
            # Decode the received data
            decoded_data = data.decode('utf-8')

            self.decode(decoded_data)
        self.sock.close()
    def sendResponse(self, data):
        self.sock.sendto(data.encode('utf-8'),self.client_address)