import socket

tcp_client_socket=socket.socket(socket.AF_INET,socket.SOCK_STREAM)

server_ip="192.168.240.42"
server_sport=3333

tcp_client_socket.connect((server_ip,server_sport))

send_data="hello, esp32"

tcp_client_socket.send(send_data.encode("utf-8"))

recvData=tcp_client_socket.recv(1024)

print('接收到的数据为:', recvData.decode('utf-8'))

tcp_client_socket.close()