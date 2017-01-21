#include <string.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay_basic.h>
#include <stdint.h>

#include "serial.h"

#define MAX_BUF 1024


#define LOOKUP _binary_lookup_bin_start
#define LOOKUP_END _binary_lookup_bin_end

extern const uint8_t LOOKUP_END[] PROGMEM;
extern const uint8_t LOOKUP[] PROGMEM;

uint8_t scanline[MAX_BUF];

// Digital 11 (Variable Spindle PWM) is PB3 (OC2A)
// Digital 2 (Step Pulse X Axis) is PD2
// Digital 3 (Step Pulse Y Axis) is PD3
// Digital 5 (Direction X Axis) is PD5
// Digital 6 (Direction Y Axis) is PD6
// Digital 8 (Enable) is PB0
// Digital 9 (Limit X Axis) is PB1
// Digital 10 (Limit Y Axis) is PB2

volatile struct {
    uint8_t from_table;
    uint8_t reverse;
	uint16_t steps;
	uint16_t total_steps;

} move_cmd;


void delay(int time)
{
	while (time--)
	{
		/* 1msec delay */
		_delay_loop_2(F_CPU / 4000);
	}
}

inline void timer1_init()
{
    // Set mode
	//TCCR1A = 0; // Normal mode
	//TCCR1B = 0; // Normal mode

	TCCR1A = 0; // Mode 4
	TCCR1B = _BV(WGM12); // Mode 4: Clear timer on compare (OCR1A)

	// Enable compare match interrupt
	TIMSK1 |= _BV(OCIE1A) | _BV(OCIE1B); // TIMER1_COMPA & TIMER1_COMPB
}

inline void timer1_start()
{
	// Restart from 0
	TCNT1 = 0;

	/* Set prescaler to start timer */
	//TCCR1B |= _BV(CS10); // No prescaling
	//TCCR1B |= _BV(CS11); // Clock/8
	//TCCR1B |= _BV(CS10) | _BV(CS11); // Clock/64
	//TCCR1B |= _BV(CS10) | _BV(CS12); // Clock/1024
	TCCR1B |= _BV(CS11); // Clock/8
}

inline void timer1_stop()
{
	/* Unset prescaler to stop timer */
	TCCR1B &= 0b11111000;
}

volatile uint8_t running = 0;
ISR(TIMER1_COMPA_vect)
{
	// X axis step pulse stop
	PORTD &= ~_BV(PORTD2);
	
    if (move_cmd.from_table)
    {
        if (move_cmd.reverse)
        {
	        if (move_cmd.steps-- == 0)
	        {
	            timer1_stop();
	            running = 0;
		        return;
	        }
        }
        else
        {
	        if (++move_cmd.steps == move_cmd.total_steps)
	        {
	            timer1_stop();
	            running = 0;
		        return;
	        }
        }

        uint16_t new_duration = pgm_read_byte(LOOKUP + (move_cmd.steps * 2) + 1) << 8;
        new_duration |= pgm_read_byte(LOOKUP + (move_cmd.steps * 2));
        
    	OCR1A = new_duration;
	    OCR1B = new_duration - 10;

        return;
    }

    // Not reading from acceleration lookup table:
	if (++move_cmd.steps == move_cmd.total_steps)
	{
	    timer1_stop();
	    running = 0;
		return;
	}
}

ISR(TIMER1_COMPB_vect)
{
    // Begin X axis step pulse
	PORTD |= _BV(PORTD2);
}


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
    timer1_init();
	//serial_init();


    // ------ output pins -------
    // Stepper enable (PB0) + Laser enable (PB3)
    PORTB = 0b00000001; // initial state: stepper drivers disabled
    DDRB = _BV(PORTB0) | _BV(PORTB3);
    
    // X and Y Axis step (PD2 and PD3) + X and Y axis direction (PD5 and PD6)
    //PORTD = 0b00000000;
    DDRD = _BV(PORTD2) | _BV(PORTD3) | _BV(PORTD5) | _BV(PORTD6);
}

/* A flat move with no lasering */
void flat_move(uint16_t rate, uint16_t steps)
{
    move_cmd.from_table = 0;
    move_cmd.steps = 0;
    move_cmd.total_steps = steps;
	OCR1A = rate;
    OCR1B = rate - 10;
    running = 1;
    timer1_start();
	while(running)
	{
	}
}

// accelerate from stopped up to step rate, then pad to a number of steps
// OR in reverse, pad to the number of steps then slow down at the end.
void accel(uint16_t rate, uint8_t reverse, uint16_t pad_steps)
{
    uint16_t table_entry;
    uint16_t step_delay = 0;

    for (table_entry = 0; LOOKUP + (table_entry * 2) < LOOKUP_END; table_entry++)
    {
        uint16_t step_delay_candidate = pgm_read_byte(LOOKUP + (table_entry * 2));
        step_delay_candidate |= pgm_read_byte(LOOKUP + (table_entry * 2) + 1) << 8;
        
        if (step_delay_candidate < rate) {
            break;
        }
        
        step_delay = step_delay_candidate;
    }
    
    // Pad (if in reverse) to the required number of steps
	if (reverse && table_entry < pad_steps) {
	    flat_move(rate, pad_steps - table_entry);
	}

    // Prepare move_cmd for accelerated move
    move_cmd.from_table = 1;
    if (!reverse)
    {
        move_cmd.reverse = 0;
    	move_cmd.steps = 0;
        move_cmd.total_steps = table_entry + 1;
    } else {
        move_cmd.steps = table_entry;
        move_cmd.reverse = 1;
    }

	OCR1A = step_delay;
    OCR1B = step_delay - 10;

    running = 1;
    timer1_start();
	while(running)
	{
	}
	
	// Pad (if not in reverse) for the remaining number of steps
	if (reverse == 0 && table_entry < pad_steps) {
	    flat_move(rate, pad_steps - table_entry);
	}
}

inline void test()
{
    // Time to settle
    delay(100);
    
	//serial_transmit('$');
	//serial_transmit('$');
	//serial_transmit('\r');
	//serial_transmit('\n');

    // Enable stepper motors
    stepper_enable();
    delay(100);
    
    sei();

	uint16_t velocity = 300;

	while (1)
	{
		    // Set direction to 1 (rightwards)
		    PORTD |= _BV(PORTD5);
        	accel(velocity, 0, 1000); // speed up
        	flat_move(velocity, 2048);
        	accel(velocity, 1, 1000); // slow down

		    // Set direction to 0 (leftwards)
		    PORTD &= ~_BV(PORTD5);
        	accel(velocity, 0, 1000); // speed up
            flat_move(velocity, 2048);
        	accel(velocity, 1, 1000); // slow down

	}
    cli();

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
