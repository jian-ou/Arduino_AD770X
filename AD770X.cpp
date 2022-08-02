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
*	函 数 名: AD770X
*	功能说明: 设置模拟spi的接口
*	形    参: 	mosi  	数据输出
				miso 	数据输入
				clk		时钟
				cs		片选
				res		复位
				drty	数据同步
				error	错误（可以不接  -1）
*	返 回 值: 无
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
*	函 数 名: Init
*	功能说明: 初始化模拟spi的接口，自校正内部寄存器，设置刷新速度
*	形    参: 无
*	返 回 值: 无
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
		在接口序列丢失的情况下，如果在DIN 高电平的写操作持续了足够长的时间（至少 32个串行时钟周期），
		将会回到默认状态。
	*/	
	ResetHard();		/* 硬件复位 */
	delay(5);
	SyncSPI();		/* 同步SPI接口时序 */
	delay(5);

	// 校准 
	CalibSelf(1);		/* 自校准。执行时间较长，约180ms */		
	delay(250);			
	
	CalibSelf(2);		/* 自校准。执行时间较长，约180ms */
	delay(250);

	WriteByte(REG_SETUP | WRITE | CH_1);	/* 写通信寄存器，下一步是写设置寄存器，通道1 */		
	WriteByte(MD_NORMAL |(__CH1_GAIN_BIPOLAR_BUF) | FSYNC_0);
	WaitDRDY();
	delay(50);

	WriteByte(REG_SETUP | WRITE | CH_2);	/* 写通信寄存器，下一步是写设置寄存器，通道1 */		
	WriteByte(MD_NORMAL |(__CH2_GAIN_BIPOLAR_BUF) | FSYNC_0); /* 启动自校准 */
	WaitDRDY();
	delay(50);

	WriteByte(REG_CLOCK | WRITE);
	WriteByte(CLKDIS_0 | CLK_4_9152M | FS_50HZ);	/* 刷新速率50Hz */
	WaitDRDY();
	delay(50);
}

/*
*********************************************************************************************************
*	函 数 名: Delay
*	功能说明: CLK之间的延迟，时序延迟. 用于ESP32  240M主频
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void AD770X::Delay(void)
{
	int i;
	for (i = 0; i < 30; i++) NOP();
}

/*
*********************************************************************************************************
*	函 数 名: ResetHard
*	功能说明: 硬件复位 芯片
*	形    参: 无
*	返 回 值: 无
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
*	函 数 名: SyncSPI
*	功能说明: 同步芯片SPI接口时序
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void AD770X::SyncSPI(void)
{
	/* AD770X串行接口失步后将其复位。复位后要延时500us再访问 */
	CS_0();
	Send8Bit(0xFF);
	Send8Bit(0xFF);
	Send8Bit(0xFF);
	Send8Bit(0xFF);	
	CS_1();
}

/*
*********************************************************************************************************
*	函 数 名: Send8Bit
*	功能说明: 向SPI总线发送8个bit数据。 不带CS控制。
*	形    参: _data : 数据
*	返 回 值: 无
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
*	函 数 名: Recive8Bit
*	功能说明: 从SPI总线接收8个bit数据。 不带CS控制。
*	形    参: 无
*	返 回 值: 无
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
*	函 数 名: WriteByte
*	功能说明: 写入1个字节。带CS控制
*	形    参: _data ：将要写入的数据
*	返 回 值: 无
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
*	函 数 名: Write3Byte
*	功能说明: 写入3个字节。带CS控制
*	形    参: _data ：将要写入的数据
*	返 回 值: 无
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
*	函 数 名: ReadByte
*	功能说明: 从AD芯片读取一个字（16位）
*	形    参: 无
*	返 回 值: 读取的字（16位）
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
*	函 数 名: Read2Byte
*	功能说明: 读2字节数据
*	形    参: 无
*	返 回 值: 读取的数据（16位）
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
*	函 数 名: Read3Byte
*	功能说明: 读3字节数据
*	形    参: 无
*	返 回 值: 读取到的数据（24bit) 高8位固定为0.
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
*	函 数 名: WaitDRDY
*	功能说明: 等待内部操作完成。 自校准时间较长，需要等待。
*	形    参: 无
*	返 回 值: 无
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
*	函 数 名: WriteReg
*	功能说明: 写指定的寄存器
*	形    参:  _RegID : 寄存器ID
*			  _RegValue : 寄存器值。 对于8位的寄存器，取32位形参的低8bit
*	返 回 值: 无
*********************************************************************************************************
*/
void AD770X::WriteReg(unsigned char _RegID, unsigned int _RegValue)
{
	unsigned char bits;

	switch (_RegID)
	{
		case REG_COMM:		/* 通信寄存器 */		
		case REG_SETUP:		/* 设置寄存器 8bit */
		case REG_CLOCK:		/* 时钟寄存器 8bit */
			bits = 8;
			break;

		case REG_ZERO_CH1:	/* CH1 偏移寄存器 24bit */
		case REG_FULL_CH1:	/* CH1 满量程寄存器 24bit */
		case REG_ZERO_CH2:	/* CH2 偏移寄存器 24bit */
		case REG_FULL_CH2:	/* CH2 满量程寄存器 24bit*/
			bits = 24;
			break;

		case REG_DATA:		/* 数据寄存器 16bit */
		default:
			return;
	}

	WriteByte(_RegID | WRITE);	/* 写通信寄存器, 指定下一步是写操作，并指定写哪个寄存器 */

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
*	函 数 名: ReadReg
*	功能说明: 写指定的寄存器
*	形    参:  _RegID : 寄存器ID
*			  _RegValue : 寄存器值。 对于8位的寄存器，取32位形参的低8bit
*	返 回 值: 读到的寄存器值。 对于8位的寄存器，取32位形参的低8bit
*********************************************************************************************************
*/
unsigned int AD770X::ReadReg(unsigned char _RegID)
{
	unsigned char bits;
	unsigned int read;

	switch (_RegID)
	{
		case REG_COMM:		/* 通信寄存器 */
		case REG_SETUP:		/* 设置寄存器 8bit */
		case REG_CLOCK:		/* 时钟寄存器 8bit */
			bits = 8;
			break;

		case REG_ZERO_CH1:	/* CH1 偏移寄存器 24bit */
		case REG_FULL_CH1:	/* CH1 满量程寄存器 24bit */
		case REG_ZERO_CH2:	/* CH2 偏移寄存器 24bit */
		case REG_FULL_CH2:	/* CH2 满量程寄存器 24bit*/
			bits = 24;
			break;

		case REG_DATA:		/* 数据寄存器 16bit */
		default:
			return 0xFFFFFFFF;
	}

	WriteByte(_RegID | READ);	/* 写通信寄存器, 指定下一步是写操作，并指定写哪个寄存器 */

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
*	函 数 名: CalibSelf
*	功能说明: 启动自校准. 内部自动短接AIN+ AIN-校准0位，内部短接到Vref 校准满位。此函数执行过程较长，
*			  实测约 180ms
*	形    参:  _ch : ADC通道，1或2
*	返 回 值: 无
*********************************************************************************************************
*/
void AD770X::CalibSelf(unsigned char _ch)
{
	if (_ch == 1)
	{
		/* 自校准CH1 */
		WriteByte(REG_SETUP | WRITE | CH_1);	/* 写通信寄存器，下一步是写设置寄存器，通道1 */		
		WriteByte(MD_CAL_SELF | __CH1_GAIN_BIPOLAR_BUF | FSYNC_0);/* 启动自校准 */
		WaitDRDY();	/* 等待内部操作完成 --- 时间较长，约180ms */
	}
	else if (_ch == 2)
	{
		/* 自校准CH2 */
		WriteByte(REG_SETUP | WRITE | CH_2);	/* 写通信寄存器，下一步是写设置寄存器，通道2 */
		WriteByte(MD_CAL_SELF | __CH2_GAIN_BIPOLAR_BUF | FSYNC_0);	/* 启动自校准 */
		WaitDRDY();	/* 等待内部操作完成  --- 时间较长，约180ms */
	}
}

/*
*********************************************************************************************************
*	函 数 名: SytemCalibZero
*	功能说明: 启动系统校准零位. 请将AIN+ AIN-短接后，执行该函数。校准应该由主程序控制并保存校准参数。
*			 执行完毕后。可以通过 _ReadReg(REG_ZERO_CH1) 和  _ReadReg(REG_ZERO_CH2) 读取校准参数。
*	形    参: _ch : ADC通道，1或2
*	返 回 值: 无
*********************************************************************************************************
*/
void AD770X::SytemCalibZero(unsigned char _ch)
{
	if (_ch == 1)
	{
		/* 校准CH1 */
		WriteByte(REG_SETUP | WRITE | CH_1);	/* 写通信寄存器，下一步是写设置寄存器，通道1 */
		WriteByte(MD_CAL_ZERO | __CH1_GAIN_BIPOLAR_BUF | FSYNC_0);/* 启动自校准 */
		WaitDRDY();	/* 等待内部操作完成 */
	}
	else if (_ch == 2)
	{
		/* 校准CH2 */
		WriteByte(REG_SETUP | WRITE | CH_2);	/* 写通信寄存器，下一步是写设置寄存器，通道1 */
		WriteByte(MD_CAL_ZERO | __CH2_GAIN_BIPOLAR_BUF | FSYNC_0);	/* 启动自校准 */
		WaitDRDY();	/* 等待内部操作完成 */
	}
}

/*
*********************************************************************************************************
*	函 数 名: SytemCalibFull
*	功能说明: 启动系统校准满位. 请将AIN+ AIN-接最大输入电压源，执行该函数。校准应该由主程序控制并保存校准参数。
*			 执行完毕后。可以通过 _ReadReg(REG_FULL_CH1) 和  _ReadReg(REG_FULL_CH2) 读取校准参数。
*	形    参:  _ch : ADC通道，1或2
*	返 回 值: 无
*********************************************************************************************************
*/
void AD770X::SytemCalibFull(unsigned char _ch)
{
	if (_ch == 1)
	{
		/* 校准CH1 */
		WriteByte(REG_SETUP | WRITE | CH_1);	/* 写通信寄存器，下一步是写设置寄存器，通道1 */
		WriteByte(MD_CAL_FULL | __CH1_GAIN_BIPOLAR_BUF | FSYNC_0);/* 启动自校准 */
		WaitDRDY();	/* 等待内部操作完成 */
	}
	else if (_ch == 2)
	{
		/* 校准CH2 */
		WriteByte(REG_SETUP | WRITE | CH_2);	/* 写通信寄存器，下一步是写设置寄存器，通道1 */
		WriteByte(MD_CAL_FULL | __CH2_GAIN_BIPOLAR_BUF | FSYNC_0);	/* 启动自校准 */
		WaitDRDY();	/* 等待内部操作完成 */
	}
}

/*
*********************************************************************************************************
*	函 数 名: ReadAdc
*	功能说明: 读通道1或2的ADC数据
*	形    参:	_ch  	设置读取的ADC通道
*	返 回 值: 	码值（uint）
*********************************************************************************************************
*/
unsigned int AD770X::ReadAdc(unsigned char _ch)
{
	unsigned char i;
	unsigned int read = 0;
	
	/* 为了避免通道切换造成读数失效，读2次 */
	for (i = 0; i < 2; i++)
	{
		WaitDRDY();		/* 等待DRDY口线为0 */		

		if (_ch == 1) WriteByte(0x38);
		else if (_ch == 2) WriteByte(0x39);

		read = Read2Byte();
	}
	return read;	
}

/*
*********************************************************************************************************
*	函 数 名: ReadmV
*	功能说明: 读通道1或2的电压数据
*	形    参:	_ch  	设置读取的ADC通道
				Zero	设置电压调零校准值
				Rate	设置电压基准值（任意点 电压 / 码值）
*	返 回 值: 	电压值（mV）
*********************************************************************************************************
*/
double AD770X::ReadmV(unsigned char _ch, int Zero, double Rate)
{
    int adc = ReadAdc(_ch) - Zero;
    return (double)adc * Rate;
}
