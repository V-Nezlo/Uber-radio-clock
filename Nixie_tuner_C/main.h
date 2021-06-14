/*
 * main.h
 *
 * Created: 12.06.2021 16:33:02
 *  Author: Nikita
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

void encoder_procedure(char state);
char check_analog_button(void);
void RTC_tweak(char what, char how);
void Radio_tune(char what, char how);

#endif /* MAIN_H_ */