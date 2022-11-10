#include <WiFi.h>
#include <WiFiUdp.h>
#include <math.h>

#define DEBUG

// #define LOW 0
// #define HIGH 255
# define PAUSE 300

const char * networkName = "rssi-cars";
#define measurements 5
#define threshold 3
#define distance 10
#define measured_power 40
#define env_val 2

const int udpPort = 3333;
boolean connected = false;
boolean leader_assigned = false;
boolean leader = false;
boolean rssi_received = false;
WiFiUDP udp;
char packet[255];

#define rf1 19
#define rf2 18
#define lf1 5
#define lf2 17
#define rb1 14
#define rb2 15
#define lb1 16
#define lb2 4
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

void send_rssi() {
  uint8_t buff[10];
  buff[0] = (uint8_t) 0x04;
  buff[1] = (uint8_t) (mac_self >> 56) & 0xFF;
  buff[2] = (uint8_t) (mac_self >> 48) & 0xFF; 
  buff[3] = (uint8_t) (mac_self >> 40) & 0xFF; 
  buff[4] = (uint8_t) (mac_self >> 32) & 0xFF; 
  buff[5] = (uint8_t) (mac_self >> 24) & 0xFF; 
  buff[6] = (uint8_t) (mac_self >> 16) & 0xFF; 
  buff[7] = (uint8_t) (mac_self >> 8) & 0xFF; 
  buff[8] = (uint8_t) (mac_self >> 0) & 0xFF; 
  buff[9] = (uint8_t) WiFi.RSSI();
  
  udp.beginPacket(WiFi.broadcastIP(), udpPort);
  udp.write(buff, 10);
  udp.endPacket();
}

void IRAM_ATTR onTimer(){
  // If this robot is the leader
  send_rssi();
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

void rotorstop() {
  for ( int i = 0; i < 4; ++i ) {
    for ( int j = 0; j < 2; ++j ) {
      digitalWrite(control[i][j], LOW);
    }
  }
}

void move_forward() {
   rotorstop();
   delay(PAUSE);
   for ( int i = 0; i < 4; ++i ) {
     digitalWrite(control[i][0], HIGH);
     digitalWrite(control[i][1], LOW);
   }
}

void move_back() {
  rotorstop();
  delay(PAUSE);
  for ( int i = 0; i < 4; ++i ) {
    digitalWrite(control[i][0], LOW);
    digitalWrite(control[i][1], HIGH);
  }
}

void turn_left() {
  rotorstop();
  delay(PAUSE);
  for ( int i = 0; i < 4; ++i ) {
    if ( i < 2 ) {
      digitalWrite(control[i][0], HIGH);
      digitalWrite(control[i][1], LOW);
    } else {
      digitalWrite(control[i][0], LOW);
      digitalWrite(control[i][1], HIGH); 
    }
  }
}

void turn_right() {
  rotorstop();
  delay(PAUSE);
  for ( int i = 0; i < 4; ++i ) {
    if ( i < 2 ) {
      digitalWrite(control[i][0], LOW);
      digitalWrite(control[i][1], HIGH);
    } else {
      digitalWrite(control[i][0], HIGH);
      digitalWrite(control[i][1], LOW); 
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
      rotorstop();
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

long get_distance_to_leader() {
  // distance = 10^((Measured Power - Instant RSSI)/10*N)
  long leader_to_ap = pow(10, ((measured_power - rssi_leader_val) / (10 * env_val)));
  long follower_to_ap = pow(10, ((measured_power - rssi_self_val) / (10 * env_val)));

  return abs(follower_to_ap - leader_to_ap);
}

void init_timer(){
  // Use 1st timer of 4 (counted from zero).
  // Set 80 divider for prescaler
  timer = timerBegin(0, 80, true);

  // Attach onTimer function to our timer.
  timerAttachInterrupt(timer, &onTimer, true);

  // Set alarm to call onTimer function (value in microseconds).
  // Repeat the alarm (third parameter)
  //trigger every 200ms
  timerAlarmWrite(timer, 200000, true);

  // Start an alarm
  timerAlarmEnable(timer);
}

void setup(){
  init_pins();

  // Initilize hardware serial:
  Serial.begin(115200);
  mac_self = ESP.getEfuseMac();

  //Connect to the WiFi network
  connectToWiFi(networkName);
  #ifdef DEBUG
  leader_assigned = true;
  leader = true;
  #endif
//  init_timer();
}

void loop(){
  //only send data when connected
  if(!connected || !leader_assigned){
    Serial.print("Connection: ");
    Serial.print(connected);
    Serial.print(" Leader assigned: ");
    Serial.println(leader_assigned);
//    udp.beginPacket(WiFi.broadcastIP(), udpPort);
//    udp.printf("MAC: %d", mac_self);
//    udp.printf(", RSSI: %d\n", WiFi.RSSI());
//    udp.endPacket();

    //Wait for 1 second
    delay(1000);
    return;
  }

//  send_rssi();

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

  // If this robot is the follower and we have received our initial RSSI value
  if (!leader && rssi_received) {
    long distance_delta = get_distance_to_leader();
    if (distance_delta > distance + threshold) {
      if (prev_state != FORWARD) {
        move_forward();
        prev_state = FORWARD;
      }
    }

    else if (distance_delta < distance + threshold) {
      if (prev_state != BACK) {
        move_back();
        prev_state = BACK;
      }
    }

    else {
      if (prev_state != STOP) {
        rotorstop();
        prev_state = STOP;
      } 
    }
  }
}
