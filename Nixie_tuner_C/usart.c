#include "usart.h"

void USART_Init( unsigned int ubrr)//Инициализация модуля USART
{
		//Зададим скорость работы USART 115200
		UBRR0H = (unsigned char)(ubrr>>8);
		UBRR0L = (unsigned char)ubrr;
		
		UCSR0B=(1<<RXEN0)|( 1<<TXEN0); //Включаем прием и передачу по USART
		//UCSR0B |= (1<<RXCIE0); //Разрешаем прерывание при передаче
		UCSR0A |= (1<<U2X0); // удваиваем скорость USART

}

void USART_Transmit( unsigned char data ) //Функция отправки данных
{
	while ( !(UCSR0A & (1<<UDRE0)) ); //Ожидание опустошения буфера приема
	UDR0 = data; //Начало передачи данных
}