#include "car.h"

#include <WiFi.h>
#include <WiFiUdp.h>

const char * networkName = "rssi-cars";
const int udpPort = 3333;

int8_t rssi_leader[measurements];
int8_t rssi_self[measurements];
uint_64_t mac_packet = 0;
uint_64_t mac_self = 0;

char packet[255];
bool connected = false;

WiFiUDP Udp;

hw_timer_t *timer = NULL;


/*
  WiFi LED timer for blinking state
*/
void IRAM_ATTR onTimer() {
  if (BlinkWLED) {
    if (WLEDState) {
      digitalWrite(WIFILED, HIGH);
    } else {
      digitalWrite(WIFILED, LOW);
    }
    WLEDState = !WLEDState;
  }
}

void initWireless() {
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(" .\n");
  }
  Serial.println("Connection succesfull\n");
  BlinkWLED = false;

  Udp.begin(udpPort);
  Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), udpPort);
}

// Packet format:
// Opcode [1 b]   | Arguments [n b]
// ---------------|----------------
// RSSI (0x00)    | Fuse MAC (8 b) RSSI value (1 b)
// Control (0x01) | Direction (1 b)
// Leader (0x02)  | Fuse MAC (8 b)
// Kill (0x03)    | (0 b)

void sendConnection() {
  uint8_t buffer[15] = "hello server";
  Udp.beginPacket(udpAddress, udpPort);
  Udp.write(buffer, 11);
  Udp.endPacket();
  memset(buffer, 0, 50);
}

uint8_t get_rssi_self() {
  int total = rssi_self[measurements];

  for (int i = measurements - 2; i >= 0; i--) {
    total += rssi_self[i];
    rssi_self[i + 1] = rssi_self[i];
  }
  rssi_self[0] = WiFi.RSSI();
  total += rssi_self[0];
  return total / measurements;
}

uint8_t get_rssi_leader() {
  int total = rssi_leader[measurements];

  for (int i = measurements - 2; i >= 0; i--) {
    total += rssi_leader[i];
    rssi_leader[i + 1] = rssi_leader[i];
  }
  rssi_leader[0] = WiFi.RSSI();
  total += rssi_leader[0];
  return total / measurements;
}

void processUdpPacket(byte *packet) {
  // RSSI packet
  if (packet[0] == 0) {
    mac_packet += packet[1] << 56;
    mac_packet += packet[2] << 48;
    mac_packet += packet[3] << 40;
    mac_packet += packet[4] << 32;
    mac_packet += packet[5] << 24;
    mac_packet += packet[6] << 16;
    mac_packet += packet[7] << 8;
    mac_packet += packet[8];

    if (mac_packet == leader_mac) {
      leader_rssi = packet[9];
      Serial.println("RSSI of leader: %d", leader_rssi);
    }
    
  // Leader
  } else if (packet[0] == 2) {
    uint_64_t mac = 0;
    mac += packet[1] << 56;
    mac += packet[2] << 48;
    mac += packet[3] << 40;
    mac += packet[4] << 32;
    mac += packet[5] << 24;
    mac += packet[6] << 16;
    mac += packet[7] << 8;
    mac += packet[8];

    if (mac == own_mac) {
      Serial.println("I'm the leader\n");
      leader = true;
    }

  // Control  
  } else if (packet[0] == 1) {
    Serial.println("Direction: %d", packet[1]);

  // Kill
  } else if (packet[0] == 'D') {
    Serial.println("Right");
    rotateRight();

  } else {
    Serial.println("else");
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);
  initWireless();
  pinMode(WIFILED, OUTPUT);
  digitalWrite(WIFILED, LOW);
  initWireless();
  initMotors();
  uint_64_t own_mac = ESP.getEfuseMac();
  for (int i = 0; i < 10; i++) {
    rssi_self[i] = WiFi.RSSI();
  }
}

void loop() {
  int8_t rssi_self = get_rssi_self();
  int8_t rssi_leader = get_rssi_leader();

  if 





  // driveForwards();
  // digitalWrite(WIFILED, HIGH);
  // driveForwards();
  // delay(1000);
  // stop();
  // delay(2000);
  // rotateLeft();
  // delay(1000);
  // rotateRight();
  // delay(1000);

  //data will be sent to server
  // uint8_t buffer[50] = "hello world";
  // //send hello world to server
  // if (++pingCount % pingModulus == 0)
  // {
  //     Serial.println("Pinging");
  //     pingCount = 0;
  //     Udp.beginPacket(udpAddress, udpPort);
  //     Udp.write(buffer, 11);
  //     Udp.endPacket();
  //     memset(buffer, 0, 50);
  // }

  // // processing incoming packet, must be called before reading the buffer
  // Udp.parsePacket();
  // // receive response from server, it will be HELLO WORLD
  // if (Udp.read(buffer, 50) > 0)
  // {
  //     // Serial.print("Server to client: ");
  //     // Serial.println((char *)buffer);
  //     processUdpPacket((byte *)buffer);
  //     memset(buffer, 0, 50);
  // }

  // //Wait for 10 milliseconds
  // delay(10);
}