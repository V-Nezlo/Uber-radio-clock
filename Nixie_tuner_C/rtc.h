/*
 * rtc.h
 *
 * Created: 12.03.2020 11:55:12
 *  Author: V.Nezlo
 */ 


#ifndef RTC_H_
#define RTC_H_

#include "main.h"

uint8_t RTC_ConvertFromBinDec(uint8_t c);
uint8_t RTC_ConvertFromDec(uint8_t c);
void RTC_Set(void);
void RTC_Read(void);

#endif /* RTC_H_ */