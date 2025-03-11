/****************************************************************************
 Header file for the Top Level State Machine for the project

 ****************************************************************************/

#ifndef TopLevelSM_H
#define TopLevelSM_H

// State definitions for use with the query function
typedef enum
{
    Idle,
    GameMode,
            SuddenDeath
    //sudden death/OT //TO BE COMPLETED
} TopLevelState_t;

// Public Function Prototypes
ES_Event_t RunTopLevelSM(ES_Event_t CurrentEvent);
void StartTopLevelSM(ES_Event_t CurrentEvent);
bool PostTopLevelSM(ES_Event_t ThisEvent);
bool InitTopLevelSM(uint8_t Priority);

#endif /*TopLEVELSM */
