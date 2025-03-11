/****************************************************************************
 Module
   GameMode_PlaceCrateSM.c

 Revision
   3.0.1

 Description
   This is a state machine for the robot to place a crate on the playing field.

 Notes

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ServoService.h"
#include "Pic2PicLeaderFSM.h"
#include "TopLevelSM.h"
#include "GameMode_MoveSM.h"
#include "dbprintf.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "GameMode_PlaceCrateSM.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE INTAKING_CRATE

#define F_FULL true
#define F_EMPTY false
#define E_FULL false
#define E_EMPTY true

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event_t DuringIntakingCrate(ES_Event_t Event);
static ES_Event_t DuringPreparingCrate(ES_Event_t Event);
static ES_Event_t DuringPlacingCrate(ES_Event_t Event);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static TemplateState_t CurrentState;

static Position CurrPos; // used to store current robot position

static uint8_t CrateLevel = 3; // CrateLevel 0 is idle

static bool CrateInArm = true;    // start game with one crate in arm
static uint8_t CrateInIntake = 3; // start game with zero crates in intake

static uint8_t AvailableStacks;
static bool ExitIntakeFlag = false;

static bool Stacks[][5] = {
    {E_EMPTY, E_EMPTY, E_EMPTY, E_EMPTY, E_FULL}, // right enemy stack, X = 1 (false = full, true = empty)
    {F_EMPTY, F_FULL, F_FULL, F_EMPTY, F_EMPTY},  // right friendly stack, X = 2 (false = empty, true = full)
    {E_EMPTY, E_EMPTY, E_EMPTY, E_EMPTY, E_FULL}, // left enemy stack, X = 3 (false = full, true = empty)
    {F_EMPTY, F_FULL, F_EMPTY, F_FULL, F_EMPTY},  // left friendly stack, X = 4 (false = empty, true = full)
};

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
ES_Event_t RunPlaceCrateSM(ES_Event_t CurrentEvent)
{
    bool MakeTransition = false; /* are we making a state transition? */
    TemplateState_t NextState = CurrentState;
    ES_Event_t EntryEventKind = {ES_ENTRY, 0}; // default to normal entry to new state
    ES_Event_t ReturnEvent = CurrentEvent;     // assume we are not consuming event

    switch (CurrentState)
    {
    case INTAKING_CRATE:
        ReturnEvent = CurrentEvent = DuringIntakingCrate(CurrentEvent);
        // process any events
        if (CurrentEvent.EventType != ES_NO_EVENT) // If an event is active
        {
            switch (CurrentEvent.EventType)
            {
            case ES_TIMEOUT:
                if (CurrentEvent.EventParam == ARM_DELAY_TIMER)
                {
                    if (ExitIntakeFlag)
                    {
                        NextState = PREPARING_CRATE; // Decide what the next state will be
                        // for internal transitions, skip changing MakeTransition
                        MakeTransition = true; // mark that we are taking a transition
                        ExitIntakeFlag = false;
                    }
                    else
                    {
                        ExitIntakeFlag = true;
                        NextState = INTAKING_CRATE; // Decide what the next state will be
                        // for internal transitions, skip changing MakeTransition
                        MakeTransition = false; // mark that we are taking a transition
                    }
                    // if transitioning to a state with history change kind of entry
                    EntryEventKind.EventType = ES_ENTRY;
                    // optionally, consume or re-map this event for the upper
                    // level state machine
                    ReturnEvent.EventType = ES_NO_EVENT;
                }
                else if (CurrentEvent.EventParam == CLEANUP_TIMER)
                {
                    // Execute action function for state one : event one
                    NextState = PLACING_DONE; // Decide what the next state will be
                    // for internal transitions, skip changing MakeTransition
                    MakeTransition = true; // mark that we are taking a transition
                    // if transitioning to a state with history change kind of entry
                    ReturnEvent = CurrentEvent;

                    DB_printf("Game timer handled in GameMode_PlaceCrateSM\n");
                }
                break;
            case ES_COMMAND_DONE: // follower has driven backwards and intaked crat
                if (ExitIntakeFlag)
                {
                    NextState = PREPARING_CRATE; // Decide what the next state will be
                    // for internal transitions, skip changing MakeTransition
                    MakeTransition = true; // mark that we are taking a transition
                    ExitIntakeFlag = false;
                }
                else
                {
                    ExitIntakeFlag = true;
                    NextState = INTAKING_CRATE; // Decide what the next state will be
                    // for internal transitions, skip changing MakeTransition
                    MakeTransition = false; // mark that we are taking a transition
                }
                // Execute action function for state one : event one
                // NextState = INTAKING_CRATE; // Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                // MakeTransition = false; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                // EntryEventKind.EventType = ES_ENTRY_HISTORY;
                // optionally, consume or re-map this event for the upper
                // level state machine
                ReturnEvent.EventType = ES_NO_EVENT;
                break;
            case ES_LEAVE_STACK:
                DB_printf("ES_OUT_OF_CRATES\n");
                // Execute action function for state one : event one
                NextState = PLACING_DONE; // Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
                // optionally, consume or re-map this event for the upper
                // level state machine
                ReturnEvent.EventType = ES_MOVE_AGAIN;
                break;
            }
        }
        break;
    case PREPARING_CRATE:
        ReturnEvent = CurrentEvent = DuringPreparingCrate(CurrentEvent);
        // process any events
        if (CurrentEvent.EventType != ES_NO_EVENT) // If an event is active
        {
            switch (CurrentEvent.EventType)
            {
            case ES_TIMEOUT:
                if (CurrentEvent.EventParam == ARM_DELAY_TIMER) // arm has moved to crate level
                {
                    DB_printf("Aligning to T\n");
                    if (CrateLevel == 2 || CrateLevel == 3)
                    {
                        SetFollowerCommand(CRATE_MID);
                    }
                    else
                    {
                        SetFollowerCommand(DFFULL_T);
                    }

                    // Execute action function for state one : event one
                    NextState = PREPARING_CRATE; // Decide what the next state will be
                    // for internal transitions, skip changing MakeTransition
                    MakeTransition = false; // mark that we are taking a transition
                    // if transitioning to a state with history change kind of entry
                    // EntryEventKind.EventType = ES_ENTRY_HISTORY;
                    // optionally, consume or re-map this event for the upper
                    // level state machine
                    ReturnEvent.EventType = ES_NO_EVENT;
                }
                else if (CurrentEvent.EventParam == DRIVE_DELAY_TIMER)
                {
                    if (CrateLevel == 4 || CrateLevel == 5)
                    {
                        if (CurrPos.x == 3 || CurrPos.x == 1)
                        {
                            SetFollowerCommand(CRATE_HIGH_KNOCK);
                        }
                        else
                        {
                            SetFollowerCommand(CRATE_HIGH);
                        }
                    }
                    else if (CrateLevel == 2 || CrateLevel == 3)
                    {
                        // SetFollowerCommand(CRATE_MID);
                        ES_Event_t Temp;
                        Temp.EventType = ES_COMMAND_DONE;
                        PostTopLevelSM(Temp);
                    }
                    else if (CrateLevel == 1)
                    {
                        SetFollowerCommand(CRATE_LOW);
                    }
                }
                else if (CurrentEvent.EventParam == CLEANUP_TIMER)
                {
                    // Execute action function for state one : event one
                    NextState = PLACING_DONE; // Decide what the next state will be
                    // for internal transitions, skip changing MakeTransition
                    MakeTransition = true; // mark that we are taking a transition
                    // if transitioning to a state with history change kind of entry
                    ReturnEvent = CurrentEvent;

                    DB_printf("Game timer handled in GameMode_PlaceCrateSM\n");
                }
                break;
            case ES_TAPE_FOUND: // robot aligned to T
                DB_printf("Aligning to correct distance for crate level %d\n", CrateLevel);

                ES_Timer_InitTimer(DRIVE_DELAY_TIMER, 500);

                // Execute action function for state one : event one
                NextState = PREPARING_CRATE; // Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = false; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                // EntryEventKind.EventType = ES_ENTRY_HISTORY;
                // optionally, consume or re-map this event for the upper
                // level state machine
                ReturnEvent.EventType = ES_NO_EVENT;
                break;
            case ES_COMMAND_DONE: // robot has driven to stack
                // Execute action function for state one : event one
                NextState = PLACING_CRATE; // Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
                // optionally, consume or re-map this event for the upper
                // level state machine
                ReturnEvent.EventType = ES_NO_EVENT;
                break;
                // repeat cases as required for relevant events
            }
        }
        break;
    case PLACING_CRATE:
        ReturnEvent = CurrentEvent = DuringPlacingCrate(CurrentEvent);
        // process any events
        if (CurrentEvent.EventType != ES_NO_EVENT) // If an event is active
        {
            switch (CurrentEvent.EventType)
            {
            case ES_TIMEOUT:
                if (CurrentEvent.EventParam == ARM_DELAY_TIMER) // crate has been placed
                {
                    DB_printf("Returning to T Intersection\n");
                    // Return to T intersection for intake
                    if (CrateLevel == 4 || CrateLevel == 5)
                    {
                        SetFollowerCommand(DR_5CM);
                    }
                    else if (CrateLevel == 2 || CrateLevel == 3)
                    {
                        SetFollowerCommand(DR_5CM);
                    }
                    else if (CrateLevel == 1)
                    {
                        SetFollowerCommand(DR_15CM);
                    }
                    // Execute action function for state one : event one
                    NextState = PLACING_CRATE; // Decide what the next state will be
                    // for internal transitions, skip changing MakeTransition
                    MakeTransition = false; // mark that we are taking a transition
                    // if transitioning to a state with history change kind of entry
                    // EntryEventKind.EventType = ES_ENTRY_HISTORY;
                    // optionally, consume or re-map this event for the upper
                    // level state machine
                    ReturnEvent.EventType = ES_NO_EVENT;
                }
                else if (CurrentEvent.EventParam == CLEANUP_TIMER)
                {
                    // Execute action function for state one : event one
                    NextState = PLACING_DONE; // Decide what the next state will be
                    // for internal transitions, skip changing MakeTransition
                    MakeTransition = true; // mark that we are taking a transition
                    // if transitioning to a state with history change kind of entry
                    ReturnEvent = CurrentEvent;

                    DB_printf("Game timer handled in GameMode_PlaceCrateSM\n");
                }
                break;
            case ES_COMMAND_DONE: // Follower has returned to T intersection
                // Execute action function for state one : event one
                DB_printf("Made clearance for arm, finding T\n");
                if (CrateLevel == 4 || CrateLevel == 5)
                { // high
                    if (CurrPos.x == 3 || CurrPos.x == 1)
                    {
                        SetArm(ARM_IDLE, true);
                        ES_Timer_StopTimer(ARM_DELAY_TIMER);
                        SetFollowerCommand(DRFULL_T);
                    }
                    else
                    {
                        SetArm(ARM_IDLE, false);
                        ES_Timer_StopTimer(ARM_DELAY_TIMER);
                        SetFollowerCommand(DRFULL_T);
                    }
                }
                else if (CrateLevel == 2 || CrateLevel == 3)
                { // mid
                    if (CurrPos.x == 3 || CurrPos.x == 1)
                    {
                        SetArm(ARM_IDLE, true);
                        ES_Timer_StopTimer(ARM_DELAY_TIMER);
                        SetFollowerCommand(DFFULL_T);
                    }
                    else
                    {
                        SetArm(ARM_IDLE, false);
                        ES_Timer_StopTimer(ARM_DELAY_TIMER);
                        SetFollowerCommand(DFFULL_T);
                    }
                }
                else if (CrateLevel == 1)
                {
                    if (CurrPos.x == 3 || CurrPos.x == 1)
                    {
                        SetArm(ARM_IDLE, true);
                        ES_Timer_StopTimer(ARM_DELAY_TIMER);
                        SetFollowerCommand(DFFULL_T);
                    }
                    else
                    {
                        SetArm(ARM_IDLE, false);
                        ES_Timer_StopTimer(ARM_DELAY_TIMER);
                        SetFollowerCommand(DFFULL_T);
                    }
                }
                NextState = PLACING_CRATE; // Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = false; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
                // optionally, consume or re-map this event for the upper
                // level state machine
                ReturnEvent.EventType = ES_NO_EVENT;
                break;
                // repeat cases as required for relevant events
            case ES_TAPE_FOUND: // Follower has returned to T intersection
                // Execute action function for state one : event one
                DB_printf("Tape found, going to intaking crate\n");
                NextState = INTAKING_CRATE; // Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
                // optionally, consume or re-map this event for the upper
                // level state machine
                ReturnEvent.EventType = ES_NO_EVENT;
                break;
                // repeat cases as required for relevant events
            }
        }
        break;
    case PLACING_DONE:

        break;
        // repeat state pattern as required for other states
    }
    //   If we are making a state transition
    if (MakeTransition == true)
    {
        //   Execute exit function for current state
        CurrentEvent.EventType = ES_EXIT;
        RunPlaceCrateSM(CurrentEvent);

        CurrentState = NextState; // Modify state variable

        //   Execute entry function for new state
        // this defaults to ES_ENTRY
        RunPlaceCrateSM(EntryEventKind);
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
void StartPlaceCrateSM(ES_Event_t CurrentEvent)
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
    RunPlaceCrateSM(CurrentEvent);
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
TemplateState_t QueryPlaceCrateSM(void)
{
    return (CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

/****************************************************************************
 Function
    DuringIntakingCrate

 Parameters
    ES_Event_t Event - the event to process

 Returns
    ES_Event_t - the event to return

 Description
    Handles the actions and transitions for the "Intaking Crate" state.
    Processes ES_ENTRY, ES_ENTRY_HISTORY, and ES_EXIT events. During entry,
    it checks the availability of crates and stack positions, commands the
    follower to approach stacks, and manages the intake and arm operations.
    If there are no available stacks but crates are present, it transitions
    to leave the stack. If out of crates, it also transitions to leave the
    stack.

****************************************************************************/

static ES_Event_t DuringIntakingCrate(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY))
    {
        DB_printf("Beginning IntakeCrate\n");
        // determine if we have crates

        // static uint8_t *CurrPos;
        CurrPos = QueryCurrentPos();

        AvailableStacks = !Stacks[CurrPos.x - 1][4] + !Stacks[CurrPos.x - 1][3] + !Stacks[CurrPos.x - 1][2] + !Stacks[CurrPos.x - 1][1] + !Stacks[CurrPos.x - 1][0];

        DB_printf("Available Stacks: %d\n", AvailableStacks);

        if ((CrateInArm + CrateInIntake > 0) && (AvailableStacks > 0))
        {
            DB_printf("We have crates\n");

            SetFollowerCommand(DR_STACK);

            if (CrateInIntake < 3)
            {
                SetIntake(INTAKE_PUSH);
            }
            if (!CrateInArm)
            {
                SetArm(ARM_INTAKE, false);
                CrateInArm = true;
                CrateInIntake--;
            }
            else
            {
                ES_Timer_InitTimer(ARM_DELAY_TIMER, 10);
            }
        }
        else if ((CrateInArm + CrateInIntake > 0) && (AvailableStacks == 0))
        {
            DB_printf("We have crates but no stacks\n");
            if (CurrPos.x == 2 || CurrPos.x == 4)
            {
                if (CrateInIntake < 3)
                {
                    SetIntake(INTAKE_PUSH);
                }
                if (!CrateInArm)
                {
                    SetArm(ARM_INTAKE, false);
                    CrateInArm = true;
                    CrateInIntake--;
                }
            }

            ES_Event_t temp;
            temp.EventType = ES_LEAVE_STACK;
            PostTopLevelSM(temp);
        }
        else // out of crates
        {
            DB_printf("We are out of crates\n");
            ES_Event_t temp;
            temp.EventType = ES_LEAVE_STACK;
            PostTopLevelSM(temp);
        }
    }
    else if (Event.EventType == ES_EXIT)
    {
    }
    else // do the 'during' function for this state
    {
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return (ReturnEvent);
}

/****************************************************************************
 Function
    DuringPreparingCrate

 Parameters
    ES_Event_t Event - the event to process

 Returns
    ES_Event_t - the event to return

 Description
    Handles the actions and transitions for the "Preparing Crate" state.
    Processes ES_ENTRY, ES_ENTRY_HISTORY, and ES_EXIT events. During entry,
    it determines the stack level to place a crate in based on the current
    positions of the stacks, and moves the arm to the corresponding level.
    During exit, it prints a message indicating that the robot has successfully
    aligned the arm with the stack.
****************************************************************************/
static ES_Event_t DuringPreparingCrate(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY))
    {
        DB_printf("Evaluating where to place crate...\n");

        // Decide what stack level to place a crate
        for (CrateLevel = 5; CrateLevel > 0; CrateLevel--) // check if there are empty stack positions
        {
            // static uint8_t *CurrPos;
            CurrPos = QueryCurrentPos();
            if (Stacks[CurrPos.x - 1][CrateLevel - 1] == false) // prioritize higher crate levels
            {
                DB_printf("Moving arm to level: %d\n", CrateLevel);
                if (CrateLevel == 4 || CrateLevel == 5)
                {
                    SetArm(ARM_HIGH, false);
                }
                else if (CrateLevel == 2 || CrateLevel == 3)
                {
                    SetArm(ARM_MID, false);
                }
                else if (CrateLevel == 1)
                {
                    SetArm(ARM_LOW, false);
                }
                break;
            }
        }
    }
    else if (Event.EventType == ES_EXIT)
    {
        DB_printf("Successfully aligned\n");
    }
    else // do the 'during' function for this state
    {
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return (ReturnEvent);
}

/****************************************************************************
 Function
    DuringPlacingCrate

 Parameters
    ES_Event_t Event - the event to process

 Returns
    ES_Event_t - the event to return

 Description
    Handles the actions and transitions for the "Placing Crate" state.
    Processes ES_ENTRY, ES_ENTRY_HISTORY, and ES_EXIT events. During entry,
    it places a crate at the designated stack level, updates the stack status,
    and prints the current stack status to the debug output. If the robot is
    in a position suitable for placing a crate, the arm is moved to the 
    appropriate position. Otherwise, a timer is initialized to simulate the arm
    delay. Exit and during actions are currently not implemented.
****************************************************************************/

static ES_Event_t DuringPlacingCrate(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY))
    {
        DB_printf("---------------------------\n");
        DB_printf("Placing crate and updating stack status for level: %d\n", CrateLevel);

        // static uint8_t *CurrPos;
        CurrPos = QueryCurrentPos();

        // place crate
        if (CurrPos.x == 2 || CurrPos.x == 4)
        {
            SetArm(ARM_SHEAR, false);
            CrateInArm = false;
        }
        else // fake arm delay timer
        {
            ES_Timer_InitTimer(ARM_DELAY_TIMER, 100);
        }

        Stacks[CurrPos.x - 1][CrateLevel - 1] = true;

        DB_printf("Stack 1: %d, %d, %d\n", Stacks[0][2], Stacks[0][1], Stacks[0][0]);
        DB_printf("Stack 2: %d, %d, %d\n", Stacks[1][2], Stacks[1][1], Stacks[1][0]);
        DB_printf("Stack 3: %d, %d, %d\n", Stacks[2][2], Stacks[2][1], Stacks[2][0]);
        DB_printf("Stack 4: %d, %d, %d\n", Stacks[3][2], Stacks[3][1], Stacks[3][0]);

        DB_printf("Current Stack Status High: %d\n", Stacks[CurrPos.x - 1][2]);
        DB_printf("Current Stack Status Mid: %d\n", Stacks[CurrPos.x - 1][1]);
        DB_printf("Current Stack Status Low: %d\n", Stacks[CurrPos.x - 1][0]);

        DB_printf("---------------------------\n");
    }
    else if (Event.EventType == ES_EXIT)
    {
    }
    else
    // do the 'during' function for this state
    {
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return (ReturnEvent);
}

void UpdateCrates(uint8_t numCrates)
{
    CrateInIntake = numCrates;
}