#include "twi.h"

void I2C_Init (void)
{
	TWBR=0x48;//Скорость
}


void I2C_StartCondition(void)
{
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
}

void I2C_StopCondition(void)
{
	TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
}

void I2C_SendByte(unsigned char a)
{
	TWDR = a;
	TWCR = (1<<TWINT)|(1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
}

void I2C_SendByteByADDR(unsigned char a,unsigned char addr)
{
	I2C_StartCondition(); 
	I2C_SendByte(addr); 
	I2C_SendByte(a);
	I2C_StopCondition();
}

unsigned char I2C_ReadByte(void)
{
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
	while (!(TWCR & (1<<TWINT)));
	return TWDR;
}

unsigned char I2C_ReadLastByte(void)
{
	TWCR = (1<<TWINT)|(1<<TWEN);//А вот тут нет экноледжа
	while (!(TWCR & (1<<TWINT)));
	return TWDR;
}