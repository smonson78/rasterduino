#include <string.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay_basic.h>
#include <stdint.h>

#include "longnum.h"

void delay(int time)
{
	while (time--)
	{
		/* 1msec delay */
		_delay_loop_2(F_CPU / 4000);
	}
}


ISR(TIMER0_COMPA_vect)
{
	// timer tick
}

/* Maintain a timer */
inline void init_clock()
{
	/* Get timer1 into CTC mode (clear on compare match) */
	TCCR0A |= _BV(1); // 0 remains clear

	/* Select clock/8 prescaler */
	TCCR0B |= _BV(1); // 0 and 2 remain clear

	/* Frequency of clock is F_CPU / 8 / OCR0A */
	OCR0A = 200; /* 5000Hz for 8MHz (200uS period) */
}

inline void init_timer1()
{
	/* Normal mode */
	TCCR1A |= 0;

	/* Use clock/8 prescaler - microseconds */
	TCCR1B |= _BV(CS11);
}

inline void setup()
{
}

inline void main_loop()
{
}

int main()
{
	setup();
	while (1) {
		main_loop();
	}

	return 0;
}
