 #include  <msp430x11x1.h>              
 #include "includes.h"

#define FOSC	770000
#define FREQ	7000

unsigned char BitCnt;
unsigned int RXTXData;

unsigned int time_to_compare;
unsigned int hold_time;

//   Conditions for 9600 Baud SW UART,

//#define Bitime_5  104                       // ~ 0.5 bit length
//#define Bitime    208                       // ~ 9615 baud

//#define Bitime_5  56                       // ~ 0.5 bit length
//#define Bitime    112                       // ~ 9615 baud

//   Conditions for 9600 Baud SW UART, 2 Mhz
#define Bitime_5  104                       // ~ 0.5 bit length
#define Bitime    208                       // ~ 9615 baud




#define BUTTON_BIT BIT0

#define  ESCAPE         0x7D
#define  EOFPACKET      0x7E    //��� �������� ����� �����

//����� ����� �����������, �� ������� ����� RC-�������
#define PORT_DIR_CAPACITOR P2DIR
//����� ����� ������, �� ������� ����� RC-�������
#define PORT_OUT_CAPACITOR P2OUT
//���������� ����� ���� �����, �� ������� ����� RC-�������
#define CAPACITOR_PIN BIT4
#define DIODE_PIN BIT3

#define PORT_DIR_REZISTOR P2DIR
#define PORT_OUT_REZISTOR P2OUT
#define REZISTOR_PIN BIT5




int divider;
volatile unsigned int c_beep;
volatile unsigned char button_pressed;
volatile unsigned char button_release;


/****************************************/
//��������� ������ �������� ������������
/****************************************/

#pragma inline=forced
inline void on_charge(void){
// CAPD&=~CAPACITOR_PIN; 			// ���������� ������� �����
// P2SEL&=~CAPACITOR_PIN;                 // ���������� ������� �����
// PORT_DIR_CAPACITOR|=CAPACITOR_PIN;      // ����������� �� �����
// PORT_OUT_CAPACITOR|=CAPACITOR_PIN;	// ������� �������

//��������� (�� ������ ������) ����� p2.4 (�����������)
 CAPD|=CAPACITOR_PIN; 			//��������� ������� �����
 PORT_DIR_CAPACITOR&=~CAPACITOR_PIN;      // ����������� �� ����
 PORT_OUT_CAPACITOR|=CAPACITOR_PIN;	// ������� �������


 PORT_DIR_REZISTOR|=REZISTOR_PIN;	//�������� ��� ��������� �� �����
 PORT_OUT_REZISTOR|=REZISTOR_PIN;	// ������� �������

}


void init_timer_a(int freq){

    P1SEL |= BIT2|BIT3;
    divider=(FOSC/freq)>>1;
    c_beep=freq<<1;

    CCTL1= OUTMOD_1 ;	//   set
    CCTL2= OUTMOD_5 ;	// reset
    TACCR1=(divider)+TAR;	
    TACCR2=TACCR1;	
    CCTL1|=CCIE    /* ��������� ���������� */;
}
// Timer_A3 Interrupt Vector (TAIV) handler
    #if __VER__ < 200
   interrupt[TIMERA1_VECTOR] void Timer_A (void)
    #else
     #pragma vector=TIMERA1_VECTOR
__interrupt void Timer_A(void)
     #endif
{
  switch( TAIV )
  {
  case  2:                                  // CCR1
    {
    CCTL1 ^= OUTMOD2;                          // Toggle timer pin set
    CCTL2 ^= OUTMOD2;                          // Toggle timer
    TACCR1+=divider;	
    TACCR2+=divider;	
    c_beep--;
    if (c_beep==0){
     CCTL1&=~CCIE    /* ��������� ���������� */;
     P1SEL&= ~(BIT2|BIT3);
     }
    }
           break;
  case  4:
    break;                           // CCR2 not used
  case 10:

   if ((P1IN&BUTTON_BIT)==0){
    if (button_pressed<10)
     button_pressed++;
    button_release=0;
    }
   else {
    if (button_release<10)
     button_release++;
    }
   if (button_release>2) init_key();
   break;                           // overflow not used
 }
}



#define RXD   0x04                          // RXD on P2.2
#define TXD   0x02                          // TXD on P1.1

void init_soft_uart(void){
  _BIS_SR(OSCOFF);                          // XTAL not used
  CCTL0 = OUT;                              // TXD Idle as Mark
  TACTL = TASSEL_2 + MC_2+TAIE;                  // SMCLK, continuous mode
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

void init_key(void){
  P1IES |= 0x01;                        // P2.0 Hi/lo edge
  P1IFG &= ~BUTTON_BIT;                       // P2.0 IFG cleared
  P1IE |= BUTTON_BIT;                         // P2.0 interrupt enabled
}
// Port 1 interrupt service routine

    #if __VER__ < 200
   interrupt[PORT1_VECTOR] void Port_1 (void)
    #else
      #pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
     #endif
{
  P1IE &= ~BUTTON_BIT;                         // P2.0 interrupt disabled
  init_timer_a(3000);
  button_pressed=1;
}

void transmit(unsigned char transfer){
     if (transfer==EOFPACKET||transfer==ESCAPE){
      RXTXData=ESCAPE;
      TX_Byte();                                // TX Back RXed Byte Received
      RXTXData=transfer^0x40;
      TX_Byte();                                // TX Back RXed Byte Received
      }
     else{
      RXTXData=transfer;
      TX_Byte();                                // TX Back RXed Byte Received
      }
}

void main(void)
{
  volatile unsigned int y=0;
  unsigned char transfer;
  WDTCTL = WDTPW + WDTHOLD;             // Stop watchdog timer
  P2DIR |= 0x03;                        // Set P2.0, P2.1 to output direction
  P1DIR |= BIT2|BIT3;
  P1OUT &= ~(BIT2|BIT3);
  button_pressed=0;
  button_release=0;
  init_params();



  init_soft_uart();
  init_key();
  on_charge();
  _EINT();                              // Enable interrupts

  for (;;)
  {
     volatile unsigned int i;
    y++;
    if (y>7) y=0;
    i = 6000;                          // Delay
    do{ i--;
     if (i==5000)
      switch(y){
       case 3: P2OUT=(P2OUT&(0xFC))|1; break;
       case 5: P2OUT=(P2OUT&(0xFC))|2; break;
       }
     if (i==3000 )
      P2OUT&=0xFC;

     if (i==101){
      on_charge();
      transfer=(time_to_compare-hold_time);
      transmit(transfer);
      transmit(~transfer);
      RXTXData=EOFPACKET;
//      RXTXData=0xF7;
      TX_Byte();                                // TX Back RXed Byte Received
      }
     if (i==50){
      fast_charge();
      }
     if (i==5){
      on_charge();
      }
//     if (i==1) init_timer_a(3000);
    } while (i != 0);
  on_comparator_external();
  hold_time=TAR;
  time_to_compare=hold_time;
  off_charge();

  }
}


void  fast_charge(void){
 CAPD&=~CAPACITOR_PIN; 			// ���������� ������� �����
 P2SEL&=~CAPACITOR_PIN;                 // ���������� ������� �����
 PORT_OUT_CAPACITOR|=CAPACITOR_PIN;	// ������� �������
 PORT_DIR_CAPACITOR|=CAPACITOR_PIN;      // ����������� �� �����
}

// COMPARATORA_VECTOR interrupt service routine
    #if __VER__ < 200
   interrupt[COMPARATORA_VECTOR] void comparator (void)
    #else
#pragma vector=COMPARATORA_VECTOR
__interrupt void comparator (void)
     #endif
{
 time_to_compare=TAR;
 on_charge();
}

// Timer A0 interrupt service routine
    #if __VER__ < 200
   interrupt[TIMERA0_VECTOR] void Timer_A0 (void)
    #else
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A0 (void)
     #endif
{
  CCR0 += Bitime;                           // Add Offset to CCR0

  if (!(CCTL0 & CCIS0))                        // RX on CCI0B?
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




/****************************************/
//������������� ����������
/****************************************/
void init_params(void){
 CACTL1=0;	//
 CAPD=DIODE_PIN;	//��������� ���� �� ������� ������� ����������
 BCSCTL1=(BCSCTL1&~(RSEL0|RSEL1|RSEL2))|RSEL1|RSEL2;
}




/****************************************/
//��������� ����������� � �������� �� 0.25Vcc
/****************************************/
void on_comparator(void){
  //� ����� ����������+ ���������
  //����������- �� �������� ����� ��������
 CACTL2=(CACTL2&~(P2CA0|P2CA1|CAF))|P2CA0|P2CA1|CAF;	
  //!!! ����� ����� ��������� (�������) �����������?
  //������� 0.25Vcc � ���������� �� ����������-
 CACTL1=(CACTL1&~(CAREF0|CAREF1))|CAON|CAIE;	//�������� ����������
}

/****************************************/
//��������� ����������� � �������� ������
/****************************************/
void on_comparator_external(void){
  //� ����� ����������+ ���������
  //����������- ��������� � �������� �����
 CACTL2=(CACTL2&~(P2CA0|P2CA1|CAF))|P2CA1|P2CA0|CAF;	
  //!!! ����� ����� ��������� (�������) �����������?
  //�������� ����������
  //�������� ���������� �������
 CACTL1=(CACTL1&~(CAREF0|CAREF1))|CAON|CAIE;	//�������� ����������
}


/****************************************/
//���������� ������ �������� ������������
/****************************************/
void off_charge(void){
 PORT_DIR_REZISTOR|=REZISTOR_PIN;	//�������� ��� ��������� �� �����
 P2SEL|=CAPACITOR_PIN;                 // ��������� ������� �����
 PORT_DIR_CAPACITOR&=~CAPACITOR_PIN;      // ����������� �� ����
 CAPD|=CAPACITOR_PIN; 			// ��������� ������� �����
 PORT_OUT_REZISTOR&=~REZISTOR_PIN;	// ������ �������

}
