import socket
from struct import unpack

UDP_IP = "127.0.0.1"
UDP_PORT = 5010

sock = socket.socket(socket.AF_INET, # Internet
                     socket.SOCK_DGRAM) # UDP
sock.bind((UDP_IP, UDP_PORT))

while True:
    data, addr = sock.recvfrom(3) # buffer size is 1024 bytes
    print "received message:", unpack("BBB", data)

