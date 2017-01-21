#include <string.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay_basic.h>
#include <stdint.h>

// Digital 11 (Variable Spindle PWM) is PB3 (OC2A)
// Digital 2 (Step Pulse X Axis) is PD2
// Digital 3 (Step Pulse Y Axis) is PD3
// Digital 5 (Direction X Axis) is PD5
// Digital 6 (Direction Y Axis) is PD6
// Digital 8 (Enable) is PB0
// Digital 9 (Limit X Axis) is PB1
// Digital 10 (Limit Y Axis) is PB2

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

#if 0
// for generating a waveform with a precise duration
ISR(TIMER0_COMPA_vect)
{
	/* After each odd-numbered phase, reset timer period
	   to 0.5ms. After each even-numbered phase use the
	   servo pulse lengths pre-calculated in the main loop. */
	if (timer0_phase & 1)
	{
		/* Pin has switched from HIGH to LOW here */
		OCR0A = SERVO_TIME1;

		/* Configure timer0 to set pin on compare match */
		TCCR0A = _BV(COM0A0) | _BV(COM0A1) | _BV(WGM01);
	}
	else
	{
		/* Pin has switched from LOW to HIGH here */
		/* After last phase (even-numbered), stop timer. */
		if (timer0_phase == 8)
		{
			//TIMSK0 = 0;
			TCCR0B = 0;
			PORTD &= ~_BV(7);

			return;
		}

		/* Configure timer0 to clear pin on compare match */
		TCCR0A = _BV(COM0A1) | _BV(WGM01);

		/* The HIGH pulse should be the long one. */
		OCR0A = timer0_servos[timer0_phase / 2];
	}

	timer0_phase++;
}
#endif

struct {
    int32_t xpos, ypos;
} state;

void stepper_enable()
{
    PORTB &= ~_BV(0);
}

void stepper_disable()
{
    PORTB |= _BV(0);
}

inline void setup()
{
    // ------ output pins -------
    // Stepper enable (PB0) + Laser enable (PB3)
    PORTB = 0b00000001; // initial state: stepper drivers disabled
    DDRB = _BV(PORTB0) | _BV(PORTB3);
    
    // X and Y Axis step (PD2 and PD3) + X and Y axis direction (PD5 and PD6)
    //PORTD = 0b00000000;
    DDRD = _BV(PORTD2) | _BV(PORTD3) | _BV(PORTD5) | _BV(PORTD6);
}

inline void test()
{
    // Time to settle
    delay(100);
    
    // Enable stepper motors
    stepper_enable();
    delay(100);
    
    // Set direction to 1 (right?)
    PORTD |= _BV(PORTD5);
    int x;
    for (x = 0; x < 3000; x++)
    {
        // step - 2 ms in total, meaning 2 seconds to travel 1 inch
        PORTD |= _BV(PORTD2);
        delay(1);
        PORTD &= ~_BV(PORTD2);
        delay(1);
    }
    
    stepper_disable();
}

inline void main_loop()
{
}

int main()
{
	setup();
	test();
	while (1) {
		main_loop();
	}

	return 0;
}
