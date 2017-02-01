#include <avr/io.h>
#include <util/delay_basic.h>
#include <avr/interrupt.h>

#include "serial.h"

// FIFOs on both send and receive
volatile uint8_t rx_buffer[RXBUFFER];
volatile uint8_t tx_buffer[TXBUFFER];
volatile uint8_t rx_ptr, rx_len, tx_ptr, tx_len, tx_idle;

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

    // Empty buffers
    rx_ptr = 0;
    rx_len = 0;
    tx_ptr = 0;
    tx_len = 0;
    tx_idle = 1;
    
	// Enable interrupts
	UCSR0B |= _BV(RXCIE0) | _BV(UDRIE0);
}

void tx_char(unsigned char data)
{
    // FIXME: we don't have to disable ALL interrupts...
    if (tx_idle)
    {
        // If there's no character already being sent
        tx_idle = 0;
        cli();
        
        // Re-enable interrupt
        UCSR0B |= _BV(UDRIE0);
	    
	    /* Put data into buffer, sends the data */
        UDR0 = data;
        sei();
        return;
    }
                
    // Put data into buffer
    cli();
    tx_buffer[(tx_ptr + tx_len++) % TXBUFFER] = data;
    sei();
}

void serial_sendchar(unsigned char data)
{
    // Wait for buffer to have room. OK to spin on tx_len because it's 8 bits.
    while (tx_len == TXBUFFER)
        ;
    tx_char(data);
}

// Send a bunch of characters at once
void serial_send(char *s)
{
    while (*s)
    {
        serial_sendchar(*(s++));
    }
}

// Return character from RX buffer if available, otherwise return -1
int16_t serial_receive_nowait()
{
    uint16_t data;
    cli();
	if (rx_len == 0)
	    data = -1;
    else
    {	    
	    data = rx_buffer[rx_ptr++];
	    rx_ptr %= RXBUFFER;
	    rx_len--;
	}
	sei();
	return data;
}

unsigned char serial_receive()
{
    int16_t data = serial_receive_nowait();
    while (data < 0)
    {
        data = serial_receive_nowait();
    }
    return data;
}

ISR(USART_RX_vect)
{
    // Receive the data (clears the interrupt bit)
    uint8_t data = UDR0;
    
    // Ignore data that won't fit into the buffer
    if (rx_len == RXBUFFER)
        return;
    
    // Otherwise dump it in the buffer
    rx_buffer[(rx_ptr + rx_len++) % RXBUFFER] = data;
}

ISR(USART_UDRE_vect)
{
    // End of data?
    if (tx_len == 0)
    {
        tx_idle = 1;
        // Must disable the interrupt here or else it'll be
        // an infinite loop.
    	UCSR0B &= ~_BV(UDRIE0);
        return;
    }
    
    // Load up the next character from the buffer
    UDR0 = tx_buffer[tx_ptr++];
    tx_ptr %= TXBUFFER;
    tx_len--;
}

