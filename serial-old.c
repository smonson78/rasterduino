#include <avr/io.h>
#include <avr/interrupt.h>

#include "serial.h"

volatile char rx_buffer[RXBUFFER];
volatile char tx_buffer[TXBUFFER];
volatile uint8_t rx_ptr, tx_ptr;

void serial_init()
{
	unsigned int ubrr;
	switch (FBAUD) {
		case 115200:
			ubrr = 8;
			break;
		default:
			ubrr = (F_CPU / 16 / FBAUD) - 1;
	}

	UBRR0H = (unsigned char)ubrr >> 8;
	UBRR0L = (unsigned char)ubrr & 0xFF;
	
	UCSR0B = _BV(RXEN0) | _BV(TXEN0);

	/* Set frame format: 8 data bits */
	UCSR0C = 3 << UCSZ00;

	// TX pin output
	DDRD |= _BV(PORTD1);

	// Enable interrupts
	//UCSRB |= _BV(RXCIE) | _BV(TXCIE);
}

void serial_transmit(unsigned char data)
{
	/* Wait for empty transmit buffer */
	while ( !( UCSR0A & (1<<UDRE0)) )
		;
	/* Put data into buffer, sends the data */
	UDR0 = data;
}

unsigned char serial_receive()
{
	/* Wait for data to be received */
	while ( !(UCSR0A & _BV(RXC0)) )
		;
	/* Get and return received data from buffer */
	return UDR0;
}


