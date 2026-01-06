import socket

MESSAGE = b'''
LEG 1 90 90 90
LEG 2 15 15 15
LEG 3 180 180 180
LEG 4 0 0 0
QUIT'''

try:
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.sendto(MESSAGE, ("127.0.0.1", 4000))
except socket.error as e:
    print("Error: %s" % e)
finally:
    sock.close()

