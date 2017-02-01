#ifndef __SERIAL_H
#define __SERIAL_H

#include <stdint.h>

#define RXBUFFER 64
#define TXBUFFER 16

#define FBAUD 57600

extern volatile uint8_t rx_buffer[RXBUFFER];
extern volatile uint8_t tx_buffer[TXBUFFER];

void serial_init();
void serial_sendchar(unsigned char data);
void serial_send(char *s);
unsigned char serial_receive();
int16_t serial_receive_nowait();

#endif
