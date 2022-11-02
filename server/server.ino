#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

const char *ssid = "rssi-cars";
WiFiUDP udp;
char incomingPacket[255];  // buffer for incoming packets

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");

  WiFi.softAP(ssid);
  IPAddress ip = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(ip);
  udp.begin(3333);

  Serial.println("Server started");
}

void loop() {
  int packetSize = udp.parsePacket();
  if (packetSize)
  {
    // receive incoming UDP packets
    // Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
    int len = udp.read(incomingPacket, 255);
    if (len > 0)
    {
      incomingPacket[len] = 0;
    }
    Serial.printf("UDP packet contents: %s\n", incomingPacket);
  }
  delay(1000);
}