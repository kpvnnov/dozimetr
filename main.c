 #include  <msp430x11x1.h>
 #include "includes.h"
void main(void)
{ 
  WDTCTL = WDTPW + WDTHOLD;             // Stop watchdog timer
  P2DIR |= 0x03;                        // Set P2.0, P2.1 to output direction
  P1DIR |= BIT2|BIT3;
  P1OUT &= ~BIT2;
  P1OUT |= BIT3;
  init_params();
  volatile unsigned int y;

  for (;;)                              
  {
     volatile unsigned int i;
    y++;
    if (y>3) y=0;
    switch(y){
     case 0:
      P2OUT=(P2OUT&0xFC);
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

    i = 15000;                          // Delay
    do{ i--;
    if ((i<1000)&&((i&0x3)==0)) 
      P1OUT^=(BIT2|BIT3);
    } while (i != 0);
  }
}
