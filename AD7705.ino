#include "AD770X.h"

int adc1, adc2;
double volt1, volt2;

AD770X AD7705(26, 14, 12, 27, 13, 25, 2);

void setup()
{
	Serial.begin(115200);
	AD7705.Init();
}

void loop()
{
	adc1 = AD7705.ReadAdc(1);	/* 执行时间 80ms */		
	adc2 = AD7705.ReadAdc(2);	/* 执行时间 80ms */

	/* 计算实际电压值（近似估算的），如需准确，请进行校准 */
	volt1 = AD7705.ReadmV(1, 480, (1250.0 / 16500.0));
	volt2 = AD7705.ReadmV(2, 480, (1250.0 / 16500.0));
	
	/* 打印ADC采样结果 */	
	Serial.printf("CH1=%5d (%5.3fmV) CH2=%5d (%5.3fmV)\r\n", adc1, volt1, adc2, volt2);
}