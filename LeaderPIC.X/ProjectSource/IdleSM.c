// LeaderPIC File
/****************************************************************************
 Module
   IdleSM.c

 Revision
   2.0.1

 Description
   This is the Idle State Machine that is initialized and responds to button/switch
   inputs

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "dbprintf.h"
#include "Pic2PicLeaderFSM.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "IdleSM.h"
// No Lower Level State Machine

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines
#define ONE_SEC 1000
#define TENTH_SEC ONE_SEC / 10
#define BIG_GAME_TIME 46000 // THIS VALUE IS GETTING DOWNSIZED

#define ENTRY_STATE IDLE_WAITING

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event_t DuringIdleWaiting(ES_Event_t Event);
static ES_Event_t DuringIdleEnteringMain(ES_Event_t Event);
static ES_Event_t DuringIdleEnteringSD(ES_Event_t Event);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static IdleState_t CurrentState;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunTemplateSM

 Parameters
   ES_Event_t: the event to process

 Returns
   ES_Event_t: an event to return

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 2/11/05, 10:45AM
****************************************************************************/
ES_Event_t RunIdleSM(ES_Event_t CurrentEvent)
{
    bool MakeTransition = false; /* are we making a state transition? */
    IdleState_t NextState = CurrentState;
    ES_Event_t EntryEventKind = {ES_ENTRY, 0}; // default to normal entry to new state
    ES_Event_t ReturnEvent = CurrentEvent;     // assume we are not consuming event

    switch (CurrentState)
    {
    case IDLE_WAITING: // If current state is state one
        // Execute During function for state one. ES_ENTRY & ES_EXIT are
        // processed here allow the lower level state machines to re-map
        // or consume the event
        ReturnEvent = CurrentEvent = DuringIdleWaiting(CurrentEvent);
        // process any events
        if (CurrentEvent.EventType != ES_NO_EVENT) // If an event is active
        {
            switch (CurrentEvent.EventType)
            {
            case ES_BUTTON_MAIN: // Will be ES_BUTTON or ES_BUTTON_SHORT
                // Execute action function for state one : event one
                NextState = IDLE_ENTERING_MAIN; // Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
                // optionally, consume or re-map this event for the upper
                // level state machine
                ReturnEvent.EventType = ES_NO_EVENT;
                break;
                // repeat cases as required for relevant events

            case ES_BUTTON_SD: // Will be ES_BUTTON or ES_BUTTON_SHORT
                // Execute action function for state one : event one
                NextState = IDLE_ENTERING_SD; // Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
                // optionally, consume or re-map this event for the upper
                // level state machine
                ReturnEvent.EventType = ES_NO_EVENT;
                break;
            }
        }
        break;

    case IDLE_ENTERING_MAIN: // If current state is state one
        // Execute During function for state one. ES_ENTRY & ES_EXIT are
        // processed here allow the lower level state machines to re-map
        // or consume the event

        ReturnEvent = CurrentEvent = DuringIdleEnteringMain(CurrentEvent);
        // process any events
        if (CurrentEvent.EventType != ES_NO_EVENT) // If an event is active
        {
            switch (CurrentEvent.EventType)
            {
            case ES_TIMEOUT: // If event is event one
                if (CurrentEvent.EventParam == STARTUP_TIMER)
                {
                    // Execute action function for state one : event one
                    NextState = IDLE_DONE; // Decide what the next state will be
                    // for internal transitions, skip changing MakeTransition
                    MakeTransition = true; // mark that we are taking a transition
                    // if transitioning to a state with history change kind of entry
                    EntryEventKind.EventType = ES_ENTRY;
                    // optionally, consume or re-map this event for the upper
                    // level state machine
                    DB_printf("Main game is starting!\n");
                    ReturnEvent.EventType = ES_START_MAIN_GAME;
                }
                break;
                // repeat cases as required for relevant events
            }
        }
        break;

    case IDLE_ENTERING_SD: // If current state is state one
        // Execute During function for state one. ES_ENTRY & ES_EXIT are
        // processed here allow the lower level state machines to re-map
        // or consume the event

        ReturnEvent = CurrentEvent = DuringIdleEnteringSD(CurrentEvent);
        // process any events
        if (CurrentEvent.EventType != ES_NO_EVENT) // If an event is active
        {
            switch (CurrentEvent.EventType)
            {
            case ES_TIMEOUT: // If event is event one
                if (CurrentEvent.EventParam == STARTUP_TIMER)
                {
                    // Execute action function for state one : event one
                    NextState = IDLE_DONE; // Decide what the next state will be
                    // for internal transitions, skip changing MakeTransition
                    MakeTransition = true; // mark that we are taking a transition
                    // if transitioning to a state with history change kind of entry
                    EntryEventKind.EventType = ES_ENTRY;
                    // optionally, consume or re-map this event for the upper
                    // level state machine
                    DB_printf("Main game is starting!\n");
                    ReturnEvent.EventType = ES_START_SD;
                }
                break;
                // repeat cases as required for relevant events
            }
        }
        break;
        // repeat state pattern as required for other states
    }
    //   If we are making a state transition
    if (MakeTransition == true)
    {
        //   Execute exit function for current state
        CurrentEvent.EventType = ES_EXIT;
        RunIdleSM(CurrentEvent);

        CurrentState = NextState; // Modify state variable

        //   Execute entry function for new state
        // this defaults to ES_ENTRY
        RunIdleSM(EntryEventKind);
    }
    return (ReturnEvent);
}
/****************************************************************************
 Function
     StartTemplateSM

 Parameters
     None

 Returns
     None

 Description
     Does any required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 2/18/99, 10:38AM
****************************************************************************/
void StartIdleSM(ES_Event_t CurrentEvent)
{
    // to implement entry to a history state or directly to a substate
    // you can modify the initialization of the CurrentState variable
    // otherwise just start in the entry state every time the state machine
    // is started
    DB_printf("\n\n");
    DB_printf("Starting IdleSM\n");
    if (ES_ENTRY_HISTORY != CurrentEvent.EventType)
    {
        CurrentState = ENTRY_STATE;
    }
    // call the entry function (if any) for the ENTRY_STATE
    RunIdleSM(CurrentEvent);
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
     J. Edward Carryer, 2/11/05, 10:38AM
****************************************************************************/
IdleState_t QueryTemplateSM(void)
{
    return (CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event_t DuringIdleWaiting(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY))
    {
        // implement any entry actions required for this state machine
        DB_printf("IdleWaiting is happening!\n");
        SetFollowerCommand(STOP);
        // after that start any lower level machines that run in this state
        // StartLowerLevelSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if (Event.EventType == ES_EXIT)
    {
        // on exit, give the lower levels a chance to clean up first
        // RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
    }
    else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);

        // repeat for any concurrent lower level machines

        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return (ReturnEvent);
}

static ES_Event_t DuringIdleEnteringMain(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY))
    {
        // implement any entry actions required for this state machine
        DB_printf("IdleEnteringMain is happening!\n");
        ES_Timer_InitTimer(STARTUP_TIMER, TENTH_SEC);

        LATBbits.LATB8 = 1;                            // Turn BIG LED ON
        ES_Timer_InitTimer(GAME_TIMER, BIG_GAME_TIME); // 20sec for testing
    }
    else if (Event.EventType == ES_EXIT)
    {
        // on exit, give the lower levels a chance to clean up first
        // RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
    }
    else
    // do the 'during' function for this state
    {
       
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return (ReturnEvent);
}

static ES_Event_t DuringIdleEnteringSD(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY))
    {
        // implement any entry actions required for this state machine
        DB_printf("IdleEnteringSD is happening!\n");
        ES_Timer_InitTimer(STARTUP_TIMER, TENTH_SEC);
    }
    else if (Event.EventType == ES_EXIT)
    {
        // on exit, give the lower levels a chance to clean up first
        // RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
    }
    else
    // do the 'during' function for this state
    {
       
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return (ReturnEvent);
}
