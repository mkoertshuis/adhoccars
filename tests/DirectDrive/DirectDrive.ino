#define LED_BUILTIN 2

int a1 = 19;
int a2 = 18;
int b1 = 5;
int b2 = 17;
int c1 = 16;
int c2 = 4;
int d1 = 14; // Changed from 2 as this was the builtin LED
int d2 = 15;

void setup()
{
    Serial.begin(115200);
    // pinMode(LED_BUILTIN, OUTPUT);
    pinMode(a1, OUTPUT);
    pinMode(a2, OUTPUT);
    pinMode(b1, OUTPUT);
    pinMode(b2, OUTPUT);
    pinMode(c1, OUTPUT);
    pinMode(c2, OUTPUT);
    pinMode(d1, OUTPUT);
    pinMode(d2, OUTPUT);
    
    delay(2000);
}

void loop()
{
    Serial.println("a1");
    digitalWrite(a1, HIGH);
    digitalWrite(a2, LOW);
    digitalWrite(b1, HIGH);
    digitalWrite(b2, LOW);
    digitalWrite(c1, HIGH);
    digitalWrite(c2, LOW);
    digitalWrite(d1, HIGH);
    digitalWrite(d2, LOW);

    delay(1000);
    Serial.println("a2");    
    digitalWrite(a1, LOW);
    digitalWrite(a2, HIGH);
    digitalWrite(b1, LOW);
    digitalWrite(b2, HIGH);
    digitalWrite(c1, LOW);
    digitalWrite(c2, HIGH);
    digitalWrite(d1, LOW);
    digitalWrite(d2, HIGH);
    delay(1000);
    digitalWrite(a1, LOW);
    digitalWrite(a2, LOW);
    digitalWrite(b1, LOW);
    digitalWrite(b2, LOW);
    digitalWrite(c1, LOW);
    digitalWrite(c2, LOW);
    digitalWrite(d1, LOW);
    digitalWrite(d2, LOW);
    delay(5000);
}
