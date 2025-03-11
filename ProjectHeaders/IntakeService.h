/****************************************************************************

  Header file for template service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef IntakeService_H
#define IntakeService_H

#include "ES_Types.h"

// Public Function Prototypes

bool InitIntakeService(uint8_t Priority);
bool PostIntakeService(ES_Event_t ThisEvent);
ES_Event_t RunIntakeService(ES_Event_t ThisEvent);
void BeginIntake(void);


#endif /* ServTemplate_H */
