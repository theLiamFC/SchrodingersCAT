/****************************************************************************
 Header file for the Sudden Death State Machine
 ****************************************************************************/

#ifndef HSMSuddenDeath_H
#define HSMSuddenDeath_H

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
    FINDING_BEACON,
    FINDING_TAPE,
            MOVING_FORWARD,
            TURNING,
            MOVING_STACK,
            PLACING,
            SD_DONE
} SDState_t;

// Public Function Prototypes

ES_Event_t RunSuddenDeathSM(ES_Event_t CurrentEvent);
void StartSuddenDeathSM(ES_Event_t CurrentEvent);
SDState_t QuerySuddenDeathSM(void);

#endif /*SHMTemplate_H */
