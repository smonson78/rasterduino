#include "timer1.h"


inline void timer1_init()
{
    // Set mode
#if defined TIMER1_MODE0
        // Not needing to be explicitly set as 0 is the default
	//TCCR1A = 0; // Normal mode
	//TCCR1B = 0; // Normal mode
#elif defined TIMER1_MODE4
        // Clear timer on compare (OCR1A)
	//TCCR1A = 0;
	TCCR1B = _BV(WGM12);
#endif
	// Enable selected interrupts
	TIMSK1 |= 0
#if defined TIMER1_ENABLE_INT_OCR1A
                |_BV(OCIE1A)
#elif defined TIMER1_ENABLE_INT_OCR1B
                |_BV(OCIE1B)
#endif
                ;
}

inline void timer1_start()
{
	// Restart from 0
	TCNT1 = 0;

	/* Set prescaler to start timer */
#if defined TIMER1_CLK_DIV_0
	TCCR1B |= _BV(CS10); // No prescaling
#elif defined TIMER1_CLK_DIV_8
	TCCR1B |= _BV(CS11); // Clock/8
#elif defined TIMER1_CLK_DIV_64
	TCCR1B |= _BV(CS10) | _BV(CS11);
#elif defined TIMER1_CLK_DIV_1024
	TCCR1B |= _BV(CS10) | _BV(CS12);
#endif
}

inline void timer1_stop()
{
	/* Unset prescaler to stop timer */
	TCCR1B &= 0b11111000;
}

