/*
 * rtc.c
 *
 * Created: 12.03.2020 11:55:49
 *  Author: V.Nezlo
 */ 

#include "rtc.h"

#define RTCADRR 0b11010001
#define RTCADRW 0b11010000

struct Time {uint8_t hour, minute, second, day; char alarm_state;} time;



uint8_t RTC_ConvertFromBinDec(uint8_t c)
{
	uint8_t ch = ((c/10)<<4)|(c%10);
	return ch;
}

uint8_t RTC_ConvertFromDec(uint8_t c)
{
	uint8_t ch = ((c>>4)*10+(0b00001111&c));
	return ch;
}

void RTC_Set(void)
{

	//Тут можно было бы написать функцию с аргументами, но зачем?
	
	I2C_StartCondition();
	I2C_SendByte(RTCADRW);//slave
	I2C_SendByte(0);//Регистр 00
	I2C_SendByte(RTC_ConvertFromBinDec(00));//секунды 00h
	I2C_SendByte(RTC_ConvertFromBinDec(00));//минуты 01h
	I2C_SendByte(RTC_ConvertFromBinDec(12));//часы   02h
	I2C_SendByte(RTC_ConvertFromBinDec(5));//день недели     03h
	I2C_SendByte(RTC_ConvertFromBinDec(13));//дата   04h
	I2C_SendByte(RTC_ConvertFromBinDec(3));//месяц   05h
	I2C_SendByte(RTC_ConvertFromBinDec(20));//год   06h
	I2C_SendByte(RTC_ConvertFromBinDec(0));//A1M1 секунды   07h
	I2C_SendByte(RTC_ConvertFromBinDec(0));//A1M2 минуты   08h
	I2C_SendByte(RTC_ConvertFromBinDec(15));//A1M3 часы   09h
	I2C_SendByte(0b10000000);				//A1M4 день+дата	 0Ah
	I2C_SendByte(RTC_ConvertFromBinDec(0));//A2 минуты   0Bh
	I2C_SendByte(RTC_ConvertFromBinDec(0));//A2 часы   0Ch
	I2C_SendByte(RTC_ConvertFromBinDec(0));//A2 день+дата   0Dh
	I2C_SendByte(0b00011101);//Контрольный  0Eh Прерывания будильника включены и включен будильник А1
	I2C_SendByte(0b00001000);//Контрольный/статусный  0Fh Записывать 0 в последний бит для сброса будильника
	I2C_StopCondition();
}

void RTC_Read(void)
{
uint8_t sec, min, hour, day, unused;
	
I2C_SendByteByADDR(0,RTCADRW);
_delay_ms(200);
I2C_StartCondition();
I2C_SendByte(RTCADRR);
sec = I2C_ReadByte();
min = I2C_ReadByte();
hour = I2C_ReadByte();
day = I2C_ReadByte();
unused = I2C_ReadByte();
unused= I2C_ReadByte();
unused = I2C_ReadByte();
unused = 0;
unused = I2C_ReadByte();
unused = I2C_ReadByte();
unused = I2C_ReadLastByte();
I2C_StopCondition();

time.second=RTC_ConvertFromDec(sec);
time.minute=RTC_ConvertFromDec(min);
time.hour=RTC_ConvertFromDec(hour);
time.day=RTC_ConvertFromDec(day);
}
