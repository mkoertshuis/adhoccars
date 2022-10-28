# ESP32 firmware and Python  UDP server
Motor control for a car with ESP32 mounted on top, controlled with Python. Example code for the TU Delft Course - Adhoc Networks ET4388 - 2021 edition. Deviating is perfectly fine and certainly encouraged.

## Folder `ESP32_2021`

The code which contains commented out code for UDP interaction (steering commands). The code now functions to test the wheels to spin correctly.
The pattern is:
1) Forward
2) Delay
3) Left
4) Right

... and this loops forever.

## Folder `server-python`

Two example UDP server scripts to listen and send broadcast messages.
The `udp2.py` file allows you to send keyboard commands to a certain UDP port (now 4444) using broadcast. At startup the script listens for any UDP message from any client and saves the address from the client in a variable.
This means that your ESP needs to ping any buffer to receive server steering commands.
The required packages to be installed with pip are: `keyboard`

`udp_server.py` allows testing broadcasts by simply pushing a regular broadcast message each second.

## Network setup

Logically your ESP's and your server need to be on the same network with UDP broadcast capabilities for this example to work.
Make sure to enable the port using Windows firewall settings or your unix firewall configuration.

## Get to work!

Feel free to completely change the way you want to communicate with the ESP using WiFi. I've added an `archive` folder with code from previous years when an Arduino Mega was connected to an ESP32, but this is just to give you inspiration. Note however that this version used an older ESP32 SDK with respect to UDP, so it will not compile.
