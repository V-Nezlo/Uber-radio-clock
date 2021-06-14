#include "eeprom.h"

void EEPROM_write(unsigned int uiAddress, unsigned char ucData)
{
while(EECR & (1<<EEPE));
EEAR = uiAddress;          /* Set up address and Data Registers */
EEDR = ucData;
EECR |= (1<<EEMPE);        /* Write logical one to EEMPE */
EECR |= (1<<EEPE);         /* Start eeprom write by setting EEPE */
}

unsigned char EEPROM_read(unsigned int uiAddress)
{
	                          
	while(EECR & (1<<EEPE));  /* Wait for completion of previous write */
	EEAR = uiAddress;         /* Set up address register */
	EECR |= (1<<EERE);        /* Start eeprom read by writing EERE */
	return EEDR;              /* Return data from Data Register */
}