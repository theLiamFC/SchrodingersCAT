/****************************************************************************
 Module
   Pic2PicFollowerFSM.c

 Revision
   1.0.1

 Description
 This file handles the communication between PIC's using SPI communication

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 01/15/12 11:12 jec      revisions for Gen2 framework
 11/07/11 11:26 jec      made the queue static
 10/30/11 17:59 jec      fixed references to CurrentEvent in RunTemplateSM()
 10/23/11 18:20 jec      began conversion from SMTemplate.c (02/20/07 rev)
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "Pic2PicFollowerFSM.h"
#include "PIC32_SPI_HAL.h"
#include <sys/attribs.h>
#include "dbprintf.h"
#include "MotorService.h"
#include "BeaconService.h"

/*----------------------------- Module Defines ----------------------------*/
#define START_COMM 0xAA
#define END_COMM 0xFF
#define THREE_SEC 3000

#define LED LATAbits.LATA0 // debugging LED

#define INTAKE_CRATE_1 -10 // units in CM
#define BACKUP_INTAKE 6
#define INTAKE_CRATE_2 -13 // units in CM
#define INTAKE_CRATE_3 -18 // units in CM
#define SHAKE_DIST 2
#define BACKUP_BEFORE_SHAKE 4

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/
void __ISR(_SPI1_VECTOR, IPL7SOFT) SPI1_Handler(void);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static Pic2PicFollowerFSM_t CurrentState;
volatile static uint8_t CurrentCommand = 0;
static uint8_t NewCommand;
static uint8_t CurrentStatus = 0;
static uint8_t ClearBuffer;

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitTemplateFSM

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, sets up the initial transition and does any
     other required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 10/23/11, 18:55
****************************************************************************/
bool InitPic2PicFollowerFSM(uint8_t Priority)
{
    ES_Event_t ThisEvent;

    MyPriority = Priority;
    // put us into the Initial PseudoState

    ////      LED FLAG initialization       ////
    //    TRISAbits.TRISA0 = 0;

    ////      SPI initialization       ////
    DB_printf("Pic2Pic Follower Init\n");

    SPISetup_BasicConfig(SPI_SPI1);
    // SPISetup_SetFollower(SPI_SPI1);
    SPI1CONbits.MSTEN = 0;
    ANSELBbits.ANSB14 = 0; // clock
    TRISBbits.TRISB8 = 0;  // SDO to output

    // Setup SS Input on RPB15
    TRISBbits.TRISB15 = 1;
    SS1R = 0b0011;

    // Setup SDI on RB11
    TRISBbits.TRISB11 = 1;
    SDI1R = 0b0011;

    uint32_t ClkPeriod_ns = 2400; // 833KHz
    SPISetup_SetBitTime(SPI_SPI1, ClkPeriod_ns);

    IFS0CLR = _IFS0_INT4IF_MASK;
    SPISetup_MapSDOutput(SPI_SPI1, SPI_RPB13);
    SPISetup_SetClockIdleState(SPI_SPI1, SPI_CLK_HI);
    SPISetup_SetActiveEdge(SPI_SPI1, SPI_SECOND_EDGE);
    SPISetup_SetXferWidth(SPI_SPI1, SPI_8BIT);
    SPISetup_Interrupts(SPI_SPI1);
    SPISetEnhancedBuffer(SPI_SPI1, true);
    SPISetup_EnableSPI(SPI_SPI1);

    __builtin_enable_interrupts();

    CurrentState = Receive;
    // post the initial transition event
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
     PostTemplateFSM

 Parameters
     EF_Event_t ThisEvent , the event to post to the queue

 Returns
     boolean False if the Enqueue operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostPic2PicFollowerFSM(ES_Event_t ThisEvent)
{
    return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunTemplateFSM

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event_t, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 01/15/12, 15:23
****************************************************************************/
ES_Event_t RunPic2PicFollowerFSM(ES_Event_t ThisEvent)
{
    ES_Event_t ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

    switch (CurrentState)
    {

    case Receive:
    {
        if (ThisEvent.EventType == PIC2_COMM)
        {
            DB_printf("Follower Start Received\n");
        }
        else if (ThisEvent.EventType == ES_TIMEOUT)
        {
            if (ThisEvent.EventParam == COMMAND_DELAY_TIMER)
            {
                ES_Event_t event;
                event.EventType = PIC2_RECEIVE;
                event.EventParam = NewCommand;
                PostPic2PicFollowerFSM(event);
            }
        }
        else if (ThisEvent.EventType == PIC2_RECEIVE)
        {

            DB_printf("Follower Receive Command %d\n", ThisEvent.EventParam);
            ES_Event_t event;

            // UpdateStatus(STANDBY);

            if (ThisEvent.EventParam == SEARCH_BEACON)
            {

                DB_printf("2 received\n");
                event.EventType = ES_RAISE_FLAG;
                PostBeaconService(event);
                event.EventType = ES_RCW_BEACON;
                PostMotorService(event);
                //                ES_Timer_InitTimer(TEST_TIMER,THREE_SEC);
            }
            else if (ThisEvent.EventParam == DFFULL_1)
            {
                event.EventType = ES_DFFULL;
                event.EventParam = 1; // telling MotorService to only move 1 intersection
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == DFFULL_2)
            {
                event.EventType = ES_DFFULL;
                event.EventParam = 2; // telling MotorService to only move 1 intersection
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == DFFULL_3)
            {
                event.EventType = ES_DFFULL;
                event.EventParam = 3; // telling MotorService to only move 1 intersection
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == DFFULL_4)
            {
                event.EventType = ES_DFFULL;
                event.EventParam = 4; // telling MotorService to only move 1 intersection
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == STOP)
            {
                event.EventType = ES_STOP;
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == DRFULL)
            {
                UpdateStatus(STANDBY);
                event.EventType = ES_DRFULL;
                event.EventParam = 1;
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == RCCW90)
            {
                event.EventType = ES_RCCW90;
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == RCW90)
            {
                event.EventType = ES_RCW90;
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == DFFULL_T)
            {
                UpdateStatus(STANDBY);
                event.EventType = ES_DFFULL;
                event.EventParam = 5; // drive until T
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == RCW_TO_TAPE)
            {
                event.EventType = ES_RCW_TAPE;
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == CRATE_HIGH_KNOCK)
            {
                UpdateStatus(STANDBY);
                event.EventType = ES_MOVE_DIST;
                event.EventParam = 12; // cm // changed from 9
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == CRATE_HIGH)
            {
                UpdateStatus(STANDBY);
                event.EventType = ES_MOVE_DIST;
                event.EventParam = 8; // cm // changed from 9
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == CRATE_MID)
            {
                UpdateStatus(STANDBY);
                event.EventType = ES_MOVE_DIST;
                event.EventParam = 19; // cm
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == CRATE_LOW)
            {
                UpdateStatus(STANDBY);
                event.EventType = ES_MOVE_DIST;
                event.EventParam = 1; // cm
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == DR_STACK)
            {
                UpdateStatus(STANDBY);
                event.EventType = ES_MOVE_DIST;
                event.EventParam = -20; // cm
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == DRFULL_T)
            {
                UpdateStatus(STANDBY);
                event.EventType = ES_DRFULL;
                event.EventParam = 5; // drive until T
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == DR_5CM)
            {
                UpdateStatus(STANDBY);
                event.EventType = ES_MOVE_DIST;
                event.EventParam = -5; // cm
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == DR_15CM)
            {
                UpdateStatus(STANDBY);
                event.EventType = ES_MOVE_DIST;
                event.EventParam = -15; // cm
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == MOVE_INTAKE)
            {
                UpdateStatus(STANDBY);
                event.EventType = ES_MOVE_DIST;
                event.EventParam = -23;
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == RCW_RPM)
            {
                UpdateStatus(STANDBY);
                event.EventParam = 20;
                event.EventType = ES_RCW90_RPM;
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == RCCW_RPM)
            {
                UpdateStatus(STANDBY);
                event.EventType = ES_RCCW90_RPM;
                event.EventParam = 20;
                PostMotorService(event);
                //            } else if (ThisEvent.EventParam == BEGIN_INTAKE) {
                //                UpdateStatus(STANDBY);
                //                event.EventType = ES_RCW90_RPM;
                //                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == GET_BLOCK1)
            {
                UpdateStatus(STANDBY);
                event.EventType = ES_MOVE_DIST;
                event.EventParam = INTAKE_CRATE_1;
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == GET_BLOCK2)
            {
                UpdateStatus(STANDBY);
                event.EventType = ES_MOVE_DIST;
                event.EventParam = INTAKE_CRATE_2;
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == GET_BLOCK3)
            {
                UpdateStatus(STANDBY);
                event.EventType = ES_MOVE_DIST;
                event.EventParam = INTAKE_CRATE_3;
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == REV_INTAKE)
            {
                UpdateStatus(STANDBY);
                event.EventType = ES_MOVE_DIST;
                event.EventParam = BACKUP_INTAKE;
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == SHAKE)
            {
                UpdateStatus(STANDBY);
                event.EventType = ES_RCW90_RPM;
                event.EventParam = SHAKE_DIST;
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == BAKE)
            {
                UpdateStatus(STANDBY);
                event.EventType = ES_RCCW90_RPM;
                event.EventParam = SHAKE_DIST;
                PostMotorService(event);
            }
            else if (ThisEvent.EventParam == BACKUP_BEFORE_SHAKE_C)
            {
                UpdateStatus(STANDBY);
                event.EventType = ES_MOVE_DIST;
                event.EventParam = BACKUP_BEFORE_SHAKE;
                PostMotorService(event);
            }
        }
    }
    break;

    // repeat state pattern as required for other states
    default:;
    } // end switch on Current State
    return ReturnEvent;
}

/****************************************************************************
 Function
     QueryTemplateSM

 Parameters
     None

 Returns
     TemplateState_t The current state of the Template state machine

 Description
     returns the current state of the Template state machine
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:21
****************************************************************************/
Pic2PicFollowerFSM_t QueryPic2PicFollowerFSM(void)
{
    return CurrentState;
}

void UpdateStatus(uint8_t Status)
{
    CurrentStatus = Status;
    DB_printf("Status Updated To: %d\n", CurrentStatus);
}

/***************************************************************************
 private functions
 ***************************************************************************/
void __ISR(_SPI1_VECTOR, IPL7SOFT) SPI1_Handler(void)
{
    // Check if RX buffer is full
    if (IFS1bits.SPI1RXIF)
    {
        // Read from buffer and store in module level var
        NewCommand = SPI1BUF;
        // Clear the RX interrupt flag
        IFS1CLR = _IFS1_SPI1RXIF_MASK;
        // ClearBuffer = SPI1BUF;
        SPI1BUF = CurrentStatus;
        //        CurrentStatus=0;//Only send an updated status once

        if (NewCommand != CurrentCommand)
        {
            LED = 1;

            // DB_printf("New Command in Follower ISR: %d\n", NewCommand);
            ES_Event_t ThisEvent;
            ES_Timer_InitTimer(COMMAND_DELAY_TIMER, 100);

            LED = 0;
        }
        CurrentCommand = NewCommand;
    }
}