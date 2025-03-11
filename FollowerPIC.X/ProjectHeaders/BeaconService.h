/****************************************************************************

  Header file for Beacon Service

 ****************************************************************************/

#ifndef Beacon_H
#define Beacon_H

#include <stdint.h>
#include <stdbool.h>

#include "ES_Events.h"
#include "ES_Port.h" // needed for definition of REENTRANT
// Public Function Prototypes

typedef union {
    uint32_t TotalTime;
    uint16_t SplitTimers[2];
} Time_t;


bool InitBeaconService(uint8_t Priority);
bool PostBeaconService(ES_Event_t ThisEvent);
ES_Event_t RunBeaconService(ES_Event_t ThisEvent);
void IncrementBeaconTimer(void);

#endif /* ServTemplate_H */
