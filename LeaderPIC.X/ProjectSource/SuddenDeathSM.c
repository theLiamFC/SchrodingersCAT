/****************************************************************************
 Module
   SuddenDeathSM.c

 Revision
   1.0.0

 Description
   This module implements the Sudden Death state machine for the game.

 Notes
   Part of the Gen2 Events and Services Framework.
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "Pic2PicLeaderFSM.h"
#include "ServoService.h"
#include "dbprintf.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "SuddenDeathSM.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE FINDING_BEACON

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event_t DuringFindingBeaconSD(ES_Event_t Event);
static ES_Event_t DuringFindingTapeSD(ES_Event_t Event);
static ES_Event_t DuringMovingForwardSD(ES_Event_t Event);
static ES_Event_t DuringTurningSD(ES_Event_t Event);
static ES_Event_t DuringMovingStackSD(ES_Event_t Event);
static ES_Event_t DuringPlacingSD(ES_Event_t Event);
/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static SDState_t CurrentState;

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
ES_Event_t RunSuddenDeathSM(ES_Event_t CurrentEvent)
{
    bool MakeTransition = false; /* are we making a state transition? */
    SDState_t NextState = CurrentState;
    ES_Event_t EntryEventKind = {ES_ENTRY, 0}; // default to normal entry to new state
    ES_Event_t ReturnEvent = CurrentEvent;     // assume we are not consuming event

    switch (CurrentState)
    {
    case FINDING_BEACON: // If current state is state one
        // Execute During function for state one. ES_ENTRY & ES_EXIT are
        // processed here allow the lower level state machines to re-map
        // or consume the event
        ReturnEvent = CurrentEvent = DuringFindingBeaconSD(CurrentEvent);
        // process any events
        if (CurrentEvent.EventType != ES_NO_EVENT) // If an event is active
        {
            switch (CurrentEvent.EventType)
            {
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
                // repeat cases as required for relevant events
            }
        }
        break;

    case FINDING_TAPE: // If current state is state one
        // Execute During function for state one. ES_ENTRY & ES_EXIT are
        // processed here allow the lower level state machines to re-map
        // or consume the event
        ReturnEvent = CurrentEvent = DuringFindingTapeSD(CurrentEvent);
        // process any events
        if (CurrentEvent.EventType != ES_NO_EVENT) // If an event is active
        {
            switch (CurrentEvent.EventType)
            {
            case ES_TAPE_FOUND_INIT: // Will be ES_TAPE_FOUND or something
            {
                DB_printf("Tape Finding is done!\n");
                NextState = MOVING_FORWARD; // Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
                // ReturnEvent.EventType = ES_TAPE_FOUND; //GameModeSM will know tape is found, start moving
            }
            // Execute action function for state one : event one
            break;
                // repeat cases as required for relevant events
            }
        }
        break;

    case MOVING_FORWARD: // If current state is state one
        // Execute During function for state one. ES_ENTRY & ES_EXIT are
        // processed here allow the lower level state machines to re-map
        // or consume the event
        ReturnEvent = CurrentEvent = DuringMovingForwardSD(CurrentEvent);
        // process any events
        if (CurrentEvent.EventType != ES_NO_EVENT) // If an event is active
        {
            switch (CurrentEvent.EventType)
            {
            case ES_COMMAND_DONE: // Will be ES_TAPE_FOUND or something
            {
                DB_printf("Tape Finding is done!\n");
                NextState = TURNING; // Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
                // ReturnEvent.EventType = ES_TAPE_FOUND; //GameModeSM will know tape is found, start moving
            }
            // Execute action function for state one : event one
            break;
                // repeat cases as required for relevant events
            }
        }
        break;

    case TURNING: // If current state is state one
        // Execute During function for state one. ES_ENTRY & ES_EXIT are
        // processed here allow the lower level state machines to re-map
        // or consume the event
        ReturnEvent = CurrentEvent = DuringTurningSD(CurrentEvent);
        // process any events
        if (CurrentEvent.EventType != ES_NO_EVENT) // If an event is active
        {
            switch (CurrentEvent.EventType)
            {
            case ES_COMMAND_DONE: // Will be ES_TAPE_FOUND or something
            {
                DB_printf("Tape Finding is done!\n");
                NextState = MOVING_STACK; // Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
                // ReturnEvent.EventType = ES_TAPE_FOUND; //GameModeSM will know tape is found, start moving
            }
            // Execute action function for state one : event one
            break;
                // repeat cases as required for relevant events
            }
        }
        break;

    case MOVING_STACK: // If current state is state one
        // Execute During function for state one. ES_ENTRY & ES_EXIT are
        // processed here allow the lower level state machines to re-map
        // or consume the event
        ReturnEvent = CurrentEvent = DuringMovingStackSD(CurrentEvent);
        // process any events
        if (CurrentEvent.EventType != ES_NO_EVENT) // If an event is active
        {
            switch (CurrentEvent.EventType)
            {
            case ES_TAPE_FOUND: // Will be ES_TAPE_FOUND or something
            {
                DB_printf("Tape Finding is done!\n");
                NextState = PLACING; // Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
                // ReturnEvent.EventType = ES_TAPE_FOUND; //GameModeSM will know tape is found, start moving
            }
            // Execute action function for state one : event one
            break;
                // repeat cases as required for relevant events
            }
        }
        break;

    case PLACING: // If current state is state one
        // Execute During function for state one. ES_ENTRY & ES_EXIT are
        // processed here allow the lower level state machines to re-map
        // or consume the event
        ReturnEvent = CurrentEvent = DuringPlacingSD(CurrentEvent);
        // process any events
        if (CurrentEvent.EventType != ES_NO_EVENT) // If an event is active
        {
            switch (CurrentEvent.EventType)
            {
            case ES_COMMAND_DONE: // Will be ES_TAPE_FOUND or something
            {
                DB_printf("Tape Finding is done!\n");
                NextState = SD_DONE; // Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
                // ReturnEvent.EventType = ES_TAPE_FOUND; //GameModeSM will know tape is found, start moving
            }
            // Execute action function for state one : event one
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
        RunSuddenDeathSM(CurrentEvent);

        CurrentState = NextState; // Modify state variable

        //   Execute entry function for new state
        // this defaults to ES_ENTRY
        RunSuddenDeathSM(EntryEventKind);
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
void StartSuddenDeathSM(ES_Event_t CurrentEvent)
{
    // to implement entry to a history state or directly to a substate
    // you can modify the initialization of the CurrentState variable
    // otherwise just start in the entry state every time the state machine
    // is started
    if (ES_ENTRY_HISTORY != CurrentEvent.EventType)
    {
        CurrentState = ENTRY_STATE;
    }
    // call the entry function (if any) for the ENTRY_STATE
    RunSuddenDeathSM(CurrentEvent);
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
SDState_t QuerySuddenDeathSM(void)
{
    return (CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event_t DuringFindingBeaconSD(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY))
    {
        // implement any entry actions required for this state machine
        SetFollowerCommand(SEARCH_BEACON);
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
        SetFollowerCommand(STOP);
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

static ES_Event_t DuringFindingTapeSD(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY))
    {
        // implement any entry actions required for this state machine
        SetFollowerCommand(RCW_TO_TAPE);
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
        SetFollowerCommand(STOP);
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

static ES_Event_t DuringMovingForwardSD(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY))
    {
        // implement any entry actions required for this state machine
        SetFollowerCommand(DFFULL_2);
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
        SetFollowerCommand(STOP);
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

static ES_Event_t DuringTurningSD(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY))
    {
        // implement any entry actions required for this state machine
        SetFollowerCommand(RCW90);
        SetArm(ARM_HIGH, false);
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
        SetFollowerCommand(STOP);
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

static ES_Event_t DuringMovingStackSD(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY))
    {
        // implement any entry actions required for this state machine
        SetFollowerCommand(DFFULL_T);
        //        SetArm(ARM_HIGH);
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
        //        SetFollowerCommand(STOP);
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

static ES_Event_t DuringPlacingSD(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY))
    {
        // implement any entry actions required for this state machine
        SetFollowerCommand(CRATE_HIGH);
        //        SetArm(ARM_HIGH);
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
        SetArm(ARM_SHEAR, false);
        SetFollowerCommand(STOP);
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
