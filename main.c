 #include  <msp430x11x1.h>
 #include "includes.h"

#define FOSC	770000
#define FREQ	7000

unsigned char BitCnt;
unsigned int RXTXData;

unsigned int time_to_compare;
unsigned int hold_time;

//   Conditions for 9600 Baud SW UART, DCO ~ 2MHz

//#define Bitime_5  104                       // ~ 0.5 bit length
//#define Bitime    208                       // ~ 9615 baud

//   Conditions for 9600 Baud SW UART, DCO ~ 770 kHz
#define Bitime_5  39                       // ~ 0.5 bit length
#define Bitime    78                       // ~ 9615 baud



void init_timer_a(int freq){

int divider=FOSC/freq;
    //очищаем таймер и ставим источником SMCLK
    TACTL=	TASSEL1+//ставим источником SMCLK
		TACLR;	//очищаем таймер

    TACCR0=divider;	//период генератора

    CCTL1= OUTMOD_2 ;	// togle/reset
    TACCR1=(divider)>>2;	//фронт
    CCTL2= OUTMOD_6 ;	// togle/set
    TACCR2=divider-TACCR1;	//спад


    // Start Timer_a in Continous upmode
    TACTL|=MC_3    /* Timer A mode control: 3 - UP/DOWN mode */;
}

#define RXD   0x04                          // RXD on P2.2
#define TXD   0x02                          // TXD on P1.1

void init_soft_uart(void){
  _BIS_SR(OSCOFF);                          // XTAL not used
  CCTL0 = OUT;                              // TXD Idle as Mark
  TACTL = TASSEL_2 + MC_2;                  // SMCLK, continuous mode
  P1SEL|= TXD;                              // P1.1/TA0 for TXD function
  P1DIR|= TXD;                              // TXD output on P1
  P2SEL|= RXD;                              // P2.2/TA0 as RXD input

}
// Function Transmits Character from RXTXData Buffer
void TX_Byte (void)
{
  BitCnt = 0xA;                             // Load Bit counter, 8data + ST/SP
  CCR0 = TAR;                               // Current state of TA counter
  CCR0 += Bitime;                           // Some time till first bit
  RXTXData |= 0x100;                        // Add mark stop bit to RXTXData
  RXTXData = RXTXData << 1;                 // Add space start bit
//  CCTL0 = OUTMOD0+OUTMOD2 + CCIE;                   // TXD = mark = idle
  CCTL0 = OUTMOD0 + CCIE;                   // TXD = mark = idle
  while ( CCTL0 & CCIE );                   // Wait for TX completion
}

void main(void)
{ 
  WDTCTL = WDTPW + WDTHOLD;             // Stop watchdog timer
//  P2DIR |= 0x03;                        // Set P2.0, P2.1 to output direction
  P1DIR |= BIT2|BIT3;
  P1OUT &= ~BIT2;
  P1OUT |= BIT3;
  P1SEL |= BIT2|BIT3;
  init_params();         
  init_soft_uart();
  on_charge();
  _EINT();                              // Enable interrupts

  for (;;)                              
  {
     volatile unsigned int i;
     volatile unsigned int y;

    i = 200;                          // Delay
    do{ i--;
    if (i==101){
     on_charge();
//     RXTXData=time_to_compare-hold_time;
     RXTXData=0x50;
     TX_Byte();                                // TX Back RXed Byte Received
     }

    if ((i<100)&&((i&0x3)==0)) 
      P1OUT^=(BIT2|BIT3);
    } while (i != 0);
  on_comparator_external();
  hold_time=TAR;
  time_to_compare=hold_time;
  off_charge();

  }
}

// COMPARATORA_VECTOR interrupt service routine
#pragma vector=COMPARATORA_VECTOR
__interrupt void comparator (void)
{
 time_to_compare=TAR;
 on_charge();
}

// Timer A0 interrupt service routine
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A0 (void)
{
  CCR0 += Bitime;                           // Add Offset to CCR0

// RX
  if (CCTL0 & CCIS0)                        // RX on CCI0B?
  {
    if( CCTL0 & CAP )                       // Capture mode = start bit edge
    {
    CCTL0 &= ~ CAP;                         // Switch from capture to compare mode
    CCR0 += Bitime_5;
    _BIC_SR_IRQ(SCG1 + SCG0);               // DCO reamins on after reti
    }
    else
    {
    RXTXData = RXTXData >> 1;
      if (CCTL0 & SCCI)                     // Get bit waiting in receive latch
      RXTXData |= 0x80;
      BitCnt --;                            // All bits RXed?
      if ( BitCnt == 0)
//>>>>>>>>>> Decode of Received Byte Here <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
      {
      CCTL0 &= ~ CCIE;                      // All bits RXed, disable interrupt
      _BIC_SR_IRQ(CPUOFF);                  // Active mode on reti 0(SR)
      }
//>>>>>>>>>> Decode of Received Byte Here <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    }
  }
// TX
  else
  {
    if ( BitCnt == 0)
    CCTL0 &= ~ CCIE;                        // All bits TXed, disable interrupt
    else
    {
      if ((RXTXData & 0x01)==0)
//      if ((RXTXData & 0x01)==1)
       CCTL0 |=  OUTMOD2;                    // TX Space
      else
       CCTL0 &= ~ OUTMOD2;                   // TX Mark
 
      RXTXData = RXTXData >> 1;
      BitCnt --;
    }
  }
}
