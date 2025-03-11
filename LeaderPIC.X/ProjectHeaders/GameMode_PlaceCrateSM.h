/****************************************************************************
 Module
     GameMode_PlaceCrateSM.h
 Description
     header file for the State Machine to control the placing of a crate
*****************************************************************************/

#ifndef HSMPlaceCrate_H
#define HSMPlaceCrate_H

// typedefs for the states
typedef enum
{
    INTAKING_CRATE,
    PREPARING_CRATE,
    PLACING_CRATE,
    PLACING_DONE
} TemplateState_t;

// Public Function Prototypes

ES_Event_t RunPlaceCrateSM(ES_Event_t CurrentEvent);
void StartPlaceCrateSM(ES_Event_t CurrentEvent);
TemplateState_t QueryPlaceCrateSM(void);

void UpdateCrates(uint8_t numCrates);

#endif /*SHMTemplate_H */
