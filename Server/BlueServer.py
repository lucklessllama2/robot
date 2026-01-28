import CMD_Servers
import socket
import threading
import json
class BLUETOOTH_Server(CMD_Servers.CMD_Server):
    def __init__(self,s):
        super().__init__(s)
        self.address = (self.getMac(),4)
        self.cState = None

        self.socket = socket.socket(socket.AF_BLUETOOTH, socket.SOCK_STREAM, socket.BTPROTO_RFCOMM)
        self.sock.bind(self.address)
        self.running = True
        self.client_socket = None
        self.client_address = None
        self.thread = threading.Thread(target=self.Loop, args=())
    def Loop(self):
        self.sock.listen(1)
        client, addr = self.sock.accept()
        self.client_socket = client
        self.client_address = addr
        while self.running:
            # Receive data and the client address
            # 1024 is the buffer size (in bytes)
            data = self.sock.recv(1024)

            # Decode the received data
            decoded_data = data.decode('utf-8')

            self.decode(decoded_data)
        self.client_socket.close()
        self.sock.close()
    def sendResponse(self, data):
        self.client_socket.send(data.encode('utf-8'))