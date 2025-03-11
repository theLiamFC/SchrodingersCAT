// Servo.h
#ifndef SERVOSERVICE_H
#define SERVOSERVICE_H

#include <stdint.h>
#include <stdbool.h>

#include "ES_Events.h"
#include "ES_Port.h"

// SERVOS
#define SHOULDER_SERVO 0
#define WRIST_SERVO 1
#define INTAKE_SERVO 2
#define INDICATOR_SERVO 3

// ARM POSITIONS
#define ARM_IDLE 0
#define ARM_INTAKE 1
#define ARM_HIGH 2
#define ARM_MID 3
#define ARM_LOW 4
#define ARM_SHEAR 5

// INTAKE POSITIONS
#define INTAKE_OPEN 0
#define INTAKE_CLOSED 1
#define INTAKE_PUSH 2

// INDICATOR POSITIONS
#define INDICATOR_GREEN 0
#define INDICATOR_BLUE 1
#define INDICATOR_NEUTRAL 2
#define INDICATOR_OFF 3

bool InitServoService(uint8_t Priority);
bool PostServoService(ES_Event_t ThisEvent);
ES_Event_t RunServoService(ES_Event_t ThisEvent);

void SetIndicator(uint8_t Position);
void SetIntake(uint8_t Position);
void SetArm(uint8_t Position, bool Delay);

void SetServoAngle(uint8_t Servo, uint16_t Angle);

#endif  // SERVO_H