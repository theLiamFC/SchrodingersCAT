/****************************************************************************
 Module
     GameMode_InitializeSM.h
 Description
     header file for the Initialize State Machine
*****************************************************************************/

#ifndef InitializeSM_H
#define InitializeSM_H

// typedefs for the states
typedef enum
{
    FINDING_BEACON,
    FINDING_TAPE,
    INITIALIZE_DONE
    //whatever other states are needed
} InitializeState_t;

// Public Function Prototypes

ES_Event_t RunInitializeSM(ES_Event_t CurrentEvent);
void StartInitializeSM(ES_Event_t CurrentEvent);
InitializeState_t QueryInitializeSM(void);

#endif /*SHMTemplate_H */
