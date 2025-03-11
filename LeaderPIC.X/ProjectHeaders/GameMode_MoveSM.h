/****************************************************************************
 Module
     GameMode_MoveSM.h
 Description
     header file for the MoveSM game mode
*****************************************************************************/

#ifndef MoveSM_H
#define MoveSM_H

// typedefs for the states
typedef enum
{
    MOVING_THETA,
    MOVING_HORIZONTAL,
    MOVING_VERTICAL,
            MOVE_DONE
} MoveState_t;

typedef struct {
    uint16_t x;
    uint16_t y;
} Position;

// Public Function Prototypes

ES_Event_t RunMoveSM(ES_Event_t CurrentEvent);
void StartMoveSM(ES_Event_t CurrentEvent);
MoveState_t QueryMoveSM(void);

Position QueryCurrentPos(void);
void SetTargetPos(uint8_t x, uint8_t y);

#endif /*SHMTemplate_H */
