/*
Самописная либа для SI4730
Автор - V-Nezlo
*/

#ifndef SI4730_H_
#define SI4730_H_

#define SI4730_ADDR 0x63
#define SI4730_POWER_UP 0x01
#define SI4730_POWER_DOWN 0x11
#define SI4730_SET_PROPERTY 0x12
#define SI4730_ANALOGOUT 0x05
#define SI4730_SET_FM_FREQ 0x20
#define SI4730_SET_AM_FREQ 0x40
#define SI4730_HARDMUTE_H 0x40
#define SI4730_HARDMUTE_L 0x01
#define SI4730_SEEK_FM 0x21
#define SI4730_SEEK_AM 0x41
#define SI4730_SEEK_UP 0x0C
#define SI4730_SEEK_DOWN 0x04
#define SI4730_MODE_FM 0x50
#define SI4730_MODE_AM 0x51 

#include "main.h"

void si4730_powerup(void);
void si4730_SetFmFreq(uint16_t freq);
void si4730_SetAmFreq(uint16_t freq);
void si4730_seekfm(char upordown);
void si4730_seekam(char upordown);
void si4730_mute(char mute);
void si4730_powerdown(void);


#endif