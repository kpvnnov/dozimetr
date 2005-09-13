 #include  <msp430x11x1.h>
 #include "includes.h"

#define FOSC	770000
#define FREQ	7000

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

void main(void)
{ 
  WDTCTL = WDTPW + WDTHOLD;             // Stop watchdog timer
//  P2DIR |= 0x03;                        // Set P2.0, P2.1 to output direction
  P1DIR |= BIT2|BIT3;
  P1OUT &= ~BIT2;
  P1OUT |= BIT3;
  P1SEL |= BIT2|BIT3;
  init_params();         
  on_charge();
  _EINT();                              // Enable interrupts

  for (;;)                              
  {
     volatile unsigned int i;

    i = 800;                          // Delay
    do{ i--;
    if (i==101) on_charge();

    if ((i<100)&&((i&0x3)==0)) 
      P1OUT^=(BIT2|BIT3);
    } while (i != 0);
  on_comparator_external();
  off_charge();
  }
}

// COMPARATORA_VECTOR interrupt service routine
#pragma vector=COMPARATORA_VECTOR
__interrupt void comparator (void)
{
 on_charge();
}

