/*
Uber radio clock
Autor - V-Nezlo (vlladimirka@gmail.com)
*/ 

#define RTCADRR 0b11010001
#define RTCADRW 0b11010000

#define MAXBRIGHT 20
#define INCREMENT 1
#define DECREMENT 2
#define SETZERO 3
#define SETMAX 4
#define MAXSUPFREQ 10800
#define MINSUPFREQ 7600
#define FREQEDITSTEP 10
#define EACHHOURSIG_MINHOUR 6
#define EACHHOURSIG_MAXHOUR 21
//8970 - 89,7 MHz

#include "main.h"

struct Time {uint8_t hour, minute, second, day;} time; // временнЫе переменные
struct Util {char eachhoursignal_state,ledprogram_state, deletefreqconf; uint8_t bright, seconds; uint8_t digits[4];} utils; //утилитарные переменные
struct Alarm {uint8_t hour, minute, second; char isenabled ;uint8_t daystates[7];} alarm1; //только 
struct Radio {uint16_t current_frequency; uint16_t bankfreq[10]; uint8_t currentbankfreq; uint8_t freqbankstate[10];} radio; //переменные для радио
struct But_flags {char mode, set, program;} but_flags; //флаги нажатия клавиш
struct Flags {char encoder_handler, mode, alarm_state, zummer, eachhoursignal, ledprogram, timetocheckbutton, timeforreadrtc;} flags; //общие флаги
enum Modes {CLOCK = 1, SETHOUR, SETMINUTE, SETALARMHOUR, SETALARMMINUTE, SETDAY, DAYSTATE, BRIGHT, EACHHOURSIG, RADIO_MANUAL, RADIO_PROGRAM, RADIO_SETPROGFREQ, STANDBY} selected_mode; //только режимы, без подрежимов
enum Days  {MONDAY = 1, TUESDAY, WEDNESDAY, THURSDAY, FRIDAY, SATURDAY, SUNDAY} days; //для подрежима дней

const uint8_t key1 = PD4;
const uint8_t key2 = PD5;
const uint8_t key3 = PD6;
const uint8_t key4 = PD7;
const uint8_t dot1 = PB4;
const uint8_t dot2 = PB5;

ISR (TIMER1_COMPA_vect)
{
	volatile static unsigned char new_state=0;
	volatile static unsigned char old_state=0;
	new_state = (PINC&0b00001100)>>2;
	switch(old_state | new_state) //проверяем пины энкодера
	{
	  case 0x01: case 0x0e:
	  flags.encoder_handler = 1; //крутится вправо
	  break;
	  case 0x04: case 0x0b:
	  flags.encoder_handler = 2; //крутится влево
	  break;
	}
	old_state = new_state<<2; //откатываемся
}



ISR (TIMER0_COMPA_vect)
{	
	volatile static uint8_t timer2_comparator;
	if (timer2_comparator <= 60) ++timer2_comparator; //таймер на 1 секунду, сделанный на двух таймерах
	else
	{
		if (timer2_comparator == 30) //раз в полсекунды 
		{
			flags.timeforreadrtc = 1; //ставим флаг необходимости прочитать RTC
			check_time();			//проверяем будильники, каждочасный сигнал и прочее
		} 
		if (utils.seconds < 59) ++utils.seconds; //а тут именно секунды
		else utils.seconds = 0;
		
		flags.timetocheckbutton = 1; //поставим флаг проверки аналоговых кнопок
		
		if ((selected_mode == CLOCK)&(alarm1.isenabled)) //при включенном будильнике - работает шиммирование светодиода
		{
			uint8_t pwm_counter = OCR2B;
			volatile static char direction = 0;
			if ((pwm_counter < 255)&(direction == 0)) OCR2B = OCR2B + 1;
			else if ((pwm_counter < 255)&(direction == 1)) OCR2B = OCR2B - 1;//вверх или вниз
			if ((pwm_counter == 255)&(direction == 0)) direction = 1;
			else if ((pwm_counter == 255)&(direction == 1)) direction = 0;
		}
		else OCR2B = 0xFF; //в других случаях светодиод просто включен

		timer2_comparator = 0;
	}

}

void flag_handler(void) //глобальный обработчик флагов
{
	//UTILITARY SECONDS
	if (utils.seconds == 7) //если бездействие 7 секунд
	{
		if ((selected_mode > 1)&(selected_mode <= 9)) //если в первых девяти режимах
		{
			selected_mode = 1; //возвращаемся в режим часов
			utils.seconds = 0; //сбрасываем счетчик на 7
		}
		else if (selected_mode == RADIO_PROGRAM) //если в режиме выбора банков - возвращаемся в мануальный
		{
			selected_mode = RADIO_MANUAL;
			utils.seconds = 0;
		}
	}
	//UTILITARY SECONDS

	if (flags.timetocheckbutton == 1)
	{
		analog_button_procedure();
		flags.timetocheckbutton = 0;
	}
	
	if (flags.timeforreadrtc) //читаем РТС
	{
		RTC_Read();
		flags.timeforreadrtc = 0;
	}
	if (flags.eachhoursignal) //отыгрываем сигнал раз в час если стоит нужный флаг
	{
		PORTC |= (1<<PC1);
		_delay_ms(500);
		PORTC &= ~(1<<PC1);
		flags.eachhoursignal = 0;
	}

	//ENDCODER HANDLER
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
	//ENCODER HANDLER
	//PRESSED SET BUTTON
	if (but_flags.set == 1) 
	{
		if ((selected_mode < 9)&(selected_mode != DAYSTATE)) //переключение режимов внутри подраздела часов
		{
			++selected_mode;
			utils.seconds = 0;
		}
		else if (selected_mode == DAYSTATE) //переключение подрежимов в режиме настройки будильников по дням
		{
			if (days < 7)
			{
				++days;
			}
			else if (days == 7) //ушли в режим настройки яркости
			{
				selected_mode == BRIGHT;
				days = 1;
			}
		}
		else if (selected_mode == EACHHOURSIG) 
		{
			selected_mode = CLOCK;
			utils.seconds = 0;
		}
		else if (selected_mode == RADIO_MANUAL)
		{
			selected_mode = RADIO_SETPROGFREQ;
			radio.currentbankfreq = 0;
			utils.seconds = 0;
		}
		else if ((selected_mode == RADIO_PROGRAM)&(utils.deletefreqconf == 0))
		{
			utils.deletefreqconf = 1;
			utils.seconds = 0;
		}
		else if ((selected_mode == RADIO_PROGRAM)&(utils.deletefreqconf == 1)) //удаление ячейки подтверждено
		{
			radio.bankfreq[radio.currentbankfreq] = 0;         //обнулили ячейку в памяти
			radio.freqbankstate[radio.currentbankfreq] = 0;    //обнулили стейтмент ячейки в памяти
			freq_to_eeprom_transfer(radio.currentbankfreq, 0); //обнулили ячейку в еепроме
			utils.deletefreqconf = 0;						   //обнулили флаг удаления
			utils.seconds = 0;								   //сбросили таймер
			selected_mode = RADIO_MANUAL;					   //вернулись в режим мануал
		}
		else if (selected_mode == RADIO_SETPROGFREQ) //запись в память
		{
			radio.bankfreq[radio.currentbankfreq] = radio.current_frequency;		//записали в память
			freq_to_eeprom_transfer(radio.currentbankfreq, radio.current_frequency);//записали в еепром
			selected_mode = RADIO_PROGRAM;											//вернулись в режим программ
		}
		but_flags.set = 0; //clear flag
	}
	// PRESSED SET BUTTON
	// PRESSED MODE BUTTON
	else if (but_flags.mode == 1) 
	{
		if (selected_mode <= 9)
		{
			selected_mode = RADIO_MANUAL;
			radio.current_frequency = eeprom_to_freq_transfer(11);
			Radio_tune(2, 0); //включаем выход радио
		}
		but_flags.mode = 0; //clear flag
	}
	else if (selected_mode == RADIO_MANUAL)
	{	
		uint16_t currenteepromfreq = eeprom_to_freq_transfer(11);
		if (currenteepromfreq != radio.current_frequency) freq_to_eeprom_transfer(11, radio.current_frequency);
		Radio_tune(2, 1); //выключаем выход радио
	}

	// PRESSED MODE BUTTON
	// PRESSED PROGRAM BUTTON
	else if (but_flags.program == 1) 
	{
		if (selected_mode == RADIO_MANUAL) 
		{
			for (uint8_t b = 0; b < 10; ++b) //проверяем состояние банка частот
			{
				if (radio.freqbankstate[b] == 1) //если там что-то есть
				{
					selected_mode = RADIO_PROGRAM;	//переключаемся на этот банк
					radio.currentbankfreq = b;
					break;
				}
				if (b == 9) selected_mode == RADIO_MANUAL;	//иначе идем на три буквы
			}
			radio.currentbankfreq = 0;
			freq_to_eeprom_transfer(11, radio.current_frequency); //храним текущее накрученное значение частоты для сохранения в банк или возврата при перезагрузке
		}
		else if (selected_mode == RADIO_PROGRAM)
		{
			selected_mode = RADIO_MANUAL;
		}
		but_flags.program = 0; 
	}
	//PRESSED PROGRAM BUTTON
	if ((flags.ledprogram)&(selected_mode == RADIO_PROGRAM))
	{
		utils.ledprogram_state = 1;
		flags.ledprogram = 0;
	}
	else if ((flags.ledprogram)&(selected_mode == RADIO_SETPROGFREQ))
	{
		utils.ledprogram_state = 2;
		flags.ledprogram = 0;
	}
	else if (flags.ledprogram == 0) utils.ledprogram_state = 0;

	if (utils.ledprogram_state == 1) PORTD |= (1<<PD2);
	else if (utils.ledprogram_state == 0) PORTD &= ~(1<<PD2);
	else if (utils.ledprogram_state == 2)
	{

		if ((utils.seconds % 2) == 0) PORTD |= (1<<PD2); //нехитрый способ мигать раз в секунду
		else PORTD &= ~(PD2);
	}
}

void freq_to_eeprom_transfer(char channel, uint16_t freq) //функция записывает в нужный канал нужную частоту
{
	cli();
	unsigned char HFreqE = freq >> 8;
	unsigned char LFreqE = freq & 0x00FF;
	
	uint8_t channelAddr1;
	uint8_t channelAddr2;
	
	if (channel == 0) channelAddr1 = 1;
	else channelAddr1 = channel*2;
	channelAddr2 = channelAddr1 + 1;
	EEPROM_write(channelAddr1, LFreqE);
	EEPROM_write(channelAddr2, HFreqE);
	sei();
}

uint16_t eeprom_to_freq_transfer(char channel) //возвращает частоту из нужного канала
{
	cli();
	unsigned char HFreqE = 0; 
	unsigned char LFreqE = 0;
	
	uint8_t channelAddr1;
	uint8_t channelAddr2;
	
	if (channel == 0) channelAddr1 = 1; //элегантно заполняем ячейки 1 и 2
	else channelAddr1 = channel*2;
	channelAddr2 = channelAddr1 + 1;
	LFreqE = EEPROM_read(channelAddr1);
	HFreqE = EEPROM_read(channelAddr2);
	uint16_t freq = LFreqE|(HFreqE<<8);
	sei();
	return freq;

}

void eeprom_readfreqbank(void)
{
	uint8_t a = 0;
	radio.currentbankfreq = 0;
	for (a = 0; a <= 9; ++a) //нагло берем частоты в банк памяти из банка еепром
	{
		radio.bankfreq[a] = eeprom_to_freq_transfer(a);			//записали в банк
		if (radio.bankfreq[a] == 0) radio.freqbankstate[a] = 0; //проверили, пуст ли банк, чтобы к нему не обращаться
	}
}

void encoder_procedure(char state)
{
	//Процедуратор энкодера, 1 - крутим вправо, 0 -крутим влево
	//Тут все связанное с энкодером внутри режимов
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
	
	if (selected_mode == CLOCK) //в этом режиме энкодер включает - выключает будильник
	{
	 if (state) alarm1.isenabled = 1;
	 else alarm1.isenabled = 0;
	}
	else if (selected_mode == SETHOUR) //в этом режиме происходит инкремент-декремент часов
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
	else if (selected_mode == SETMINUTE) //инкремент-декремент минут
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
	else if (selected_mode == SETALARMHOUR) //установка часов в будильнике
	{
		if (state)
		{
			if (alarm1.hour < 23) ++alarm1.hour;
			else alarm1.hour = 0;
		}
		else
		{
			if (alarm1.hour > 1) --alarm1.hour;
			else alarm1.hour = 0;
		}
	}
	else if (selected_mode == SETALARMMINUTE) //установка минут в будильнике
	{
		if (state)
		{
			if (alarm1.minute < 59) ++alarm1.minute;
			else alarm1.minute = 0;
		}
		else
		{
			if (alarm1.minute > 1) --alarm1.minute;
			else alarm1.minute = 0;
		}
	}
	else if (selected_mode == SETDAY) //установка текущего дня
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
	else if (selected_mode == DAYSTATE)//а вот тут подрежимы, которые мы переключаем кнопкой энкодера
	{
		if (state) alarm1.daystates[days] = 1;
		else alarm1.daystates[days] = 0;
	}
	else if (selected_mode == BRIGHT) //установка "яркости"
	{
		if (state)
		{
			if (utils.bright < MAXBRIGHT) ++utils.bright;
			else utils.bright = 0;
		}
		else
		{
			if (utils.bright > 1) --utils.bright;
			else utils.bright = MAXBRIGHT;
		}
	}
	else if (selected_mode == EACHHOURSIG) //вкл - выкл сигнал каждый час
	{
		if (state) utils.eachhoursignal_state = 1;
		else utils.eachhoursignal_state = 0;
	}
	else if (selected_mode == RADIO_PROGRAM) //самая веселая часть, скачем как кабан между банками в поиске данных и вперед и назад
	{
		if (state)
		{
			if (radio.currentbankfreq < 9) 
			{
				uint8_t nextbank = radio.currentbankfreq + 1;
				for (nextbank; nextbank <= 9; ++nextbank)
				{
					if (radio.freqbankstate[nextbank])
					{
						radio.currentbankfreq = nextbank;
						break;
					}
				}
				if (radio.currentbankfreq == 9)
				{
					for (uint8_t c = 0; c < 9; ++c )
					{
						if (radio.freqbankstate[c])
						{
							radio.currentbankfreq = c;
							break;
						}
					}
				}
			}
		}
		else
		{
			if (radio.currentbankfreq != 0)
			{
				uint8_t prevbank = radio.currentbankfreq - 1;
				for (prevbank; prevbank != 0; --prevbank)
				{
					if (radio.freqbankstate[prevbank])
					{
						radio.currentbankfreq = prevbank;
						break;
					}
				}
			}
			else if (radio.currentbankfreq == 0)
			{
				for (uint8_t a = 9;a != 0; --a)
				{
					if (radio.freqbankstate[a])
					{
						radio.currentbankfreq = a;
						break;
					}
				}
			}
		}
	}
	else if (selected_mode == RADIO_SETPROGFREQ)
	{
		if (state)
		{
			if (radio.currentbankfreq)
			{
				if (radio.currentbankfreq < 9) ++radio.currentbankfreq;
				else radio.currentbankfreq = 0;
			}
		}
		else
		{
			if (radio.currentbankfreq > 0) --radio.currentbankfreq;
			else if (radio.currentbankfreq == 0) radio.currentbankfreq = 9;
		}
	}
}

unsigned int ADC_Conversion(void)
{
	ADCSRA |= (1<<ADSC); //5В референсное напряжение, MUX0, ADC0
	while((ADCSRA & (1<<ADSC))); //ждем конца преобразования
	return (unsigned int) ADC;
}

char check_analog_button(void){ //смотрим на напряжения и решаем, какая из кнопок нажата
	unsigned int analog_value = ADC_Conversion(); 
	if (analog_value <= 100) return 0; //если напряжение меньше 100 - выходим
	else if ((analog_value >= 100)&(analog_value <= 600))   return 3;
	else if ((analog_value >= 600)&(analog_value <= 800))   return 2;
	else if (analog_value >= 800)						    return 1;
}

void analog_button_procedure(void) //ставим флаги нажатых кнопок при необходимости
{
	char pressedbutton = check_analog_button(); 
	switch (pressedbutton)
	{
		case 0:
		break;
		case 1:
		but_flags.set = 1;
		break;
		case 2:
		but_flags.mode = 1;
		break;
		case 3:
		but_flags.program = 1;
		break;
	}
}

void check_time(void)
{
	if ((alarm1.hour == time.hour)&(alarm1.minute == time.minute)&(alarm1.second == time.second)&(alarm1.isenabled)) //если часы, минуты, секунды совпадают и будильник включен
	{
		if (alarm1.daystates[time.day]) //проверяем, нужный ли день
		{
			flags.alarm_state = 1; //флаг будильника ставим
		}
	}
	if ((utils.eachhoursignal_state)&(time.minute==0)&(time.second==0)&(time.hour > EACHHOURSIG_MINHOUR)&(time.hour < EACHHOURSIG_MAXHOUR)) flags.eachhoursignal = 1; //кричим в зуммер при каждом часе

}

void ADC_init(void)
{
	ADCL = 0x00;
	ADCH = 0x00;
	ADMUX = 0x00;  // ADC0
	ADCSRA |= (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); //включили-поделили
}

void timer1_init(void) // таймер для энкодера и всякой мелочи
{
	OCR1A = 25; //100кгц таймер получается
	TCCR1B |= (1 << WGM12);  //CTC
	TCCR1B |= (1 << CS11) | (1 << CS10); //нужный делитель
	TIMSK1 |= (1 << OCIE1A); //включили прерывание
}

void timer0_init(void) //таймер для секунд
{
	TCCR0A |= (1<<WGM01);
	TCCR0B |= (1<<CS02)|(1<<CS00); //prescaler 1024
	OCR0A = 0xFF;
	TIMSK0 |= (1<<OCIE0A); //прерывание для секунд
}

void timer2_init(void)
{
	TCCR2A |= (1<<COM2B1)|(1<<WGM21)|(1<<WGM20);
	TCCR2B |= (1<<CS21)|(1<<CS22)|(1<<CS20); //15кгц частота
	OCR2B = 0x00;
	TCNT2 = 0x00;
	TIMSK2 &= ~(OCIE2B); //выключили прерывание, этот таймер только для шим
}

void port_init(void) //направление и инициация портов
{
	DDRB = 0xFF; // all to output
	DDRD = 0xFF; // all to output
	DDRC = 0b00110010; 
	PORTB = 0x00;
	PORTD = 0x00;
	PORTC = 0x00;
}

void setCathode(uint8_t num)
{
	switch(num)
	{
		case 0:
		PORTD &= ~(1<<PD0);
		PORTD &= ~(1<<PD1);
		PORTD |= (1<<PD2);
		PORTD |= (1<<PD3);
		break;
		case 1:
		PORTD &= ~(1<<PD0);
		PORTD &= ~(1<<PD1);
		PORTD |= (1<<PD2);
		PORTD &= ~(1<<PD3);
		break;
		case 2:
		PORTD &= ~(1<<PD0);
		PORTD |= (1<<PD1);
		PORTD |= (1<<PD2);
		PORTD |= (1<<PD3);
		break;
		case 3:
		PORTD &= ~(1<<PD0);
		PORTD |= (1<<PD1);
		PORTD &= ~(1<<PD2);
		PORTD &= ~(1<<PD3);
		break;
		case 4:
		PORTD &= ~(1<<PD0);
		PORTD &= ~(1<<PD1);
		PORTD &= ~(1<<PD2);
		PORTD |= (1<<PD3);
		break;
		case 5:
		PORTD |= (1<<PD0);
		PORTD &= ~ (1<<PD1);
		PORTD &= ~(1<<PD2);
		PORTD |= (1<<PD3);
		break;
		case 6:
		PORTD |= (1<<PD0);
		PORTD &= ~(1<<PD1);
		PORTD &= ~(1<<PD2);
		PORTD &= ~(1<<PD3);
		break;
		case 7:
		PORTD &= ~(1<<PD0);
		PORTD &= ~(1<<PD1);
		PORTD &= ~(1<<PD2);
		PORTD &= ~(1<<PD3);
		break;
		case 8:
		PORTD &= ~(1<<PD0);
		PORTD |= (1<<PD1);
		PORTD &= ~(1<<PD2);
		PORTD |= (1<<PD3);
		break;
		case 9:
		PORTD &= ~(1<<PD0);
		PORTD |= (1<<PD1);
		PORTD |= (1<<PD2);
		PORTD &= ~(1<<PD3);
		break;
	}
}

void show(uint8_t a[]){
	uint8_t keys[] = { key1, key2, key3, key4 };
	for (uint8_t i=0; i<=4; ++i){
		PORTD &= ~(1<<(keys[i]));
		setCathode(a[i]);
		_delay_ms(1); //Тут должна быть BRIGHT, но ее нет, а почему? А потому что аргумент delay - compile-time переменная(константная), 
		// и runtime-переменную туда не положить. И что делать? Муравью х** приделать. А если серьезно то нужно завести еще один таймер, благо
		// таймер2 свободен, настроить его на 1 миллисекунду и считать им, на финальном этапе добавим
		PORTD |= (1<<(keys[i]));
	}
}

void display(void)
{
		if (selected_mode == CLOCK) //показываем время
		{
			utils.digits[0] = time.hour/100;
			utils.digits[1] = time.hour%100/10;
			utils.digits[2] = time.minute%10;
			utils.digits[3] = time.minute%100/10;
		}
		if (selected_mode == SETHOUR) //настраиваем часы
		{
			utils.digits[0] = time.hour/100;
			utils.digits[1] = time.hour%100/10;
			utils.digits[2] = 0;
			utils.digits[3] = 0;
		}
		if (selected_mode == SETMINUTE) //настраиваем минуты
		{
			utils.digits[0] = 0;
			utils.digits[1] = 0;
			utils.digits[2] = time.minute%10;
			utils.digits[3] = time.minute%100/10;
		}
		if (selected_mode == SETALARMHOUR) //настраиваем часы будильника
		{
			utils.digits[0] = alarm1.hour/100;
			utils.digits[1] = alarm1.hour%100/10;
			utils.digits[2] = 0;
			utils.digits[3] = 0;
		}
		if (selected_mode == SETALARMMINUTE) //настраиваем минуты будильника
		{
			utils.digits[0] = 0;
			utils.digits[1] = 0;
			utils.digits[2] = alarm1.minute%10;
			utils.digits[3] = alarm1.minute%100/10;
		}
		if (selected_mode == SETDAY) //настраиваем текущий день
		{
			utils.digits[0] = 0;
			utils.digits[1] = 0;
			utils.digits[2] = 0;
			utils.digits[3] = time.day;
		}
		if (selected_mode == DAYSTATE) //подрежимы, инкремент не selected_mode а days, переключаем по очереди от 1 до 7
		{
			utils.digits[0] = days;
			utils.digits[1] = 0;
			utils.digits[2] = 0;
			utils.digits[3] = alarm1.daystates[days]; //выводим статус этого дня
		}
		if (selected_mode == BRIGHT) //настройка типа яркости
		{
			utils.digits[0] = 8;
			utils.digits[1] = 0;
			utils.digits[2] = utils.bright%10;
			utils.digits[3] = utils.bright%10/10;
		}
		if (selected_mode == EACHHOURSIG) //вкл-выкл звук каждый час
		{
			utils.digits[0] = 9;
			utils.digits[1] = 0;
			utils.digits[2] = 0;
			utils.digits[3] = utils.eachhoursignal_state;
		}
		if (selected_mode == RADIO_MANUAL) //режим отображения радио - мануальный - показываем текущую частоту
		{
			utils.digits[0] = radio.current_frequency/1000;
			utils.digits[1] = radio.current_frequency%1000/100;
			utils.digits[2] = radio.current_frequency%100/10;
			utils.digits[3] = radio.current_frequency%10;
		}
		if (selected_mode == RADIO_PROGRAM) //тут у нас банки памяти
		{
			utils.digits[0] = radio.bankfreq[radio.currentbankfreq]/1000;
			utils.digits[1] = radio.bankfreq[radio.currentbankfreq]%1000/100;
			utils.digits[2] = radio.bankfreq[radio.currentbankfreq]%100/10;
			utils.digits[3] = radio.bankfreq[radio.currentbankfreq]%10;
		}
		if (selected_mode == RADIO_SETPROGFREQ) 
		{
			if (radio.freqbankstate[radio.currentbankfreq])
			{
				utils.digits[0] = radio.bankfreq[radio.currentbankfreq]/1000;
				utils.digits[1] = radio.bankfreq[radio.currentbankfreq]%1000/100;
				utils.digits[2] = radio.bankfreq[radio.currentbankfreq]%100/10;
				utils.digits[3] = radio.bankfreq[radio.currentbankfreq]%10;
			}
			else
			{
				utils.digits[0] = radio.currentbankfreq;
				utils.digits[1] = 0;
				utils.digits[2] = 0;
				utils.digits[3] = 0;
			}
		}
		
		show(utils.digits);
}

void RTC_tweak(char what, char how)//what - 1 - hour, 2 - minute, 3 - day
{
	cli();
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
	sei();
}

void Radio_tune(char what, char how) //what - 1 - freq, 2 -mutestae; how - 1 - muteset, 0 - muteunset
{
	cli();
	if (what == 1)
	{
		if (how == INCREMENT)
		{
			si4730_SetAmFreq(radio.current_frequency + FREQEDITSTEP);
			radio.current_frequency = radio.current_frequency + FREQEDITSTEP;
		} 
		else if (how == DECREMENT)
		{
			si4730_SetAmFreq(radio.current_frequency - FREQEDITSTEP);
			radio.current_frequency = radio.current_frequency - FREQEDITSTEP;
		}
		else if (how == SETZERO)
		{
			si4730_SetAmFreq(MINSUPFREQ);
			radio.current_frequency = MINSUPFREQ;
		} 
		else if (how == SETMAX)
		{
			si4730_SetAmFreq(MAXSUPFREQ);
			radio.current_frequency = MAXSUPFREQ;
		} 
	}
	else if (what == 2)
	{
		if (how == 1) si4730_mute(1);
		else si4730_mute(0);
	}
	sei();
}

void variable_init(void) //на всякий пожарный проинициализируем все элементы в структурах
{
	//это трогать не надо, совсем
	flags = (struct Flags) {0, 0, 0, 0, 0, 0, 0, 0};
	utils = (struct Util)  {0, 0, 0, 2, 0, {0,0,0,0}};
	alarm1 = (struct Alarm) {0, 0, 0, 0, {0,0,0,0,0,0,0}};
	radio = (struct Radio) {8970, {0,0,0,0,0,0,0,0,0,0}, 0, {0,0,0,0,0,0,0,0,0,0}};
	but_flags = (struct But_flags) {0, 0, 0};
	selected_mode = CLOCK;
	days = MONDAY;
}

int main(void)
{
	port_init(); //проинициазировали порты
	timer1_init(); //включили таймеры 0 и 1
	timer0_init();
    I2C_Init(); //включили i2c
	USART_Init(6); //включили юарт на скорости 6(см. в даташит)
	ADC_init(); //включили ацп
	si4730_powerup(); //включили радиомикросхему
	eeprom_readfreqbank(); //прочитали банки частот из еепром
	sei(); //пошли прерывания

    while (1) 
    {
		display();
		flag_handler();
    }
}
 