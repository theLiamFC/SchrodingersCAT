/****************************************************************************
 Module
     ButtonService.h
 Description
     header file for the Button Service
*****************************************************************************/

#ifndef ServButton_H
#define ServButton_H

#include "ES_Types.h"

// Public Function Prototypes

bool InitButtonService(uint8_t Priority);
bool PostButtonService(ES_Event_t ThisEvent);
ES_Event_t RunButtonService(ES_Event_t ThisEvent);
bool CheckButton(void);

#endif /* ServTemplate_H */
