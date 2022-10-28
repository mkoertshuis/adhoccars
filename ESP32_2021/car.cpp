#include <Arduino.h>
#include "car.h"

#define pause 250

void stop()
{
    digitalWrite(a1, LOW);
    digitalWrite(a2, LOW);
    digitalWrite(b1, LOW);
    digitalWrite(b2, LOW);
    digitalWrite(c1, LOW);
    digitalWrite(c2, LOW);
    digitalWrite(d1, LOW);
    digitalWrite(d2, LOW);
}

void initMotors()
{
    pinMode(a1, OUTPUT);
    pinMode(a2, OUTPUT);
    pinMode(b1, OUTPUT);
    pinMode(b2, OUTPUT);
    pinMode(c1, OUTPUT);
    pinMode(c2, OUTPUT);
    pinMode(d1, OUTPUT);
    pinMode(d2, OUTPUT);
}

void setWheel(uint8_t wheel, uint8_t alt, int8_t forward)
{
    if (forward)
    {
        digitalWrite(wheel, HIGH);
        digitalWrite(alt, LOW);
    }
    else if (forward == -1)
    {
        digitalWrite(wheel, LOW);
        digitalWrite(alt, LOW);
    }
    else
    {
        digitalWrite(wheel, LOW);
        digitalWrite(alt, HIGH);
    }
}

void driveForwards()
{
    stop();
    delay(pause);
    setWheel(a1, a2, true);
    setWheel(b1, b2, true);
    setWheel(c1, c2, true);
    setWheel(d1, d2, true);
}

void driveBackwards()
{
    stop();
    delay(pause);
    setWheel(a1, a2, false);
    setWheel(b1, b2, false);
    setWheel(c1, c2, false);
    setWheel(d1, d2, false);
}

void rotateLeft()
{
    stop();
    delay(pause);
    setWheel(a1, a2, true);
    setWheel(b1, b2, false);
    setWheel(c1, c2, false);
    setWheel(d1, d2, true);
}

void rotateRight()
{
    stop();
    delay(pause);
    setWheel(a1, a2, false);
    setWheel(b1, b2, true);
    setWheel(c1, c2, true);
    setWheel(d1, d2, false);
}
