import socket

macAddress=("a8:e2:91:02:c2:5d",4)

client_socket = socket.socket(socket.AF_BLUETOOTH, socket.SOCK_STREAM, socket.BTPROTO_RFCOMM)
client_socket.connect(macAddress)

try:
    message = "getData"
    client_socket.send(message.encode())
    data = client_socket.recv(1024).decode()
    if not data:
        break
    print(data)
except OSError as e:
    pass
client_socket.close()
