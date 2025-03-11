/****************************************************************************
 Module
   GameModeSM.c

 Revision
   2.0.1

 Description
   This module defines the state machine for handling game modes.

 Notes
   This state machine is part of the Gen2 Events and Services Framework.

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "dbprintf.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "GameModeSM.h"
#include "GameMode_InitializeSM.h"
#include "GameMode_MoveSM.h"
#include "GameMode_PlaceCrateSM.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE Initialize // to test beacon/what we ultimately want is this to be "Initialize"

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event_t DuringInitialize(ES_Event_t Event);
static ES_Event_t DuringMove(ES_Event_t Event);
static ES_Event_t DuringPlaceCrate(ES_Event_t Event);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static GameModeState_t CurrentState;

static uint8_t CurrentMove = 0;

// Game Winning Strategy
static uint16_t GameStrategy[][2] = {
    {3, 2},
    {4, 0}, // CRATE NEAR
    {4, 2}, // PLACE NEAR
    {2, 0}, // ATTACK FAR
    {2, 2}, // CRATE FAR
    {1, 2}, // PLACE FAR
    {1, 2}};

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
ES_Event_t RunGameModeSM(ES_Event_t CurrentEvent)
{
    bool MakeTransition = false; /* are we making a state transition? */
    GameModeState_t NextState = CurrentState;
    ES_Event_t EntryEventKind = {ES_ENTRY, 0}; // default to normal entry to new state
    ES_Event_t ReturnEvent = {ES_NO_EVENT, 0}; // assume no error

    switch (CurrentState)
    {
    case Initialize: // initialize lower level state machines
        DB_printf("In GameModeSM Initialize State\n");
        // initialize lower level state machines

        // Execute During function for state one. ES_ENTRY & ES_EXIT are
        // processed here allow the lowere level state machines to re-map
        // or consume the event
        CurrentEvent = DuringInitialize(CurrentEvent);

        // process any events
        if (CurrentEvent.EventType != ES_NO_EVENT) // If an event is active
        {
            switch (CurrentEvent.EventType)
            {
            case ES_TAPE_FOUND: // We found the beacon, indicated what side and are parallel with tape
            {
                // Execute action function for state one : event one
                NextState = Move; // Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
            }
            break;
            case ES_TIMEOUT:
            {
                if (CurrentEvent.EventParam == CLEANUP_TIMER)
                {
                    // Execute action function for state one : event one
                    NextState = GameModeDone; // Decide what the next state will be
                    // for internal transitions, skip changing MakeTransition
                    MakeTransition = true; // mark that we are taking a transition
                    // if transitioning to a state with history change kind of entry
                    ReturnEvent = CurrentEvent;
                    DB_printf("Game timer handled in GameModeSM\n");
                }
            }
            break;
                // repeat cases as required for relevant events
            }
        }
        break;
        // repeat state pattern as required for other states

        //
    case Move: // If current state is state one
        // Execute During function for state one. ES_ENTRY & ES_EXIT are
        // processed here allow the lowere level state machines to re-map
        // or consume the event

        CurrentEvent = DuringMove(CurrentEvent);

        // process any events
        if (CurrentEvent.EventType != ES_NO_EVENT) // If an event is active
        {
            switch (CurrentEvent.EventType)
            {
            case ES_MOVE_DONE: // If event is event one
            {
                DB_printf("Entering PlaceCrateSM\n");
                // Execute action function for state one : event one
                NextState = PlaceCrate; // Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
            }
            break;

            case ES_MOVE_AGAIN: // If event is event one
            {
                DB_printf("Entering PlaceCrateSM\n");
                // Execute action function for state one : event one
                NextState = Move; // Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
            }
            break;

            case ES_TIMEOUT:
            {
                if (CurrentEvent.EventParam == CLEANUP_TIMER)
                {
                    // Execute action function for state one : event one
                    NextState = GameModeDone; // Decide what the next state will be
                    // for internal transitions, skip changing MakeTransition
                    MakeTransition = true; // mark that we are taking a transition
                    // if transitioning to a state with history change kind of entry
                    ReturnEvent = CurrentEvent;
                    DB_printf("Game timer handled in GameModeSM\n");
                }
            }
            break;
                // repeat cases as required for relevant events
            }
        }
        break;
        // repeat state pattern as required for other states

    case PlaceCrate: // If current state is state one
        // Execute During function for state one. ES_ENTRY & ES_EXIT are
        // processed here allow the lowere level state machines to re-map
        // or consume the event

        CurrentEvent = DuringPlaceCrate(CurrentEvent);

        // process any events
        if (CurrentEvent.EventType != ES_NO_EVENT) // If an event is active
        {
            switch (CurrentEvent.EventType)
            {
            case ES_MOVE_AGAIN: // If event is event one
            {
                DB_printf("Exiting PlaceCrateSM\n");
                // retrieve new event from command list to determine where to go
                // Execute action function for state one : event one
                NextState = Move; // Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
            }
            break;

            case ES_TIMEOUT:
            {
                if (CurrentEvent.EventParam == CLEANUP_TIMER)
                {
                    // Execute action function for state one : event one
                    NextState = GameModeDone; // Decide what the next state will be
                    // for internal transitions, skip changing MakeTransition
                    MakeTransition = true; // mark that we are taking a transition
                    // if transitioning to a state with history change kind of entry
                    ReturnEvent = CurrentEvent;
                    DB_printf("Game timer handled in GameModeSM\n");
                }
            }
            break;
                // repeat cases as required for relevant events
            }
        }
        break;
    }

    if (MakeTransition == true) //   If we are making a state transition
    {
        //   Execute exit function for current state
        CurrentEvent.EventType = ES_EXIT;
        RunGameModeSM(CurrentEvent);

        CurrentState = NextState; // Modify state variable

        // Execute entry function for new state
        // this defaults to ES_ENTRY
        RunGameModeSM(EntryEventKind);
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
void StartGameModeSM(ES_Event_t CurrentEvent)
{
    // to implement entry to a history state or directly to a substate
    // you can modify the initialization of the CurrentState variable
    // otherwise just start in the entry state every time the state machine
    // is started
    DB_printf("\n\n");
    DB_printf("Starting GameModeSM\n");
    if (ES_ENTRY_HISTORY != CurrentEvent.EventType)
    {
        CurrentState = ENTRY_STATE;
    }
    // call the entry function (if any) for the ENTRY_STATE
    RunGameModeSM(CurrentEvent);
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
GameModeState_t QueryGameModeSM(void)
{
    return (CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event_t DuringInitialize(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY))
    {
        StartInitializeSM(Event); // this function needs to be written inside GameMode_InitializeSM
    }
    else if (Event.EventType == ES_EXIT)
    {
        RunInitializeSM(Event); // this function needs to be written inside GameMode_InitializeSM
    }
    else
    // do the 'during' function for this state
    {
        // this function needs to be written inside GameMode_InitializeSM
        ReturnEvent = RunInitializeSM(Event);
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return (ReturnEvent);
}

static ES_Event_t DuringMove(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY))
    {
        SetTargetPos(GameStrategy[CurrentMove][0], GameStrategy[CurrentMove][1]);
        CurrentMove++;
        StartMoveSM(Event); // this function needs to be written inside GameMode_InitializeSM
    }
    else if (Event.EventType == ES_EXIT)
    {
        RunMoveSM(Event); // this function needs to be written inside GameMode_InitializeSM
    }
    else
    // do the 'during' function for this state
    {
        // this function needs to be written inside GameMode_InitializeSM
        ReturnEvent = RunMoveSM(Event);
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return (ReturnEvent);
}

static ES_Event_t DuringPlaceCrate(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY))
    {
        StartPlaceCrateSM(Event); // this function needs to be written inside GameMode_InitializeSM
    }
    else if (Event.EventType == ES_EXIT)
    {
        RunPlaceCrateSM(Event); // this function needs to be written inside GameMode_InitializeSM
    }
    else
    // do the 'during' function for this state
    {
        // this function needs to be written inside GameMode_InitializeSM
        ReturnEvent = RunPlaceCrateSM(Event);
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return (ReturnEvent);
}
