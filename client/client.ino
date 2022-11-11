#include <WiFi.h>
#include <WiFiUdp.h>
#include <math.h>

#define DEBUG

#define LOW 0
#define HIGH 127
#define PAUSE 300
#define TIMER_DELAY 300
#define TIMER_DELAY2 1000

const char *networkName = "rssi-cars";
// #define measurements 5
#define threshold 0.2
#define distance 0.4
#define measured_power -40
#define env_val 2.5
#define filter 0.3

const int udpPort = 3333;
boolean connected = false;
boolean leader_assigned = false;
boolean leader = false;
boolean rssi_received = false;
WiFiUDP udp;
uint8_t packet[255];

#define rf1 19
#define rf2 18
#define lf1 5
#define lf2 17
#define rb1 14
#define rb2 15
#define lb1 16
#define lb2 4
const int control[4][2] = { { rf1, rf2 }, { rb1, rb2 }, { lf1, lf2 }, { lb1, lb2 } };

// int8_t rssi_leader[measurements];
// int8_t rssi_self[measurements];

int8_t rssi_leader_val = 0;
int8_t rssi_self_val = 0;

uint64_t mac_packet = 0;
uint64_t mac_leader = 0;
uint64_t mac_self = 0;

hw_timer_t *timer = NULL;
hw_timer_t *timer2 = NULL;

enum movement {
  FORWARD,
  BACK,
  LEFT,
  RIGHT,
  STOP
};

enum movement prev_state;
enum movement control_state = STOP;  // Used for control

// Packet format:
// Opcode [1 b]   | Arguments [n b]
// ---------------|----------------
// RSSI (0x04)    | Fuse MAC (8 b) RSSI value (1 b)
// Control (0x01) | Direction (1 b)
// Leader (0x02)  | Fuse MAC (8 b)
// Kill (0x03)    | (0 b)

void send_rssi() {
  uint8_t buff[10];
  buff[0] = (uint8_t)0x04;
  buff[1] = (uint8_t)(mac_self >> 56) & 0xFF;
  buff[2] = (uint8_t)(mac_self >> 48) & 0xFF;
  buff[3] = (uint8_t)(mac_self >> 40) & 0xFF;
  buff[4] = (uint8_t)(mac_self >> 32) & 0xFF;
  buff[5] = (uint8_t)(mac_self >> 24) & 0xFF;
  buff[6] = (uint8_t)(mac_self >> 16) & 0xFF;
  buff[7] = (uint8_t)(mac_self >> 8) & 0xFF;
  buff[8] = (uint8_t)(mac_self >> 0) & 0xFF;
  // buff[9] = (uint8_t) WiFi.RSSI();
  buff[9] = (uint8_t)rssi_self_val;

  udp.beginPacket(WiFi.broadcastIP(), udpPort);
  udp.write(buff, 10);
  udp.endPacket();
}

void onTimer() {
  if (timerReadMilis(timer) >= TIMER_DELAY) {
    set_rssi_self();
    send_rssi();
    timerRestart(timer);
  }
}

bool onTimer2() {
  if (timerReadMilis(timer2) >= TIMER_DELAY2) {
    timerRestart(timer2);
    return true;
  }
  return false;
}

void connectToWiFi(const char *ssid) {
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
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      //When connected set
      Serial.print("WiFi connected! IP address: ");
      Serial.println(WiFi.localIP());
      //initializes the UDP state
      //This initializes the transfer buffer
      udp.begin(WiFi.localIP(), udpPort);
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
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 2; ++j) {
      analogWrite(control[i][j], LOW);
    }
  }
}

void move_forward() {
  rotorstop();
  delay(PAUSE);
  for (int i = 0; i < 4; ++i) {
    analogWrite(control[i][0], HIGH);
    analogWrite(control[i][1], LOW);
  }
}

void move_back() {
  rotorstop();
  delay(PAUSE);
  for (int i = 0; i < 4; ++i) {
    analogWrite(control[i][0], LOW);
    analogWrite(control[i][1], HIGH);
  }
}

void turn_left() {
  rotorstop();
  delay(PAUSE);
  for (int i = 0; i < 4; ++i) {
    if (i < 2) {
      analogWrite(control[i][0], HIGH);
      analogWrite(control[i][1], LOW);
    } else {
      analogWrite(control[i][0], LOW);
      analogWrite(control[i][1], HIGH);
    }
  }
}

void turn_right() {
  rotorstop();
  delay(PAUSE);
  for (int i = 0; i < 4; ++i) {
    if (i < 2) {
      analogWrite(control[i][0], LOW);
      analogWrite(control[i][1], HIGH);
    } else {
      analogWrite(control[i][0], HIGH);
      analogWrite(control[i][1], LOW);
    }
  }
}

void set_rssi_leader(int8_t rssi) {
  // filter is in leader itself, just like done in set_rssi_self
  rssi_leader_val = rssi;
  // rssi_leader_val += 0.2 * (rssi - rssi_leader_val);
}

void set_rssi_self() {
  rssi_self_val += filter * (WiFi.RSSI() - rssi_self_val);
  // rssi_self_val = WiFi.RSSI();
}

void control_handler(uint8_t direction) {
  switch (direction) {
    case 1:  // forward
      if (control_state != FORWARD) {
        move_forward();
        control_state = FORWARD;
      }
      break;
    case 2:  // back
      if (control_state != BACK) {
        move_back();
        control_state = BACK;
      }
      break;
    case 3:  // left
      if (control_state != LEFT) {
        turn_left();
        control_state = LEFT;
      }
      break;
    case 4:  // right
      if (control_state != RIGHT) {
        turn_right();
        control_state = RIGHT;
      }
      break;
    default:  // break
      rotorstop();
      control_state = STOP;
      break;
  }
}

void rssi_handler(uint64_t mac, int8_t rssi) {
  if (mac == mac_leader) {
    set_rssi_leader(rssi);
    rssi_received = true;
  }
}

void leader_handler(uint64_t mac) {
  mac_leader = mac;
  if (mac_leader == mac_self) {
    leader = true;
  } else {
    leader = false;
  }
  leader_assigned = true;
}

void packet_handler(uint8_t *packet) {
  // Change the char of the packet to a byte for easier handling
  uint8_t op = (uint8_t)packet[0];
  switch (op) {
    case 1:  // control
      {
        uint8_t direction = (uint8_t)packet[1];
        if (leader) {
          control_handler(direction);
        }
        break;
      }
    case 2:  // Leader
      {
        mac_packet = 0;
        mac_packet += (uint64_t)packet[1] << 56;
        mac_packet += (uint64_t)packet[2] << 48;
        mac_packet += (uint64_t)packet[3] << 40;
        mac_packet += (uint64_t)packet[4] << 32;
        mac_packet += (uint64_t)packet[5] << 24;
        mac_packet += (uint64_t)packet[6] << 16;
        mac_packet += (uint64_t)packet[7] << 8;
        mac_packet += (uint64_t)packet[8];

#ifdef DEBUG
        Serial.print("Leader: ");
        Serial.println(mac_packet);
#endif

        leader_handler(mac_packet);
        break;
      }
    case 3:  // KILL
      {
        rotorstop();
        leader_assigned = false;
        leader = false;
        rssi_received = false;
      }
    case 4:  // RSSI
      {
        mac_packet = 0;
        mac_packet += (uint64_t)packet[1] << 56;
        mac_packet += (uint64_t)packet[2] << 48;
        mac_packet += (uint64_t)packet[3] << 40;
        mac_packet += (uint64_t)packet[4] << 32;
        mac_packet += (uint64_t)packet[5] << 24;
        mac_packet += (uint64_t)packet[6] << 16;
        mac_packet += (uint64_t)packet[7] << 8;
        mac_packet += (uint64_t)packet[8];
        int8_t rssi = (int8_t)packet[9];
        // #ifdef DEBUG
        // Serial.print("RSSI packet received from: ");
        // Serial.print(mac_packet);
        // Serial.print(", RSSI: ");
        // Serial.println(rssi);
        // #endif

        rssi_handler(mac_packet, rssi);
        break;
      }
    default:
      break;
  }
}

float get_distance_to_leader() {
  // distance = 10^((Measured Power - Instant RSSI)/10*N)
  float leader_to_ap = 0.4*pow(10, ((measured_power - (float) rssi_leader_val) / (10 * env_val)));
  float follower_to_ap = 0.4*pow(10, ((measured_power - (float) rssi_self_val) / (10 * env_val)));

#ifdef DEBUG
  Serial.print("L: ");
  Serial.print(leader_to_ap);
  Serial.print(", F: ");
  Serial.print(follower_to_ap);
  Serial.print(", D: ");
  Serial.println(abs(follower_to_ap - leader_to_ap));
#endif

  return abs(follower_to_ap - leader_to_ap);
}

void setup() {
  init_pins();

  // Initilize hardware serial:
  Serial.begin(115200);
  mac_self = ESP.getEfuseMac();
  Serial.print("My MAC: ");
  Serial.println(mac_self);


  //Connect to the WiFi network
  connectToWiFi(networkName);
#ifdef DEBUG
// leader_assigned = true;
// leader = true;
#endif
  Serial.println("Starting timer...");
  timer = timerBegin(0, 80, true);
  timerStart(timer);
  Serial.println("Timer started!");

  Serial.println("Starting timer2...");
  timer2 = timerBegin(1, 80, true);
  timerStart(timer2);
  Serial.println("Timer2 started!");


  rssi_self_val = WiFi.RSSI();
}

void follow_leader(int distance_delta) {
  if (distance_delta > distance + threshold) {
    if (prev_state != FORWARD) {
      move_forward();
      prev_state = FORWARD;
    }
  }

  else if (distance_delta < distance - threshold) {
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

void loop() {
  onTimer();
  //only send data when connected

  int packetSize = udp.parsePacket();
  if (packetSize) {
// #ifdef DEBUG
//     Serial.print("Received packet! Size: ");
//     Serial.println(packetSize);
// #endif
    int len = udp.read(packet, 255);
    if (len > 0) {
      packet[len] = '\0';
    }
    // for (int i = 1; i < 9; i++) {
    //   Serial.print(packet[i]);
    // }
    // Serial.println();
    packet_handler(packet);
  }

  //only send data when connected
  if (!connected || !leader_assigned) {
    if (onTimer2()) {
      Serial.print("Connection: ");
      Serial.print(connected);
      Serial.print(" Leader assigned: ");
      Serial.println(leader_assigned);
    }
    return;
  }

  // If this robot is the follower and we have received our initial RSSI value
  if (!leader && rssi_received && leader_assigned && onTimer2()) {
    // if (!leader && rssi_received && leader_assigned) {

    float distance_delta = get_distance_to_leader();


    // #ifdef DEBUG
    // Serial.print("Distance to leader: ");
    // Serial.println(distance_delta);
    // #endif

    // if (distance_delta > distance + threshold) {
    //   if (prev_state != FORWARD) {
    //     move_forward();
    //     prev_state = FORWARD;
    //   }
    // }

    // else if (distance_delta < distance + threshold) {
    //   if (prev_state != BACK) {
    //     move_back();
    //     prev_state = BACK;
    //   }
    // }

    // else {
    //   if (prev_state != STOP) {
    //     rotorstop();
    //     prev_state = STOP;
    //   }
    // }
  }

  if (leader_assigned && onTimer2()) {
    float distance_delta = get_distance_to_leader();
    if (!leader) {
      follow_leader(distance_delta);
    }
  }
}