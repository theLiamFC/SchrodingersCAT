/****************************************************************************
 Module
   IntakeService.c

 Revision
   1.0.1

 Description
   This is a service for supporting crate intake

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
#include "Pic2PicLeaderFSM.h"
#include "ServoService.h"
#include "GameMode_PlaceCrateSM.h"

/*----------------------------- Module Defines ----------------------------*/

#define SERVO_WAIT_TIME 500

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
static uint8_t  LastPinState = 1;
static int8_t IntakeStatus = -1;
static bool insideIntake = false;

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
bool InitIntakeService(uint8_t Priority)
{
    ES_Event_t ThisEvent;
    IntakeStatus = -1;
    MyPriority = Priority;
    DB_printf("Button Service Init\n");
    /********************************************
     in here you write your initialization code
     *******************************************/
    
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
bool PostIntakeService(ES_Event_t ThisEvent)
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
ES_Event_t RunIntakeService(ES_Event_t ThisEvent)
{
    ES_Event_t ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
    /********************************************
     in here you write your service code
     *******************************************/
    
    ES_EventType_t Process = ThisEvent.EventType;
    
    if (Process == ES_BEGIN_INTAKE && IntakeStatus == 0)
    {
        SetIntake(INTAKE_OPEN); //open the intake servo
        IntakeStatus++;
        ES_Event_t NewEvent;
        NewEvent.EventType = ES_GET_BLOCK1;
        PostIntakeService(NewEvent);
        DB_printf("Servo Moved UP: NEXT STEP MOVE TO BLOCK 1\n ");
//        SetFollowerCommand(RCW_RPM); //assume the intake servo arm is up
    }
    
    else if (Process == ES_GET_BLOCK1 && IntakeStatus == 1)
    {
        DB_printf("Telling follower GET_BLOCK1\n");
        SetFollowerCommand(GET_BLOCK1); //need to handle distance to object 1
    }
    
    else if (Process == ES_REV_INTAKE && IntakeStatus == 2) 
    {
        DB_printf("Telling follower REV_INTAKE\n");
        SetFollowerCommand(REV_INTAKE);
    }
    
    else if (Process == ES_MOVE_INTAKE_SERVO && IntakeStatus == 3)
    {
        DB_printf("Servo Moved PUSH: \n");
        SetIntake(INTAKE_PUSH);
        ES_Timer_InitTimer(INTAKE_TIMER, SERVO_WAIT_TIME);
    }
    
    else if (Process == ES_TIMEOUT)
    {
        SetIntake(INTAKE_OPEN);
        
        ES_Event_t NewEvent;
        if (IntakeStatus == 3)
        {
            NewEvent.EventType = ES_GET_BLOCK2;
            IntakeStatus++;
            PostIntakeService(NewEvent);
        }
        else if (IntakeStatus == 6)
        {
            NewEvent.EventType = ES_GET_BLOCK3;
            IntakeStatus++;
            PostIntakeService(NewEvent);
        }
        
    }
    
    else if (Process == ES_GET_BLOCK2 && IntakeStatus == 4)
    {
        SetFollowerCommand(GET_BLOCK2); //need to handle distance to object 1
    }
    
    else if (Process == ES_REV_INTAKE && IntakeStatus == 5) 
    {
        SetFollowerCommand(REV_INTAKE);
    }
    
    else if (Process == ES_MOVE_INTAKE_SERVO && IntakeStatus == 6)
    {
        SetIntake(INTAKE_PUSH);
        ES_Timer_InitTimer(INTAKE_TIMER, SERVO_WAIT_TIME);
    }
    
    else if (Process == ES_GET_BLOCK3 && IntakeStatus == 7)
    {
        SetFollowerCommand(GET_BLOCK3); //need to handle distance to object 1
    }
    
    else if (Process == ES_BACKUP_BEFORE_SHAKE && IntakeStatus == 8) 
    {
        SetFollowerCommand(BACKUP_BEFORE_SHAKE_C);
    }
    
    else if (Process == ES_SHAKE && IntakeStatus == 9) 
    {
        SetFollowerCommand(SHAKE);
    }
    
    else if (Process == ES_BAKE && IntakeStatus == 10)
    {
        SetFollowerCommand(BAKE);
        DB_printf("BAKE \n");
    }

    else if (Process == ES_COMMAND_DONE && insideIntake == true)
    {
        IntakeStatus++;
        ES_Event_t NewEvent;
        DB_printf("INTAKE STATUS: %d\n", IntakeStatus);
        
        if (IntakeStatus == 2)
        {
            DB_printf("REV DIRECTION \n");
            NewEvent.EventType = ES_REV_INTAKE;
        }
        
        else if (IntakeStatus == 3)
        {
            NewEvent.EventType = ES_MOVE_INTAKE_SERVO;
        }
        
        else if (IntakeStatus == 5)
        {
            NewEvent.EventType = ES_REV_INTAKE;
        }
        
        else if (IntakeStatus == 6)
        {
            NewEvent.EventType = ES_MOVE_INTAKE_SERVO;
        }
        
        else if (IntakeStatus == 8)
        {
            NewEvent.EventType = ES_BACKUP_BEFORE_SHAKE;
        }
                
        else if (IntakeStatus == 9){
            NewEvent.EventType = ES_SHAKE;
        }
        
        else if (IntakeStatus == 10){
            NewEvent.EventType = ES_BAKE;
        }
        
        else if (IntakeStatus == 11){
            SetIntake(INTAKE_CLOSED);
            NewEvent.EventType = ES_INTAKE_DONE;
            insideIntake = false;
            PostTopLevelSM(NewEvent);
            UpdateCrates(3);
        }

        PostIntakeService(NewEvent);        
    }
    
    return ReturnEvent;
}

/***************************************************************************
 private functions
 ***************************************************************************/

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

/*-------------------------------------------------------------------------
 * Function: BeginIntake
 * Param: void
 * Returns: void
 * Description: Starts the intake service
 *------------------------------------------------------------------------*/
void BeginIntake(void)
{
    ES_Event_t Event;
    Event.EventType = ES_BEGIN_INTAKE;
    PostIntakeService(Event);
    IntakeStatus = -1;
    insideIntake = true;
}

