/****************************************************************************

  Header file for Motor Service

 ****************************************************************************/

 #ifndef MotorService_H
 #define MotorService_H
 
 #include <stdint.h>
 #include <stdbool.h>
 
 #include "ES_Events.h"
 #include "ES_Port.h" // needed for definition of REENTRANT
 // Public Function Prototypes
 
 bool InitMotorService(uint8_t Priority);
 bool PostMotorService(ES_Event_t ThisEvent);
 ES_Event_t RunMotorService(ES_Event_t ThisEvent);
 void SetMotorRPM(int8_t LeftRPM, int8_t RightRPM);
 bool Check4Intersection(void);
 bool Check4Turn(void);
 bool Check4T(void);
 void moveDistance(int16_t distance);
 //bool Check4TFront(void);
 #endif /* ServTemplate_H */
 