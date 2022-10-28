import time
import socket
import keyboard

# bind all IP
HOST = '0.0.0.0'
# Listen on Port
PORT = 4444
# Size of receive buffer
BUFFER_SIZE = 1024
# Create a TCP/IP socket
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
# Bind the socket to the host and port
s.bind((HOST, PORT))

lastKeyPressed = None

# Client needs to call once to set this
client_address = None


def send(buffer, addr):
    s.sendto(bytes(buffer, "raw_unicode_escape"), addr)


def drive(direction):
    tx = direction
    send(tx, client_address)
    print("Direction")


def recv_client_address():
    data = s.recvfrom(BUFFER_SIZE)
    client_address = data[1]

    print("Received client address. Now connected to:", client_address)

    return client_address

# Announce server startup
# send("Cannounce", ("255.255.255.255", PORT))

while True:
    if client_address is None:
        print("Awaiting client address")
        new_client = recv_client_address()
        client_address = new_client
        continue

    keypress = keyboard.read_key()
    if keypress == lastKeyPressed:
        print("skipped")
        lastKeyPressed = None
        continue
    else:
        lastKeyPressed = keypress
        print("Key",  keypress)
        if keypress == "w":
            drive("F")
        elif keypress == "s":
            drive("S")
        elif keypress == "a":
            drive("A")
        elif keypress == "d":
            drive("D")
        elif keypress == "space":
            drive("0");
            

    # # Receive BUFFER_SIZE bytes data (data, client_address)
    # data = s.recvfrom(BUFFER_SIZE)
    # if data:
    #     # print received data
    #     print('Client to Server: ', data)
    #     # Convert to upper case and send back to Client
    #     tx_data = "F100"  # bytearray(data[0].upper())

    #     s.sendto(bytes(tx_data, "raw_unicode_escape"), data[1])
    #     time.sleep(2)
    #     tx_data = "S100"
    #     s.sendto(bytes(tx_data, "raw_unicode_escape"), data[1])

# Close connection
s.close()
