#include <WiFi.h>
#include <WiFiUdp.h>

// WiFi network name and password:
const char * networkName = "yourAP";
// const char * networkPswd = "your-password";

//IP address to send UDP data to:
// either use the ip address of the server or 
// a network broadcast address
// const char * udpAddress = "192.168.0.255";
const int udpPort = 3333;

//Are we currently connected?
boolean connected = false;

//The udp library class
WiFiUDP udp;
char packet[255];

void setup(){
  // Initilize hardware serial:
  Serial.begin(115200);
  
  //Connect to the WiFi network
  connectToWiFi(networkName);
}

void loop(){
  //only send data when connected
  if(connected){
    //Send a packet
    udp.beginPacket(WiFi.broadcastIP(), udpPort);
    udp.printf("Seconds since boot: %lu", millis()/1000);
    udp.endPacket();

    Serial.println(udp.readStringUntil('\n'));
    

    int packetSize = udp.parsePacket();
    if (packetSize) {
      Serial.print("Received packet! Size: ");
      Serial.println(packetSize); 
      int len = udp.read(packet, 255);
      if (len > 0)
      {
        packet[len] = '\0';
      }
      Serial.println(packet);
    }
  }
  //Wait for 1 second
  delay(1000);
}

void connectToWiFi(const char * ssid){
  Serial.println("Connecting to WiFi network: " + String(ssid));

  // delete old config
  WiFi.disconnect(true);
  //register event handler
  WiFi.onEvent(WiFiEvent);
  
  //Initiate connection
  WiFi.begin(ssid);

  Serial.println("Waiting for WIFI connection...");
}

//wifi event handler
void WiFiEvent(WiFiEvent_t event){
    switch(event) {
      case ARDUINO_EVENT_WIFI_STA_GOT_IP:
          //When connected set 
          Serial.print("WiFi connected! IP address: ");
          Serial.println(WiFi.localIP());  
          //initializes the UDP state
          //This initializes the transfer buffer
          udp.begin(WiFi.localIP(),udpPort);
          connected = true;
          break;
      case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
          Serial.println("WiFi lost connection");
          connected = false;
          break;
      default: break;
    }
}