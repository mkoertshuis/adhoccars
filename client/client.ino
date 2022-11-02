#include <WiFi.h>
#include <WiFiUdp.h>

#define measurements 5
#define threshold 3
#define distance 10

const char *networkName = "rssi-cars";
const int udpPort = 3333;
boolean connected = false;
boolean leader_assigned = false;
boolean leader = false;
boolean rssi_received = false;
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

int8_t rssi_leader[measurements];
int8_t rssi_self[measurements];

int8_t rssi_leader_val = 0;
int8_t rssi_self_val = 0;

uint64_t mac_packet = 0;
uint64_t mac_leader = 0;
uint64_t mac_self = 0;

hw_timer_t *timer = NULL;

enum movement {
  FORWARD,
  BACK,
  STOP
};

enum movement prev_state;


// Packet format:
// Opcode [1 b]   | Arguments [n b]
// ---------------|----------------
// RSSI (0x04)    | Fuse MAC (8 b) RSSI value (1 b)
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

void set_rssi_leader(uint8_t rssi) {
  rssi_leader_val = rssi;
}

void set_rssi_self(uint8_t rssi) {
  rssi_self_val = rssi;
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

void rssi_handler(uint64_t mac, uint8_t rssi) {
  if (mac == mac_leader) {
    set_rssi_leader(rssi);
    rssi_received = true;
  }
}

void leader_handler(uint64_t mac) {
  mac_leader = mac;
  if (mac_leader == mac_self) {
    leader = true;
  }
  else {
    leader = false;
  }
  leader_assigned = true;
}

void packet_handler(char * packet) {
  // Change the char of the packet to a byte for easier handling
  uint8_t op = (uint8_t) packet[0];
  switch (op) {
        case 1: // control
    {
      uint8_t direction = (uint8_t) packet[1];
      if (leader) {
        control_handler(direction);
      }
      break;
    }
    case 2: // Leader
    {
      mac_packet = 0;
      mac_packet += (uint64_t) packet[1] << 56;
      mac_packet += (uint64_t) packet[2] << 48;
      mac_packet += (uint64_t) packet[3] << 40;
      mac_packet += (uint64_t) packet[4] << 32;
      mac_packet += (uint64_t) packet[5] << 24;
      mac_packet += (uint64_t) packet[6] << 16;
      mac_packet += (uint64_t) packet[7] << 8;
      mac_packet += (uint64_t) packet[8];
      leader_handler(mac_packet);
      break;
    }
    case 4: // RSSI
    {
      mac_packet = 0;
      mac_packet += (uint64_t) packet[1] << 56;
      mac_packet += (uint64_t) packet[2] << 48;
      mac_packet += (uint64_t) packet[3] << 40;
      mac_packet += (uint64_t) packet[4] << 32;
      mac_packet += (uint64_t) packet[5] << 24;
      mac_packet += (uint64_t) packet[6] << 16;
      mac_packet += (uint64_t) packet[7] << 8;
      mac_packet += (uint64_t) packet[8];
      uint8_t rssi = (uint8_t) packet[9];
      rssi_handler(mac_packet, rssi);
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
  mac_self = ESP.getEfuseMac();

  
  //Connect to the WiFi network
  connectToWiFi(networkName);
  leader_assigned = true;
  leader = true;
}

void loop(){
  //only send data when connected
  if(!connected || !leader_assigned){
    Serial.print("Connection: ");
    Serial.print(connected);
    Serial.print(" Leader assigned: ");
    Serial.println(leader_assigned);
    udp.beginPacket(WiFi.broadcastIP(), udpPort);
    udp.printf(mac_self);
    udp.printf(WiFi.RSSI());
    udp.endPacket();

    //Wait for 1 second
    delay(1000);
    return;
  }

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
    packet_handler(packet);
  }


  // If this robot is the leader
  if (leader) {
    //Send a packet
    udp.beginPacket(WiFi.broadcastIP(), udpPort);
    udp.printf("MAC: %d", mac_self);
    udp.printf("RSSI: %d", WiFi.RSSI());

    // udp.write();
    // udp.read();
    udp.endPacket();
  }

  // If this robot is the follower
  if (!leader && rssi_received) {
    if (rssi_leader_val - rssi_self_val > distance + threshold) {
      if (prev_state != FORWARD) {
        move_forward();
        prev_state = FORWARD;
      }
    }

    else if (rssi_leader_val - rssi_self_val < distance + threshold) {
      if (prev_state != BACK) {
        move_back();
        prev_state = BACK;
      }
    }

    else {
      if (prev_state != STOP) {
        stop();
        prev_state = STOP;
      } 
    }

  }

}