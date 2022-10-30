#include <EEPROM.h>
#include "PacketSerial.h"

#define STARTBYTE 0xFF
#define PACKETSIZE 254
#define MINPACKET 8
#define TCP 1
#define ADHOC 0
#define FWD 0
#define ACK 1
#define ADDRESS 0
#define MOTOR_FRONT_LEFT_P 35
#define MOTOR_FRONT_LEFT_N 33
#define MOTOR_BACK_LEFT_P 27
#define MOTOR_BACK_LEFT_N 25
#define MOTOR_FRONT_RIGHT_P 39
#define MOTOR_FRONT_RIGHT_N 37
#define MOTOR_BACK_RIGHT_P 31
#define MOTOR_BACK_RIGHT_N 29
#define ULTRASONIC_FRONT_TRIGGER 43
#define ULTRASONIC_FRONT_ECHO 41

#define REDLED A11
#define WHITELED 44
#define YELLOWLED A12
#define ORANGELED 42

#define TIMER1COUNT 64286 //50Hz

// External commands, communicated with another robot (in Adhoc mode) or TCP
#define NOCOMMAND 0
#define MOVEFORWARD 0x01
#define MOVEFORWARDTIME 0x02
#define MOVEBACK 0x03
#define MOVEBACKTIME 0x04
#define TURNLEFT 0x05
#define TURNRIGHT 0x06
#define STOP 0x07
#define DISTANCEFRONT 0x0A
#define GETHEADING 0x0D
#define GETID 0x0F

// Internal commands, communicated with ESP32
#define INT_ID 0x01
#define INT_SSID_PWD 0x02
#define INT_MATRIX 0x03
#define INT_RSSI 0x04
#define INT_IP 0x05
#define INT_DEMO 0x06

#define NODECOUNT 16

//Matrix - Robot ID 0 to ID 15
uint8_t matrix[NODECOUNT][NODECOUNT] = {{0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1},
                                        {1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0},
                                        {1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1},
                                        {0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0},
                                        {1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1},
                                        {1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0},
                                        {0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1},
                                        {0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0},
                                        {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0},
                                        {1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0},
                                        {0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1},
                                        {0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0},
                                        {0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1},
                                        {1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0},
                                        {0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1},
                                        {0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0}};
char ssid[] = "ES";
char password[] = "passcode123";
char ip[] = {192, 168, 43, 131};

uint8_t Command = 0;
long Rssi = 0;
unsigned long distance = 0;

uint8_t nodeID = 0;
uint8_t movementTime = 0;
uint16_t tempMovementTime = 0;

uint16_t PacketCounter = 0;
long RSSI_Value = 0;

PacketSerial packetSerial;

uint8_t dat[2];

//Handle commands. USER CAN ADD MODE COMMANDS IF NECESSARY
void handleCommands(uint8_t src, uint8_t dst, uint8_t internal, uint8_t tcp, uint8_t fwd, uint8_t counterH, uint8_t counterL, uint8_t datalen, uint8_t command, uint8_t *data)
{
  uint8_t tempData[32] = {0};
  data = data + 1;
  switch (command)
  {
  case MOVEFORWARD:
    Command = MOVEFORWARD;
    moveForward();
    break;

  case MOVEFORWARDTIME:
    moveForwardForTime(*data);
    break;

  case MOVEBACK:
    Command = MOVEBACK;
    moveBack();
    break;

  case MOVEBACKTIME:
    moveBackForTime(*data);
    break;

  case STOP:
    Command = STOP;
    stopMotors();
    break;

  case TURNLEFT:
    turnLeft(*data);
    break;

  case TURNRIGHT:
    turnRight(*data);
    break;

  case DISTANCEFRONT:
    distance = getDistanceFront();
    if (distance > 254)
    {
      distance = 254;
    }
    tempData[0] = command;
    tempData[1] = distance & 0xFF;
    tempData[2] = 0;
    sendPacket(dst, src, internal, tcp, ACK, counterH, counterL, 2, tempData);
    break;

  case GETHEADING:
    break;

  case GETID:
    nodeID = getID();
    tempData[0] = command;
    tempData[1] = nodeID;
    sendPacket(dst, src, internal, tcp, ACK, counterH, counterL, 2, tempData);
    break;
  }
}

//Timer 1 interrupt service routine
ISR(TIMER1_OVF_vect)
{

  long time = millis();

  switch (Command)
  {

  case MOVEFORWARDTIME:
  case TURNLEFT:
  case TURNRIGHT:
  case MOVEBACKTIME:
    if (((uint16_t)(millis() / 1000) - tempMovementTime) >= movementTime)
    {
      stopMotors();
      Command = NOCOMMAND;
    }
    break;
  }

  TCNT1 = TIMER1COUNT;
}

void initGPIO()
{
  pinMode(MOTOR_FRONT_LEFT_P, OUTPUT);
  pinMode(MOTOR_FRONT_LEFT_N, OUTPUT);
  pinMode(MOTOR_BACK_LEFT_P, OUTPUT);
  pinMode(MOTOR_BACK_LEFT_N, OUTPUT);
  pinMode(MOTOR_FRONT_RIGHT_P, OUTPUT);
  pinMode(MOTOR_FRONT_RIGHT_N, OUTPUT);
  pinMode(MOTOR_BACK_RIGHT_P, OUTPUT);
  pinMode(MOTOR_BACK_RIGHT_N, OUTPUT);
  pinMode(REDLED, OUTPUT);
  pinMode(WHITELED, OUTPUT);
  pinMode(YELLOWLED, OUTPUT);
  pinMode(ORANGELED, OUTPUT);

  stopMotors();

  pinMode(ULTRASONIC_FRONT_TRIGGER, OUTPUT);
  pinMode(ULTRASONIC_FRONT_ECHO, INPUT);
}

void initTimer()
{
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;

  TCNT1 = TIMER1COUNT;
  TCCR1B |= (1 << CS12);
  TIMSK1 |= (1 << TOIE1);
  interrupts();
}

//Set LEDs
void setLED(uint8_t led, bool state)
{
  if (state)
  {
    digitalWrite(led, HIGH);
  }
  else
  {
    digitalWrite(led, LOW);
  }
}

//Get distance in cm from front ultrasonic sensor (In blocking mode)
uint8_t getDistanceFront()
{
  digitalWrite(ULTRASONIC_FRONT_TRIGGER, LOW);
  delayMicroseconds(2);

  digitalWrite(ULTRASONIC_FRONT_TRIGGER, HIGH);
  delayMicroseconds(10);

  digitalWrite(ULTRASONIC_FRONT_TRIGGER, LOW);
  long duration = pulseIn(ULTRASONIC_FRONT_ECHO, HIGH);

  return duration / 58.2;
}

//Move forward
void moveForward()
{
  stopMotors();
  digitalWrite(MOTOR_FRONT_LEFT_P, HIGH);
  digitalWrite(MOTOR_FRONT_LEFT_N, LOW);

  digitalWrite(MOTOR_BACK_LEFT_P, HIGH);
  digitalWrite(MOTOR_BACK_LEFT_N, LOW);

  digitalWrite(MOTOR_FRONT_RIGHT_P, HIGH);
  digitalWrite(MOTOR_FRONT_RIGHT_N, LOW);

  digitalWrite(MOTOR_BACK_RIGHT_P, HIGH);
  digitalWrite(MOTOR_BACK_RIGHT_N, LOW);
}

//Stop movement
void stopMotors()
{
  digitalWrite(MOTOR_FRONT_LEFT_P, LOW);
  digitalWrite(MOTOR_FRONT_LEFT_N, LOW);

  digitalWrite(MOTOR_BACK_LEFT_P, LOW);
  digitalWrite(MOTOR_BACK_LEFT_N, LOW);

  digitalWrite(MOTOR_FRONT_RIGHT_P, LOW);
  digitalWrite(MOTOR_FRONT_RIGHT_N, LOW);

  digitalWrite(MOTOR_BACK_RIGHT_P, LOW);
  digitalWrite(MOTOR_BACK_RIGHT_N, LOW);

  delay(200);
}

//Move forward for specific time (in seconds)
void moveForwardForTime(uint8_t data)
{
  moveForward();
  movementTime = data;
  tempMovementTime = (uint16_t)(millis() / 1000);
  Command = MOVEFORWARDTIME;
}

//Move back for specific time (in seconds)
void moveBackForTime(uint8_t data)
{
  moveBack();
  movementTime = data;
  tempMovementTime = (uint16_t)(millis() / 1000);
  Command = MOVEBACKTIME;
}

//Move back
void moveBack()
{
  stopMotors();
  digitalWrite(MOTOR_FRONT_LEFT_P, LOW);
  digitalWrite(MOTOR_FRONT_LEFT_N, HIGH);

  digitalWrite(MOTOR_BACK_LEFT_P, LOW);
  digitalWrite(MOTOR_BACK_LEFT_N, HIGH);

  digitalWrite(MOTOR_FRONT_RIGHT_P, LOW);
  digitalWrite(MOTOR_FRONT_RIGHT_N, HIGH);

  digitalWrite(MOTOR_BACK_RIGHT_P, LOW);
  digitalWrite(MOTOR_BACK_RIGHT_N, HIGH);
}

//Turn left for specific time (in seconds)
void turnLeft(uint8_t data)
{
  stopMotors();
  digitalWrite(MOTOR_FRONT_LEFT_P, LOW);
  digitalWrite(MOTOR_FRONT_LEFT_N, HIGH);

  digitalWrite(MOTOR_BACK_LEFT_P, LOW);
  digitalWrite(MOTOR_BACK_LEFT_N, HIGH);

  digitalWrite(MOTOR_FRONT_RIGHT_P, HIGH);
  digitalWrite(MOTOR_FRONT_RIGHT_N, LOW);

  digitalWrite(MOTOR_BACK_RIGHT_P, HIGH);
  digitalWrite(MOTOR_BACK_RIGHT_N, LOW);
  movementTime = data;
  tempMovementTime = (uint16_t)(millis() / 1000);
  Command = TURNLEFT;
}

//Turn right for specific time (in seconds)
void turnRight(uint8_t data)
{
  stopMotors();
  digitalWrite(MOTOR_FRONT_LEFT_P, HIGH);
  digitalWrite(MOTOR_FRONT_LEFT_N, LOW);

  digitalWrite(MOTOR_BACK_LEFT_P, HIGH);
  digitalWrite(MOTOR_BACK_LEFT_N, LOW);

  digitalWrite(MOTOR_FRONT_RIGHT_P, LOW);
  digitalWrite(MOTOR_FRONT_RIGHT_N, HIGH);

  digitalWrite(MOTOR_BACK_RIGHT_P, LOW);
  digitalWrite(MOTOR_BACK_RIGHT_N, HIGH);
  movementTime = data;
  tempMovementTime = (uint16_t)(millis() / 1000);
  Command = TURNRIGHT;
}

//Get RSSI from ESP32
void getRSSI()
{
  uint8_t data;
  sendPacket(nodeID, nodeID, INT_RSSI, TCP, FWD, 0, 0, 0, &data);
  //RSSI value is updated in RSS_Value variable as soon as there is reply from ESP32. This is implemented in OnPacket() function
}

//This is internal API used to enable demo mode in ESP32. Demo mode should be enabled in all the robots to make it work
void enableDemo()
{
  uint8_t data;
  sendPacket(nodeID, nodeID, INT_DEMO, TCP, FWD, 0, 0, 0, &data);
}

//Get ID of robot
uint8_t getID()
{
  return EEPROM.read(ADDRESS);
}

//Set ID of robot
void setID(uint8_t ID)
{
  EEPROM.write(ADDRESS, ID);
  delay(50);
  nodeID = getID();
}

//Send ID of robot to ESP32
void sendID()
{
  nodeID = getID();
  sendPacket(nodeID, nodeID, INT_ID, TCP, FWD, 0, 0, 1, &nodeID);
}

//Send connection matrix to ESP32
void sendMatrix()
{
  nodeID = getID();
  sendPacket(nodeID, nodeID, INT_MATRIX, TCP, FWD, 0, 0, NODECOUNT, (uint8_t *)matrix[nodeID]);
}

//Send IP address of server to ESP32
void sendIP()
{
  sendPacket(nodeID, nodeID, INT_IP, TCP, FWD, 0, 0, sizeof(ip), ip);
}

//Send AP SSID and password to ESP32
void sendSSIDandPassword()
{
  char *ssid_pwd = (char *)calloc(strlen(ssid) + strlen(password) + 2, sizeof(char));
  strcpy(ssid_pwd, ssid);
  int delimiterLoc = strlen(ssid);
  ssid_pwd[delimiterLoc] = 0xA9;
  strcat(ssid_pwd, password);
  sendPacket(nodeID, nodeID, INT_SSID_PWD, TCP, FWD, 0, 0, strlen(ssid_pwd), ssid_pwd);
  free(ssid_pwd);
}

//This function is called when data is received from serial port (from PacketSerial library)
void onPacket(const uint8_t *buffer, size_t size)
{
  uint8_t src, dst, internal, tcp, fwd, counterH, counterL, datalen, command, *data;
  nodeID = getID();
  if ((buffer[0] != STARTBYTE))
  {
    return;
  }
  if (size < 7)
  {
    return;
  }
  src = (buffer[1] >> 4) & 0x0F;
  dst = buffer[1] & 0x0F;
  internal = (buffer[3] >> 5) & 0x07;
  tcp = (buffer[3] >> 4) & 0x01;
  fwd = (buffer[3] >> 3) & 0x01;
  counterH = buffer[4];
  counterL = buffer[5];
  datalen = buffer[6];
  command = buffer[7];
  data = (buffer + 7);

  // Checksum is not calculated. Can be implemented if necessary

  //Check if the command is internal, especially get RSSI from ESP32
  if (internal == INT_RSSI)
  {
    if (datalen != 5)
    {
      return;
    }
    //Update RSSI_Value variable with latest RSSI
    RSSI_Value = (long)(buffer[8] << 24) | (long)(buffer[9] << 16) | (long)(buffer[10] << 8) | (long)(buffer[11]);
    uint8_t temp[5];
    temp[0] = 0x08;
    temp[1] = RSSI_Value >> 24;
    temp[2] = RSSI_Value >> 16;
    temp[3] = RSSI_Value >> 8;
    temp[4] = RSSI_Value;
    //sendPacket(0, 0, 0, TCP, ACK, counterH, counterL, 5, temp);
  }
  else if (internal == INT_ID)
  {
    sendID();
  }
  else if (internal == INT_MATRIX)
  {
    sendMatrix();
  }
  else if (internal == INT_SSID_PWD)
  {
    sendSSIDandPassword();
  }
  else if (internal == INT_IP)
  {
    sendIP();
  }

  //Call callback function
  OnReceive(src, dst, internal, tcp, fwd, counterH, counterL, datalen, command, data);
}

//For internal use only
void sendPacket(uint8_t src, uint8_t dst, uint8_t internal, uint8_t isTCP, uint8_t isACK, uint8_t counterHigh, uint8_t counterLow, uint8_t dataLength, uint8_t *data)
{
  uint8_t packet[PACKETSIZE] = {0};
  int index = 0;
  uint8_t checksum = 0;
  nodeID = getID();

  packet[0] = STARTBYTE;
  packet[1] = (src << 4) | (dst & 0x0F);
  packet[2] = nodeID << 4;
  packet[3] = (internal << 5) | (isTCP << 4) | (isACK << 3);
  packet[4] = counterHigh;
  packet[5] = counterLow;
  packet[6] = dataLength;
  for (index = 0; index < dataLength; index++)
  {
    packet[7 + index] = data[index];
  }
  // packet[7+index] = checksum;  // Checksum is not calculated. Can be implemented if necessary
  packetSerial.send(packet, 7 + index);
}

//Initial setup
void setup()
{
  Serial.begin(115200);
  //setID(4);
  nodeID = getID();
  packetSerial.setPacketHandler(&onPacket);
  packetSerial.begin(115200);
  initGPIO();
  initTimer();
  delay(2000);
  sendID();
  delay(1000);
  sendMatrix();
  delay(1000);
  sendIP();
  delay(1000);
  sendSSIDandPassword();

  /*
setLED(WHITELED,true);
setLED(YELLOWLED,true);
setLED(REDLED,true);
setLED(ORANGELED,true); 

  moveForward();
 */
}

bool once = true;
void loop()
{
  packetSerial.update();
}

/** USER FUNCTION FOR AD-HOC NETWORKS COURSE. Create function and send over WiFi network
src -> ID of robot. Send nodeID variable here (This is don't care for TCP packet)
dst -> ID of robot to which you want to send the packet (This is don't care for TCP packet)
isTCP -> Set with macro TCP or ADHOC depending where you want to send
dataLength -> Length of data
data -> Data that has to be sent. The first byte is COMMAND and subsequent bytes are arguments
**/
void CreatePacket(uint8_t src, uint8_t dst, uint8_t isTCP, uint8_t dataLength, uint8_t *data)
{
  PacketCounter++;
  uint8_t counterLow = PacketCounter & 0xFF;
  uint8_t counterHigh = (PacketCounter >> 8) & 0xFF;
  sendPacket(src, dst, 0x00, isTCP, FWD, counterHigh, counterLow, dataLength, data);
}

/** USER FUNCTION FOR AD-HOC NETWORKS COURSE. This function is called when a packet is received from TCP or AD-HOC node
//void onPacket(const uint8_t* buffer, size_t size) calls this function after parsing the packet.
**/
void OnReceive(uint8_t src, uint8_t dst, uint8_t internal, uint8_t tcp, uint8_t fwd, uint8_t counterH, uint8_t counterL, uint8_t datalen, uint8_t command, uint8_t *data)
{
  //Execute commands if the command is from TCP OR if ID is equal to destination (in Ad-hoc mode)
  if (tcp == TCP || ((tcp == ADHOC) && (nodeID == dst)))
  {
    handleCommands(src, dst, internal, tcp, fwd, counterH, counterL, datalen, command, data);
  }
}
