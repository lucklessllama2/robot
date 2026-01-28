import socket

macAddress=("",4)

client_socket = socket.socket(socket.AF_BLUETOOTH, socket.SOCK_STREAM, socket.BTPROTO_RFCOMM)
client_socket.connect(macAddress)

