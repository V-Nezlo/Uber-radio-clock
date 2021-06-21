#include "usart.h"

void USART_Init( unsigned int ubrr)//������������� ������ USART
{
		//������� �������� ������ USART 115200
		UBRR0H = (unsigned char)(ubrr>>8);
		UBRR0L = (unsigned char)ubrr;
		
		UCSR0B=(1<<RXEN0)|( 1<<TXEN0); //�������� ����� � �������� �� USART
		//UCSR0B |= (1<<RXCIE0); //��������� ���������� ��� ��������
		UCSR0A |= (1<<U2X0); // ��������� �������� USART

}

void USART_Transmit( unsigned char data ) //������� �������� ������
{
	while ( !(UCSR0A & (1<<UDRE0)) ); //�������� ����������� ������ ������
	UDR0 = data; //������ �������� ������
}