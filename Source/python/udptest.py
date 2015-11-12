import socket
from struct import pack

UDP_IP = "127.0.0.1"
UDP_PORT = 5000
MESSAGE = "Hello, World!"

print "UDP target IP:", UDP_IP
print "UDP target port:", UDP_PORT
print "message:", MESSAGE

sock = socket.socket(socket.AF_INET, # Internet
             socket.SOCK_DGRAM) # UDP
sock.sendto(pack("BBB", 0, 0, 1), (UDP_IP, UDP_PORT))

