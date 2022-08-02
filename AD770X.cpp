/*
	FILE: AD770X.cpp
	AUTHOR: Jianou
	VERSION: 0.0.1
	DATE: 2022-08-02
	PURPOSE: Class for AD770X function generator
*/

#include "AD770X.h"

void AD770X::RESET_0()	    {digitalWrite(pinreset, LOW);}
void AD770X::RESET_1()	    {digitalWrite(pinreset, HIGH);}
void AD770X::CS_0()		    {digitalWrite(pinCS, LOW);}
void AD770X::CS_1()		    {digitalWrite(pinCS, HIGH);}
void AD770X::SCK_0()		{digitalWrite(pinSPIClock, LOW);}
void AD770X::SCK_1()		{digitalWrite(pinSPIClock, HIGH);}
void AD770X::DI_0()         {digitalWrite(pinMOSI, LOW);}		
void AD770X::DI_1()	        {digitalWrite(pinMOSI, HIGH);}
char AD770X::DO_IS_HIGH()	{return (digitalRead(pinMISO) == 1);}
char AD770X::DRDY_IS_LOW()	{return (digitalRead(pindrty) == 0);}

/*
*********************************************************************************************************
*	�� �� ��: AD770X
*	����˵��: ����ģ��spi�Ľӿ�
*	��    ��: 	mosi  	�������
				miso 	��������
				clk		ʱ��
				cs		Ƭѡ
				res		��λ
				drty	����ͬ��
				error	���󣨿��Բ���  -1��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
AD770X::AD770X(char mosi, char miso, char clk, char cs, char res, char drty, char error)
{
    pinMOSI        = mosi;          //MOSI
    pinMISO        = miso;          //MISO
    pinSPIClock    = clk;           //SCK
    pinCS          = cs;            //CS
    pinreset       = res;           //Reset
    pindrty        = drty;          //DRTY
    pinerror	   = error;         //error
}

AD770X::~AD770X()
{
}


/*
*********************************************************************************************************
*	�� �� ��: Init
*	����˵��: ��ʼ��ģ��spi�Ľӿڣ���У���ڲ��Ĵ���������ˢ���ٶ�
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void AD770X::Init(void)
{
	pinMode(pinMOSI, OUTPUT);
	pinMode(pinMISO, INPUT_PULLUP);
	pinMode(pinSPIClock, OUTPUT);
	pinMode(pinCS, OUTPUT);
	pinMode(pinreset, OUTPUT);
	pinMode(pindrty, INPUT);
	pinMode(pinerror, OUTPUT);
	
	CS_1();
	SCK_1();
	DI_1();	

	delay(10);
	
	/*
		�ڽӿ����ж�ʧ������£������DIN �ߵ�ƽ��д�����������㹻����ʱ�䣨���� 32������ʱ�����ڣ���
		����ص�Ĭ��״̬��
	*/	
	ResetHard();		/* Ӳ����λ */
	delay(5);
	SyncSPI();		/* ͬ��SPI�ӿ�ʱ�� */
	delay(5);

	// У׼ 
	CalibSelf(1);		/* ��У׼��ִ��ʱ��ϳ���Լ180ms */		
	delay(250);			
	
	CalibSelf(2);		/* ��У׼��ִ��ʱ��ϳ���Լ180ms */
	delay(250);

	WriteByte(REG_SETUP | WRITE | CH_1);	/* дͨ�żĴ�������һ����д���üĴ�����ͨ��1 */		
	WriteByte(MD_NORMAL |(__CH1_GAIN_BIPOLAR_BUF) | FSYNC_0);
	WaitDRDY();
	delay(50);

	WriteByte(REG_SETUP | WRITE | CH_2);	/* дͨ�żĴ�������һ����д���üĴ�����ͨ��1 */		
	WriteByte(MD_NORMAL |(__CH2_GAIN_BIPOLAR_BUF) | FSYNC_0); /* ������У׼ */
	WaitDRDY();
	delay(50);

	WriteByte(REG_CLOCK | WRITE);
	WriteByte(CLKDIS_0 | CLK_4_9152M | FS_50HZ);	/* ˢ������50Hz */
	WaitDRDY();
	delay(50);
}

/*
*********************************************************************************************************
*	�� �� ��: Delay
*	����˵��: CLK֮����ӳ٣�ʱ���ӳ�. ����ESP32  240M��Ƶ
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void AD770X::Delay(void)
{
	int i;
	for (i = 0; i < 30; i++) NOP();
}

/*
*********************************************************************************************************
*	�� �� ��: ResetHard
*	����˵��: Ӳ����λ оƬ
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void AD770X::ResetHard(void)
{
	RESET_1();
	delay(1);	
	RESET_0();
	delay(2);
	RESET_1();
	delay(1);
}

/*
*********************************************************************************************************
*	�� �� ��: SyncSPI
*	����˵��: ͬ��оƬSPI�ӿ�ʱ��
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void AD770X::SyncSPI(void)
{
	/* AD770X���нӿ�ʧ�����临λ����λ��Ҫ��ʱ500us�ٷ��� */
	CS_0();
	Send8Bit(0xFF);
	Send8Bit(0xFF);
	Send8Bit(0xFF);
	Send8Bit(0xFF);	
	CS_1();
}

/*
*********************************************************************************************************
*	�� �� ��: Send8Bit
*	����˵��: ��SPI���߷���8��bit���ݡ� ����CS���ơ�
*	��    ��: _data : ����
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void AD770X::Send8Bit(unsigned char _data)
{
	unsigned char i;

	for(i = 0; i < 8; i++)
	{
		if (_data & 0x80) DI_1();
		else DI_0();
		SCK_0();
		_data <<= 1;
		Delay();
		SCK_1();	
		Delay();		
	}
}

/*
*********************************************************************************************************
*	�� �� ��: Recive8Bit
*	����˵��: ��SPI���߽���8��bit���ݡ� ����CS���ơ�
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
unsigned char AD770X::Recive8Bit(void)
{
	unsigned char i;
	unsigned char read = 0;

	for (i = 0; i < 8; i++)
	{
		SCK_0();
		Delay();		
		read = read<<1;
		if (DO_IS_HIGH())
		{
			read++;
		}
		SCK_1();		
		Delay();
	}
	return read;
}

/*
*********************************************************************************************************
*	�� �� ��: WriteByte
*	����˵��: д��1���ֽڡ���CS����
*	��    ��: _data ����Ҫд�������
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void AD770X::WriteByte(unsigned char _data)
{
	CS_0();
	Send8Bit(_data);
	CS_1();
}

/*
*********************************************************************************************************
*	�� �� ��: Write3Byte
*	����˵��: д��3���ֽڡ���CS����
*	��    ��: _data ����Ҫд�������
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void AD770X::Write3Byte(unsigned int _data)
{
	CS_0();
	Send8Bit((_data >> 16) & 0xFF);
	Send8Bit((_data >> 8) & 0xFF);
	Send8Bit(_data);
	CS_1();
}

/*
*********************************************************************************************************
*	�� �� ��: ReadByte
*	����˵��: ��ADоƬ��ȡһ���֣�16λ��
*	��    ��: ��
*	�� �� ֵ: ��ȡ���֣�16λ��
*********************************************************************************************************
*/
unsigned char AD770X::ReadByte(void)
{
	unsigned char read;

	CS_0();
	read = Recive8Bit();
	CS_1();
	
	return read;
}

/*
*********************************************************************************************************
*	�� �� ��: Read2Byte
*	����˵��: ��2�ֽ�����
*	��    ��: ��
*	�� �� ֵ: ��ȡ�����ݣ�16λ��
*********************************************************************************************************
*/
unsigned int AD770X::Read2Byte(void)
{
	unsigned int read;

	CS_0();
	read = Recive8Bit();
	read <<= 8;
	read += Recive8Bit();
	CS_1();

	return read;
}

/*
*********************************************************************************************************
*	�� �� ��: Read3Byte
*	����˵��: ��3�ֽ�����
*	��    ��: ��
*	�� �� ֵ: ��ȡ�������ݣ�24bit) ��8λ�̶�Ϊ0.
*********************************************************************************************************
*/
unsigned int AD770X::Read3Byte(void)
{
	unsigned int read;

	CS_0();
	read = Recive8Bit();
	read <<= 8;
	read += Recive8Bit();
	read <<= 8;
	read += Recive8Bit();
	CS_1();
	return read;
}

/*
*********************************************************************************************************
*	�� �� ��: WaitDRDY
*	����˵��: �ȴ��ڲ�������ɡ� ��У׼ʱ��ϳ�����Ҫ�ȴ���
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void AD770X::WaitDRDY(void)
{
	unsigned long long i;

	for (i = 0; i < 4000000; i++)
	{
		if (DRDY_IS_LOW()) break;
	}
	digitalWrite(pinerror, 0);
	if (i >= 4000000) digitalWrite(pinerror, 1);
}

/*
*********************************************************************************************************
*	�� �� ��: WriteReg
*	����˵��: дָ���ļĴ���
*	��    ��:  _RegID : �Ĵ���ID
*			  _RegValue : �Ĵ���ֵ�� ����8λ�ļĴ�����ȡ32λ�βεĵ�8bit
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void AD770X::WriteReg(unsigned char _RegID, unsigned int _RegValue)
{
	unsigned char bits;

	switch (_RegID)
	{
		case REG_COMM:		/* ͨ�żĴ��� */		
		case REG_SETUP:		/* ���üĴ��� 8bit */
		case REG_CLOCK:		/* ʱ�ӼĴ��� 8bit */
			bits = 8;
			break;

		case REG_ZERO_CH1:	/* CH1 ƫ�ƼĴ��� 24bit */
		case REG_FULL_CH1:	/* CH1 �����̼Ĵ��� 24bit */
		case REG_ZERO_CH2:	/* CH2 ƫ�ƼĴ��� 24bit */
		case REG_FULL_CH2:	/* CH2 �����̼Ĵ��� 24bit*/
			bits = 24;
			break;

		case REG_DATA:		/* ���ݼĴ��� 16bit */
		default:
			return;
	}

	WriteByte(_RegID | WRITE);	/* дͨ�żĴ���, ָ����һ����д��������ָ��д�ĸ��Ĵ��� */

	if (bits == 8)
	{
		WriteByte((unsigned char)_RegValue);
	}
	else	/* 24bit */
	{
		Write3Byte(_RegValue);
	}
}

/*
*********************************************************************************************************
*	�� �� ��: ReadReg
*	����˵��: дָ���ļĴ���
*	��    ��:  _RegID : �Ĵ���ID
*			  _RegValue : �Ĵ���ֵ�� ����8λ�ļĴ�����ȡ32λ�βεĵ�8bit
*	�� �� ֵ: �����ļĴ���ֵ�� ����8λ�ļĴ�����ȡ32λ�βεĵ�8bit
*********************************************************************************************************
*/
unsigned int AD770X::ReadReg(unsigned char _RegID)
{
	unsigned char bits;
	unsigned int read;

	switch (_RegID)
	{
		case REG_COMM:		/* ͨ�żĴ��� */
		case REG_SETUP:		/* ���üĴ��� 8bit */
		case REG_CLOCK:		/* ʱ�ӼĴ��� 8bit */
			bits = 8;
			break;

		case REG_ZERO_CH1:	/* CH1 ƫ�ƼĴ��� 24bit */
		case REG_FULL_CH1:	/* CH1 �����̼Ĵ��� 24bit */
		case REG_ZERO_CH2:	/* CH2 ƫ�ƼĴ��� 24bit */
		case REG_FULL_CH2:	/* CH2 �����̼Ĵ��� 24bit*/
			bits = 24;
			break;

		case REG_DATA:		/* ���ݼĴ��� 16bit */
		default:
			return 0xFFFFFFFF;
	}

	WriteByte(_RegID | READ);	/* дͨ�żĴ���, ָ����һ����д��������ָ��д�ĸ��Ĵ��� */

	if (bits == 16)
	{
		read = Read2Byte();
	}
	else if (bits == 8)
	{
		read = ReadByte();
	}
	else	/* 24bit */
	{
		read = Read3Byte();
	}
	return read;
}

/*
*********************************************************************************************************
*	�� �� ��: CalibSelf
*	����˵��: ������У׼. �ڲ��Զ��̽�AIN+ AIN-У׼0λ���ڲ��̽ӵ�Vref У׼��λ���˺���ִ�й��̽ϳ���
*			  ʵ��Լ 180ms
*	��    ��:  _ch : ADCͨ����1��2
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void AD770X::CalibSelf(unsigned char _ch)
{
	if (_ch == 1)
	{
		/* ��У׼CH1 */
		WriteByte(REG_SETUP | WRITE | CH_1);	/* дͨ�żĴ�������һ����д���üĴ�����ͨ��1 */		
		WriteByte(MD_CAL_SELF | __CH1_GAIN_BIPOLAR_BUF | FSYNC_0);/* ������У׼ */
		WaitDRDY();	/* �ȴ��ڲ�������� --- ʱ��ϳ���Լ180ms */
	}
	else if (_ch == 2)
	{
		/* ��У׼CH2 */
		WriteByte(REG_SETUP | WRITE | CH_2);	/* дͨ�żĴ�������һ����д���üĴ�����ͨ��2 */
		WriteByte(MD_CAL_SELF | __CH2_GAIN_BIPOLAR_BUF | FSYNC_0);	/* ������У׼ */
		WaitDRDY();	/* �ȴ��ڲ��������  --- ʱ��ϳ���Լ180ms */
	}
}

/*
*********************************************************************************************************
*	�� �� ��: SytemCalibZero
*	����˵��: ����ϵͳУ׼��λ. �뽫AIN+ AIN-�̽Ӻ�ִ�иú�����У׼Ӧ������������Ʋ�����У׼������
*			 ִ����Ϻ󡣿���ͨ�� _ReadReg(REG_ZERO_CH1) ��  _ReadReg(REG_ZERO_CH2) ��ȡУ׼������
*	��    ��: _ch : ADCͨ����1��2
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void AD770X::SytemCalibZero(unsigned char _ch)
{
	if (_ch == 1)
	{
		/* У׼CH1 */
		WriteByte(REG_SETUP | WRITE | CH_1);	/* дͨ�żĴ�������һ����д���üĴ�����ͨ��1 */
		WriteByte(MD_CAL_ZERO | __CH1_GAIN_BIPOLAR_BUF | FSYNC_0);/* ������У׼ */
		WaitDRDY();	/* �ȴ��ڲ�������� */
	}
	else if (_ch == 2)
	{
		/* У׼CH2 */
		WriteByte(REG_SETUP | WRITE | CH_2);	/* дͨ�żĴ�������һ����д���üĴ�����ͨ��1 */
		WriteByte(MD_CAL_ZERO | __CH2_GAIN_BIPOLAR_BUF | FSYNC_0);	/* ������У׼ */
		WaitDRDY();	/* �ȴ��ڲ�������� */
	}
}

/*
*********************************************************************************************************
*	�� �� ��: SytemCalibFull
*	����˵��: ����ϵͳУ׼��λ. �뽫AIN+ AIN-����������ѹԴ��ִ�иú�����У׼Ӧ������������Ʋ�����У׼������
*			 ִ����Ϻ󡣿���ͨ�� _ReadReg(REG_FULL_CH1) ��  _ReadReg(REG_FULL_CH2) ��ȡУ׼������
*	��    ��:  _ch : ADCͨ����1��2
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void AD770X::SytemCalibFull(unsigned char _ch)
{
	if (_ch == 1)
	{
		/* У׼CH1 */
		WriteByte(REG_SETUP | WRITE | CH_1);	/* дͨ�żĴ�������һ����д���üĴ�����ͨ��1 */
		WriteByte(MD_CAL_FULL | __CH1_GAIN_BIPOLAR_BUF | FSYNC_0);/* ������У׼ */
		WaitDRDY();	/* �ȴ��ڲ�������� */
	}
	else if (_ch == 2)
	{
		/* У׼CH2 */
		WriteByte(REG_SETUP | WRITE | CH_2);	/* дͨ�żĴ�������һ����д���üĴ�����ͨ��1 */
		WriteByte(MD_CAL_FULL | __CH2_GAIN_BIPOLAR_BUF | FSYNC_0);	/* ������У׼ */
		WaitDRDY();	/* �ȴ��ڲ�������� */
	}
}

/*
*********************************************************************************************************
*	�� �� ��: ReadAdc
*	����˵��: ��ͨ��1��2��ADC����
*	��    ��:	_ch  	���ö�ȡ��ADCͨ��
*	�� �� ֵ: 	��ֵ��uint��
*********************************************************************************************************
*/
unsigned int AD770X::ReadAdc(unsigned char _ch)
{
	unsigned char i;
	unsigned int read = 0;
	
	/* Ϊ�˱���ͨ���л���ɶ���ʧЧ����2�� */
	for (i = 0; i < 2; i++)
	{
		WaitDRDY();		/* �ȴ�DRDY����Ϊ0 */		

		if (_ch == 1) WriteByte(0x38);
		else if (_ch == 2) WriteByte(0x39);

		read = Read2Byte();
	}
	return read;	
}

/*
*********************************************************************************************************
*	�� �� ��: ReadmV
*	����˵��: ��ͨ��1��2�ĵ�ѹ����
*	��    ��:	_ch  	���ö�ȡ��ADCͨ��
				Zero	���õ�ѹ����У׼ֵ
				Rate	���õ�ѹ��׼ֵ������� ��ѹ / ��ֵ��
*	�� �� ֵ: 	��ѹֵ��mV��
*********************************************************************************************************
*/
double AD770X::ReadmV(unsigned char _ch, int Zero, double Rate)
{
    int adc = ReadAdc(_ch) - Zero;
    return (double)adc * Rate;
}
