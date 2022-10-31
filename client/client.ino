#include <WiFi.h>
#include <WiFiUdp.h>

const char * networkName = "rssi-cars";
const int udpPort = 3333;
boolean connected = false;
WiFiUDP udp;
char packet[255];

const int rf1 = 19;
const int rf2 = 18;
const int lf1 = 5;
const int lf2 = 17;
const int rb1 = 14;
const int rb2 = 15;
const int lb1 = 16;
const int lb2 = 4;
const int control[4][2] = { {rf1, rf2}, {rb1, rb2}, {lf1, lf2}, {lb1, lb2} }; 

// Packet format:
// Opcode [1 b]   | Arguments [n b]
// ---------------|----------------
// RSSI (0x00)    | Fuse MAC (8 b) RSSI value (1 b)
// Control (0x01) | Direction (1 b)
// Leader (0x02)  | Fuse MAC (8 b)
// Kill (0x03)    | (0 b)

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

void init_pins() {
  pinMode(rf1, OUTPUT);
  pinMode(rf2, OUTPUT);
  pinMode(lf1, OUTPUT);
  pinMode(lf2, OUTPUT);
  pinMode(rb1, OUTPUT);
  pinMode(rb2, OUTPUT);
  pinMode(lb1, OUTPUT);
  pinMode(lb2, OUTPUT);
}

void stop() {
  for ( int i = 0; i < 4; ++i ) {
    for ( int j = 0; i < 2; ++j ) {
      analogWrite(control[i][j], 0);
    }
  }
}

void move_forward() {
  for ( int i = 0; i < 4; ++i ) {
    analogWrite(control[i][0], 255);
    analogWrite(control[i][1], 0);
  }
}

void move_back() {
  for ( int i = 0; i < 4; ++i ) {
    analogWrite(control[i][0], 0);
    analogWrite(control[i][1], 255);
  }
}

void turn_left() {
  for ( int i = 0; i < 4; ++i ) {
    if ( i < 2 ) {
      analogWrite(control[i][0], 255);
      analogWrite(control[i][1], 0);
    } else {
      analogWrite(control[i][0], 0);
      analogWrite(control[i][1], 255); 
    }
  }
}

void turn_right() {
  for ( int i = 0; i < 4; ++i ) {
    if ( i < 2 ) {
      analogWrite(control[i][0], 0);
      analogWrite(control[i][1], 255);
    } else {
      analogWrite(control[i][0], 255);
      analogWrite(control[i][1], 0); 
    }
  }
}

void control_handler(uint8_t direction) {
  switch (direction) {
    case 1: // forward
      move_forward();
      break;
    case 2: // back
      move_back();
      break;
    case 3: // left
      turn_left();
      break;
    case 4: // right
      turn_right();
      break;
    default: // break
      stop();
      break;
  }
}

void packet_handler(char * packet) {
  // Change the char of the packet to a byte for easier handling
  uint8_t op = (uint8_t) packet[0];
  switch (op) {
    case 1: // control
    {
      uint8_t direction = (uint8_t) packet[1];
      control_handler(direction);
      break;
    }
    default:
      break;
  }
}

void setup(){
  init_pins();

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
    // udp.write();
    udp.read();
    udp.endPacket();

    // ESP.getEfuseMac()

    int packetSize = udp.parsePacket();
    // udp.remoteIP()
    if (packetSize) {
      Serial.print("Received packet! Size: ");
      Serial.println(packetSize); 
      int len = udp.read(packet, 255);
      if (len > 0)
      {
        packet[len] = '\0';
      }
      Serial.println(packet);
      packet_handler(packet);
    }
  }
  //Wait for 1 second
  delay(1000);
}