import socket

HOST = ''  # use '' to bind to all available interfaces
PORT = 1234  # choose a port number greater than 1024

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))
    s.listen()
    print(f"Server listening on {HOST}:{PORT}")
    conn, addr = s.accept()
    print(f"Connected by {addr}")

    with conn:
        while True:
            data = conn.recv(1024)  # receive up to 1024 bytes of data
            if not data:
                break  # end the connection if no more data is received
            message = data.decode('utf-8')
            print(f"Received message: {message}")
