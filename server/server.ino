#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

const char *ssid = "rssi-cars";
const int udpPort = 3333;

byte packet[255];
byte serial[255];
WiFiUDP udp;

// Packet format:
// Opcode [1 b]   | Arguments [n b]
// ---------------|----------------
// Control (0x01) | Direction (1 b)
// Leader (0x02)  | Fuse MAC (8 b)
// Kill (0x03)    | (0 b)
// RSSI (0x04)    | Fuse MAC (8 b) RSSI value (1 b)
// Devices (0x10) | Amount [1 b]

void setup() {
  Serial.begin(115200);

  WiFi.softAP(ssid);
  udp.begin(WiFi.softAPIP(),udpPort);
}

void loop() {
  Serial.write(0x10);
  Serial.write(WiFi.softAPgetStationNum());

  if(Serial.available() > 0) {
    int len = Serial.available();
    Serial.readBytes(serial, len);

    udp.beginPacket(WiFi.broadcastIP(), udpPort);
    udp.write(serial, len);
    udp.endPacket();
  }

  int packetSize = udp.parsePacket();
  if (packetSize > 0) {
    udp.read(packet, packetSize);
    Serial.write(packet, packetSize);
  }

  delay(50);
}
