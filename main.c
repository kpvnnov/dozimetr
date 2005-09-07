 #include  <msp430x11x1.h>

void main(void)
{ 
  WDTCTL = WDTPW + WDTHOLD;             // Stop watchdog timer
  P2DIR |= 0x03;                        // Set P2.0, P2.1 to output direction

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

    i = 10000;                          // Delay
    do (i--);
    while (i != 0);
  }
}
