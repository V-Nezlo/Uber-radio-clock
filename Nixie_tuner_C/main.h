/*
 * main.h
 *
 * Created: 12.06.2021 16:33:02
 *  Author: V-Nezlo
 */ 


#ifndef MAIN_H_
#define MAIN_H_

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include "twi.h"
#include "rtc.h"
#include "si4730.h"
#include "eeprom.h"
#include "usart.h"

void flag_handler(void);
void freq_to_eeprom_transfer(char channel, uint16_t freq);
uint16_t eeprom_to_freq_transfer(char channel);
void eeprom_readfreqbank(void);
void encoder_procedure(char state);
void analog_button_procedure(void);
unsigned int ADC_Conversion(void);
char check_analog_button(void);
void check_time(void);
void ADC_init(void);
void timer1_init(void);
void timer0_init(void);
void timer2_init(void);
void port_init(void);
void setCathode(uint8_t num);
void show(uint8_t a[]);
void display(void);
void RTC_tweak(char what, char how);
void Radio_tune(char what, char how);


#endif /* MAIN_H_ */