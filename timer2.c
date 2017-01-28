#include "timer2.h"

inline void timer2_init()
{
    // Set mode
#if defined TIMER2_MODE0
        // Not needing to be explicitly set as 0 is the default
	//TCCR2A = 0; // Normal mode
	//TCCR2B = 0; // Normal mode
#elif defined TIMER2_MODE3
        // Non-Inverting Fast PWM Mode
	TCCR2A = _BV(WGM20) | _BV(WGM21)
#if defined TIMER2_ENABLE_OC2A
                | _BV(COM2A1)
#endif
#if defined TIMER2_ENABLE_OC2A_INVERTED
                | _BV(COM2A1) | _BV(COM2A0)
#endif
                ;
	//TCCR2B = 0;
#endif

	// Enable selected interrupts
	TIMSK2 |= 0
#if defined TIMER2_ENABLE_INT_OCR2
                |_BV(OCIE2)
#endif
                ;
}

inline void timer2_start()
{
	// Restart from 0
	TCNT2 = 0;

	/* Set prescaler to start timer */
#if defined TIMER2_CLK_DIV_1
	TCCR2B |= _BV(CS20); // No prescaling
#elif defined TIMER2_CLK_DIV_8
	TCCR2B |= _BV(CS21); // Clock/8
#elif defined TIMER2_CLK_DIV_32
	TCCR2B |= _BV(CS20) | _BV(CS21);
#elif defined TIMER2_CLK_DIV_64
	TCCR2B |= _BV(CS22);
#elif defined TIMER2_CLK_DIV_128
	TCCR2B |= _BV(CS22) | _BV(CS20);
#elif defined TIMER2_CLK_DIV_256
	TCCR2B |= _BV(CS22) | _BV(CS21);
#elif defined TIMER2_CLK_DIV_1024
	TCCR2B |= _BV(CS22) | _BV(CS21) | _BV(CS20);
#endif
}

inline void timer2_stop()
{
	/* Unset prescaler bits to stop timer */
	TCCR2B &= ~(_BV(CS22) | _BV(CS21) | _BV(CS20));
}

