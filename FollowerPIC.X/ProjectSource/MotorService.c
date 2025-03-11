/****************************************************************************
 Module
   MotorService.c

 Revision
   1.0.1

 Description
 This service operates the drive motors based on commands from the Leader PIC
 Both line following and RPM control is implemented here

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 10/26/17 18:26 jec     moves definition of ALL_BITS to ES_Port.h
 10/19/17 21:28 jec     meaningless change to test updating
 10/19/17 18:42 jec     removed referennces to driverlib and programmed the
                        ports directly
 08/21/17 21:44 jec     modified LED blink routine to only modify bit 3 so that
                        I can test the new new framework debugging lines on PF1-2
 08/16/17 14:13 jec      corrected ONE_SEC constant to match Tiva tick rate
 11/02/13 17:21 jec      added exercise of the event deferral/recall module
 08/05/13 20:33 jec      converted to test harness service
 01/16/12 09:58 jec      began conversion from TemplateFSM.c
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// This module
#include "../ProjectHeaders/MotorService.h"
// #include "../ProjectHeaders/ADService.h"
#include "../ProjectHeaders/PIC32_SPI_HAL.h"
#include "../ProjectHeaders/PIC32_AD_Lib.h"
#include "../ProjectHeaders/BeaconService.h"
// Hardware
#include <xc.h>
// #include <proc/p32mx170f256b.h>

// Event & Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"
#include "ES_Port.h"
#include "terminal.h"
#include "dbprintf.h"
#include <sys/attribs.h> //for writing ISRs
#include "InterruptSetup.h"
#include "Pic2PicFollowerFSM.h"
#include "BeaconService.h"

/*----------------------------- Module Defines ----------------------------*/
// these times assume a 10.000mS/tick timing
#define ONE_SEC 1000
#define HALF_SEC (ONE_SEC / 2)
#define TWO_SEC (ONE_SEC * 2)
#define FIVE_SEC (ONE_SEC * 5)

#define ENTER_POST ((MyPriority << 3) | 0)
#define ENTER_RUN ((MyPriority << 3) | 1)
#define ENTER_TIMEOUT ((MyPriority << 3) | 2)
#define MOTOR_FORWARD 1
#define MOTOR_BACKWARD -1
#define MOTOR_STOP 0
#define MOTOR_TURNING_CW 1
#define MOTOR_TURNING_CCW -1
#define TAPE_THRESHOLD 67

#define QUERY_FREQ 200               // Hz
#define QUERY_TIME 1000 / QUERY_FREQ // ms

#define INT_DELAY 750

#define DUTY_TIME 100
#define WHEEL_DIAMETER 8 // cm
#define PI 3.14159

// #define TEST_INT_POST
// #define BLINK LED
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

void newKey(ES_Event_t);
static void ClearRPMErrors(void);
static void ClearLineErrors(void);
void SetDutyCycle(int8_t LeftDC, int8_t RightDC);
static void ReadTapeValues(void);

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
// add a deferral queue for up to 3 pending deferrals +1 to allow for overhead
static ES_Event_t DeferralQueue[3 + 1];

// Tape Following Constants
static volatile int32_t sigma_e = 0;
static volatile int32_t sigma_error = 0;
static volatile int32_t error = 0;
static volatile int32_t error_p = 0;
static volatile int8_t LeftTarget = 0;
static volatile int8_t RightTarget = 0;
static volatile int8_t DirectionS = MOTOR_STOP; // straight direction [-1,0,1]
static volatile int8_t DirectionT = MOTOR_STOP; // turning direction [-1,0,1]
static uint8_t LineFollowingSpeed = 15;         // base speed for line following

static float P_Constant = 0.1;
static float I_Constant = 0.03;

static volatile int16_t LeftTargetRPM = 15;
volatile bool LeftMotorDirection = MOTOR_FORWARD;
volatile uint32_t LeftLastEncoderEdge = 0;
volatile uint32_t LeftEncoderEdgeDif = 0;
volatile uint32_t LeftTimerRollOver = 0;
volatile float LeftSigmaError = 0;
volatile float LeftError = 0;

static volatile int16_t RightTargetRPM = 15;
volatile bool RightMotorDirection = MOTOR_FORWARD;
volatile uint32_t RightLastEncoderEdge = 0;
volatile uint32_t RightEncoderEdgeDif = 0;
volatile uint32_t RightTimerRollOver = 0;
volatile float RightSigmaError = 0;
volatile float RightError = 0;

static volatile int8_t LeftDC = 0;
static volatile int8_t RightDC = 0;

static int8_t DriveMode = MOTOR_STOP; // 1, 0, -1

static uint32_t ADValues[5]; // Stores ADC readings for A0, A1, B2
volatile static uint32_t leftVal, centerVal, rightVal, leftT, rightT;

static bool DesiredTs = 0;
static uint8_t DesiredInts = 0;
static bool FoundT = 0;
static uint8_t FoundInts = 0;
static bool readyNewDetection = 0;
static bool readyNewDetectionDelayTimer = 0;

static bool readyTurningDone = 0;
static bool initTurn = 0;

static bool searchingT = 0;

uint8_t MovingHorizontal = 1; // Starting moving horizontal
static bool RPMControl = 0;

static volatile uint32_t encoder_count = 0;
static volatile uint32_t end_encoder_count = 0;
static volatile bool CountDistance = 0;

#define THRESH_DIV 4

static uint32_t MinLeftC = 20;
static uint32_t MinCentC = 20;
static uint32_t MinRightC = 20;
static uint32_t MinLeftT = 20;
static uint32_t MinRightT = 20;

static uint32_t MaxLeftC = 90;
static uint32_t MaxCentC = 90;
static uint32_t MaxRightC = 90;
static uint32_t MaxLeftT = 90;
static uint32_t MaxRightT = 90;

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

// extern uint32_t potVal;

static volatile uint32_t t1; // older time
static volatile uint32_t t2; // new time
static volatile uint32_t deltaT;
// union CapturedTime time_reading;

static volatile uint16_t rollover;

static volatile int32_t RightRPM = 0;
static volatile int32_t LeftRPM = 0;

bool InitMotorService(uint8_t Priority)
{
    ES_Event_t ThisEvent;

    MyPriority = Priority;

    // clrScrn();
    // puts("\rStarting MotorService \r");
    DB_printf("Motor SERVICE\n");

    // Configure Motor Output Pins
    // Right PWM
    //    ANSELAbits.ANSA0 = 1;
    TRISBbits.TRISB4 = 0;

    // Left PWM
    //    ANSELAbits.ANSA1 = 0;
    TRISBbits.TRISB5 = 0;

    // Right Digital
    TRISAbits.TRISA3 = 0;

    // Left Digital
    TRISAbits.TRISA2 = 0;

    // Right Motor Output Compare PWM
    FullOCSetup(1, 12499, 2, 0); // OC1, PR=12499, Timer2, Prescale 1:1
    RPB4R = 5;                   // set pin to use OC1
    OC1RS = 0;                   // Ensure Duty Cycle starts at 0
    OC1R = 0;

    // Left Motor Output Compare PWM
    FullOCSetup(2, 12499, 2, 0); // OC2, PR=12499, Timer2, Prescale 1:1
    RPB5R = 5;                   // set pin to use OC2
    OC2RS = 0;                   // Ensure Duty Cycle starts at 0
    OC2R = 0;

    // Encoder Timer and InputCapture
    // TIMER 3 INFORMATION SET IN BEACONSERVICE.C
    // Left Encoder
    // IC3 Init
    IC3CONbits.ON = 0;          // turn off IC
    IC3CONbits.SIDL = 0;        // continuous idle mode
    IC3CONbits.C32 = 0;         // 16 bit mode
    IC3CONbits.ICTMR = 0;       // use timer 3
    IC3CONbits.ICI = 0;         // interrupt on every capture event
    IC3CONbits.ICM = 1;         // simple capture mode - every edge
    IC3R = 4;                   // use RPB8 pin
    IC3CONbits.ON = 1;          // turn on IC
    IFS0CLR = _IFS0_IC3IF_MASK; // clear
    IEC0SET = _IEC0_IC3IE_MASK; // enable
    IPC3bits.IC3IP = 6;         // set IC3 priority
                                //

    //    // Right Encoder
    //    // IC2 Init
    IC2CONbits.ON = 0;          // turn off IC
    IC2CONbits.SIDL = 0;        // continuous idle mode
    IC2CONbits.C32 = 0;         // 16 bit mode
    IC2CONbits.ICTMR = 0;       // use timer 3
    IC2CONbits.ICI = 0;         // interrupt on every capture event
    IC2CONbits.ICM = 1;         // simple capture mode - every edge
    IC2R = 4;                   // use RPB9 pin
    IC2CONbits.ON = 1;          // turn on IC
    IFS0CLR = _IFS0_IC2IF_MASK; // clear
    IEC0SET = _IEC0_IC2IE_MASK; // enable
    IPC2bits.IC2IP = 6;         // set IC2 priority
                                ////
                                ////    // Conclude Timer and IC Setup
                                //    INTCONbits.MVEC = 1; // multivec
                                //
                                ////    // Configure Open Drain for 5v In
    TRISBbits.TRISB8 = 1;
    ODCBbits.ODCB8 = 1; // enable open drain configuration to take in 5V

    TRISBbits.TRISB9 = 1;
    ODCBbits.ODCB9 = 1;        // enable open drain configuration to take in 5V
                               //
                               //    // Init Timer 4 for Control
    T4CONbits.ON = 0;          // start by turning Timer4 off
    T4CONbits.TCS = 0;         // use PBClk as clock source
    T4CONbits.TGATE = 0;       // turn off gating
    T4CONbits.TCKPS = 0b010;   // divide by 4
    TMR4 = 0;                  // clear any leftover counts
    PR4 = 20000;               // want 4ms period control loop
    IFS0CLR = _IFS0_T4IF_MASK; // clear interrupt flag
    IEC0SET = _IEC0_T4IE_MASK; // enable interrupts

    IPC4bits.T4IP = 5; // timer priority
    T4CONbits.ON = 1;
    //

    ES_Timer_InitTimer(MOTOR_TIMER, QUERY_TIME);
    ES_Timer_InitTimer(DUTY_TIMER, DUTY_TIME);

    SetMotorRPM(0, 0);

    // Configure ADC to scan AN0 (A0), AN1 (A1), and AN2 (B2)
    ADC_ConfigAutoScan(BIT0HI | BIT1HI | BIT4HI | BIT5HI | BIT12HI);

    // Read initial ADC values
    ADC_MultiRead(ADValues);
    leftT = ADValues[0];
    rightT = ADValues[1];
    leftVal = ADValues[2];   // AN0 (RA0)
    centerVal = ADValues[3]; // AN1 (RA1)
    rightVal = ADValues[4];  // AN2 (RB2)

    ThisEvent.EventType = ES_INIT;
    DB_printf("end of init reached\n");
    // SetDutyCycle(100, 100);
    //    __builtin_disable_interrupts();
    // SetDutyCycle(20,20); //robot driving forward to follow tape
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
bool PostMotorService(ES_Event_t ThisEvent)
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
ES_Event_t RunMotorService(ES_Event_t ThisEvent)
{
    ES_Event_t ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

    switch (ThisEvent.EventType)
    {
    case ES_TIMEOUT:
    {
        if (ThisEvent.EventParam == DUTY_TIMER)
        {
            if (RPMControl == 1)
            {
                SetDutyCycle(DirectionS * LeftDC + DirectionT * LeftDC, DirectionS * RightDC - DirectionT * RightDC);
            }
            ES_Timer_InitTimer(DUTY_TIMER, DUTY_TIME);
        }

        if (ThisEvent.EventParam == DELAY_TIMER)
        {
            readyNewDetectionDelayTimer = 1;
            DB_printf("Delay Timer for new intersection complete\n");
        }

        if (ThisEvent.EventParam == TURN_TIMER)
        {
            readyTurningDone = 1;
            DB_printf("Turning Timer is Done!\n");
        }

        if (ThisEvent.EventParam == MOTOR_TIMER)
        {
            ES_Timer_InitTimer(MOTOR_TIMER, QUERY_TIME);
            ADC_MultiRead(ADValues);
            leftT = ADValues[0];
            if (leftT < MinLeftT)
            {
                MinLeftT = leftT;
            }
            if (leftT > MaxLeftT)
            {
                MaxLeftT = leftT;
            }
            rightT = ADValues[1];
            if (rightT < MinLeftT)
            {
                MinRightT = rightT;
            }
            if (rightT > MaxRightT)
            {
                MaxRightT = rightT;
            }
            leftVal = ADValues[2]; // AN0 (RA0)
            if (leftVal < MinLeftC)
            {
                MinLeftC = leftVal;
            }
            if (leftVal > MaxLeftC)
            {
                MaxLeftC = leftVal;
            }
            centerVal = ADValues[3]; // AN1 (RA1)
            if (centerVal < MinCentC)
            {
                MinCentC = centerVal;
            }
            if (centerVal > MaxCentC)
            {
                MaxCentC = centerVal;
            }
            rightVal = ADValues[4]; // AN2 (RB2)
            if (rightVal < MinRightC)
            {
                MinRightC = rightVal;
            }
            if (rightVal > MaxRightC)
            {
                MaxRightC = rightVal;
            }
            // ReadTapeValues(); //reads tape values to TeraTerm

            if (DirectionS == MOTOR_FORWARD && RPMControl == 0)
            {
                error = (rightVal - leftVal);

                sigma_e += error;

                static float kp = 0.07;
                static float kd = 0.1;
                static float ki = 0;

                LeftDC = DirectionS * LineFollowingSpeed + (error * kp + (error - error_p) * kd + sigma_e * ki);
                RightDC = DirectionS * LineFollowingSpeed - ((error * kp) + (error - error_p) * kd + sigma_e * ki);

                error_p = error;
                sigma_e += error;
                SetDutyCycle(LeftDC, RightDC);
            }
        }
    }
    break;

    case ES_NEW_KEY:
    {
        DB_printf("ES_NEW_KEY\n");
        newKey(ThisEvent);
    }
    break;

    case ES_STOP:
    {
        RPMControl = 0;
        LineFollowingSpeed = 0;

        DirectionS = MOTOR_STOP;
        DirectionT = MOTOR_STOP;

        ClearRPMErrors();
        SetMotorRPM(0, 0);

        //            __builtin_disable_interrupts();
        SetDutyCycle(LineFollowingSpeed, LineFollowingSpeed);
        DB_printf("ES_STOP\n");
        readyTurningDone = 0;
    }
    break;

    case ES_DFFULL:
    {
        // reset error
        UpdateStatus(STANDBY);
        ClearRPMErrors();

        RPMControl = 0;
        LineFollowingSpeed = 36; // Duty Cycle
        DirectionS = MOTOR_FORWARD;
        DirectionT = MOTOR_STOP;

        // resetting errors for line following
        sigma_error = 0;
        error = 0;
        error_p = 0;

        DesiredInts = ThisEvent.EventParam;

        if (ThisEvent.EventParam < 5)
        {
            DesiredInts = ThisEvent.EventParam;
            ES_Timer_InitTimer(DELAY_TIMER, INT_DELAY);
        }
        else
        {
            searchingT = 1;
        }

        SetDutyCycle(DirectionS * LeftDC + DirectionT * LeftDC, DirectionS * RightDC - DirectionT * RightDC);
        //            __builtin_enable_interrupts();
        DB_printf("ES_DFFULL %d intersections\n", DesiredInts);
    }
    break;

    case ES_MOVE_DIST:
    {
        UpdateStatus(STANDBY);
        DB_printf("Event Param: %d", ThisEvent.EventParam);
        RPMControl = true;
        if (ThisEvent.EventParam < 0)
        {
            DirectionS = MOTOR_BACKWARD;
        }
        else if (ThisEvent.EventParam > 0)
        {
            DirectionS = MOTOR_FORWARD;
        }
        DB_printf("DirectionS: %d", MOTOR_BACKWARD == DirectionS);
        DirectionT = MOTOR_STOP;

        ClearRPMErrors();

        RightEncoderEdgeDif = 0;
        LeftEncoderEdgeDif = 0;

        LeftDC = 0;
        RightDC = 0;

        LineFollowingSpeed = 30; // RPM

        SetMotorRPM(LineFollowingSpeed, LineFollowingSpeed);
        DB_printf("ES_MOVE_DIST\n");
        moveDistance(ThisEvent.EventParam);
    }
    break;

    case ES_DRFULL:
    {
        // reset error
        ClearRPMErrors();
        ClearLineErrors();

        RPMControl = 1;
        LineFollowingSpeed = 30; // Duty Cycle
        DirectionS = MOTOR_BACKWARD;
        DirectionT = MOTOR_STOP;

        RightEncoderEdgeDif = 0;
        LeftEncoderEdgeDif = 0;

        LeftDC = 0;
        RightDC = 0;

        LineFollowingSpeed = 30; // RPM

        SetMotorRPM(LineFollowingSpeed, LineFollowingSpeed);

        DesiredInts = ThisEvent.EventParam;

        if (ThisEvent.EventParam < 5)
        {
            FoundInts = 0;
            DesiredInts = ThisEvent.EventParam;
            ES_Timer_InitTimer(DELAY_TIMER, INT_DELAY);
        }
        else
        {
            searchingT = 1;
        }

        DB_printf("ES_DFFULL %d intersections\n", DesiredInts);
    }
    break;

    case ES_RCCW90:
    {
        UpdateStatus(STANDBY);
        DirectionS = MOTOR_STOP;
        DirectionT = MOTOR_TURNING_CCW;

        readyTurningDone = 0;
        SetDutyCycle(-35, 35);
        ES_Timer_InitTimer(TURN_TIMER, 700);
        DB_printf("ES_RCCW90\n");
    }
    break;

    case ES_RCW90:
    {
        UpdateStatus(STANDBY);
        DirectionS = MOTOR_STOP;
        DirectionT = MOTOR_TURNING_CW;

        readyTurningDone = 0;
        SetDutyCycle(35, -35);
        ES_Timer_InitTimer(TURN_TIMER, 700);
        DB_printf("ES_RCW90\n");
    }
    break;

    case ES_RCW90_RPM:
    {
        RPMControl = 1;
        DirectionS = MOTOR_STOP;
        DirectionT = MOTOR_TURNING_CW;

        ClearRPMErrors();

        LeftDC = 0;
        RightDC = 0;

        LineFollowingSpeed = 30; // RPM

        SetMotorRPM(LineFollowingSpeed, LineFollowingSpeed);
        moveDistance(ThisEvent.EventParam); // 201 comes from 2 * pi * 12.8 / 4 * 10 since 12.8cm

        UpdateStatus(STANDBY);
        //        readyTurningDone = 0;

        DB_printf("ES_RCCW90\n");
    }
    break;

    case ES_RCCW90_RPM:
    {
        RPMControl = 1;
        DirectionS = MOTOR_STOP;
        DirectionT = MOTOR_TURNING_CCW;

        ClearRPMErrors();

        LeftDC = 0;
        RightDC = 0;

        LineFollowingSpeed = 30; // RPM

        SetMotorRPM(LineFollowingSpeed, LineFollowingSpeed);
        moveDistance(ThisEvent.EventParam); // 201 comes from 2 * pi * 12.8 / 4 * 10 since 12.8cm

        UpdateStatus(STANDBY);
        //        readyTurningDone = 0;

        DB_printf("ES_RCCW90\n");
    }
    break;

    case ES_RCW_BEACON:
    {
        DirectionT = MOTOR_TURNING_CW;
        SetDutyCycle(30, -30);
        DB_printf("ES_RCW90\n");
    }
    break;

    case ES_RCW_TAPE:
    {
        DirectionT = MOTOR_TURNING_CW;
        readyTurningDone = 1;
        initTurn = 1;
        SetDutyCycle(30, -30);
        DB_printf("ES_RCW UNTIL TAPE IN INIT\n");
    }
    break;

    case ES_T_REACHED:
    {
        FoundT = 1;
        if (DesiredTs == FoundT && DesiredTs > 0)
        {
            ES_Event_t temp;
            temp.EventType = ES_STOP;
            DirectionS = MOTOR_STOP;
            PostMotorService(temp);
            FoundT = 0;
        }
    }
    break;

    case ES_INT_REACHED:
    {
        FoundInts++;
        if (DesiredInts == FoundInts && DesiredInts > 0)
        {
            ES_Event_t temp;
            temp.EventType = ES_STOP;
            PostMotorService(temp);
            FoundInts = 0;
            DesiredInts = 0;
            readyNewDetectionDelayTimer = 0;
            UpdateStatus(COMMAND_DONE);
        }
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
void SetMotorRPM(int8_t LeftRPM, int8_t RightRPM)
{
    LeftTargetRPM = LeftRPM;
    RightTargetRPM = RightRPM;
}

/***************************************************************************
 Private functions
 ***************************************************************************/
/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/
// Set Duty cycle for motor in both directions
void SetDutyCycle(int8_t LeftDC, int8_t RightDC)
{
    if (RightDC == 0)
    {
        OC2RS = 0;
        LATAbits.LATA2 = 0;
    }

    if (LeftDC == 0)
    {
        OC1RS = 0;
        LATAbits.LATA3 = 0;
    }

    if (RightDC > 0)
    {
        LATAbits.LATA2 = 0;
        OC2RS = (PR2 + 1) * RightDC / 100;
    }

    else if (RightDC < 0)
    {
        LATAbits.LATA2 = 1;
        OC2RS = (PR2 + 1) * (100 + RightDC) / 100;
    }

    if (LeftDC > 0)
    {
        LATAbits.LATA3 = 1;
        OC1RS = (PR2 + 1) * (100 - LeftDC) / 100;
    }

    else if (LeftDC < 0)
    {
        LATAbits.LATA3 = 0;
        OC1RS = (PR2 + 1) * (-LeftDC) / 100;
    }
}

bool Check4Intersection(void)
{
    if (DirectionS == MOTOR_FORWARD || DirectionS == MOTOR_BACKWARD)
    {
        static uint32_t LeftThreshT;
        LeftThreshT = (MaxLeftT - MinLeftT) / THRESH_DIV + MinLeftT;
        static uint32_t RightThreshT;
        RightThreshT = (MaxRightT - MinRightT) / THRESH_DIV + MinRightT;
        if ((leftT > LeftThreshT || rightT > RightThreshT) && readyNewDetection == 1)
        {
            DB_printf("INTERSECTION DETECTED\n");
            ES_Event_t new_event;
            new_event.EventType = ES_INT_REACHED;
            PostMotorService(new_event);
            readyNewDetection = 0;
            return true;
        }
        else if (readyNewDetection == 0 && readyNewDetectionDelayTimer == 1)
        {
            if (leftT < LeftThreshT && rightT < RightThreshT)
            {
                readyNewDetection = 1;
                DB_printf("NEW DETECTION RESET\n");
            }
        }
    }
    return false;
}
//
bool Check4Turn(void)
{
    if ((DirectionT == MOTOR_TURNING_CW || DirectionT == MOTOR_TURNING_CCW) && readyTurningDone == 1)
    {
        // DB_printf("waiting for centerVal to go above TAPE_THRESH");
        //        DB_printf("%d", centerVal);
        static uint32_t LeftThreshC;
        LeftThreshC = (MaxLeftC - MinLeftC) / THRESH_DIV + MinLeftC;
        if (leftVal > LeftThreshC)
        {
            ES_Event_t event;
            if (initTurn == 1)
            {
                UpdateStatus(TAPE_FOUND);
                DB_printf("Tape Found on initial turn!\n");
            }
            else
            {
                UpdateStatus(COMMAND_DONE);
            }

            event.EventType = ES_STOP;
            PostMotorService(event);
            readyTurningDone = 0;
            initTurn = 0;

            return true;
        }
    } // else if (direction == MOTOR_TURNING && readyTurningDone == 0) {
    //        if (leftVal < 30) {
    //            readyTurningDone = 1;
    //            DB_printf("READYTURNINGDONE is HIGH\n");
    //        }
    //    }
    return false;
}

bool Check4T(void)
{
    if ((DirectionS == MOTOR_FORWARD || DirectionS == MOTOR_BACKWARD) && searchingT == 1)
    {
        static uint32_t LeftThreshC;
        LeftThreshC = (MaxLeftC - MinLeftC) / THRESH_DIV + MinLeftC;
        static uint32_t RightThreshC;
        RightThreshC = (MaxRightC - MinRightC) / THRESH_DIV + MinRightC;
        if (leftVal > LeftThreshC && rightVal > RightThreshC)
        {
            UpdateStatus(T_FOUND);
            ES_Event_t event;
            event.EventType = ES_STOP;
            PostMotorService(event);
            searchingT = 0;
            // UpdateStatus(COMMAND_DONE); // Getting rid of this @ nick, will cause problems in MoveSM
        }
    }
}

void __ISR(_TIMER_3_VECTOR, IPL6SOFT) Timer3Handler(void)
{
    __builtin_disable_interrupts();
    if (IFS0bits.T3IF == 1)
    {
        LeftTimerRollOver++;
        RightTimerRollOver++;
        IncrementBeaconTimer();
        IFS0CLR = _IFS0_T3IF_MASK; // clearing TMR3 flag
    }
    __builtin_enable_interrupts();
}

// Left Motor Input Capture
void __ISR(_INPUT_CAPTURE_3_VECTOR, IPL6SOFT) LeftEncoderIntHandler(void)
{
    // immediately capture edge time
    uint16_t ThisEdge = IC3BUF;

    // check if buffer is not empty
    while (IC3CONbits.ICBNE == 1)
    {
        ThisEdge = IC3BUF;
    }

    // clear flag only after buffer is empty
    IFS0CLR = _IFS0_IC3IF_MASK;

    // check for TMR3 interrupt edge case
    // T2IF is pending and ThisEdge is after roll over
    if (IFS0bits.T3IF == 1 && ThisEdge < 0x8000)
    {
        LeftTimerRollOver++;
        RightTimerRollOver++;
        IFS0CLR = _IFS0_T3IF_MASK; // clear timer interrupt
    }

    // Combine roll over with ThisEdge time for total time
    uint32_t LeftThisEdgeTotal = ((uint32_t)LeftTimerRollOver << 16) | ThisEdge;
    LeftEncoderEdgeDif = LeftThisEdgeTotal - LeftLastEncoderEdge;
    LeftLastEncoderEdge = LeftThisEdgeTotal;
}

// Right Motor Input Capture
void __ISR(_INPUT_CAPTURE_2_VECTOR, IPL6SOFT) RightEncoderIntHandler(void)
{
    // immediately capture edge time
    uint16_t ThisEdge = IC2BUF;

    if (CountDistance == 1)
    {
        encoder_count++;
    }

    if (encoder_count >= end_encoder_count && CountDistance == 1)
    {
        // post event that distance has been reached and STOP THE VEHICLE ASAP!
        ES_Event_t new_event;
        new_event.EventType = ES_STOP;
        PostMotorService(new_event);
        UpdateStatus(COMMAND_DONE);
        CountDistance = 0;
    }

    // check if buffer is not empty
    while (IC2CONbits.ICBNE == 1)
    {
        ThisEdge = IC2BUF;
    }

    // clear flag only after buffer is empty
    IFS0CLR = _IFS0_IC2IF_MASK;

    // check for TMR3 interrupt edge case
    // T2IF is pending and ThisEdge is after roll over
    if (IFS0bits.T3IF == 1 && ThisEdge < 0x8000)
    {
        LeftTimerRollOver++;
        RightTimerRollOver++;
        //        IncrementBeaconTimer();
        IFS0CLR = _IFS0_T3IF_MASK; // clear timer interrupt
    }

    // Combine roll over with ThisEdge time for total time
    uint32_t RightThisEdgeTotal = ((uint32_t)RightTimerRollOver << 16) | ThisEdge;
    RightEncoderEdgeDif = RightThisEdgeTotal - RightLastEncoderEdge;
    RightLastEncoderEdge = RightThisEdgeTotal;
}
//
//// Control ISR
// Control ISR
void __ISR(_TIMER_4_VECTOR, IPL5SOFT) ControlHandler(void)
{
    if (IFS0bits.T4IF == 1)
    {
        IFS0CLR = _IFS0_T4IF_MASK; // clearing TMR3 flag

        if (LeftEncoderEdgeDif == 0)
        {
            LeftRPM = 0;
        }
        else
        {
            LeftRPM = 60 * 5e6 / (6 * 50 * LeftEncoderEdgeDif);
        }

        LeftError = (float)LeftTargetRPM - (float)LeftRPM;
        LeftSigmaError = LeftSigmaError + LeftError;
        LeftDC = P_Constant * LeftError + I_Constant * LeftSigmaError;

        // anti windup
        if (LeftDC > 100)
        {
            LeftSigmaError = LeftSigmaError - LeftError;
            LeftDC = 100;
        }

        else if (LeftDC < 0)
        {
            LeftDC = 0;
            LeftSigmaError = LeftSigmaError - LeftError;
        }

        if (RightEncoderEdgeDif == 0)
        {
            RightRPM = 0;
        }
        else
        {
            RightRPM = 60 * 5e6 / (6 * 50 * RightEncoderEdgeDif);
        }

        RightError = (float)RightTargetRPM - (float)RightRPM;
        // DB_printf("RightError = %d\n", RightError);

        RightSigmaError = RightSigmaError + RightError;
        RightDC = P_Constant * RightError + I_Constant * RightSigmaError;

        // anti windup
        if (RightDC > 100)
        {
            RightSigmaError = RightSigmaError - RightError;
            RightDC = 100;
        }
        else if (RightDC < 0)
        {
            RightSigmaError = RightSigmaError - RightError;
            RightDC = 0;
        }
    }
}

void newKey(ES_Event_t ThisEvent)
{
    ES_Event_t newEvent;
    if (ThisEvent.EventParam == 'w')
    {
        newEvent.EventType = ES_DFFULL;
    }
    else if (ThisEvent.EventParam == 's')
    {
        newEvent.EventType = ES_DRFULL;
    }
    else if (ThisEvent.EventParam == 'a')
    {
        newEvent.EventType = ES_RCCW90;
    }
    else if (ThisEvent.EventParam == 'd')
    {
        newEvent.EventType = ES_RCW90;
    }
    else if (ThisEvent.EventParam == 'q')
    {
        newEvent.EventType = ES_STOP;
    }
    else if (ThisEvent.EventParam == 'l')
    {
        newEvent.EventType = ES_TAPEL;
    }
    else if (ThisEvent.EventParam == 'r')
    {
        newEvent.EventType = ES_TAPER;
    }
    else if (ThisEvent.EventParam == 'm')
    {
        newEvent.EventType = ES_MOVE_DIST;
    }
    PostMotorService(newEvent);
}

void moveDistance(int16_t distance)
{
    encoder_count = 0;
    float temp = (((float)(abs(distance))) / (PI * WHEEL_DIAMETER)) * 6 * 50;
    end_encoder_count = (uint32_t)(temp);
    CountDistance = 1;
    DB_printf("TARGET DISTANCE: %d\n", end_encoder_count);
}

static void ClearRPMErrors(void)
{
    LeftError = 0;
    LeftSigmaError = 0;
    RightError = 0;
    RightSigmaError = 0;
}

static void ClearLineErrors(void)
{
    sigma_error = 0;
    error = 0;
    error_p = 0;
}

static void ReadTapeValues(void)
{
    DB_printf("LeftT = %d    ", leftT);
    DB_printf("RightT = %d   ", rightT);
    DB_printf("LeftVal = %d   ", leftVal);
    DB_printf("LeftVal = %d   ", centerVal);
    DB_printf("LeftVal = %d   ", rightVal);
    DB_printf("-------------------------------------\n");
}