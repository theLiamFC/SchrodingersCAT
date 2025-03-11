/****************************************************************************
 Module
     IdleSM.h
 Description
     header file for the Idle State Machine
*****************************************************************************/

#ifndef IdleSM_H
#define IdleSM_H

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
    IDLE_WAITING,
    IDLE_ENTERING_MAIN,
    IDLE_ENTERING_SD,
    IDLE_DONE
} IdleState_t;

// Public Function Prototypes

ES_Event_t RunIdleSM(ES_Event_t CurrentEvent);
void StartIdleSM(ES_Event_t CurrentEvent);
IdleState_t QueryIdleSM(void);

#endif /*SHMTemplate_H */
