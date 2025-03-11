/****************************************************************************
 Module
     EventCheckers.h
 Description
     header file for the Event Checkers
*****************************************************************************/

#ifndef EventCheckers_H
#define EventCheckers_H

// the common headers for C99 types
#include <stdint.h>
#include <stdbool.h>

// prototypes for event checkers

bool Check4Keystroke(void);
bool CheckTape(void);
bool CheckBeacon(void);

#endif /* EventCheckers_H */
