/****************************************************************************
 Module
   GameMode_MoveSM.c

 Revision
   2.0.2

 Description
   This is a state machine to control the movement of the robot during the game.

 Notes

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "Pic2PicLeaderFSM.h"
#include "TopLevelSM.h"
#include "dbprintf.h"
#include "ServoService.h"
#include "IntakeService.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "GameMode_MoveSM.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE MOVING_VERTICAL

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event_t DuringMovingVertical(ES_Event_t Event);
static ES_Event_t DuringMovingHorizontal(ES_Event_t Event);
static ES_Event_t DuringMovingTheta(ES_Event_t Event);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static MoveState_t CurrentState;

static uint8_t CurrentX = 0;        // [4,3,2,1,0] left to right (0 is start pos)
static uint8_t CurrentY = 1;        // [0,1,2] bottom to top (1 is middle)
static uint16_t CurrentTheta = 180; // [0:360] east is 0, inits to unknown!!!

static uint8_t TargetX; // [4,3,2,1,0] left to right (0 is start pos)
static uint8_t TargetY; // [0,1,2] bottom to top (1 is middle)

static uint8_t TurnsDone = 0; // If 0, then we want to turn based on where we need to move X, if 1, then Y
static uint8_t VerticalMoves = 0;
static uint8_t DesiredTheta = 0; // internal variable for turning
static MoveState_t LastState = MOVE_DONE;
static bool intakeDone = false;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunMoveSM

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
ES_Event_t RunMoveSM(ES_Event_t CurrentEvent)
{
    bool MakeTransition = false; /* are we making a state transition? */
    MoveState_t NextState = CurrentState;
    ES_Event_t EntryEventKind = {ES_ENTRY, 0}; // default to normal entry to new state
    ES_Event_t ReturnEvent = CurrentEvent;     // assume we are not consuming event

    switch (CurrentState)
    {
    case MOVING_VERTICAL: // If current state is state one
        // Execute During function for state one. ES_ENTRY & ES_EXIT are
        // processed here allow the lower level state machines to re-map
        // or consume the event
        ReturnEvent = CurrentEvent = DuringMovingVertical(CurrentEvent);
        // process any events
        if (CurrentEvent.EventType != ES_NO_EVENT) // If an event is active
        {
            switch (CurrentEvent.EventType)
            {
            case ES_INTAKE_DONE: // Robot is in middle
                // Execute action function for state one : event one
                if (LastState == MOVE_DONE)
                {
                    NextState = MOVING_THETA;
                    ReturnEvent.EventType = ES_NO_EVENT;
                }
                else if (LastState == MOVING_THETA)
                {
                    DB_printf("SENDING ES_MOVE_DONE to GameModeSM\n");
                    ReturnEvent.EventType = ES_MOVE_AGAIN;
                    NextState = MOVE_DONE;
                    intakeDone = true;
                }

                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
                // optionally, consume or re-map this event for the upper
                // level state machine

                break;

            case ES_TAPE_FOUND: // Robot is in middle
                // Execute action function for state one : event one
                if (LastState == MOVE_DONE)
                {
                    NextState = MOVING_THETA;
                    ReturnEvent.EventType = ES_NO_EVENT;
                }
                else if (LastState == MOVING_THETA)
                {
                    DB_printf("SENDING ES_MOVE_DONE to GameModeSM\n");
                    NextState = MOVE_DONE;
                    ReturnEvent.EventType = ES_MOVE_DONE;
                }

                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
                // optionally, consume or re-map this event for the upper
                // level state machine

                break;

            case ES_COMMAND_DONE:
            {
                if (intakeDone == true)
                {
                    if (LastState == MOVE_DONE)
                    {
                        NextState = MOVING_THETA;
                        ReturnEvent.EventType = ES_NO_EVENT;
                    }
                    else if (LastState == MOVING_THETA)
                    {
                        NextState = MOVE_DONE;
                        ReturnEvent.EventType = ES_MOVE_DONE;
                    }
                    MakeTransition = true;
                    EntryEventKind.EventType = ES_ENTRY;
                    intakeDone = false;
                }
            }
            break;

            case ES_TIMEOUT:
            {
                if (CurrentEvent.EventParam == CLEANUP_TIMER)
                {
                    // DB_printf("TimerHasBeenHandled\n");
                    //  Execute action function for state one : event one
                    NextState = MOVE_DONE; // Decide what the next state will be
                    // for internal transitions, skip changing MakeTransition
                    MakeTransition = true; // mark that we are taking a transition
                    // if transitioning to a state with history change kind of entry
                    ReturnEvent = CurrentEvent;

                    //                            EntryEventKind.EventParam =
                    DB_printf("Game timer handled in GameMode_MoveSM While MovingVertical\n");
                }
            }
            break;
                // repeat cases as required for relevant events
            }
        }
        break;

    case MOVING_THETA: // If current state is state one
        // Execute During function for state one. ES_ENTRY & ES_EXIT are
        // processed here allow the lower level state machines to re-map
        // or consume the event
        ReturnEvent = CurrentEvent = DuringMovingTheta(CurrentEvent);
        // process any events
        if (CurrentEvent.EventType != ES_NO_EVENT) // If an event is active
        {
            switch (CurrentEvent.EventType)
            {
            case ES_COMMAND_DONE: // Robot has turned
                // Execute action function for state one : event one
                if (LastState == MOVING_VERTICAL)
                {
                    NextState = MOVING_HORIZONTAL;
                }
                else if (LastState == MOVING_HORIZONTAL)
                {
                    NextState = MOVING_VERTICAL;
                } // Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
                // optionally, consume or re-map this event for the upper
                // level state machine
                ReturnEvent.EventType = ES_NO_EVENT;
                break;

            case ES_TIMEOUT:
            {
                if (CurrentEvent.EventParam == CLEANUP_TIMER)
                {
                    // DB_printf("TimerHasBeenHandled\n");
                    //  Execute action function for state one : event one
                    NextState = MOVE_DONE; // Decide what the next state will be
                    // for internal transitions, skip changing MakeTransition
                    MakeTransition = true; // mark that we are taking a transition
                    // if transitioning to a state with history change kind of entry
                    ReturnEvent = CurrentEvent;

                    //                            EntryEventKind.EventParam =
                    DB_printf("Game timer handled in GameMode_MoveSM While MovingTheta\n");
                }
            }
            break;
                // repeat cases as required for relevant events
            }
        }
        break;

    case MOVING_HORIZONTAL: // If current state is state one
        // Execute During function for state one. ES_ENTRY & ES_EXIT are
        // processed here allow the lower level state machines to re-map
        // or consume the event
        ReturnEvent = CurrentEvent = DuringMovingHorizontal(CurrentEvent);
        // process any events
        if (CurrentEvent.EventType != ES_NO_EVENT) // If an event is active
        {
            switch (CurrentEvent.EventType)
            {
            case ES_COMMAND_DONE: // Robot has aligned with target in the x direction
                // Execute action function for state one : event one
                NextState = MOVING_THETA; // Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; // mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
                // optionally, consume or re-map this event for the upper
                // level state machine
                ReturnEvent.EventType = ES_NO_EVENT;
                break;

            case ES_TIMEOUT:
            {
                if (CurrentEvent.EventParam == CLEANUP_TIMER)
                {
                    // DB_printf("TimerHasBeenHandled\n");
                    //  Execute action function for state one : event one
                    NextState = MOVE_DONE; // Decide what the next state will be
                    // for internal transitions, skip changing MakeTransition
                    MakeTransition = true; // mark that we are taking a transition
                    // if transitioning to a state with history change kind of entry
                    ReturnEvent = CurrentEvent;

                    //                            EntryEventKind.EventParam =
                    DB_printf("Game timer handled in GameMode_MoveSM while Movinghorizontal\n");
                }
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
        RunMoveSM(CurrentEvent);

        CurrentState = NextState; // Modify state variable

        //   Execute entry function for new state
        // this defaults to ES_ENTRY
        RunMoveSM(EntryEventKind);
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
void StartMoveSM(ES_Event_t CurrentEvent)
{
    // to implement entry to a history state or directly to a substate
    // you can modify the initialization of the CurrentState variable
    // otherwise just start in the entry state every time the state machine
    // is started
    if (ES_ENTRY_HISTORY != CurrentEvent.EventType)
    {
        CurrentState = ENTRY_STATE;
    }
    DB_printf("MoveSM has been Entered!\n");
    LastState = MOVE_DONE;
    SetArm(ARM_IDLE, false);
    // call the entry function (if any) for the ENTRY_STATE
    RunMoveSM(CurrentEvent);
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
MoveState_t QueryMoveSM(void)
{
    return (CurrentState);
}

/*
 * Sets the target X and Y coordinates of the robot. The target coordinates
 * are used by the state machine to determine when to stop moving the robot.
 * The coordinates are 0-indexed, with the origin at the bottom left corner of
 * the maze. The x-coordinate increases as you move to the right, and the
 * y-coordinate increases as you move up.
 */
void SetTargetPos(uint8_t x, uint8_t y)
{
    TargetX = x;
    TargetY = y;
}

/*
 * Returns the current position of the robot as a Position struct.
 */
Position QueryCurrentPos(void)
{
    Position currentPos;
    currentPos.x = CurrentX; // Assuming CurrentX is defined elsewhere
    currentPos.y = CurrentY; // Assuming CurrentY is defined elsewhere
    return currentPos;
}

/***************************************************************************
 private functions
 ***************************************************************************/

/****************************************************************************
 Function
    DuringMovingVertical

 Parameters
    ES_Event_t: the event to process

 Returns
    ES_Event_t: an event to return

 Description
    Handles the actions and transitions for the MOVING_VERTICAL state. 
    Processes entry, during, and exit events. On entry, it determines the 
    direction of movement based on the robot's current position and target Y 
    coordinate. On exit, it updates the robot's current Y position and stops 
    the motors. During the state, it tracks the tape and manages lower level 
    state machines.

****************************************************************************/

static ES_Event_t DuringMovingVertical(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY))
    {
        // implement any entry actions required for this state machine

        /*
         * If in back, move forward, if in front, move backward, if in middle, exit
         */

        DB_printf("ENTERING MOVING_VERTICAL\n");
        DB_printf("Last State = %d\n", (LastState == MOVING_THETA));
        if (LastState == MOVE_DONE)
        {
            if (CurrentY == 0) // bottom row
            {
                SetFollowerCommand(DFFULL_1); // move forward
                intakeDone = true;
            }
            else if (CurrentY == 2) // top row
            {
                DB_printf("Calling DRFULL for some reason\n");
                SetFollowerCommand(DRFULL); // move backward
                intakeDone = true;
            }
            else // already in the middle row
            {
                ES_Event_t temp;
                temp.EventType = ES_INTAKE_DONE; // transition to turn
                PostTopLevelSM(temp);
            }
        }
        else if (LastState == MOVING_THETA)
        {
            if (TargetY == 2)
            {
                SetFollowerCommand(DFFULL_T);
                intakeDone = true;
            }
            else
            {
                DB_printf("Beginning Intake\n");
                BeginIntake();
                //                SetFollowerCommand(MOVE_INTAKE);
            }
        }

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
        VerticalMoves++;
        DB_printf("Exiting vertical movement\n;");

        SetFollowerCommand(STOP);
        if (LastState == MOVE_DONE)
        {
            CurrentY = 1;
        }
        else if (LastState == MOVING_THETA)
        {
            CurrentY = TargetY;
        }
        LastState = MOVING_VERTICAL;
        //        if (VerticalMoves == 1) {
        //            CurrentY = 1;
        //        } else if (VerticalMoves == 2) {
        //            CurrentY = TargetY;
        //            VerticalMoves = 0;
        //
        //        }
        /*
         * We are in the middle, stop the motors
         */
    }
    else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);

        // repeat for any concurrent lower level machines

        // do any activity that is repeated as long as we are in this state
        /*
         * Continue tracking the tape! (Is it feasible for this to be the during?)
         */
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return (ReturnEvent);
}

/*
 * Handles the actions and transitions for the MOVING_THETA state. 
 * Processes entry, during, and exit events. On entry, it determines the 
 * direction of movement based on the robot's current position and target Y 
 * coordinate. On exit, it updates the robot's current Y position and stops 
 * the motors. During the state, it tracks the tape and manages lower level 
 * state machines. 
 */
static ES_Event_t DuringMovingTheta(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY))
    {
        DB_printf("Entering DuringMovingTheta!\n");
        // implement any entry actions required for this state machine
        if (TurnsDone == 0)
        { // at middle, facing forward (or West at beginning)
            int8_t DeltaX = TargetX - CurrentX;
            if (DeltaX > 0)
            {
                DesiredTheta = 180;
                if (DesiredTheta == CurrentTheta)
                {
                    ES_Event_t temp;
                    temp.EventType = ES_COMMAND_DONE;
                    PostTopLevelSM(temp);
                }
                else if (DesiredTheta > CurrentTheta)
                { // Need to turn 90 deg CCW
                    SetFollowerCommand(RCCW90);
                }
            }
            else if (DeltaX < 0)
            {
                DesiredTheta = 0;
                if (DesiredTheta == CurrentTheta)
                {
                    ES_Event_t temp;
                    temp.EventType = ES_COMMAND_DONE;
                    PostTopLevelSM(temp);
                }
                else if (DesiredTheta < CurrentTheta)
                { // Need to turn 90 deg CCW
                    SetFollowerCommand(RCW90);
                }
            }
            else
            {
                DesiredTheta = 90;
                ES_Event_t temp;
                temp.EventType = ES_COMMAND_DONE;
                PostTopLevelSM(temp);
            }
        }
        else if (TurnsDone == 1)
        {
            DesiredTheta = 90;
            if (DesiredTheta == CurrentTheta)
            {
                ES_Event_t temp;
                temp.EventType = ES_COMMAND_DONE;
                PostTopLevelSM(temp);
            }
            else if (DesiredTheta > CurrentTheta && TargetY == 0)
            {                                 // Need to turn 90 deg CCW
                SetFollowerCommand(RCCW_RPM); // TODO: add logic for when turn RPM vs tape
            }
            else if (DesiredTheta < CurrentTheta && TargetY == 0)
            {
                SetFollowerCommand(RCW_RPM); // TODO: add logic for when turn RPM vs tape
            }
            else if (DesiredTheta > CurrentTheta)
            {                               // Need to turn 90 deg CCW
                SetFollowerCommand(RCCW90); // TODO: add logic for when turn RPM vs tape
            }
            else if (DesiredTheta < CurrentTheta)
            {
                SetFollowerCommand(RCW90); // TODO: add logic for when turn RPM vs tape
            }
        }
    }
    else if (Event.EventType == ES_EXIT)
    {
        // on exit, give the lower levels a chance to clean up first
        // RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
        if (LastState == MOVING_VERTICAL)
        {
            TurnsDone = 1;
            CurrentTheta = DesiredTheta;
        }
        else if (LastState == MOVING_HORIZONTAL)
        {
            TurnsDone = 0;
            CurrentTheta = 90;
        }
        LastState = MOVING_THETA;
        /*
         * We have completed turn, stop the motors
         */
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
 * @brief Moves the robot to the target horizontal position.
 *
 * This function determines the amount of horizontal movement required to reach the target position
 * and sends the appropriate command to the follower.
 *
 * @param[in] Event The event that triggered this state machine.
 *
 * @return The remapped event if the lower level machine needs to remap the event, or the original event
 * if not.
 */
static ES_Event_t DuringMovingHorizontal(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY))
    {
        // implement any entry actions required for this state machine
        DB_printf("MovingHorizontal!!\n");
        int8_t temp = abs(TargetX - CurrentX);
        uint8_t DeltaX = (uint8_t)temp;
        if (DeltaX == 1)
        {
            SetFollowerCommand(DFFULL_1);
            DB_printf("Sending move 1 intersections to follower!\n");
        }
        else if (DeltaX == 2)
        {
            SetFollowerCommand(DFFULL_2);
            DB_printf("Sending move 2 intersections to follower!\n");
        }
        else if (DeltaX == 3)
        {
            SetFollowerCommand(DFFULL_3);
            DB_printf("Sending move 3 intersections to follower!\n");
        }
        else if (DeltaX == 4)
        {
            SetFollowerCommand(DFFULL_4);
            DB_printf("Sending move 4 intersections to follower!\n");
        }
        else
        {
            ES_Event_t event;
            event.EventType = ES_COMMAND_DONE;
            PostTopLevelSM(event);
            SetFollowerCommand(STOP);
            DB_printf("we are already aligned horizontally\n");
        }
        /*
         * If target - current > 0 move forward, if target - current < 0 move backward, if at target, exit
         */

        // assuming in this state robot is always at CurrentTheta = 180 !!!

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
        CurrentX = TargetX;
        LastState = MOVING_HORIZONTAL;
        /*
         * We are aligned with target, stop the motors
         */
    }
    else
    // do the 'during' function for this state
    {
    
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return (ReturnEvent);
}
