import socket
import time 
port=4204

# Create a UDP socket
server = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
server.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
# Bind the socket to the port
server_address = ("", port)
server.bind(server_address)

message = b"your very important message"
print("Do Ctrl+c to exit the program !!")

# https://gist.github.com/cry/9e435d54cbe95fe9fddc2e0596409265
while True:
    print("####### Server is looping #######")
    # data, address = s.recvfrom(4096)
    # print("\n\n 2. Server received: ", data.decode('utf-8'), "\n\n")
    # send_data = input("Type some text to send => ")
    # s.sendto(send_data.encode('utf-8'), address)
    # print("\n\n 1. Server sent : ", send_data,"\n\n")

    server.sendto(message, ('<broadcast>', 4210))
    print("message sent!")
    time.sleep(1)