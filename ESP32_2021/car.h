#ifndef CAR_H
#define CAR_H

// motor pins
#define a1 19
#define a2 18
#define b1 5
#define b2 17
#define c1 16
#define c2 4
#define d1 14  // changed from pin 2
#define d2 15

#pragma once

void initMotors();

void driveForwards();
void driveBackwards();
void rotateLeft();
void rotateRight();
void stop();

#endif
