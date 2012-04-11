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
#define TIME_HALF_BIT 5*4 // almost a half bit
#define TIME_BIT      13*4

int isReceiving;
int hasReceived;
unsigned int RXByte;
//unsigned int TXByte;
unsigned char BitCnt;
void ConfigureTimerUart(void);

void Transmit(void);
static void __inline__ delay(register unsigned int n);

inline void ConfigureTimer(void)
{
    // stop WDT
    WDTCTL = WDTPW + WDTHOLD;

    /* Set range */
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;

    /* SMCLK = DCO / 8 = 1MHz */
    BCSCTL2 &= ~(DIVS_3);

    /* Configure TimerA0 UART TX */
//    timerMode = TIMER_UART_MODE;

    // set TimerA to use sub-main clock
    // (SMCLK) as its clock source.
    // Set TimerA to count in continuous mode
    
    // don't start timer yet
    //TACTL = TASSEL_2 | MC_2;

    CCTL0 = CCIE;
}

int main(void) {

    ConfigureTimer();

    // setup lights
    P1DIR |= LED1 | LED2 | BIT7;
    P1OUT &= ~(LED1 | LED2 | BIT7);

    // setup output pins
    //P1SEL |= TXD + RXD;
    //P1DIR |= TXD;

    // set up pins
    //P1SEL |= TXD;
    P1OUT |= TXD; // set tx to high (idle)
    P1DIR |= TXD;

    // interrupt on recv Hi->Lo edge by setting RXD's bit to 1
    // (0 is rising edge)
    P1DIR &= ~RXD;
    P1IES |= RXD;
    // clear RXD (flag) before enabling interrupt
    P1IFG &= ~RXD;
    P1IE |= RXD;

    P1IES |= BIT3;
    P1IFG &= ~BIT3;
    P1IE |= BIT3;

    // initial states
    isReceiving = false;
    hasReceived = false;

    // interrupts enabled
    __enable_interrupt();
    __bis_SR_register(GIE);

//    TXByte = 'R';
//    Transmit();

    //ConfigureTimerUart();

    while(1) {
        if(hasReceived) {
            P1OUT ^= LED2;
            hasReceived = false;
            if(RXByte == 'y') {
                P1OUT |= BIT7;
            }
           // TXByte = RXByte-1;
           // Transmit();
        }
        /*
        if(~hasReceived) {
            //__bis_SR_register(CPUOFF+GIE);
            __enable_interrupt();
        }
        */
    }
}
void ConfigureTimerUart(void)
{
    /* TXD Idle as Mark, SMCLK/8, continuous mode */
    CCTL0 = OUT;
    TACTL = TASSEL_2 + MC_2 + ID_3;// + ID_3;
}

// below code copy-pasted
interrupt(PORT1_VECTOR) Port_1(void) {
    isReceiving = true;
    P1OUT |= LED1;          // turn on RX light

    P1IE &= ~RXD;             // Disable RXD interrupt
    P1IFG &= ~RXD;            // Clear RXD IFG (interrupt flag)
    
    //TACTL |= TACLR; // reset the timer
    TACTL |= TASSEL_2 | MC_2;

    RXByte = 0;               // Initialize RXByte
    BitCnt = 0x9;             // Load Bit counter, 8 bits + ST
}

// Timer A0 interrupt service routine
interrupt(TIMERA0_VECTOR) Timer_A(void)
{
    if(isReceiving)
    {
        P1OUT ^= BIT7;
        CCR0 += TIME_BIT;                // Add Offset to CCR0  
        if ( BitCnt == 0)
        {
              TACTL |= TACLR;            // SMCLK, timer off (for power consumption)
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
                P1OUT |= BIT7;
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

/* Delay Routine from mspgcc help file */
static void __inline__ delay(register unsigned int n)
{
    __asm__ __volatile__(
        "1: \n"
        " dec %[n] \n"
        " jne 1b \n"
        : [n] "+r"(n));
}

