 #include  <msp430x11x1.h>
 #include "includes.h"

#define FOSC	2000000
#define FREQ	7000
#define TICKS	30	//количество прерываний таймера по переполнению/сек
#define FREQ_BEEP 2500	//частота звука
#define DARK_0  1	//время, в тиках, горения светодиода
#define FLASH_0 (TICKS*2)	//период мигания режима 0
#define DARK_1  1	//время, в тиках, горения светодиода
#define FLASH_1 (TICKS*2)	//период мигания режима 0
#define DARK_2  1	//время, в тиках, горения светодиода
#define FLASH_2 (TICKS*2)	//период мигания режима 0
#define NUMBER_OF_MODES	3
#define GO_TO_SLEEP (3*TICKS)
#define CONVERSION_SPEED 8
#define WAIT_CONV_TIME  (1+1)	//всегда должно быть +1

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
#define Bitime_5  101                       // ~ 0.5 bit length
#define Bitime    202                       // ~ 9615 baud




#define BUTTON_BIT BIT0

#define  ESCAPE         0x7D
#define  EOFPACKET      0x7E    //код признака конца кадра

//номер порта направления, на котором стоит RC-цепочка
#define PORT_DIR_CAPACITOR P2DIR
//номер порта вывода, на котором стоит RC-цепочка
#define PORT_OUT_CAPACITOR P2OUT
//порядковый номер пина порта, на котором стоит RC-цепочка
#define CAPACITOR_PIN BIT4
#define DIODE_PIN BIT3

#define PORT_DIR_REZISTOR P2DIR
#define PORT_OUT_REZISTOR P2OUT
#define REZISTOR_PIN BIT1




int divider;
volatile unsigned int c_beep;
volatile unsigned char button_pressed;
volatile unsigned char button_release;
volatile unsigned char go_sleep;
volatile unsigned char sleep_beeped;
volatile unsigned char mode_collect;
volatile unsigned char post_charge;
volatile unsigned int  time_pressed;
volatile unsigned int  counter_flash;
volatile unsigned int  counter_dark;
volatile unsigned int  value_convert;
volatile unsigned int  counter2begin;
volatile unsigned char wait_end_convert;
volatile unsigned char value_placed;
volatile unsigned char in_sleep_mode;
volatile unsigned char knopka_otpushena;
volatile unsigned char pusk_komparator;
//1 - готовимся ко сну


/****************************************/
//включение заряда внешнего конденсатора
/****************************************/

#pragma inline=forced
inline void on_charge(void){
// CAPD&=~CAPACITOR_PIN; 			// подключаем входной буфер
// P2SEL&=~CAPACITOR_PIN;                 // подключаем входной буфер
// PORT_DIR_CAPACITOR|=CAPACITOR_PIN;      // направление на выход
// PORT_OUT_CAPACITOR|=CAPACITOR_PIN;	// высокий уровень

//отключаем (на всякий случай) выход p2.4 (конденсатор)
 CAPD|=CAPACITOR_PIN; 			//отключаем входной буфер
 PORT_DIR_CAPACITOR&=~CAPACITOR_PIN;      // направление на вход
 PORT_OUT_CAPACITOR|=CAPACITOR_PIN;	// высокий уровень


 PORT_DIR_REZISTOR|=REZISTOR_PIN;	//включаем пин резистора на выход
 PORT_OUT_REZISTOR|=REZISTOR_PIN;	// высокий уровень

}


void init_timer_a(int freq,int delay){

    P1SEL |= BIT2|BIT3;
    divider=(FOSC/freq)>>1;
    c_beep=(freq>>2)*delay;

    CCTL1= OUTMOD_1 ;	//   set
    CCTL2= OUTMOD_5 ;	// reset
    TACCR1=(divider)+TAR;	
    TACCR2=TACCR1;	
    CCTL1|=CCIE    /* разрешаем прерывания */;
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
     CCTL1&=~CCIE    /* запрещаем прерывания */;
     P1SEL&= ~(BIT2|BIT3);
     }
    }
           break;
  case  4:
    break;                           // CCR2 not used
  case 10:
    _BIC_SR_IRQ(CPUOFF);                // Clear CPUOFF bit from 0(SR)
    switch(post_charge){
     case 0:
      break;
     case 1:
      fast_charge();
      post_charge++;
      break;
     case 2:
      on_charge();
      post_charge=0;
      break;
     default:
      post_charge=0;
     }

   counter_flash++;
   if (counter_dark){
    if (--counter_dark==0){
     P2OUT|=0x1;
     P1OUT|=0x80;
     }
   }
   if (in_sleep_mode==0){
    counter2begin++;//ждём начала преобразования
    if (counter2begin>CONVERSION_SPEED){
     counter2begin=0;
     wait_end_convert=WAIT_CONV_TIME;
     on_comparator_external();
     pusk_komparator=1; //при выходе из прерывания запустим всё
    }
    if (wait_end_convert){
     if (--wait_end_convert==0){ //преобразование закончилось
      on_charge();
      post_charge=1;
      value_convert=(time_to_compare-hold_time)>>2;
//      if (value_convert>90) value_convert-=90; else value_convert=0;
      value_convert=value_convert>255 ? 255:value_convert;
      value_placed=1;
     }
    }
    switch(mode_collect){
     case 0:
      if (counter_flash>FLASH_0){
        P2OUT=(P2OUT&(0xFE));
        counter_flash=0;
        counter_dark=DARK_0;
      }
      break;
     case 1:
      if (counter_flash>FLASH_1){
        P2OUT=(P2OUT&(0xFD));
        counter_flash=0;
        counter_dark=DARK_1;
      }
      break;
     case 2:
      if (counter_flash>FLASH_2){
        P1OUT=(P1OUT&(0x7F));
        counter_flash=0;
        counter_dark=DARK_2;
      }
      break;
     default:
      init_timer_a(2000,18);
     }
   }
   if ((P1IN&BUTTON_BIT)==0){
    if (button_pressed<10)
     button_pressed++;
     time_pressed++;		//увеличиваем время нажатия кнопки
     button_release=0;
    }
   else {
    if (button_release<10)
     button_release++;
    }
   if ((sleep_beeped==0) && (time_pressed>(GO_TO_SLEEP))){
     init_timer_a(3000,8);
     if (in_sleep_mode==2){ //мы в спящем режиме
      in_sleep_mode=0;
      sleep_beeped=1;
     }
     else{
      sleep_beeped=1;
      in_sleep_mode=1;
     }
   }

   if (button_release>2){    //кнопку отпустили
    time_pressed=0;
    sleep_beeped=0;
    if (knopka_otpushena==0){
     knopka_otpushena=1;
     if (in_sleep_mode==0) mode_collect++;
     if (mode_collect>=NUMBER_OF_MODES){
      mode_collect=0;
     }
     if (in_sleep_mode) in_sleep_mode=2;
     init_key();
    }
   }
   break;                           // overflow not used
 }
if (pusk_komparator){
 pusk_komparator=0;
 CACTL1&=~CAIFG;
 CACTL1|=CAIE;	//разрешаем прерывание от компаратора
/****************************************/
//выключение заряда внешнего конденсатора
/****************************************/
// off_charge();
//void off_charge(void){
 PORT_DIR_REZISTOR|=REZISTOR_PIN;	//включаем пин резистора на выход
 P2SEL|=CAPACITOR_PIN;                 // отключаем входной буфер
 PORT_DIR_CAPACITOR&=~CAPACITOR_PIN;      // направление на вход
 CAPD|=CAPACITOR_PIN; 			// отключаем входной буфер
 PORT_OUT_REZISTOR&=~REZISTOR_PIN;	// низкий уровень

//}
 hold_time=TAR;
 time_to_compare=hold_time;
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
//  P2SEL|= RXD;                              // P2.2/TA0 as RXD input

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
  if (in_sleep_mode==0) init_timer_a(3000,8);
  button_pressed=1;
  time_pressed=0; //время нажатия кнопки
  knopka_otpushena=0;
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
  P1DIR |= BIT2|BIT3|BIT7;
  P2DIR|=BIT2;
  P2SEL|=BIT2;
  P2OUT|=0x3;
  P1OUT|=0x80;
  P1OUT &= ~(BIT2|BIT3);
  P1SEL |=BIT4;
  P1DIR |=BIT4;
  button_pressed=0;
  button_release=0;
  sleep_beeped=0;
  time_pressed=0;
  mode_collect=0;
  counter2begin=0;
  post_charge=0;
  go_sleep=0;
  knopka_otpushena=0;
  init_params();



  init_soft_uart();
  init_key();
  on_charge();
  _EINT();                              // Enable interrupts

  for (;;)
  {
  if (value_placed){
   value_placed=0;
   transfer=value_convert;
   transmit(transfer);
   transmit(~transfer);
   RXTXData=EOFPACKET;
//      RXTXData=0xF7;
   TX_Byte();                                // TX Back RXed Byte Received
   }
  _BIS_SR(CPUOFF);              // LPM0
  }
}


void  fast_charge(void){
 CAPD&=~CAPACITOR_PIN; 			// подключаем входной буфер
 P2SEL&=~CAPACITOR_PIN;                 // подключаем входной буфер
 PORT_OUT_CAPACITOR|=CAPACITOR_PIN;	// высокий уровень
 PORT_DIR_CAPACITOR|=CAPACITOR_PIN;      // направление на выход
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
 CACTL1&=~CAIE;	//запрещаем прерывание от компаратора
 _BIC_SR_IRQ(CPUOFF);                // Clear CPUOFF bit from 0(SR)

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
//инициализация параметров
/****************************************/
void init_params(void){
 CACTL1=0;	//
 CAPD=DIODE_PIN;	//отключаем диод от входных буферов микросхемы
 BCSCTL1=(BCSCTL1&~(RSEL0|RSEL1|RSEL2))|RSEL2;
 BCSCTL2|=DCOR;
}




/****************************************/
//включение компаратора и опорника на 0.25Vcc
/****************************************/
void on_comparator(void){
  //к диоду компаратор+ подключен
  //компаратор- от внешнего входа отключен
 CACTL2=(CACTL2&~(P2CA0|P2CA1|CAF))|P2CA0|P2CA1|CAF;	
  //!!! какое время включения (заранее) компаратора?
  //выбрано 0.25Vcc и подключено на компаратор-
 CACTL1=(CACTL1&~(CAREF0|CAREF1))|CAON|CAIE;	//включаем компаратор
}

/****************************************/
//включение компаратора и внешнего выхода
/****************************************/
void on_comparator_external(void){
  //к диоду компаратор+ подключен
  //компаратор- подключен к внешнему входу
 CACTL2=(CACTL2&~(P2CA0|P2CA1|CAF))|P2CA1|P2CA0|CAF;	
  //!!! какое время включения (заранее) компаратора?
  //включаем компаратор
  //выключен внутренний опорник
 CACTL1=(CACTL1&~(CAREF0|CAREF1))|CAON;	//включаем компаратор
}


