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
  P2DIR |= 0x03;                        // Set P2.0, P2.1 to output direction
  P1DIR |= BIT2|BIT3;
  P1OUT &= ~BIT2;
  P1OUT |= BIT3;
  P1SEL |= BIT2|BIT3;
  init_params();
  init_timer_a(7000);
  volatile unsigned int y;

  for (;;)                              
  {
     volatile unsigned int i;
    y++;
    if (y>20) y=0;
    init_timer_a(300*y+3000);
    
    switch(y&0x3){
     case 0:
      P2OUT=(P2OUT&0xFC)|0;
      break;
     case 1:
      P2OUT=(P2OUT&0xFC)|1;
      break;
     case 2:
      P2OUT=(P2OUT&0xFC)|2;
      break;
     case 3:
      P2OUT=(P2OUT&0xFC)|3;
      break;
     }

    i = 25000;                          // Delay
    do{ i--;
    if ((i<1000)&&((i&0x3)==0)) 
      P1OUT^=(BIT2|BIT3);
    } while (i != 0);
  }
}
