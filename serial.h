#ifndef __SERIAL_H
#define __SERIAL_H

#define RXBUFFER 64
#define TXBUFFER 16

#define FBAUD 115200

extern volatile char rx_buffer[RXBUFFER];
extern volatile char tx_buffer[TXBUFFER];

void serial_init();
void serial_transmit(unsigned char data);

#endif
