/****************************************************************************
 Module
   ServoService.c

 Revision
   1.0.1

 Description
   This service controls the PWM output to the servos

 Notes

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// This module
#include "../ProjectHeaders/ServoService.h"

// Hardware
#include <xc.h>

// Event & Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"
#include "ES_Port.h"
#include "terminal.h"
#include "dbprintf.h"
#include <sys/attribs.h> //for writing ISRs
#include "InterruptSetup.h"

/*----------------------------- Module Defines ----------------------------*/
// CLOCK VALUES
#define PWM_FREQ 50   // Hz
#define PB_CLOCK 20e6 // 20MHz
#define PRESCALER 3   // 1:1 (0) 1:2 (1) 1:4 (2) 1:8 (3) 1:16 (4)

// DC RANGES
#define TCKS_PER_US 2.5      // 20 / PRESCALER
#define MIN_DC 540           // us @ 50Hz
#define MAX_DC 2400          // us @ 50Hz
#define MIN_DC_SHOULDER 400  // us @ 50Hz
#define MAX_DC_SHOULDER 2640 // us @ 50Hz

// PHYSICAL ANGLE BOUNDARIES
#define MIN_SHOULDER_ANGLE 39
#define MAX_SHOULDER_ANGLE 299

#define BIG_GAME_TIMER 46000
#define SERVO_DELAY 12       // ms / shoulder degree
#define SIZE_GUARD_DELAY 700 // ms

/*---------------------------- Module Functions ---------------------------*/

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
// add a deferral queue for up to 3 pending deferrals +1 to allow for overhead
static ES_Event_t DeferralQueue[3 + 1];

uint16_t PR_VAL = 49999; // PB_CLOCK / (PRESCALER * PWM_FREQ)

static uint8_t gameTimerCounter = 0;

// Flags for servo motion logic
static bool RobotSizeGuard = false;
static bool delayingArm = true;

// ARM POSITIONS
static uint16_t ShoulderAngle = 180;
static uint16_t WristAngle = 90;

// Interpolation for intake
static uint8_t NumInterps = 11;
static bool IntakeDown = true;
static uint8_t InterpCounter = 0;
static uint16_t ArmIntake[][3] = {
    {297, 138, 400}, // [shoulder, wrist, time] crate level
    {294, 140, 120},
    {291, 148, 90},
    {288, 160, 80},
    {285, 170, 70},
    {282, 175, 60},
    {279, 180, 50},
    {276, 180, 50},
    {273, 180, 50},
    {270, 180, 50},
    {260, 180, 50} // [shoulder, wrist, time] upper level
};

// Crate placement positions
static uint16_t ArmCrateHigh[] = {142, 31}; // [shoulder, wrist]
static uint16_t ArmCrateMid[] = {92, 92};   // [shoulder, wrist]
static uint16_t ArmCrateLow[] = {50, 142};  // [shoulder, wrist]

// Arm idle positions
static uint16_t ArmIdle[] = {260, 180}; // [shoulder, wrist]

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitTestHarnessService0

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
 Notes

 Author
     J. Edward Carryer, 01/16/12, 10:00
****************************************************************************/

bool InitServoService(uint8_t Priority)
{
    ES_Event_t ThisEvent;

    MyPriority = Priority;

    DB_printf("Servo SERVICE\n");

    /* ### CONFIGURE SERVO PINS & PWM ### */

    // SHOULDER SERVO
    ANSELAbits.ANSA0 = 0;
    TRISAbits.TRISA0 = 0;

    // TMR3 = 0;
    FullOCSetup(1, PR_VAL, 3, PRESCALER);
    RPA0R = 5;
    OC1RS = 0;
    OC1R = 0;

    // WRIST SERVO
    ANSELAbits.ANSA1 = 0;
    TRISAbits.TRISA1 = 0;

    FullOCSetup(2, PR_VAL, 3, PRESCALER);
    RPA1R = 5;
    OC2RS = 0;
    OC2R = 0;

    // INTAKE SERVO
    TRISAbits.TRISA2 = 0;

    FullOCSetup(4, PR_VAL, 3, PRESCALER);
    RPA2R = 5;
    OC4RS = 0;
    OC4R = 0;

    // INDICATOR SERVO
    TRISAbits.TRISA3 = 0;

    FullOCSetup(3, PR_VAL, 3, PRESCALER);
    RPA3R = 5;
    OC3RS = 0;
    OC3R = 0;

    PR3 = PR_VAL;
    /* ### END CONFIGURE SERVO PINS & PWM ### */

    //    ES_Timer_InitTimer(SERVICE0_TIMER, 100);

    ThisEvent.EventType = ES_INIT;
    DB_printf("end of init reached\n");
    // SetDutyCycle(100, 100);
    if (ES_PostToService(MyPriority, ThisEvent) == true)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/****************************************************************************
 Function
     PostTestHarnessService0

 Parameters
     ES_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostServoService(ES_Event_t ThisEvent)
{
    return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunTestHarnessService0

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes

 Author
   J. Edward Carryer, 01/15/12, 15:23
****************************************************************************/
ES_Event_t RunServoService(ES_Event_t ThisEvent)
{
    ES_Event_t ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

    switch (ThisEvent.EventType)
    {
    case ES_TIMEOUT:
    {
        if (ThisEvent.EventParam == SERVO_TIMER)
        {
            if (RobotSizeGuard)
            {
                SetServoAngle(WRIST_SERVO, WristAngle);
                RobotSizeGuard = false;
            }
            else
            {
                static uint8_t i;
                i = IntakeDown ? (NumInterps - InterpCounter - 1) : InterpCounter;
                ShoulderAngle = ArmIntake[i][0];
                WristAngle = ArmIntake[i][1];

                SetServoAngle(SHOULDER_SERVO, ShoulderAngle);
                SetServoAngle(WRIST_SERVO, WristAngle);

                InterpCounter++;

                if (InterpCounter == NumInterps && IntakeDown) // reverse direction of motion
                {
                    IntakeDown = false; // begin intake up
                    InterpCounter = 0;

                    ES_Timer_InitTimer(SERVO_TIMER, ArmIntake[i][2]);
                }
                else if (InterpCounter == NumInterps && !IntakeDown) // done with interpolation
                {
                    IntakeDown = true; // prepare for next intake
                    InterpCounter = 0;
                }
                else // continue interpolation
                {
                    ES_Timer_InitTimer(SERVO_TIMER, ArmIntake[i][2]);
                }
            }
        }
        else if (ThisEvent.EventParam == GAME_TIMER)
        {
            gameTimerCounter++;
            if (gameTimerCounter == 3)
            {
                ES_Timer_InitTimer(CLEANUP_TIMER, 100);
            }
            else if (gameTimerCounter < 3)
            {
                ES_Timer_InitTimer(GAME_TIMER, BIG_GAME_TIMER);
            }
            else
            {
                gameTimerCounter = 1;
            }
        }
        else if (ThisEvent.EventParam == ARM_IDLE_TIMER)
        {
            SetArm(ARM_IDLE, false);
        }
    }
    break;

    case ES_NEW_KEY:
    {
    }
    break;

    default:
    {
    }
    break;
    }

    return ReturnEvent;
}

/***************************************************************************
 Public functions
 ***************************************************************************/

/**
 * @brief Set the position of the arm servos.
 *
 * This function adjusts the shoulder and wrist servos to one of four predefined positions:
 * INTAKE, HIGH, MIDDLE, or LOW, based on the provided position parameter.
 *
 * @param Position The desired position for the arm servos. It can be one of the following:
 *                 ARM_INTAKE - Moves the arm to the intake position.
 *                 ARM_HIGH - Moves the arm to the high position.
 *                 ARM_MIDDLE - Moves the arm to the middle position.
 *                 ARM_LOW - Moves the arm to the low position.
 */

void SetArm(uint8_t Position, bool Delay)
{
    if (Position == ARM_IDLE)
    {
        if (Delay == true)
        {
            ES_Timer_InitTimer(ARM_IDLE_TIMER, 750);
            ;
        }
        else
        {
            uint16_t dif = abs(ShoulderAngle - ArmIdle[0]);

            ShoulderAngle = ArmIdle[0];
            WristAngle = ArmIdle[1];

            SetServoAngle(SHOULDER_SERVO, ShoulderAngle);
            SetServoAngle(WRIST_SERVO, WristAngle);

            ES_Timer_InitTimer(ARM_DELAY_TIMER, dif * SERVO_DELAY);
        }
    }
    else if (Position == ARM_INTAKE)
    {
        // restart intake counter
        IntakeDown = true;
        InterpCounter = 0;

        uint16_t dif = abs(ShoulderAngle - ArmIntake[NumInterps - 1][0]);

        ShoulderAngle = ArmIntake[NumInterps - 1][0];
        WristAngle = ArmIntake[NumInterps - 1][1];

        SetServoAngle(SHOULDER_SERVO, ShoulderAngle);
        SetServoAngle(WRIST_SERVO, WristAngle);

        InterpCounter++;

        if (dif == 0)
        {
            dif = 100;
        }
        ES_Timer_InitTimer(SERVO_TIMER, dif * SERVO_DELAY);
        ES_Timer_InitTimer(ARM_DELAY_TIMER, 300 * SERVO_DELAY);
    }
    else if (Position == ARM_HIGH)
    {
        uint16_t dif = abs(ShoulderAngle - ArmCrateHigh[0]);

        ShoulderAngle = ArmCrateHigh[0];
        WristAngle = ArmCrateHigh[1];
        SetServoAngle(SHOULDER_SERVO, ShoulderAngle);
        // SetServoAngle(WRIST_SERVO, WristAngle);

        RobotSizeGuard = true;
        ES_Timer_InitTimer(SERVO_TIMER, SIZE_GUARD_DELAY);
        ES_Timer_InitTimer(ARM_DELAY_TIMER, dif * 10);
    }
    else if (Position == ARM_MID)
    {
        uint16_t dif = abs(ShoulderAngle - ArmCrateMid[0]);

        ShoulderAngle = ArmCrateMid[0];
        WristAngle = ArmCrateMid[1];
        SetServoAngle(SHOULDER_SERVO, ShoulderAngle);
        // SetServoAngle(WRIST_SERVO, WristAngle);

        RobotSizeGuard = true;
        ES_Timer_InitTimer(SERVO_TIMER, SIZE_GUARD_DELAY);
        ES_Timer_InitTimer(ARM_DELAY_TIMER, dif * 10);
    }
    else if (Position == ARM_LOW)
    {
        uint16_t dif = abs(ShoulderAngle - ArmCrateLow[0]);

        ShoulderAngle = ArmCrateLow[0];
        WristAngle = ArmCrateLow[1];
        SetServoAngle(SHOULDER_SERVO, ShoulderAngle);
        // SetServoAngle(WRIST_SERVO, WristAngle);

        RobotSizeGuard = true;
        ES_Timer_InitTimer(SERVO_TIMER, SIZE_GUARD_DELAY);
        ES_Timer_InitTimer(ARM_DELAY_TIMER, dif * 10);
    }
    else if (Position == ARM_SHEAR)
    {
        WristAngle -= 29;
        SetServoAngle(SHOULDER_SERVO, ShoulderAngle);
        SetServoAngle(WRIST_SERVO, WristAngle);

        ES_Timer_InitTimer(ARM_DELAY_TIMER, 100);
    }
}

/**
 * @brief Set the position of the intake servo.
 *
 * This function adjusts the intake servo to one of three predefined positions:
 * OPEN, CLOSED, or PUSH, based on the provided position parameter.
 *
 * @param Position The desired position for the intake servo. It can be one of the following:
 *                 INTAKE_OPEN - Moves the servo to the open position.
 *                 INTAKE_CLOSED - Moves the servo to the closed position.
 *                 INTAKE_PUSH - Moves the servo to the push position.
 */

void SetIntake(uint8_t Position)
{
    if (Position == INTAKE_OPEN)
    {
        SetServoAngle(INTAKE_SERVO, 0);
    }
    else if (Position == INTAKE_CLOSED)
    {
        SetServoAngle(INTAKE_SERVO, 90);
    }
    else if (Position == INTAKE_PUSH)
    {
        SetServoAngle(INTAKE_SERVO, 180);
    }
}

/**
 * @brief Set the position of the indicator servo.
 *
 * This function adjusts the indicator servo to one of two predefined positions:
 * GREEN or BLUE, based on the provided position parameter.
 *
 * @param Position The desired position for the indicator servo. It can be one of the following:
 *                 INDICATOR_GREEN - Moves the servo to the green position.
 *                 INDICATOR_BLUE - Moves the servo to the blue position.
 */
void SetIndicator(uint8_t Position)
{
    if (Position == INDICATOR_GREEN)
    {
        SetServoAngle(INDICATOR_SERVO, 180);
    }
    else if (Position == INDICATOR_BLUE)
    {
        SetServoAngle(INDICATOR_SERVO, 0);
    }
    else if (Position == INDICATOR_NEUTRAL)
    {
        SetServoAngle(INDICATOR_SERVO, 90);
    }
    else if (Position == INDICATOR_OFF)
    {
        OC3RS = 0;
    }
}

/***************************************************************************
 Private functions
 ***************************************************************************/
/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

/**
 * @brief Set the angle of the specified servo.
 *
 * This function adjusts the pulse width to move the specified servo to the desired angle.
 *
 * @param Servo The servo to adjust. It can be one of the following:
 *              SHOULDER_SERVO - Adjusts the shoulder servo.
 *              WRIST_SERVO - Adjusts the wrist servo.
 *              INTAKE_SERVO - Adjusts the intake servo.
 *              INDICATOR_SERVO - Adjusts the indicator servo.
 *
 * @param Angle The desired angle for the servo. For the shoulder servo, it ranges from 0 to 360 degrees.
 *              For the other servos, it ranges from 0 to 180 degrees.
 */
void SetServoAngle(uint8_t Servo, uint16_t Angle)
{
    if (Servo == SHOULDER_SERVO)
    {
        // Safety net for shoulder
        if (Angle < MAX_SHOULDER_ANGLE && Angle > MIN_SHOULDER_ANGLE)
        {
            OC1RS = (uint16_t)(TCKS_PER_US * ((((float)Angle / 360) * (MAX_DC_SHOULDER - MIN_DC_SHOULDER)) + MIN_DC_SHOULDER));
        }
    }
    else if (Servo == WRIST_SERVO)
    {
        OC2RS = (uint16_t)(TCKS_PER_US * ((((float)Angle / 180) * (MAX_DC - MIN_DC)) + MIN_DC));
    }
    else if (Servo == INTAKE_SERVO)
    {
        OC4RS = (uint16_t)(TCKS_PER_US * ((((float)Angle / 180) * (MAX_DC - MIN_DC)) + MIN_DC));
    }
    else if (Servo == INDICATOR_SERVO)
    {

        OC3RS = (uint16_t)(TCKS_PER_US * ((((float)Angle / 180) * (MAX_DC - MIN_DC)) + MIN_DC));
    }
}