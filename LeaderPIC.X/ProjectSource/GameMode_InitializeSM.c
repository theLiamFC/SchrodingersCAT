/****************************************************************************
 Module
   GameMode_InitializeSM.c

 Revision
   2.0.1

 Description
   This module manages the initialization state machine for game mode.
   It handles transitions and actions required during the initialization phase.

 Notes

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "Pic2PicLeaderFSM.h"
#include "dbprintf.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "GameMode_InitializeSM.h"
// No lower SM here

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE FINDING_BEACON

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event_t DuringFindingBeacon(ES_Event_t Event);
static ES_Event_t DuringFindingTape(ES_Event_t Event);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static InitializeState_t CurrentState;

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
ES_Event_t RunInitializeSM(ES_Event_t CurrentEvent)
{
    bool MakeTransition = false; /* are we making a state transition? */
    InitializeState_t NextState = CurrentState;
    ES_Event_t EntryEventKind = {ES_ENTRY, 0}; // default to normal entry to new state
    ES_Event_t ReturnEvent = {ES_NO_EVENT, 0}; // assume no error

    switch (CurrentState)
    {
    case FINDING_BEACON: // initialize lower level state machines
        // initialize lower level state machines
        DB_printf("Searching For BEACON!!!\n");
        CurrentEvent = DuringFindingBeacon(CurrentEvent);

        // process any events
        if (CurrentEvent.EventType != ES_NO_EVENT) // If an event is active
        {
            switch (CurrentEvent.EventType)
            {
            // sent over SPI
            // To be done: Figure out how to move servo depending on Green/Blue
            case ES_STACKB: // We are on the blue side
            {
                DB_printf("Blue Side has been detected (B Beacon)\n");
                // Execute action function for state one : event one
                NextState = FINDING_TAPE; // If testing rotation after beacon, change to "FINDING_TAPE"
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
                //                    ReturnEvent.EventType = ES_TAPE_FOUND;
            }
            break;
                // repeat cases as required for relevant events

            case ES_STACKR: // We are on the green side
            {
                DB_printf("Green Side has been detected (R Beacon)\n");
                // Execute action function for state one : event one
                NextState = FINDING_TAPE; // If testing rotation after beacon, change to "FINDING_TAPE"
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;

                //                    ReturnEvent.EventType = ES_TAPE_FOUND;
            }
            break;
            case ES_TIMEOUT:
            {
                if (CurrentEvent.EventParam == CLEANUP_TIMER)
                {
                    // DB_printf("TimerHasBeenHandled\n");
                    //  Execute action function for state one : event one
                    NextState = INITIALIZE_DONE; // Decide what the next state will be
                    // for internal transitions, skip changing MakeTransition
                    MakeTransition = true; // mark that we are taking a transition
                    // if transitioning to a state with history change kind of entry
                    ReturnEvent = CurrentEvent;

                    //                            EntryEventKind.EventParam =
                    DB_printf("Game timer handled in GameMode_InitializeSM\n");
                }
            }
            break;
            }
        }
        break;

    case FINDING_TAPE: // initialize lower level state machines
        // initialize lower level state machines
        DB_printf("FINDING_TAPE Has Been Entered Within InitializeSM\n");
        CurrentEvent = DuringFindingTape(CurrentEvent);

        // process any events
        if (CurrentEvent.EventType != ES_NO_EVENT) // If an event is active
        {
            DB_printf("Event Received: %d\n", CurrentEvent.EventType);
            switch (CurrentEvent.EventType)
            {
            // To be: case ES_PARALLEL sent from follower via SPI
            case ES_TAPE_FOUND_INIT: // Will be ES_TAPE_FOUND or something
            {
                DB_printf("Tape Finding is done!\n");
                NextState = INITIALIZE_DONE; // Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
                ReturnEvent.EventType = ES_TAPE_FOUND; // GameModeSM will know tape is found, start moving
            }
            // Execute action function for state one : event one
            break;

            case ES_TIMEOUT:
            {
                if (CurrentEvent.EventParam == CLEANUP_TIMER)
                {
                    // DB_printf("TimerHasBeenHandled\n");
                    //  Execute action function for state one : event one
                    NextState = INITIALIZE_DONE; // Decide what the next state will be
                    // for internal transitions, skip changing MakeTransition
                    MakeTransition = true; // mark that we are taking a transition
                    // if transitioning to a state with history change kind of entry
                    ReturnEvent = CurrentEvent;

                    //                            EntryEventKind.EventParam =
                    DB_printf("Game timer handled in GameMode_InitializeSM\n");
                }
            }
            break;
                // repeat cases as required for relevant events
            }
        }
        break;
        // repeat state pattern as required for other states
    }

    if (MakeTransition == true) //   If we are making a state transition
    {
        //   Execute exit function for current state
        CurrentEvent.EventType = ES_EXIT;
        RunInitializeSM(CurrentEvent);

        CurrentState = NextState; // Modify state variable

        // Execute entry function for new state
        // this defaults to ES_ENTRY
        RunInitializeSM(EntryEventKind);
    }
    // in the absence of an error the top level state machine should
    // always return ES_NO_EVENT, which we initialized at the top of func
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
void StartInitializeSM(ES_Event_t CurrentEvent)
{
    // to implement entry to a history state or directly to a substate
    // you can modify the initialization of the CurrentState variable
    // otherwise just start in the entry state every time the state machine
    // is started
    DB_printf("\n\n");
    DB_printf("Starting GameMode_InitializeSM\n");
    if (ES_ENTRY_HISTORY != CurrentEvent.EventType)
    {
        CurrentState = ENTRY_STATE;
    }
    // call the entry function (if any) for the ENTRY_STATE
    RunInitializeSM(CurrentEvent);
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
InitializeState_t QueryInitializeSM(void)
{
    return (CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

/**
 * This function is the state machine for when the leader is finding the beacon
 * @param Event the event that triggered this state
 * @return the event after being processed by this state machine
 */
static ES_Event_t DuringFindingBeacon(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY))
    {
        // Send SPI Command to start rotating
        // Will be sent back which side we are on
        SetFollowerCommand(SEARCH_BEACON);
        DB_printf("Starting to find beacon!\n");
        // implement any entry actions required for this state machine

        // after that start any lower level machines that run in this state
        // StartLowerLevelSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if (Event.EventType == ES_EXIT)
    {
        // Send an SPI command to stop rotating
        // Stop Rotating
        // Potential issue to fix, sometime SPI sends this Exit, sometimes next entry
        SetFollowerCommand(STOP);
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

/**
 * @brief Runs the state machine code for the "FindingTape" state.
 *
 * This state is entered when the robot is in the "FindingTape" state,
 * and is exited when the robot transitions to another state.
 *
 * @param Event The event that triggered this state transition.
 *
 * @return The event to be posted back into the event queue.
 */
static ES_Event_t DuringFindingTape(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY))
    {

        // implement any entry actions required for this state machine
        // Start moving forward (for testing purposes, real code will find tape)
        SetFollowerCommand(RCW_TO_TAPE); // To be rotate CW until tape (should make robot parallel)
        // after that start any lower level machines that run in this state
        // StartLowerLevelSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if (Event.EventType == ES_EXIT)
    {
        // TURN OFF INPUT CAPTURE FOR THE BEACON

        DB_printf("Leaving FINDING TAPE\n");
        // Stop
        SetFollowerCommand(STOP);
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
