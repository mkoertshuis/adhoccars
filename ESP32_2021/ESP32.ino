#include "car.h"

#include <WiFi.h>
#include <IPAddress.h>
#include <WiFiUdp.h>

#define WIFILED LED_BUILTIN
#define __DEBUG__ 1
const char *udpAddress = "255.255.255.255";
unsigned int udpPort = 4444; // local port to listen on
char ssid[] = "ssayedee";
char password[] = "password123";

WiFiUDP Udp;
WiFiClient client;

hw_timer_t *timer = NULL;
bool WLEDState = false;
bool BlinkWLED = false;
enum LEDSTATE
{
    OFF,
    ON,
    BLINK
} ledState;

int pingCount = 0;
int pingModulus = 200;

/*
  WiFi LED timer for blinking state
*/
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
}

void initWireless()
{
    Serial.printf("Connecting to %s ", ssid);
    WiFi.softAP("ESPsoftAP_01", "password123");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".\n");
    }
    Serial.println(" connected");
    BlinkWLED = false;

    Udp.begin(udpPort);
    Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), udpPort);
}

void sendConnection()
{
    uint8_t buffer[15] = "hello server";
    Udp.beginPacket(udpAddress, udpPort);
    Udp.write(buffer, 11);
    Udp.endPacket();
    memset(buffer, 0, 50);
}

void processUdpPacket(byte *packet)
{
    if (packet[0] == 'F')
    {
        Serial.println("Driving forwards");
        driveForwards();
    }
    else if (packet[0] == 'S')
    {
        Serial.println("Driving backwards");
        driveBackwards();
    }
    else if (packet[0] == 'A')
    {
        Serial.println("Left");
        rotateLeft();
    }
    else if (packet[0] == 'D')
    {
        Serial.println("Right");
        rotateRight();
    }
    else if (packet[0] == '0')
    {
        Serial.println("Stop");
        stop();
    }
    else
    {
        stop();
    }
}

void setup()
{
    Serial.begin(115200);
    delay(10);

#ifdef __DEBUG__
    Serial.println("Booting ESP module");
#endif

    pinMode(WIFILED, OUTPUT);
    digitalWrite(WIFILED, LOW);

    // Setup timer
    // Use 1st timer of 4 (counted from zero).
    // Set 80 divider for prescaler
    // timer = timerBegin(0, 80, true);
    // // Attach onTimer function to our timer.
    // timerAttachInterrupt(timer, &onTimer, true);
    // // Trigger every 200ms
    // timerAlarmWrite(timer, 200000, true);
    // // Start an alarm
    // timerAlarmEnable(timer);

    // initWireless();
    initMotors();

#ifdef __DEBUG__
    Serial.println("Setup done ... ");
#endif

    digitalWrite(WIFILED, LOW);
    // delay(1000);
    // driveForwards();
    // delay(1000);
    // driveBackwards();
    // delay(1000);
    // rotateLeft();
    // delay(1000);
    // rotateRight();

#ifdef __DEBUG__
    Serial.println("Test done ... ");
#endif
}

void loop()
{
    // driveForwards();
    digitalWrite(WIFILED, HIGH);
    driveForwards();
    delay(1000);
    stop();
    delay(2000);
    rotateLeft();
    delay(1000);
    rotateRight();
    delay(1000);

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
