/****************************************************************************
 Module
     GameModeSM.h
 Description
     header file for the GameMode State Machine
*****************************************************************************/

#ifndef GameModeSM_H
#define GameModeSM_H

// typedefs for the states
typedef enum
{
    Initialize, 
    Move,
    PlaceCrate,
            GameModeDone
} GameModeState_t;
// Public Function Prototypes

ES_Event_t RunGameModeSM(ES_Event_t CurrentEvent);
void StartGameModeSM(ES_Event_t CurrentEvent);
GameModeState_t QueryGameModeSM(void);

#endif /*SHMTemplate_H */
