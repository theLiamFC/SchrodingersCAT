#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "InterruptSetup.h"

// Define struct pointers to OCxCONbits (bitfield access)
// Union to store any OCxCONbits type
typedef union {
    volatile __OC1CONbits_t *OC;  // Use a single struct type for all OC modules
} OCxCON_Union;

// Array of OCxCON register pointers
static OCxCON_Union OCxCON[] = {
    { &OC1CONbits },
    { &OC2CONbits },
    { &OC3CONbits },
    { &OC4CONbits },
    { &OC5CONbits }
};


volatile void *const ICxCON[] = { &IC1CONbits, &IC2CONbits, 
                                            &IC3CONbits, &IC4CONbits, &IC5CONbits };

// Define array of pointers to OCxR and OCxRS (they remain uint32_t registers)
static volatile uint32_t *const OCxR[] = {&OC1R, &OC2R, &OC3R, &OC4R, &OC5R};
static volatile uint32_t *const OCxRS[] = {&OC1RS, &OC2RS, &OC3RS, &OC4RS, &OC5RS};

// Define struct pointers to Timer Control Registers (bitfield access)
static volatile __T2CONbits_t *const Timers[] = {&T2CONbits, &T3CONbits, 
                                                 &T4CONbits, &T5CONbits};

// Define array of pointers to PRx Period Registers (unchanged)
static volatile uint32_t *const Pr[] = {&PR1, &PR2, &PR3, &PR4, &PR5};

static const uint32_t TimerFlags[] = {_IFS0_T1IF_MASK, _IFS0_T2IF_MASK,
                                                _IFS0_T3IF_MASK, _IFS0_T4IF_MASK,
                                                _IFS0_T5IF_MASK};

static const uint32_t InterruptFlags[] = {_IEC0_IC1IE_MASK, _IEC0_IC2IE_MASK,
                                                _IEC0_IC3IE_MASK, _IEC0_IC4IE_MASK,
                                                _IEC0_IC5IE_MASK};

static const uint32_t TimerInterruptFlags[] = {_IEC0_T1IE_MASK, _IEC0_T2IE_MASK,
                                                _IEC0_T3IE_MASK, _IEC0_T4IE_MASK,
                                                _IEC0_T5IE_MASK};


// Define pointers to the PPS Output Registers for each OC module
static volatile uint32_t *const OC1_PinMap[] = { &RPA0R, &RPA1R, &RPA2R, &RPB3R, &RPB4R, &RPB7R };
static volatile uint32_t *const OC2_PinMap[] = { &RPA3R, &RPB5R, &RPB6R, &RPB8R, &RPB9R, &RPC6R };
static volatile uint32_t *const OC3_PinMap[] = { &RPA4R, &RPB1R, &RPB2R, &RPB10R, &RPB11R, &RPC7R };
static volatile uint32_t *const OC4_PinMap[] = { &RPA2R, &RPB0R, &RPB3R, &RPB15R, &RPC1R, &RPC8R };
static volatile uint32_t *const OC5_PinMap[] = { &RPA1R, &RPB9R, &RPB14R, &RPC2R, &RPC5R, &RPC9R };



//NOTE: YOU MUST INDIVIDUALLY SET THE ANSEL AND TRIS BITS and then MAP the 
//register to the correct OC reference. 
//NOTE2: YOU MUST TURN ON THE TIMER OUTSIDE THIS FUNCTION
bool FullOCSetup(uint8_t OC_module, uint16_t PRval, uint8_t timer, uint8_t prescale_idx)
{
    bool pwm = setOC_PWM(OC_module, timer);
    bool t = setTimer(timer, prescale_idx, PRval);
    return (pwm && t);
}
//
//bool setIC(uint8_t ICreg, uint8_t timer, uint8_t num_captures,
//           uint8_t prescale_idx, uint16_t PRval,
//           bool timer_interrupt_enable,  uint8_t priority, 
//           uint8_t timer_priority, uint8_t pin_map)
//{
//    if (timer!=2 || timer!=3)
//    {
//        return false;
//    }
//    
//    if (num_captures < 1 || num_captures > 4)
//    {
//        return false;
//    }
//    
//    //selecting information for which IC module to use
//    volatile __IC1CONbits_t *pIC = (volatile __IC1CONbits_t *)(ICxCON[ICreg - 1]);
//    volatile __T2CONbits_t *pTx = Timers[timer - 2]; //timer 1 is a different type
//    
//    __builtin_disable_interrupts();
//    
//    pIC->ON = 0;
//    pIC->SIDL = 0;
//    pIC->C32 = 0;
//    pIC->ICTMR = timer - 2;
//    pIC->ICI = num_captures - 1;
//    pIC->ICM = 3; //rising edge
//    pIC->ON = 1;
//    pTx->ON = 1; //enable timer 
//    
//    setTimer(timer, prescale_idx, PRval); //
//    
//    
//    IEC0SET = InterruptFlags[ICreg-1]; //enable local timeout interrupt IC reg
//    
//    if (timer_interrupt_enable)
//    {
//        IEC0SET = TimerInterruptFlags[timer-1]; //enable for timer
//        
//        if (timer == 2)
//        {
//            TMR2 = 0;
//            IPC2bits.T2IP = timer_priority;
//        }
//        else
//        {
//            TMR3 = 0;
//            IPC3bits.T3IP = timer_priority;
//        }
//    }
//    
//    
//    INTCONbits.MVEC = 1; //enable multivector 
//    
//    if (ICreg == 1)
//    {
//        IPC1bits.IC1IP = priority;
//        IC1Rbits.IC1R = pin_map; 
//        
//    }
//    else if (ICreg == 2)
//    {
//        IPC2bits.IC2IP = priority;
//        IC2Rbits.IC2R = pin_map; 
//        
//    }
//    else if (ICreg == 3)
//    {
//        IPC3bits.IC3IP = priority;
//        IC3Rbits.IC3R = pin_map; 
//        
//    }
//    else if (ICreg == 3)
//    {
//        IPC3bits.IC3IP = priority;
//        IC3Rbits.IC3R = pin_map; 
//        
//    }
//    else if (ICreg == 4)
//    {
//        IPC4bits.IC4IP = priority;
//        IC4Rbits.IC4R = pin_map; 
//        
//    }
//    else
//    {
//        IPC5bits.IC5IP = priority;
//        IC5Rbits.IC5R = pin_map; 
//    }
//    __builtin_enable_interrupts();
//    return true;
//}

bool setServoAngle(uint8_t OC_module, uint8_t timer, int8_t angle, uint8_t range) {
    // Ensure OC_module is within a valid range (1-5)
    //guard conditions
    if (OC_module < 1 || OC_module > 5)
        return false;
    
    if (timer!=2 && timer!= 3)
        return false;
    
    static float DC = 0;
    volatile __T2CONbits_t *pTx = Timers[timer - 2];
    volatile uint32_t *pOCRSx = OCxRS[OC_module - 1];
    volatile uint32_t *pPrx = Pr[timer - 1];

    // Timer 2 or 3 must be used
    if (timer != 2 && timer != 3)
        return false;
    DC = (((float)angle / (float)range)*(4800.0/50000.0) + 1200.0/50000.0)*100.0; //empirically found
//    DB_printf("DC = %d", (uint8_t)DC);
    
    //0 deg = 1000
    //180 deg = 6000
    //5000
    
    *pOCRSx = 1100 + (6000 - 1100) / range * angle;//800;//(25000 - 3500) / 180 * 90;//3500;//6440; //(uint16_t) (DC * (1 + *pPrx) / 100);
    return true;
}


//will need to manually do the clearing, setting and mapping of bits to output compare in other module
static bool setOC_PWM(uint8_t OC_module, uint8_t timer)
{
    // Ensure OC_module is within a valid range (1-5)
    if (OC_module < 1 || OC_module > 5)
        return false;

    // Timer 2 or 3 must be used
    if (timer != 2 && timer != 3)
        return false;

    // Get struct pointers for Output Compare and Timer modules
    volatile __OC1CONbits_t *pOCx = OCxCON[OC_module - 1].OC;    
    volatile uint32_t *pOCRx = OCxR[OC_module - 1];
    volatile uint32_t *pOCRSx = OCxRS[OC_module - 1];
//    volatile __T2CONbits_t *pTx = Timers[timer - 2]; //timer 1 is a different type
//    volatile uint32_t *pPrx = Pr[timer - 1];

    // Output Compare Enable
    pOCx->ON = 0;
    pOCx->SIDL = 0;
    pOCx->OC32 = 0;
    pOCx->OCTSEL = timer - 2; //only timer 2 or 3 can be used
    pOCx->OCM = 6;
    pOCRx = 0; //setting initial duty cylce to 0
    pOCRSx = 0; //setting initial duty cylce to 0
    pOCx->ON = 1; 
    

    return true;
}


//the period already subtracts 1 from the value. 
//You must set the period value to be the proper value based on prescaler
static bool setTimer(uint8_t timer, uint8_t prescale_idx, uint16_t PRval)
{
    if (timer != 2 && timer != 3)
    {
        return false;
    }
        
    volatile __T2CONbits_t *pTx = Timers[timer - 2];
    volatile uint32_t *pPrx = Pr[timer - 1];
    pTx->ON = 0;    // turn timer off to change stuff
    pTx->TCS = 0;   // selecting internal clock
    pTx->TGATE = 0; // disabling the gate
    pTx->TCKPS = prescale_idx; // prescale 
    pTx->T32 = 0;

    // try 1600 since the minimum value is 1421 Hz
    *pPrx = PRval - 1;           // prescaling by 2 and then subtracting 1 for rollover
    IFS0CLR = TimerFlags[timer-1]; // clearing the interrupt flag
    pTx->ON = 1; 
    return true;
}
