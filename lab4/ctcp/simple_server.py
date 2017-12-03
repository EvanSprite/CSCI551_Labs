import sys
import socket

if(len(sys.argv) < 4):
    print 'Usage: python simple_server.py LOCAL_ADDRESS LOCAL_PORT FILE_NAME'
    sys.exit(0)


SERVER_IP = sys.argv[1]
SERVER_PORT = int(sys.argv[2])
OUTPUT_NAME = sys.argv[3]

f = open(OUTPUT_NAME, 'wb')
if f == None:
    print OUTPUT_NAME, 'cannot be opened'
    sys.exit(0)
print 'Open file', OUTPUT_NAME

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind((SERVER_IP, SERVER_PORT))
print 'Bind to {0}:{1}'.format(SERVER_IP, SERVER_PORT)
print 'Listen on', SERVER_IP, ":", SERVER_PORT
s.listen(1)
(conn, addr) = s.accept()
print 'Accept connection from', addr
while True:
    str = conn.recv(1024)
    if len(str) == 0:
        break;
    f.write(str)

f.close()
conn.close()
s.close()
