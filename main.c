#include <msp430.h>
#include <legacymsp430.h>

#define TXD        BIT1
#define RXD        BIT2
#define     LED1                  BIT0
#define     LED2                  BIT6

// conditions for 2400 Baud SW UART can be calculated
// by taking DCO_SPEED / BAUD_RATE / clock divide
// (I think)
//#define DCO_SPEED 1100000 // 1.1 MHz CPU
//#define BAUD_RATE 2400

#define false 0
#define true 1

// conditions for 9600/4=2400 Baud SW UART, SMCLK=1MHz
#define TIME_HALF_BIT 26 // almost a half bit
#define TIME_BIT      52

int isReceiving;
int hasReceived;
unsigned int RXByte;
unsigned int TXByte;
unsigned char BitCnt;

void Transmit(void);
static void __inline__ delay(register unsigned int n);

inline void ConfigureTimer(void)
{
    // stop WDT
    WDTCTL = WDTPW + WDTHOLD;

    // set range
    BCSCTL1 = CALBC1_1MHZ;
    // SMCLK = DCO = 1MHz
    DCOCTL = CALDCO_1MHZ;

    /* Configure TimerA0 UART TX */
//    timerMode = TIMER_UART_MODE;

    // set TimerA to use sub-main clock
    // (SMCLK) as its clock source.
    // Set TimerA to count in continuous mode
    TACTL = TASSEL_2 | MC_2;

    CCTL0 = CCIE;
}

int main(void) {

    ConfigureTimer();

    // setup lights
    P1DIR |= LED1 | LED2;
    P1OUT &= ~(LED1 | LED2);
    P1OUT ^= LED2;

    // setup output pins
    P1SEL |= TXD + RXD;
    P1DIR |= TXD;

    // set up pins
    P1SEL |= TXD;
    P1DIR |= TXD;

    // interrupt on recv Hi->Lo edge
    P1IES |= RXD;
    // clear RXD (flag) before enabling interrupt
    P1IFG &= ~RXD;
    P1IE |= RXD;

    // initial states
    isReceiving = false;
    hasReceived = false;

    // interrupts enabled
    __bis_SR_register(GIE);

    TXByte = 'R';
    Transmit();

    while(1) {
        if(hasReceived) {
            P2OUT ^= LED2;
            hasReceived = false;
            TXByte = RXByte-1;
            Transmit();
        }
        /*
        if(~hasReceived) {
            //__bis_SR_register(CPUOFF+GIE);
            __enable_interrupt();
        }
        */
    }
}

// below code copy-pasted
interrupt(PORT1_VECTOR) Port_1(void) {
    isReceiving = true;
    P1OUT |= LED1;          // turn on RX light

    P1IE &= ~RXD;             // Disable RXD interrupt
    P1IFG &= ~RXD;            // Clear RXD IFG (interrupt flag)

    TACTL = TASSEL_1 + MC_1;  // SMCLK, up mode
    CCR1 = TAR;               // Initialize compare register
    CCR0 += TIME_HALF_BIT;    // Set time till first bit
    CCTL0 = OUTMOD1 + CCIE;   // Dissable TX and enable interrupts

    RXByte = 0;               // Initialize RXByte
    BitCnt = 0x9;             // Load Bit counter, 8 bits + ST
}

// Timer A0 interrupt service routine
interrupt(TIMERA0_VECTOR) Timer_A(void)
{
    if(!isReceiving)
    {
        CCR0 += TIME_BIT;            // Add Offset to CCR0  
        if ( BitCnt == 0)            // If all bits TXed
        {
              TACTL = TASSEL_2;        // SMCLK, timer off (for power consumption)
            CCTL0 &= ~ CCIE ;        // Disable interrupt
        }
        else
        {
            CCTL0 |=  OUTMOD2;        // Set TX bit to 0
            if (TXByte & 0x01)
                CCTL0 &= ~ OUTMOD2;    // If it should be 1, set it to 1
            TXByte = TXByte >> 1;
            BitCnt --;
        }
    }
    else
    {
        CCR0 += TIME_BIT;                // Add Offset to CCR0  
        if ( BitCnt == 0)
        {
              TACTL = TASSEL_2;            // SMCLK, timer off (for power consumption)
            CCTL0 &= ~ CCIE ;            // Disable interrupt
            
            isReceiving = false;
            P1OUT &= ~LED1;         // turn off RX light
            
            P1IFG &= ~RXD;                // clear RXD IFG (interrupt flag)
            P1IE |= RXD;                // enabled RXD interrupt
            
            if ( (RXByte & 0x201) == 0x200)        // Validate the start and stop bits are correct
            {
                RXByte = RXByte >> 1;        // Remove start bit
                RXByte &= 0xFF;            // Remove stop bit
                hasReceived = true;
                P2OUT ^= LED2;
            }
              __bic_SR_register_on_exit(CPUOFF);    // Enable CPU so the main while loop continues
        }
        else
        {
            if ( (P1IN & RXD) == RXD)        // If bit is set?
                RXByte |= 0x400;        // Set the value in the RXByte 
            RXByte = RXByte >> 1;            // Shift the bits down
            BitCnt --;
        }
    }
}

/* transmits character from TXByte */
void Transmit()
{
    /* Prevent async capture */

#if 0
    while (CCR0 != TAR)
        /* current state of TA counter */
        CCR0 = TAR;
#endif

    /* Re-implement timer capture in assembly */
    asm(
        "JMP 2f\n"
        "1:\n"
        "MOV &0x0170, &0x0172\n"
        "2:\n"
        "CMP &0x0170, &0x0172\n"
        "JNZ 1b\n");

    /* Some time till first bit */
    CCR0 += TIME_BIT;

    /* Load Bit counter, 8data + ST/SP */
    BitCnt = 0xA;

    /* Add mark stop bit to TXByte */
    TXByte |= 0x100;

    /* Add space start bit */
    TXByte = TXByte << 1;

    /* TXD = mark = idle */
    CCTL0 =  CCIS0 + OUTMOD0 + CCIE;

    /* Wait for TX completion, added short delay */
    while (CCTL0 & CCIE)
        delay(5);
}

/* Delay Routine from mspgcc help file */
static void __inline__ delay(register unsigned int n)
{
    __asm__ __volatile__(
        "1: \n"
        " dec %[n] \n"
        " jne 1b \n"
        : [n] "+r"(n));
}

