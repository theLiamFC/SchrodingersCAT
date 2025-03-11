/****************************************************************************
 Module
   TopLevelSM.c

 Revision
   1.0.1

 Description
   This is the top level Hierarchical state machine for the Leader PIC

 Notes
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "TopLevelSM.h"
#include "ServoService.h"
#include "dbprintf.h"

#include "IdleSM.h"
#include "GameModeSM.h"
#include "SuddenDeathSM.h"
// #include "SuddenDeathSM.h" //TO BE ADDED

/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
static ES_Event_t DuringIdle(ES_Event_t Event);
static ES_Event_t DuringGameMode(ES_Event_t Event);
static ES_Event_t DuringSuddenDeath(ES_Event_t Event);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, though if the top level state machine
// is just a single state container for orthogonal regions, you could get
// away without it
static TopLevelState_t CurrentState;
// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitMasterSM

 Parameters
     uint8_t : the priorty of this service

 Returns
     boolean, False if error in initialization, True otherwise

 Description
     Saves away the priority,  and starts
     the top level state machine
 Notes

 Author
     J. Edward Carryer, 02/06/12, 22:06
****************************************************************************/
bool InitTopLevelSM(uint8_t Priority)
{
    ES_Event_t ThisEvent;

    MyPriority = Priority; // save our priority

    ThisEvent.EventType = ES_ENTRY;
    // Start the Master State machine

    // CurrentState = Idle;
    DB_printf("\n\n\n\n");
    DB_printf("Init TopLevelSM \n");
    StartTopLevelSM(ThisEvent);

    SetIndicator(INDICATOR_NEUTRAL);
    SetIntake(INTAKE_PUSH);
    SetArm(ARM_IDLE, false);

    return true;
}

/****************************************************************************
 Function
     PostMasterSM

 Parameters
     ES_Event_t ThisEvent , the event to post to the queue

 Returns
     boolean False if the post operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostTopLevelSM(ES_Event_t ThisEvent)
{
    return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunMasterSM

 Parameters
   ES_Event: the event to process

 Returns
   ES_Event: an event to return

 Description
   the run function for the top level state machine
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 02/06/12, 22:09
****************************************************************************/
ES_Event_t RunTopLevelSM(ES_Event_t CurrentEvent)
{
    bool MakeTransition = false; /* are we making a state transition? */
    TopLevelState_t NextState = CurrentState;
    ES_Event_t EntryEventKind = {ES_ENTRY, 0}; // default to normal entry to new state
    ES_Event_t ReturnEvent = {ES_NO_EVENT, 0}; // assume no error
                                               //    DB_printf("Running TopLevelSM\n");

    switch (CurrentState)
    {
    case Idle: // initialize lower level state machines
        // initialize lower level state machines

        // Execute During function for state one. ES_ENTRY & ES_EXIT are
        // processed here allow the lowere level state machines to re-map
        // or consume the event
        DB_printf("In TopLevelSM Idle State\n");
        CurrentEvent = DuringIdle(CurrentEvent); // should wait

        // process any events
        if (CurrentEvent.EventType != ES_NO_EVENT) // If an event is active
        {
            switch (CurrentEvent.EventType)
            {
            case ES_START_MAIN_GAME: // Coming from IdleSM
            {
                DB_printf("Switching to gamemode\n");
                // Execute action function for state one : event one
                NextState = GameMode; // Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
            }

            break;

            case ES_START_SD: // Coming from IdleSM
            {
                DB_printf("Switching to sudden death\n");
                // Execute action function for state one : event one
                NextState = SuddenDeath; // Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
            }

            break;
                // repeat cases as required for relevant events
            }
        }
        break;
        // repeat state pattern as required for other states

        //
    case GameMode: // If current state is state one
        DB_printf("In TopLevelSM GameMode State!!! \n");
        // Execute During function for state one. ES_ENTRY & ES_EXIT are
        // processed here allow the lowere level state machines to re-map
        // or consume the event

        CurrentEvent = DuringGameMode(CurrentEvent);

        // process any events
        if (CurrentEvent.EventType != ES_NO_EVENT) // If an event is active
        {
            switch (CurrentEvent.EventType)
            {
            case ES_New_CrateAction: // To be changed, whatever state comes out of GameModeSM to be done
            {
                // Execute action function for state one : event one
                NextState = Idle; // Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
            }
            break;

            case ES_TIMEOUT: // To be changed, whatever state comes out of GameModeSM to be done
            {
                if (CurrentEvent.EventParam == CLEANUP_TIMER)
                {
                    DB_printf("Should be moving to Idle here");
                    // Execute action function for state one : event one
                    NextState = Idle; // Decide what the next state will be
                    // for internal transitions, skip changing MakeTransition
                    MakeTransition = true; // mark that we are taking a transition
                    // if transitioning to a state with history change kind of entry
                    EntryEventKind.EventType = ES_ENTRY;
                }
            }
            break;
                // repeat cases as required for relevant events
            }
        }
        break;

    case SuddenDeath: // If current state is state one
        DB_printf("In TopLevelSM GameMode State!!! \n");
        // Execute During function for state one. ES_ENTRY & ES_EXIT are
        // processed here allow the lowere level state machines to re-map
        // or consume the event

        CurrentEvent = DuringSuddenDeath(CurrentEvent);

        // process any events
        if (CurrentEvent.EventType != ES_NO_EVENT) // If an event is active
        {
            switch (CurrentEvent.EventType)
            {
            case ES_New_CrateAction: // To be changed, whatever state comes out of GameModeSM to be done
            {
                // Execute action function for state one : event one
                NextState = Idle; // Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
            }
            break;

            case ES_TIMEOUT: // To be changed, whatever state comes out of GameModeSM to be done
            {
                if (CurrentEvent.EventParam == CLEANUP_TIMER)
                {
                    DB_printf("Should be moving to Idle here");
                    // Execute action function for state one : event one
                    NextState = Idle; // Decide what the next state will be
                    // for internal transitions, skip changing MakeTransition
                    MakeTransition = true; // mark that we are taking a transition
                    // if transitioning to a state with history change kind of entry
                    EntryEventKind.EventType = ES_ENTRY;
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
        RunTopLevelSM(CurrentEvent);

        CurrentState = NextState; // Modify state variable

        // Execute entry function for new state
        // this defaults to ES_ENTRY
        RunTopLevelSM(EntryEventKind);
    }
    // in the absence of an error the top level state machine should
    // always return ES_NO_EVENT, which we initialized at the top of func
    return (ReturnEvent);
}
/****************************************************************************
 Function
     StartMasterSM

 Parameters
     ES_Event CurrentEvent

 Returns
     nothing

 Description
     Does any required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 02/06/12, 22:15
****************************************************************************/
void StartTopLevelSM(ES_Event_t CurrentEvent)
{

    // initialize any bits that need to read sensory data
    DB_printf("Start TopLevelSM \n");

    // now we need to let the Run function init the lower level state machines
    // use LocalEvent to keep the compiler from complaining about unused var
    // inside the init function, CurrentEvent is set to ES_ENTRY;
    CurrentState = Idle; // To be set to Idle, set otherwise for testing
    RunTopLevelSM(CurrentEvent);
    return;
}

/****************************************************************************
 Function
     QueryTopHSMTemplateSM

 Parameters
     None

 Returns
     MasterState_t  The current state of the Top Level Template state machine

 Description
     returns the current state of the Template state machine
 Notes

 Author
     J. Edward Carryer, 2/05/22, 10:30AM
****************************************************************************/
TopLevelState_t QueryTopLevelSM(void)
{
    return (CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event_t DuringIdle(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY))
    {
        StartIdleSM(Event); // this function needs to be written inside GameMode_InitializeSM
    }
    else if (Event.EventType == ES_EXIT)
    {
        RunIdleSM(Event); // this function needs to be written inside GameMode_InitializeSM
    }
    else
    // do the 'during' function for this state
    {
        ReturnEvent = RunIdleSM(Event);
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return (ReturnEvent);
}

static ES_Event_t DuringGameMode(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY))
    {
        StartGameModeSM(Event); // this function needs to be written inside GameMode_InitializeSM
    }
    else if (Event.EventType == ES_EXIT)
    {
        RunGameModeSM(Event); // this function needs to be written inside GameMode_InitializeSM
        LATBbits.LATB8 = 0;   // Turn LED off when gamemode is over
    }
    else
    // do the 'during' function for this state
    {
        ReturnEvent = RunGameModeSM(Event);
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return (ReturnEvent);
}

static ES_Event_t DuringSuddenDeath(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY))
    {
        StartSuddenDeathSM(Event); // this function needs to be written inside GameMode_InitializeSM
    }
    else if (Event.EventType == ES_EXIT)
    {
        RunSuddenDeathSM(Event); // this function needs to be written inside GameMode_InitializeSM
        LATBbits.LATB8 = 0;      // Turn LED off when gamemode is over
    }
    else
    // do the 'during' function for this state
    {
        ReturnEvent = RunSuddenDeathSM(Event);
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return (ReturnEvent);
}
