#ifndef InterruptSetup
#define InterruptSetup

#include <stdbool.h>
typedef enum
{
    IC1_RPA2  = 0x0000,  // RPA2 -> 0000
    IC1_RPB6  = 0x0001,  // RPB6 -> 0001
    IC1_RPA4  = 0x0010,  // RPA4 -> 0010
    IC1_RPB13 = 0x0011,  // RPB13 -> 0011
    IC1_RPB2  = 0x0100,  // RPB2 -> 0100
    IC1_RPC6  = 0x0101   // RPC6 -> 0101 (Note 1)
} IC1_PinMap;

typedef enum
{
    IC2_RPA3  = 0x0000,  // RPA3 -> 0000
    IC2_RPB0  = 0x0001,  // RPB0 -> 0001
    IC2_RPB10 = 0x0010,  // RPB10 -> 0010
    IC2_RPB9  = 0x0011,  // RPB9 -> 0011
    IC2_RPC9  = 0x0100,  // RPC9 -> 0100 (Note 2)
    IC2_RPC4  = 0x0101   // RPC4 -> 0101 (Note 2)
} IC2_PinMap;

typedef enum
{
    IC3_RPA1  = 0x0000,  // RPA1 -> 0000
    IC3_RPB5  = 0x0001,  // RPB5 -> 0001
    IC3_RPB11 = 0x0010,  // RPB11 -> 0010
    IC3_RPB8  = 0x0011,  // RPB8 -> 0011
    IC3_RPA8  = 0x0100   // RPA8 -> 0100 (Note 2)
} IC3_PinMap;

typedef enum
{
    IC4_RPA0  = 0x0000,  // RPA0 -> 0000
    IC4_RPB3  = 0x0001,  // RPB3 -> 0001
    IC4_RPB4  = 0x0010,  // RPB4 -> 0010
    IC4_RPB15 = 0x0011,  // RPB15 -> 0011
    IC4_RPB7  = 0x0100,  // RPB7 -> 0100
    IC4_RPC7  = 0x0101,  // RPC7 -> 0101 (Note 2)
    IC4_RPC0  = 0x0110,  // RPC0 -> 0110 (Note 1)
    IC4_RPC5  = 0x0111   // RPC5 -> 0111 (Note 2)
} IC4_PinMap;













bool FullOCSetup(uint8_t OC_module, uint16_t PRval, uint8_t timer, uint8_t prescale_idx);

bool setIC(uint8_t ICreg, uint8_t timer, uint8_t num_captures,
           uint8_t prescale_idx, uint16_t PRval,
           bool timer_interrupt_enable,  uint8_t priority, 
           uint8_t timer_priority, uint8_t pin_map);

//static functions
static bool setOC_PWM(uint8_t OC_module, uint8_t timer);
static bool setTimer(uint8_t timer, uint8_t prescale_idx, uint16_t period);

#endif // PIC32_SPI_HAL defined
