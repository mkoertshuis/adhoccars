#include "ESP32Adhoc.h"
#include "PacketSerial.h"
#include <WiFi.h>
#include <IPAddress.h>
#include <WiFiUdp.h>

#define WIFILED 22
#define BLELED 23
#define REDLED 2

#define __DEBUG__ 1

ESP32Adhoc Adhoc;
PacketSerial serial;
int TCPport = 5555;
int UDPport = 5000;

WiFiUDP Udp;
WiFiClient client;
int TCPBytes = 0;
bool condatareceived = false;
hw_timer_t *timer = NULL;
bool BLEDState = false;
bool WLEDState = false;
bool REDLEDState = false;
bool BlinkWLED = false;
bool BlinkBLED = false;
bool BlinkREDLED = false;

enum LEDSTATE
{
  OFF,
  ON,
  BLINK
} ledState;
void setLED(uint8_t LED, LEDSTATE state);

char packetBuffer[1460];
long count = 0;

const byte numChars = 255;
char receivedChars[numChars];
boolean newData = false;

char *tcpBuffer;
uint8_t dummy[] = {0x00, 0x00, 0x00, 0x00, 0x00};

void set_id(char *data)
{
  Adhoc.ID_SELF = *data;
}

/* Request connection matrix
   with length = 0
*/
void request_matrix()
{
  int l, i, j;
  char checksum;
  char *packet;
  packet = (char *)calloc(8, sizeof(char));
  packet[0] = 0xFF;
  packet[1] = 0x00;
  packet[2] = 0x00;
  /* Set internal command to request matrix */
  packet[3] = 0b011 << 5 | 0b0 << 4 | 0b0 << 3 | 0b000;
  packet[4] = 0x00;
  packet[5] = 0x01;
  packet[6] = 0x00;
  checksum = packet[0];
  for (i = 1; i < 7; i++)
  {
    checksum = checksum ^ packet[i];
  }
  packet[7] = checksum;
#ifdef __DEBUG__
  Serial.println("Sending internal command to request matrix");
#endif
  sendToArduino(packet, 8);
}

/*
   Request
   with length = 0
*/
void request_server_ip()
{
  int l, i, j;
  char checksum;
  char *packet;
  packet = (char *)calloc(8, sizeof(char));
  packet[0] = 0xFF;
  packet[1] = 0x00;
  packet[2] = 0x00;
  /* Set internal command to request server IP */
  packet[3] = 0b101 << 5 | 0b0 << 4 | 0b0 << 3 | 0b000;
  packet[4] = 0x00;
  packet[5] = 0x01;
  packet[6] = 0x00;
  checksum = packet[0];
  for (i = 1; i < 7; i++)
  {
    checksum = checksum ^ packet[i];
  }
  packet[7] = checksum;
#ifdef __DEBUG__
  Serial.println("Sending internal command to request Server IP");
#endif
  sendToArduino(packet, 8);
}

/*
   Request ID of the bot
   with length = 0
*/
void requestID()
{
  int l, i, j;
  char checksum;
  char *packet;
  packet = (char *)calloc(8, sizeof(char));
  packet[0] = 0xFF;
  packet[1] = 0x00;
  packet[2] = 0x00;
  packet[3] = 0b001 << 5 | 0b0 << 4 | 0b0 << 3 | 0b000;
  packet[4] = 0x00;
  packet[5] = 0x01;
  packet[6] = 0x00;
  checksum = packet[0];
  for (i = 1; i < 7; i++)
  {
    checksum = checksum ^ packet[i];
  }
  packet[7] = checksum;
#ifdef __DEBUG__
  Serial.println("Sending internal command to request ID");
#endif
  sendToArduino(packet, 8);
}

/*
   Request Wifi SSID and
   password
   with length = 0
*/
void requestWifi()
{
  int l, i, j;
  char checksum;
  char *packet;
  packet = (char *)calloc(8, sizeof(char));
  packet[0] = 0xFF;
  packet[1] = 0x00;
  packet[2] = 0x00;
  packet[3] = 0b010 << 5 | 0b0 << 4 | 0b0 << 3 | 0b000;
  packet[4] = 0x00;
  packet[5] = 0x00;
  packet[6] = 0x00;
  checksum = packet[0];
  for (i = 1; i < 7; i++)
  {
    checksum = checksum ^ packet[i];
  }
  packet[7] = checksum;
#ifdef __DEBUG__
  Serial.println("Sending internal command to request Wifi");
#endif
  sendToArduino(packet, 8);
}

/* Function to send packet to Arduino
    over serial interface
*/
void sendToArduino(char packet[], int numBytes)
{

  uint8_t tmp[numBytes];
  memcpy(tmp, packet, numBytes);

  serial.send(dummy, sizeof(dummy));
  serial.send(tmp, numBytes);
}

void adhoc_recv_filter(char packet[])
{
  /* if self == dst -> forward the packet to arduino
     Enter in the table if no entry present
     Send ACK
  */
  char src = Adhoc.get_src(packet);
  char dst = Adhoc.get_dst(packet);
  char inter = Adhoc.get_inter(packet);
  unsigned int counter = Adhoc.get_counter(packet);
  char ACK = Adhoc.is_ACK(packet);

  if (src == Adhoc.ID_SELF)
  {

    /* should not receive from myself */
#ifdef __DEBUG__
    Serial.println("Receving from self ignored");
#endif
    return;
  }

  /* Drop the packet if you cannot
     receive from this source
  */
  if (!Adhoc.can_rcv(inter))
  {
    /* drop the packet */
#ifdef __DEBUG__
    Serial.print("Cannot receive UDP from robot");
    Serial.print(inter, DEC);
    Serial.println("because of matrix settings. Hence packet is ignored ");
#endif
    return;
  }

  /* Chck if self is the destination
  */
  if (Adhoc.ID_SELF == dst)
  {

#ifdef __DEBUG__
    Serial.print("I am the destination ");
#endif

    /* Check if entry for the packet already present
        Ignore if already present
    */

    if (Adhoc.is_record_present(counter, src, dst))
    {
      int counter_index = Adhoc.get_table_index(counter);
#ifdef __DEBUG__
      Serial.print("Getting index value for counter ");
      Serial.println(counter, DEC);
      Serial.print("Found index : ");
      Serial.println(counter_index, DEC);
#endif

      if (Adhoc.fwd_in_table(counter_index) && !Adhoc.ack_in_table(counter_index) && ACK)
      {
        /* Forward set in table and ACK not set in table and
            ACK in packet means

            I received the ACK for the packet i previously
            sent accept it
        */
#ifdef __DEBUG__
        Serial.print("Received ACK for counter : ");
        Serial.println(counter);
        Serial.print("From source : ");
        Serial.println(src, DEC);
#endif
        /* Send to arduino */
        return;
      }

      if (Adhoc.fwd_in_table(counter_index) && !Adhoc.ack_in_table(counter_index) && !ACK)
      {
        /* Forward in table set , ACK in table not set
            and Forward in packet
            Then I have to ignore as i have already
            received it.
        */
#ifdef __DEBUG__
        Serial.print("Already received this packet for counter : ");
        Serial.println(counter);
        Serial.print("From inter source : ");
        Serial.println(inter, DEC);
        Serial.println("Ignoring it");
#endif
        /* Ignore */
        return;
      }
    }
    else
    {
      /* Make an entry in table
          Send to arduino over serial
          SEND ACK by reversing SRC and DST in
          same packet
      */
      if (Adhoc.store_record(packet))
      {
#ifdef __DEBUG__
        Serial.print("Counter could not be stored in table.");
#endif
      }
      else
      {
#ifdef __DEBUG__
        Serial.print("Counter stored");
#endif
      }
      //Serial.write(packet,sizeof(packet));

      //Send to Arduino
#ifdef __DEBUG__
      Serial.println("Sending UDP packet to Arduino");
#endif
      sendToArduino(packet, Adhoc.recvBytes);

      /* Reverse source and destination */
      packet[1] = (dst << 4 | src);

      /* TBD dont change the values directly */
      /* mAKE inter as self */
      packet[2] = Adhoc.ID_SELF << 4;

      /* Set ACK bit */
      packet[3] |= (1 << 3);

#ifdef __DEBUG__

      Serial.print("Stored the record for counter : ");
      Serial.println(counter);
      Adhoc.print_packet(packet, 9);
      Serial.println("Sending ACK ");
#endif
      if (Udp.writeUDP((byte *)packet, Adhoc.recvBytes) <= 0)
      {
#ifdef __DEBUG__
        Serial.println("UDP send failed");
#endif
      }
      return;
    }
  }

  /* If SELF is not the destination
      then make intermediate source == SELF and broadcast
      again.

  */

#ifdef __DEBUG__
  Serial.print("Checking if record for counter : ");
  Serial.print(counter);
  Serial.print(" present ");
  Serial.println(" && ACK ");
#endif

  if (!Adhoc.is_record_present(counter, src, dst) && !ACK)
  {

#ifdef __DEBUG__
    Serial.print("Record not present for counter : ");
    Serial.print(counter);
    Serial.print(" Storing it ");
    Serial.println("");
    Serial.print("Changing inter to SELF ");
    Serial.print(Adhoc.ID_SELF);
    Serial.println("");
    Serial.println("Broadcasting again");
#endif

    if (Adhoc.store_record(packet))
    {
#ifdef __DEBUG__
      Serial.print("Counter could not be stored in table.");
#endif
    }
    else
    {
#ifdef __DEBUG__
      Serial.print("Counter stored");
#endif
    }
    packet[2] = Adhoc.ID_SELF << 4;
    if (Udp.writeUDP((byte *)packet, Adhoc.recvBytes) <= 0)
    {
#ifdef __DEBUG__
      Serial.println("UDP send failed");
#endif
    }
    return;
  }

  /* Record present in counter and ACK in packet */

#ifdef __DEBUG__
  Serial.print("Checking if record for counter : ");
  Serial.print(counter);
  Serial.print(" && ACK in packet ");
  Serial.println("");
#endif

  if (Adhoc.is_record_present(counter, src, dst) && ACK)
  {

    /* Check in table if ACK is set for this
        counter
    */

    int counter_index = Adhoc.get_table_index(counter);

    if (Adhoc.ack_in_table(counter_index))
    {
      //ignore it
#ifdef __DEBUG__
      Serial.print("Already received ACK for counter : ");
      Serial.print(counter);
      Serial.print(" Ignoring packet ");
      Serial.println("");
#endif
      return;
    }
    else
    {
      /* set ACK bit in table
         and broadcast the ACK
      */
#ifdef __DEBUG__
      Serial.print("Received ACK for counter : ");
      Serial.print(counter);
      Serial.print(" Setting ACK bit in table ");
      Serial.println("");
      Serial.println("Broadcasting again");
#endif
      Adhoc.set_ack_table(counter_index);
      packet[2] = Adhoc.ID_SELF << 4;
      if (Udp.writeUDP((byte *)packet, Adhoc.recvBytes) <= 0)
      {
#ifdef __DEBUG__
        Serial.println("UDP send failed");
#endif
      }
      return;
    }
  }
}

void adhoc_send_filter(char packet[], int numBytes)
{
  /* Store the details about
     packet in the table
  */
  if (Adhoc.store_record(packet))
  {
#ifdef __DEBUG__
    Serial.println("Record stored before broadcast");
    Adhoc.print_record(Adhoc.record_count - 1);
#endif
  }
  else
  {
#ifdef __DEBUG__
    Serial.println("Record could not be stored.");
    Adhoc.print_record(Adhoc.record_count - 1);
#endif
  }

  /* Broacast the packet */
  Serial.println("Broadcasting again");
  if (Udp.writeUDP((byte *)packet, numBytes) <= 0)
  {
#ifdef __DEBUG__
    Serial.println("Broadcasting failed");
#endif
  }
}

//Timer

void IRAM_ATTR onTimer()
{
  if (BlinkWLED)
  {
    if (WLEDState)
    {
      digitalWrite(WIFILED, HIGH);
    }
    else
    {
      digitalWrite(WIFILED, LOW);
    }
    WLEDState = !WLEDState;
  }
  if (BlinkBLED)
  {
    if (BLEDState)
    {
      digitalWrite(BLELED, HIGH);
    }
    else
    {
      digitalWrite(BLELED, LOW);
    }
    BLEDState = !BLEDState;
  }
  if (BlinkREDLED)
  {
    if (REDLEDState)
    {
      digitalWrite(REDLED, HIGH);
    }
    else
    {
      digitalWrite(REDLED, LOW);
    }
    REDLEDState = !REDLEDState;
  }
}

void setLED(uint8_t led, LEDSTATE state)
{
  switch (led)
  {
  case WIFILED:
    if (state == ON)
    {
      BlinkWLED = false;
      digitalWrite(WIFILED, HIGH);
    }
    else if (state == OFF)
    {
      BlinkWLED = false;
      digitalWrite(WIFILED, LOW);
    }
    else if (state == BLINK)
    {
      BlinkWLED = true;
    }
    break;
  case BLELED:
    if (state == ON)
    {
      BlinkBLED = false;
      digitalWrite(BLELED, HIGH);
    }
    else if (state == OFF)
    {
      BlinkBLED = false;
      digitalWrite(BLELED, LOW);
    }
    else if (state == BLINK)
    {
      BlinkBLED = true;
    }
    break;
  case REDLED:
    if (state == ON)
    {
      BlinkREDLED = false;
      digitalWrite(REDLED, HIGH);
    }
    else if (state == OFF)
    {
      BlinkREDLED = false;
      digitalWrite(REDLED, LOW);
    }
    else if (state == BLINK)
    {
      BlinkREDLED = true;
    }
    break;
  default:
    break;
  }
}

void init_routine()
{
  /* Send some dummy data to Arduino */
  //sendToArduino(dummy,sizeof(dummy));
  delay(1000);
  /* Request Arduino to provide ID */
  requestID();
  delay(1000);
  /* Request Arduino to provide Connection matrix for the bot */
  request_matrix();
  delay(1000);
  /* Request Arduino to provide server ip address */
  request_server_ip();
  delay(1000);
  /* Request Arduino to provide SSID and password */
  requestWifi();
  delay(1000);
}

void setup()
{

  Serial.begin(115200);

  serial.begin(115200, 2);
  serial.setPacketHandler(&onPacket);
  delay(10);

#ifdef __DEBUG__
  Serial.println("Booting ESP module");
#endif

  init_routine();

  pinMode(BLELED, OUTPUT);
  pinMode(WIFILED, OUTPUT);
  pinMode(REDLED, OUTPUT);
  digitalWrite(BLELED, LOW);
  digitalWrite(WIFILED, LOW);
  digitalWrite(REDLED, LOW);

  //Setup timer
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

#ifdef __DEBUG__
  Serial.println("Setup done ... ");
#endif
}

void loop()
{

  int noBytes = Udp.readUDP(packetBuffer);

  Adhoc.recvBytes = noBytes;

  if (noBytes > 0)
  {
    // Read the data from it
#ifdef __DEBUG__
    Serial.println("Received bytes from UDP");
    Serial.print("Bytes count : ");
    Serial.println(noBytes, DEC);
#endif
    Adhoc.print_packet(packetBuffer, noBytes);
    Adhoc.recvBytes = noBytes;
    adhoc_recv_filter(packetBuffer);

  } // end if
  recvWithStartEndMarkers();
  getTCPData();
  serial.update();
}

/* TBD Hardcoding size for now */
void send_RSSI(char packet[], bool isTCP)
{
  long RSSI_val = Adhoc.get_RSSI();
#ifdef __DEBUG__
  Serial.print("RSSI command received : ");
  Serial.println(RSSI_val, DEC);
#endif

  char rssi_packet[13];

  memcpy(rssi_packet, packet, 8);
  rssi_packet[6] = sizeof(long) + 1;
  rssi_packet[8] = (RSSI_val & 0xFF000000) >> 24;
  rssi_packet[9] = (RSSI_val & 0x00FF0000) >> 16;
  rssi_packet[10] = (RSSI_val & 0x0000FF00) >> 8;
  rssi_packet[11] = (RSSI_val & 0x000000FF);
#ifdef __DEBUG__
  Adhoc.print_packet(rssi_packet, 13);
#endif
  if (isTCP)
  {
    client.write(&rssi_packet[0], 13);
  }
  else
  {
    sendToArduino(rssi_packet, 12);
  }
}

void onPacket(const uint8_t *buffer, size_t size)
{
  // Make a temporary buffer.
  char tmp[size];
  memcpy(tmp, buffer, size);
  //Serial.println("Serial packet received from Arduino");
  //Adhoc.print_packet(tmp,size);

#ifdef __DEBUG__
  Serial.println("");
  Serial.println("Serial packet received from Arduino");
  Adhoc.print_packet(tmp, size);
#endif

  if (!Adhoc.is_valid(tmp))
  {
    Serial.println("Doesnot contain start byte rejecting");

#ifdef __DEBUG__
    Serial.println("Doesnot contain start byte rejecting");
#endif
    return;
  }

  if (Adhoc.is_internal_cmd(tmp))
  {

    if (Adhoc.is_internal_cmd(tmp) == 4)
    {
      /* Special case handle it here only */
      //TBD get RSSI

      send_RSSI(tmp, false);
#ifdef __DEBUG__
      Serial.println("RSSI value sent to Arduino");
#endif
    }
    if (Adhoc.is_internal_cmd(tmp) == 2)
    {
      /* Special case handle it here only */

      if (Adhoc.wifi_status == 1)
      {
        /* Already connected
            return from function
        */
        return;
      }

      Adhoc.extract_ssid_pwd(tmp, size);
#ifdef __DEBUG__
      Serial.print("Initiating connection to AP");
#endif
      WiFi.mode(WIFI_STA);
      WiFi.begin(Adhoc.ssid, Adhoc.password);
      int tries = 0;
      while (WiFi.status() != WL_CONNECTED)
      {
        setLED(REDLED, BLINK);
        delay(1000);
#ifdef __DEBUG__
        Serial.print(".");
#endif
        tries++;
        if (tries > 30)
        {
#ifdef __DEBUG__
          Serial.println("\r\nAP NOT FOUND");
#endif
          setLED(REDLED, OFF);
          while (1)
            ;
        }
      }
      setLED(REDLED, ON);
#ifdef __DEBUG__
      Serial.print("Connected to AP \"");
      Serial.print(Adhoc.ssid);
      Serial.println('\"');
#endif

      IPAddress broadcastIP = WiFi.localIP();
      broadcastIP[3] = 255;
      setLED(BLELED, BLINK);
      if (Udp.setupUDP(broadcastIP, UDPport))
      {
#ifdef __DEBUG__
        Serial.println("Error initializing UDP");
#endif
        setLED(BLELED, OFF);
      }
      else
      {
#ifdef __DEBUG__
        Serial.println("UDP connection succesful");
#endif
        setLED(BLELED, ON);
      }

      setLED(WIFILED, BLINK);
      if (!client.connect(Adhoc.server_ip, TCPport))
      {
#ifdef __DEBUG__
        Serial.println("TCP connection failed");
#endif
        setLED(WIFILED, OFF);
      }
      else
      {
#ifdef __DEBUG__
        Serial.println("TCP connection successful");
#endif
        setLED(WIFILED, ON);
      }
#ifdef __DEBUG__

      Serial.println("WiFi connected. IP address: ");
      Serial.println(WiFi.localIP());

#endif
      Adhoc.wifi_status = 1;
      return;
    }
    Adhoc.process_internal_cmd(tmp);
    return;
  }

  if (Adhoc.is_TCP(tmp))
  {
    client.write(tmp, size);
#ifdef __DEBUG__
    Serial.println("Packet sent to TCP Server");
#endif
    return;
  }
  else
  {
    adhoc_send_filter(tmp, size);
#ifdef __DEBUG__
    Serial.println("Packet sent to UDP network");
#endif
  }
}

void recvWithStartEndMarkers()
{
  static boolean recvInProgress = false;
  static byte ndx = 0;
  byte startMarker = 0x80;
  byte endMarker = 0x81;
  byte rc;

  while (client.available() > 0 && newData == false)
  {
    rc = client.read();
    if (recvInProgress == true)
    {
      if (rc != endMarker)
      {
        receivedChars[ndx] = rc;
        ndx++;
        TCPBytes++;
      }
      else
      {
        //receivedChars[ndx] = '\0'; // terminate the string
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    }

    else if (rc == startMarker)
    {
      recvInProgress = true;
    }
  }
}

void check_data(char packet[])
{

  char ID;
  long RSSI_Val;
  char tcp_reply[10];

  if (packet[7] == 0x0F)
  {
#ifdef __DEBUG__
    Serial.println("Sending ID");
#endif

    client.print(Adhoc.ID_SELF, DEC);
  }
  if (packet[7] == 0x0E)
  {
#ifdef __DEBUG__
    Serial.println("Sending RSSI");
#endif

    client.print(Adhoc.get_RSSI(), DEC);
  }
}

int canBroadcast(char packet[])
{
  char src = Adhoc.get_src(packet);
  char dst = Adhoc.get_dst(packet);

  /* I am not the destination this packet is
      not intended for me.
  */

  /* If both are zero means
     probably requesting ID
     so dont broadcast this.
  */
  if (!src && !dst)
  {
    return 0;
  }

  if (dst != Adhoc.ID_SELF)
  {
    return 1;
  }
  else
  {
    /* I am the destination
        Send to Arduino.
    */
    return 0;
  }
}

void getTCPData()
{

  int numBytes;
  if (newData == true)
  {
#ifdef __DEBUG__
    Serial.println("");
    Serial.println("Serial packet received from TCP server ... ");
    Serial.print("Received bytes : ");
    Serial.println(TCPBytes);

    if (TCPBytes == 0)
    {
      Serial.println("Ignored Junk in TCP");
      newData = false;

      return;
    }

    for (int i = 0; i < TCPBytes; i++)
    {
      Serial.print(receivedChars[i], HEX);
      Serial.print(" ");
    }
    Serial.println("");
    //
#endif
    //Serial.println(receivedChars);
    numBytes = TCPBytes;
    TCPBytes = 0;
    //client.print(buff);
    tcpBuffer = (char *)malloc(numBytes * sizeof(char));
    memcpy(tcpBuffer, receivedChars, numBytes);

    if (!Adhoc.is_valid(tcpBuffer))
    {
      newData = false;
      return;
    }
#ifdef __DEBUG__
    Adhoc.print_packet(tcpBuffer, numBytes);
#endif

#if 0
    if (!Adhoc.get_src(tcpBuffer)) {
#ifdef __DEBUG__
      Serial.println("Source does not know the ID");
      Serial.println("Send ID");
#endif
      client.print(Adhoc.ID_SELF, DEC);
    }

#endif

    /* This is not actually a TCP packet but
        an Adhoc one so broadcast it

        Check if destination is SELF else
        broadcast it
    */
    if (canBroadcast(tcpBuffer))
    {
#ifdef __DEBUG__
      Serial.println("This is actually a broadcast packet");
#endif
      adhoc_send_filter(tcpBuffer, numBytes);
      free(tcpBuffer);
      newData = false;
      return;
    }

    if (tcpBuffer[7] == 0x0E)
    {
#ifdef __DEBUG__
      Serial.println("Sending RSSI value to server");
#endif

      send_RSSI(tcpBuffer, true);
      return;
    }
    //check_data(tcpBuffer);

#ifdef __DEBUG__
    Serial.println("Sending TCP Buffer to Arduino");
#endif

    /* Added for debugging */

    sendToArduino(tcpBuffer, numBytes);
    free(tcpBuffer);
    newData = false;
  }
}
