// FOLLOWER BEACONSERVICE
/****************************************************************************
 Module
   BeaconService.c

 Revision
   1.0.1

 Description
 This is a service for IR beacon detection, calculating the frequency of the
 beacon using interrupts

 Notes

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// This module
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

#include "Pic2PicFollowerFSM.h"

/*----------------------------- Module Defines ----------------------------*/
// these times assume a 10.000mS/tick timing
#define ONE_SEC 1000
#define HALF_SEC (ONE_SEC / 2)
#define TWO_SEC (ONE_SEC * 2)
#define FIVE_SEC (ONE_SEC * 5)

#define ENTER_POST ((MyPriority << 3) | 0)
#define ENTER_RUN ((MyPriority << 3) | 1)
#define ENTER_TIMEOUT ((MyPriority << 3) | 2)

#define Beacon_Buffer 100

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
// add a deferral queue for up to 3 pending deferrals +1 to allow for overhead
static ES_Event_t DeferralQueue[3 + 1];

static bool SearchBeacon = 0; // FLAG MUST BE ON TO SEND EVENT TO HSM
volatile static uint8_t RCounter = 0;
volatile static uint8_t BCounter = 0;
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
#define STEP 5
#define PWM_PERIOD = 2 // 200Hz: 1/200 = 5ms
// hello

#define TOTAL_ENCODER_PULSES 512    // 0 - 511 encoder pulses
#define PRINT_TERA_TERM 100         // 100 ms timer for printing
#define INTERNAL_TIMER3_COUNTER 200 // time in nano seconds
Time_t t1;
Time_t t2;
uint8_t edges = 0;

bool InitBeaconService(uint8_t Priority)
{
    ES_Event_t ThisEvent;

    MyPriority = Priority;

    // When doing testing, it is useful to announce just which program
    // is running.
    //  clrScrn();
    puts("\rStarting BeaconService \r");

    PR2 = 2000 / 2 - 1; // prescaling by 2 and then subtracting 1 for rollover

    // setup input capture for part 2
    T3CONbits.ON = 0;    // turn timer 2 off to change stuff
    T3CONbits.TCS = 0;   // selecting internal clock
    T3CONbits.TGATE = 0; // disabling the gate
    T3CONbits.TCKPS = 2; // prescale by 4
    PR3 = 65535;         // prescaling by 2 and then subtracting 1 for rollover
    TMR3 = 0;
    IFS0CLR = _IFS0_T3IF_MASK; // clearing the interrupt flag

    TRISAbits.TRISA4 = 1; // setting this to digital input
    //  ODCBbits.ODCB10 = 1; //enable open drain configuration to take in 5V

    // input capture settings

    __builtin_disable_interrupts();

    IC1CONbits.ON = 0;
    IC1CONbits.SIDL = 0;
    IC1CONbits.C32 = 0;
    IC1CONbits.ICTMR = 0;
    IC1CONbits.ICI = 0;
    IC1CONbits.ICM = 0b010;
    IC1CONbits.ON = 1;

    T3CONbits.ON = 1; // enabling the timer

    IEC0SET = _IEC0_T3IE_MASK;  // enable local timeout interrupt for timer 3
    IEC0SET = _IEC0_IC1IE_MASK; // enable local interrupt for input capture
    INTCONbits.MVEC = 1;

    IPC1bits.IC1IP = 7; // setting the priority of the Encoder
    IPC3bits.T3IP = 6;

    IC1Rbits.IC1R = 0b0010; // mapping IC1 to RA4

    // clearing interrupts
    IFS0CLR = _IFS0_T3IF_MASK;
    IFS0CLR = _IFS0_IC1IF_MASK;

    __builtin_enable_interrupts();

    // setting initial time calculations
    t2.TotalTime = 0;
    t1.TotalTime = 0;
    t2.SplitTimers[1] = 0;
    t2.SplitTimers[0] = 0;
    t1.SplitTimers[0] = 0;
    t1.SplitTimers[1] = 0;
    edges = 0;

    // ES_Timer_InitTimer(TERATIMER, PRINT_TERA_TERM);
    ThisEvent.EventType = ES_INIT;
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
bool PostBeaconService(ES_Event_t ThisEvent)
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
ES_Event_t RunBeaconService(ES_Event_t ThisEvent)
{
    ES_Event_t ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

    switch (ThisEvent.EventType)
    {
    case ES_INIT:
    {
        ThisEvent.EventType = ES_FIND_BEACON;
        PostBeaconService(ThisEvent);
    }
    break;

    case ES_BEACON_FOUND:
    {
        //          __builtin_disable_interrupts();
        int16_t period = ThisEvent.EventParam;
        edges = 0;
        DB_printf("IR Period Detected: = %d us\n", period);

        if (700 - Beacon_Buffer < period && period < 700 + Beacon_Buffer)
        {
            BCounter++;
            RCounter = 0;
            DB_printf("Stack B Detected for Counter: %d\n", BCounter);
            if (BCounter >= 10)
            {
                ES_Event_t event;
                event.EventType = ES_STACKB;
                DB_printf("Stack B Detected!\n");
                UpdateStatus(BEACON_FOUND_B);
                SearchBeacon = 0;
            }
        }
        else if (1100 - Beacon_Buffer < period && period < 1100 + Beacon_Buffer)
        {
            RCounter++;
            DB_printf("Stack R Detected for Counter: %d\n", RCounter);
            BCounter = 0;

            if (RCounter >= 10)
            {
                ES_Event_t event;
                event.EventType = ES_STACKR;
                DB_printf("Stack R Detected!\n");
                UpdateStatus(BEACON_FOUND_R);
                SearchBeacon = 0;
            }
        }
        else
        {
            RCounter = 0;
            BCounter = 0;
            // DB_printf("No beacon detected, but bad signal\n");
        }
    }
    break;

    case ES_RAISE_FLAG:
    {
        SearchBeacon = 1;
        DB_printf("Beacon Flag RAISED\n");
    }
    break;

        break;
    default:
    {
    }
    break;
    }

    return ReturnEvent;
}

/***************************************************************************
 private functions
 ***************************************************************************/
/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

// ISR for the input capture
void __ISR(_INPUT_CAPTURE_1_VECTOR, IPL7SOFT) InputCaptureResponse(void)
{
    edges++;

    t2.SplitTimers[0] = IC1BUF; // store the newest number
    IFS0CLR = _IFS0_IC1IF_MASK; // clear the interrupt from the input capture

    // if the timer interrupt is pending and after rollover
    if (IFS0bits.T3IF && t2.SplitTimers[1] < 0x8000) // a rollover occurred
    {
        t2.SplitTimers[1]++;
        IFS0CLR = _IFS0_T3IF_MASK; // clear the rollover interrupt
    }
    
    if (edges == 1)
    {
        t1.TotalTime = t2.TotalTime;
    }
    else if (edges == 4)
    {

        // ticks t2 - t1 = number of ticks
        // each tick goes off at a rate of 50ns * prescale value
        uint16_t dt = (uint16_t)((t2.TotalTime - t1.TotalTime) / (5 * (edges - 1)));

        ES_Event_t event = {ES_BEACON_FOUND, dt};
        edges = 0;
        if (SearchBeacon == 1)
        {
            PostBeaconService(event);
        }
    }

    if (edges != 4)
    {
        //        ES_Event_t event = {ES_FIND_BEACON,0};
        //        PostBeacon(event);
    }

    IFS0CLR = _IFS0_IC1IF_MASK;
    IEC0SET = _IEC0_IC1IE_MASK; // Enable IC2 interrupt
}

void IncrementBeaconTimer(void)
{
    t2.SplitTimers[1]++;
}