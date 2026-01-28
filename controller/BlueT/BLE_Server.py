import socket
macAddress=("",4)
server_socket = socket.socket(socket.AF_BLUETOOTH, socket.SOCK_STREAM, socket.BTPROTO_RFCOMM)

server_socket.bind(macAddress)
server_socket.listen(1)

client_socket,addr = server_socket.accept()


