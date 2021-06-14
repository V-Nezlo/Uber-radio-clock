#include "si4730.h"

void si4730_powerup(void)
{
	/*Тут мог бы быть код для управления ресетом, но в моей реализации 
	ресет подтянут к питанию, соответственно модуль всегда включен*/
	
	I2C_StartCondition();
	I2C_SendByte(SI4730_ADDR);    
	I2C_SendByte(SI4730_POWER_UP);
	I2C_SendByte(0b00010000);     //внешний кварц
	I2C_SendByte(0b00000101);     //вкл только аудиовыходы, можно выводить на I2S (но зачем)
	I2C_StopCondition();
	_delay_ms(100);
	
}

void si4730_SetFmFreq(uint16_t freq)
{
	uint8_t Hfreq = freq >> 8;
	uint8_t LFreq = freq & 0x00FF;
	
	I2C_StartCondition();
	I2C_SendByte(SI4730_ADDR);
	I2C_SendByte(SI4730_SET_FM_FREQ);
	I2C_SendByte(0x00); //FREESE&FAST не нужны
	I2C_SendByte(Hfreq);
	I2C_SendByte(LFreq);
	I2C_StopCondition();
	_delay_ms(50);
}

void si4730_SetAmFreq(uint16_t freq)
{
	uint8_t Hfreq = freq >> 8;
	uint8_t LFreq = freq & 0x00FF;
	
	I2C_StartCondition();
	I2C_SendByte(SI4730_ADDR);
	I2C_SendByte(SI4730_SET_AM_FREQ);
	I2C_SendByte(0x00); //FREESE&FAST не нужны
	I2C_SendByte(Hfreq);
	I2C_SendByte(LFreq);
	I2C_StopCondition();
	_delay_ms(50);
}

void si4730_mute(char mute)
{
	I2C_StartCondition();
	I2C_SendByte(SI4730_ADDR);
	I2C_SendByte(SI4730_HARDMUTE_H);
	I2C_SendByte(SI4730_HARDMUTE_L);
	I2C_SendByte(0x00);
	if (mute) I2C_SendByte(0x03);
	else I2C_SendByte(0x00);
	I2C_StopCondition();
	_delay_ms(50);
}

void si4730_powerdown()
{
	I2C_StartCondition();
	I2C_SendByte(SI4730_ADDR);
	I2C_SendByte(SI4730_POWER_DOWN);
	I2C_StopCondition();
	_delay_ms(50);
}
