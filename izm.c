 #include  <msp430x11x1.h>

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



/****************************************/
//������������� ����������
/****************************************/
void init_params(){
 CACTL1=0;	//
 CAPD=DIODE_PIN;	//��������� ���� �� ������� ������� ����������
}

/****************************************/
//��������� ������ �������� ������������
/****************************************/


void on_charge(void){
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

void  fast_charge(void){
 CAPD&=~CAPACITOR_PIN; 			// ���������� ������� �����
 P2SEL&=~CAPACITOR_PIN;                 // ���������� ������� �����
 PORT_OUT_CAPACITOR|=CAPACITOR_PIN;	// ������� �������
 PORT_DIR_CAPACITOR|=CAPACITOR_PIN;      // ����������� �� �����
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
