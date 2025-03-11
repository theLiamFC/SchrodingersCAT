/****************************************************************************
 Module
   ButtonService.c

 Revision
   1.0.1

 Description
   This service handles the button press upon the game beginning

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 01/16/12 09:58 jec      began conversion from TemplateFSM.c
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ButtonService.h"
#include <sys/attribs.h>
#include "dbprintf.h"
#include "TopLevelSM.h"

/*----------------------------- Module Defines ----------------------------*/
#define UP 1
#define DOWN 0
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
static uint8_t  LastPinState = 1;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitTemplateService

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
bool InitButtonService(uint8_t Priority)
{
    ES_Event_t ThisEvent;

    MyPriority = Priority;
    DB_printf("Button Service Init\n");
    /********************************************
     in here you write your initialization code
     *******************************************/
    //Configure BIG LED as digital output
    TRISBbits.TRISB8 = 0;
    LATBbits.LATB8 = 0;
    //Configure Start button as digital input
    TRISBbits.TRISB5 = 1;
    //configure main/SD switch as digital input
    TRISBbits.TRISB10 = 1;
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
     PostTemplateService

 Parameters
     EF_Event_t ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostButtonService(ES_Event_t ThisEvent)
{
    return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunTemplateService

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes

 Author
   J. Edward Carryer, 01/15/12, 15:23
****************************************************************************/
ES_Event_t RunButtonService(ES_Event_t ThisEvent)
{
    ES_Event_t ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
    /********************************************
     in here you write your service code
     *******************************************/
    return ReturnEvent;
}

/***************************************************************************
 private functions
 ***************************************************************************/

/****************************************************************************
 Function
    CheckButton

 Description
    Monitors the state of a button connected to PORTB pin 5. If the button
    state changes from the last check and is pressed (logic low), it posts
    an event to the top-level state machine indicating whether the button
    is set to "Main" or "Sudden Death" mode based on the state of PORTB pin 10.

 Returns
    bool - true if a button press event was detected and posted, false otherwise.
****************************************************************************/

bool CheckButton(void)
{
  //ES_Event_t ThisEvent;
  uint8_t CurrentPinState = PORTBbits.RB5;
  bool ReturnVal = false;

  // check for pin high AND different from last time
  // do the check for difference first so that you don't bother with a test
  // of a port/variable that is not going to matter, since it hasn't changed
  if (CurrentPinState != LastPinState && CurrentPinState == DOWN)
  {
    ES_Event_t ThisEvent;
    if (PORTBbits.RB10 == 0) {
        ThisEvent.EventType = ES_BUTTON_MAIN;
        DB_printf("Button Down to Main just happned!\n");
    } else {
        ThisEvent.EventType = ES_BUTTON_SD;
        DB_printf("Button Down to Sudden Death just happned!\n");
    }
    //ThisEvent.EventParam = ES_Timer_GetTime();
    // this could be any of the service post functions, ES_PostListx or
    // ES_PostAll functions
    PostTopLevelSM(ThisEvent);
    ReturnVal = true;
    // update the state for next time
    
  }
  LastPinState = CurrentPinState; 
  return ReturnVal;
}

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/
