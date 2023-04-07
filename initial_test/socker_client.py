import socket

HOST = '192.168.2.73'
# HOST = '127.0.0.1'  # replace with the IP address of the server
PORT = 1234  # replace with the port number used by the server

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    while True:
        message = input("Enter a message to send: ")
        s.sendall(message.encode('utf-8'))
        print("Message sent!")
