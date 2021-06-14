/*
Никсевый цифровой тюнер
Автор - V-Nezlo (vlladimirka@gmail.com)
Будильник нужно реализовывать внутри микроконтроллера, ds3231 поддерживает только 2 будильника в определенные дни недели
*/ 

#define RTCADRR 0b11010001
#define RTCADRW 0b11010000

#define MAXBRIGHT 20
#define INCREMENT 1
#define DECREMENT 2
#define SETZERO 3
#define SETMAX 4
#define MAXSUPFREQ 10800
#define MINSUPFREQ 6400
#define FREQEDITSTEP 10
//8970 - 89,7 MHz

#include "main.h"

struct Time {uint8_t hour, minute, second, day;} time; //в этих переменных храним время
struct Util {char setup_state, eachhoursignal_state; uint8_t bright, seconds;} utils; //утилитарные переменные
struct Alarm {uint8_t hour, minute, second; char isenabled ;uint8_t daystates[7];} alarm1; //тут храним все для будильника
struct Radio {uint16_t current_frequency; char stereo_state;} radio; //тут все для радио
struct But_flags {char mode, set, program;} but_flags; //ФЛАГИ для кнопок
struct Flags {char encoder_handler, mode, alarm_state, zummer;} flags; //общая библиотека флагов
enum Modes {CLOCK, SETHOUR, SETMINUTE, SETALARMHOUR, SETALARMMINUTE, SETDAY, DAYSTATE, BRIGHT, EACHHOURSIG, RADIO_MANUAL, STANDBY} selected_mode; //режимы отображения без подрежимов
enum Days  {MONDAY = 1, TUESDAY, WEDNESDAY, THURSDAY, FRIDAY, SATURDAY, SUNDAY} days; //перечисление дней для настройки непосредственно дней
uint8_t digits[4]; //служебная переменная для отображения разрядов

const uint8_t key1 = PC2;
const uint8_t key2 = PC3;
const uint8_t key3 = PC4;
const uint8_t key4 = PC5;
const uint8_t dot1 = PB0;
const uint8_t dot2 = PB2;

ISR (TIMER1_COMPA_vect)
{
	static unsigned char new_state=0;
	static unsigned char old_state=0;
	new_state = (PIND&0b01100000)>>5;
	switch(old_state | new_state)
	{
	  case 0x01: case 0x0e:
	  flags.encoder_handler = 1; //вперед
	  break;
	  case 0x04: case 0x0b:
	  flags.encoder_handler = 2; //назад
	  break;
	}
	old_state = new_state<<2;
	
	char pressedbutton = check_analog_button();
	switch (pressedbutton)
	{
		case 0:
		break;
		case 1:
		but_flags.mode = 1;
		break;
		case 2:
		but_flags.program = 1;
		break;
		case 3:
		but_flags.set = 1;
		break;
	}
}

ISR (TIMER0_COMPA_vect)
{
	utils.seconds++;
}

void flag_handler(void)
{
	if (flags.encoder_handler == 1)
	{
		encoder_procedure(1);
		flags.encoder_handler = 0;
	}
	else if (flags.encoder_handler == 2)
	{
		encoder_procedure(0);
		flags.encoder_handler = 0;
	}
	
}

void freq_to_eeprom_transfer2(char channel, uint16_t freq)
{
	unsigned char HFreqE = freq >> 8;
	unsigned char LFreqE = freq & 0x00FF;
	uint8_t channelAddr1 = channel*2;
	uint8_t channelAddr2 = channelAddr1 + 1;
	EEPROM_write(channelAddr1, LFreqE);
	EEPROM_write(channelAddr2, HFreqE);
}

uint16_t eeprom_to_freq_transfer(char channel)
{
	unsigned char HFreqE = 0; 
	unsigned char LFreqE = 0;
	uint8_t channelAddr1 = channel*2;
	uint8_t channelAddr2 = channelAddr1 + 1;
	LFreqE = EEPROM_read(channelAddr1);
	HFreqE = EEPROM_read(channelAddr2);
	uint16_t freq = LFreqE|(HFreqE<<8);
	return freq;
}

void encoder_procedure(char state)
{
	//смысл таков - если приходит 0 - происходит кручение влево, 1 - вправо
	//Вообще в этом участке кода мы будем отдельно обрабатывать движения энкодера
	if (selected_mode == RADIO_MANUAL)
	{
		if (state)
		{
			if (radio.current_frequency < MAXSUPFREQ) Radio_tune(1, INCREMENT);
			else Radio_tune(1, SETZERO);
		}
		else
		{
			if (radio.current_frequency > MINSUPFREQ) Radio_tune(1, DECREMENT);
			else Radio_tune(1, SETMAX);
		}
	}
	
	if (selected_mode == CLOCK)
	{
	//???? Не понятно
	}
	if (selected_mode == SETHOUR)
	{
		if (state)
		{
			if (time.hour < 23) RTC_tweak(1,INCREMENT);
			else RTC_tweak(1,SETZERO);
		}
		else
		{
			if (time.hour > 1) RTC_tweak(1, DECREMENT);
			else RTC_tweak(1, SETMAX);
		}
	}
	if (selected_mode == SETMINUTE)
	{
		if (state)
		{
			if (time.minute < 59) RTC_tweak(2,INCREMENT);
			else RTC_tweak(2,SETZERO);
		}
		else
		{
			if (time.minute > 1) RTC_tweak(2, DECREMENT);
			else RTC_tweak(2, SETMAX);
		}
	}
	if (selected_mode == SETALARMHOUR)
	{
		if (state)
		{
			if (alarm1.hour < 23) alarm1.hour++;
			else alarm1.hour = 0;
		}
		else
		{
			if (alarm1.hour > 1) alarm1.hour--;
			else alarm1.hour = 0;
		}
	}
	if (selected_mode == SETALARMMINUTE)
	{
		if (state)
		{
			if (alarm1.minute < 59) alarm1.minute++;
			else alarm1.minute = 0;
		}
		else
		{
			if (alarm1.minute > 1) alarm1.minute--;
			else alarm1.minute = 0;
		}
	}
	if (selected_mode == SETDAY)
	{
		if (state)
		{
			if (time.day < 6) RTC_tweak(3, INCREMENT);
			else RTC_tweak(3, SETZERO);
		}
		else
		{
			if (time.day > 1) RTC_tweak(3, DECREMENT);
			else RTC_tweak(3, SETMAX);
		}
	}
	if (selected_mode == DAYSTATE)//Вправо покрутили - установили будильник, влево - выключили
	{
		uint8_t i = days; //узнали в каком мы сейчас дне
		if (state) alarm1.daystates[i] = 1;
		else alarm1.daystates[i] = 0;
	}
	if (selected_mode == BRIGHT)
	{
		if (state)
		{
			if (utils.bright < MAXBRIGHT) utils.bright++;
			else utils.bright = 0;
		}
		else
		{
			if (utils.bright > 1) utils.bright--;
			else utils.bright = MAXBRIGHT;
		}
	}
	if (selected_mode == EACHHOURSIG)
	{
		if (state) utils.eachhoursignal_state = 1;
		else utils.eachhoursignal_state = 0;
	}
}

unsigned int ADC_Conversion(void)
{
	ADCSRA |= (1<<ADSC); //начинаем преобразование
	while((ADCSRA & (1<<ADSC))); //при окончании преобразования пишет 0 в бит
	return (unsigned int) ADC;
}

char check_analog_button(void){
	unsigned int analog_value = ADC_Conversion();
	if (analog_value <= 100) return 0;
	if ((analog_value >= 100)&(analog_value <= 300))   return 1;
	if ((analog_value >= 300)&(analog_value <= 600))   return 2;
	if ((analog_value >= 500)&(analog_value <= 800))   return 3;
	if ((analog_value >= 1000)&(analog_value <= 1100)) return 4;
	else return 0;
}

char check_time(void)
{
	if ((alarm1.hour == time.hour)&(alarm1.minute == time.minute)&(alarm1.second == time.second)) //совпали часы, минуты, секунды
	{
		uint8_t i = time.day; //присваиваем переменной i число, соответствующее дню недели
		if (alarm1.daystates[i]) //если в соответсвующей ячейке с будильниками находится единица, то есть будильник включен
		{
			flags.alarm_state = 1; //ставится флаг сработавшего будильника
		}
	}
}

void ADC_init(void)
{
	ADCL = 0x00;
	ADCH = 0x00;
	ADMUX = 0x00;  //Опорное напряжение 5В, нормальное направление данных, мультиплексор на ADC0
	ADCSRA |= (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); //АЦП вкл, делитель на 128 (корректировать)
}

void timer1_init(void) //16 битный таймер на энкодер и пару периферий
{
	OCR1A = 25; //интервал в 100 микросекунд (10 кГц) при 16 мГц
	TCCR1B |= (1 << WGM12);  //CTC
	TCCR1B |= (1 << CS11) | (1 << CS10); //для 10 кГц
	TIMSK1 |= (1 << OCIE1A); //включено прерывание
}

void timer0_init(void) //этот таймер будет считать секунды
{
	TCCR0A |= (1<<WGM01);
	TCCR0B |= (1<<CS02)|(1<<CS00);
	OCR0A = 0xFF;
	TIMSK0 |= (1<<OCIE0A);
}

void port_init(void)
{
	DDRB = 0xFF;
	DDRD = 0xFF;
	DDRC = 0xFF;
	PORTB = 0x00;
	PORTD = 0x00;
	PORTC = 0x00;
}

void setCathode(uint8_t num)
{
	switch(num)
	{
		case 0:
		PORTD |= (1<<PD0)|(1<<PD1);
		PORTD &=~(1<<PD2)|(1<<PD3);
		break;
		case 1:
		PORTD |= (1<<PD0)|(1<<PD1);
		PORTD &=~(1<<PD2)|(1<<PD3);
		break;
		case 2:
		PORTD |= (1<<PD0)|(1<<PD1);
		PORTD &=~(1<<PD2)|(1<<PD3);
		break;
		case 3:
		PORTD |= (1<<PD0)|(1<<PD1);
		PORTD &=~(1<<PD2)|(1<<PD3);
		break;
		case 4:
		PORTD |= (1<<PD0)|(1<<PD1);
		PORTD &=~(1<<PD2)|(1<<PD3);
		break;
		case 5:
		PORTD |= (1<<PD0)|(1<<PD1);
		PORTD &=~(1<<PD2)|(1<<PD3);
		break;
		case 6:
		PORTD |= (1<<PD0)|(1<<PD1);
		PORTD &=~(1<<PD2)|(1<<PD3);
		break;
		case 7:
		PORTD |= (1<<PD0)|(1<<PD1);
		PORTD &=~(1<<PD2)|(1<<PD3);
		break;
		case 8:
		PORTD |= (1<<PD0)|(1<<PD1);
		PORTD &=~(1<<PD2)|(1<<PD3);
		break;
		case 9:
		PORTD |= (1<<PD0)|(1<<PD1);
		PORTD &=~(1<<PD2)|(1<<PD3);
		break;
	}
}

void show(uint8_t a[]){
	uint8_t keys[] = { key1, key2, key3, key4 };
	for (uint8_t i=0; i<=4; ++i){
		PORTC |= (1<<(keys[i]));
		setCathode(a[i]);
		_delay_ms(1);
		PORTC &= ~(1<<(keys[i]));
	}
}

void display(void)
{
		if (selected_mode == CLOCK)
		{
			digits[0] = time.hour/100;
			digits[1] = time.hour%100/10;
			digits[2] = time.minute%10;
			digits[3] = time.minute%100/10;
		}
		if (selected_mode == SETHOUR)
		{
			digits[0] = time.hour/100;
			digits[1] = time.hour%100/10;
			digits[2] = 0;
			digits[3] = 0;
		}
		if (selected_mode == SETMINUTE)
		{
			digits[0] = 0;
			digits[1] = 0;
			digits[2] = time.minute%10;
			digits[3] = time.minute%100/10;
		}
		if (selected_mode == SETALARMHOUR)
		{
			digits[0] = alarm1.hour/100;
			digits[1] = alarm1.hour%100/10;
			digits[2] = 0;
			digits[3] = 0;
		}
		if (selected_mode == SETALARMMINUTE)
		{
			digits[0] = 0;
			digits[1] = 0;
			digits[2] = alarm1.minute%10;
			digits[3] = alarm1.minute%100/10;
		}
		if (selected_mode == SETDAY)
		{
			digits[0] = 0;
			digits[1] = 0;
			digits[2] = 0;
			digits[3] = time.day;
		}
		if (selected_mode == DAYSTATE) //ВНИМАНИЕ ВОТ ТУТ ПОДРЕЖИМЫ РАБОТАЮТ, ПРИ ПЕРЕХОДЕ НЕ МЕНЯТЬ selected_mode а менять days, повторное использование кода потому что
		{
			uint8_t i = days; //спрашиваю какой сейчас день
			digits[0] = days;
			digits[1] = 0;
			digits[2] = 0;
			digits[3] = alarm1.daystates[i]; //вывожу стейтмент будильника для этого дня
		}
		if (selected_mode == BRIGHT)
		{
			digits[0] = 8;
			digits[1] = 0;
			digits[2] = utils.bright%10;
			digits[3] = utils.bright%10/10;
		}
		if (selected_mode == EACHHOURSIG)
		{
			digits[0] = 9;
			digits[1] = 0;
			digits[2] = 0;
			digits[3] = utils.eachhoursignal_state;
		}
		if (selected_mode == RADIO_MANUAL)
		{
			digits[0] = radio.current_frequency/100; //все настроить надобно
			digits[1] = radio.current_frequency%100/10;//
			digits[2] = radio.current_frequency%10;//
			digits[3] = radio.current_frequency%100/10;//п
		}
}

void RTC_tweak(char what, char how)//what - 1 - часы, 2 - минуты, 3 - день, how - 1 - увеличить, 2 - уменьшить, 3 - обнулить, 4 - задать максимальное значение
{
	I2C_StartCondition();
	I2C_SendByte(RTCADRW);
	
	if (what == 1)
	{
		I2C_SendByte(0x02);
		if (how == INCREMENT) I2C_SendByte(RTC_ConvertFromBinDec(time.hour+1));
		else if (how == DECREMENT) I2C_SendByte(RTC_ConvertFromBinDec(time.hour-1));
		else if (how == SETZERO) I2C_SendByte(RTC_ConvertFromBinDec(0));
		else if (how == SETMAX) I2C_SendByte(RTC_ConvertFromBinDec(23));
	}
	if (what == 2)
	{
		I2C_SendByte(0x00);
		I2C_SendByte(RTC_ConvertFromBinDec(0));//sec = 0;
		if (how == INCREMENT) I2C_SendByte(RTC_ConvertFromBinDec(time.minute+1));
		else if (how == DECREMENT) I2C_SendByte(RTC_ConvertFromBinDec(time.minute-1));
		else if (how == SETZERO) I2C_SendByte(RTC_ConvertFromBinDec(0));
		else if (how == SETMAX) I2C_SendByte(RTC_ConvertFromBinDec(59));
	}
	if (what == 3)
	{
		I2C_SendByte(0x03);
		if (how == INCREMENT) I2C_SendByte(RTC_ConvertFromBinDec(time.day+1));
		else if (how == DECREMENT) I2C_SendByte(RTC_ConvertFromBinDec(time.day-1));
		else if (how == SETZERO) I2C_SendByte(RTC_ConvertFromBinDec(1));
		else if (how == SETMAX) I2C_SendByte(RTC_ConvertFromBinDec(7));
	}
	I2C_StopCondition();
}

void Radio_tune(char what, char how)
{
	if (what == 1)
	{
		if (how == INCREMENT) si4730_SetAmFreq(radio.current_frequency + FREQEDITSTEP);
		else if (how == DECREMENT) si4730_SetAmFreq(radio.current_frequency - FREQEDITSTEP);
		else if (how == SETZERO) si4730_SetAmFreq(MINSUPFREQ);
		else if (how == SETMAX) si4730_SetAmFreq(MAXSUPFREQ);
	}
}

int main(void)
{
	selected_mode = CLOCK;
	port_init();
	timer1_init();
	timer0_init();
    I2C_Init();
	ADC_init();
	si4730_powerup();

    while (1) 
    {
		//RTC_Read(); - раз в полсекунды надо проверять время а не в основном цикле
		display();
		flag_handler();
    }
}
 