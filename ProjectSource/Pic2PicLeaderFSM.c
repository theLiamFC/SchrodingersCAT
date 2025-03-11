/****************************************************************************
 Module
   Pic2PicLeaderFSM.c

 Revision
   1.0.1

 Description
   This is the state machine for the leader PIC in the Pic2Pic game.

 Notes
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "Pic2PicLeaderFSM.h"
#include "PIC32_SPI_HAL.h"
#include <sys/attribs.h>
#include "dbprintf.h"
#include "ServoService.h"
#include "TopLevelSM.h"
#include "IntakeService.h"

/*----------------------------- Module Defines ----------------------------*/
#define START_COMM 0xAA
#define END_COMM 0xFF
#define COMM_RATE 10 // ms -> 10Hz
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static Pic2PicLeaderFSM_t CurrentState;
static uint8_t CurrentCommand = 0;
static uint8_t NewStatus = 0;
volatile static uint8_t CurrentStatus;
static uint8_t LastSent = 0; //potentially unnecessary

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
bool InitPic2PicLeaderFSM(uint8_t Priority)
{
    ES_Event_t ThisEvent;

    MyPriority = Priority;
    // put us into the Initial PseudoState
    CurrentState = Send;
    
    ////      SPI initialization       ////
    DB_printf("Pic2Pic Leader Init\n");
    uint32_t ClkPeriod_ns = 1200; // 833KHz

    SPISetup_BasicConfig(SPI_SPI1);
    SPISetup_SetLeader(SPI_SPI1, SPI_SMP_MID);
    SPISetup_SetBitTime(SPI_SPI1, ClkPeriod_ns);
    SPISetup_MapSSOutput(SPI_SPI1, SPI_RPB15);

    // configure the SDI pin to work for taking in information
    TRISBbits.TRISB11 = 1;
    SDI1R = 0b0011;
    // ^ should move into SPI HAL

    SPISetup_MapSDOutput(SPI_SPI1, SPI_RPB13);
    SPISetup_SetClockIdleState(SPI_SPI1, SPI_CLK_HI);
    SPISetup_SetActiveEdge(SPI_SPI1, SPI_SECOND_EDGE);
    SPISetup_SetXferWidth(SPI_SPI1, SPI_8BIT);
    SPISetup_Interrupts(SPI_SPI1);
    SPISetEnhancedBuffer(SPI_SPI1, true);
    SPISetup_EnableSPI(SPI_SPI1);

    IFS0CLR = _IFS0_INT4IF_MASK;
    
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
bool PostPic2PicLeaderFSM(ES_Event_t ThisEvent)
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
ES_Event_t RunPic2PicLeaderFSM(ES_Event_t ThisEvent)
{
    ES_Event_t ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

    switch (CurrentState)
    {
    case Send:
    {
        if (ThisEvent.EventType == ES_INIT)
        {
            ES_Timer_InitTimer(PIC2_TIMER, COMM_RATE);
        }
        if (ThisEvent.EventType == ES_TIMEOUT) // send new command
        {
            if (ThisEvent.EventParam == PIC2_TIMER)
            {
                //DB_printf("Leader Send Comm %d\n", CurrentCommand);
                //LastSent currently unused, thought of as a possible fix
                //To sometimes missing sending a new command if 2 come through
                //quickly
                //LastSent = CurrentCommand; 
                SPI1BUF = CurrentCommand; 
                //CurrentCommand++;
                ES_Timer_InitTimer(PIC2_TIMER, COMM_RATE);
            }
            
        }
        else if (ThisEvent.EventType == PIC2_RECEIVE) // received new status
        {   
            DB_printf("Leader Receive Status %d\n", CurrentStatus);
            ES_Event_t event;
            if (ThisEvent.EventParam == BEACON_FOUND_R) {
                SetIndicator(INDICATOR_GREEN);

                event.EventType = ES_STACKR;
                PostTopLevelSM(event);
            } else if (ThisEvent.EventParam == BEACON_FOUND_B) {
                SetIndicator(INDICATOR_BLUE);

                event.EventType = ES_STACKB;
                PostTopLevelSM(event);
            } else if (ThisEvent.EventParam == COMMAND_DONE) {
                event.EventType = ES_COMMAND_DONE;
                PostTopLevelSM(event);
                PostIntakeService(event);
            } else if (ThisEvent.EventParam == TAPE_FOUND_INIT) {
                event.EventType = ES_TAPE_FOUND_INIT;
                PostTopLevelSM(event);
            } else if (ThisEvent.EventParam == T_FOUND) {
                event.EventType = ES_TAPE_FOUND;
                PostTopLevelSM(event);
            } else if (ThisEvent.EventParam == CRATE_ALIGNED) {
                event.EventType = ES_EXIT;
                PostTopLevelSM(event);
            } else if (ThisEvent.EventParam == DISTANCE_REACHED) {
                event.EventType = ES_DISTANCE_REACHED;
                PostTopLevelSM(event);
            } 
        }
    }
    break;
    
    default:
    {
    } // end switch on Current State
    return ReturnEvent;
}
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
Pic2PicLeaderFSM_t QueryPic2PicLeaderFSM(void)
{
    return CurrentState;
}

void SetFollowerCommand(uint8_t Command)
{
    CurrentCommand = Command;
    DB_printf("Leader Command %d\n", Command);
}

uint8_t QueryFollowerStatus(void)
{
    return CurrentStatus;
}

/***************************************************************************
 private functions
 ***************************************************************************/

/*
 ****************************************************************************
 Function
     SPI1_Handler

 Parameters
     None

 Returns
     None

 Description
     Interrupt response routine for the SPI1 receive interrupt
     which is set to trigger when a byte is received from the follower
     The received byte is stored in NewStatus and if this is different
     from CurrentStatus, an event of type PIC2_RECEIVE is posted to the
     Pic2PicLeaderFSM with the new status as the parameter
****************************************************************************/
void __ISR(_SPI1_VECTOR, IPL7SOFT) SPI1_Handler(void)
{
    // Check if RX buffer is full
    if (IFS1bits.SPI1RXIF)
    {
        // Read from buffer and store in module level var
        NewStatus = (uint8_t)SPI1BUF;
        // Clear the RX interrupt flag
        IFS1CLR = _IFS1_SPI1RXIF_MASK;
        
        if (NewStatus != CurrentStatus) {
            //DB_printf("Current Status in Leader ISR: %d\n",CurrentStatus / 2);
            ES_Event_t ThisEvent;
            ThisEvent.EventType = PIC2_RECEIVE;
            ThisEvent.EventParam = NewStatus;
            PostPic2PicLeaderFSM(ThisEvent);
        
        }
        
        CurrentStatus = NewStatus;
    }
}