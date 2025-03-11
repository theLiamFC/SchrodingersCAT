/****************************************************************************

  Header file for template Flat Sate Machine
  based on the Gen2 Events and Services Framework

 ****************************************************************************/

#ifndef Pic2PicLeaderFSM_H
#define Pic2PicLeaderFSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
    //Idle,
    Confirm,
    Send,
    Receive
} Pic2PicLeaderFSM_t;

#define SEARCH_BEACON 2
#define RCCW90 3
#define RCW90 12
#define DFFULL_1 4
#define DFFULL_2 9
#define DFFULL_3 10
#define DFFULL_4 11
#define STOP 5
#define BEACON_FOUND_R 6
#define BEACON_FOUND_B 7
#define DRFULL 8
#define COMMAND_DONE 13
#define DFFULL_T 14
#define STANDBY 15
#define RCW_TO_TAPE 16
#define TAPE_FOUND_INIT 17
#define T_FOUND 18
#define CRATE_HIGH 19
#define CRATE_HIGH_KNOCK 39
#define CRATE_MID 20
#define CRATE_LOW 21
#define CRATE_ALIGNED 22
#define DISTANCE_REACHED 23
#define DR_STACK 24
#define DRFULL_T 25
#define MOVE_INTAKE 26
#define RCW_RPM 27
#define RCCW_RPM 28

//INTAKE SEQUENCE COMMANDS
#define BEGIN_INTAKE 29
#define GET_BLOCK1 30
#define GET_BLOCK2 31
#define GET_BLOCK3 32
#define REV_INTAKE 33 //activates intake timer
#define MOVE_INTAKE_SERVO_DOWN 34
#define MOVE_INTAKE_SERVO_UP 35
#define SHAKE 36
#define BAKE 37
#define BACKUP_BEFORE_SHAKE_C 38


#define DR_5CM 50
#define DR_15CM 150
// Public Function Prototypes

bool InitPic2PicLeaderFSM(uint8_t Priority);
bool PostPic2PicLeaderFSM(ES_Event_t ThisEvent);
ES_Event_t RunPic2PicLeaderFSM(ES_Event_t ThisEvent);
Pic2PicLeaderFSM_t QueryPic2PicLeaderFSM(void);
void SetFollowerCommand(uint8_t Command);
uint8_t QueryFollowerStatus(void);

#endif /* FSMTemplate_H */
