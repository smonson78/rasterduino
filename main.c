#include <string.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay_basic.h>
#include <stdint.h>

#include "serial.h"
#include "timer1.h"
#include "timer2.h"

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
    enum {
        MOVE_NORMAL, MOVE_FROM_TABLE, MOVE_RASTER
    } mode;
    
    uint8_t reverse;
    uint16_t steps;
    uint16_t total_steps;

    // the ratio steps:total_steps matches x:pixels (where scanline[x] is PWM value)
    uint16_t pixels;
    uint16_t scanline_index;
    
    uint16_t y_steps;
} move_cmd;

void delay(int time)
{
   while (time--)
   {
       /* 1msec delay */
       _delay_loop_2(F_CPU / 4000);
   }
}

volatile uint8_t running = 0;
ISR(TIMER1_COMPA_vect)
{
    // X axis step pulse stop
    PORTD &= ~_BV(PORTD2);
    
    if (move_cmd.reverse)
    {
        // Reverse: 1023 .. 0
        if (move_cmd.steps-- == 0)
        {
            timer1_stop();
            running = 0;
            return;
        }
    }
    else
    {
        // Forward: 0 .. 1023
        if (++move_cmd.steps == move_cmd.total_steps)
        {
            timer1_stop();
            running = 0;
            return;
        }
    }
        
    // Move from acceleration lookup table:
    if (move_cmd.mode == MOVE_FROM_TABLE)
    {
        uint16_t new_duration = pgm_read_byte(LOOKUP + (move_cmd.steps * 2) + 1) << 8;
        new_duration |= pgm_read_byte(LOOKUP + (move_cmd.steps * 2));
        
        OCR1A = new_duration;
        OCR1B = new_duration - 10;

        return;
    }
    
    // Raster moves: control PWM value
    else if (move_cmd.mode == MOVE_RASTER)
    {
        OCR2A = scanline[move_cmd.scanline_index + move_cmd.steps];  
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
    timer2_init();
    serial_init();

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
    move_cmd.mode = MOVE_NORMAL;
    move_cmd.reverse = 0;
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

void enable_laser_pwm()
{
    // Enable timer2 OC2A override on PORTB3 pin
    TCCR2A |= _BV(COM2A1);
}

void disable_laser_pwm()
{
    timer2_stop();
    
    // Disable timer2 OC2A override on PORTB3 pin
    TCCR2A &= ~(_BV(COM2A1) | _BV(COM2A0));
    
    // Ensure pin is driving low.
    PORTB &= ~_BV(PORTB3);
}

/* A flat move WITH lasering */
void raster_move(uint16_t rate, uint16_t steps, uint16_t index, uint8_t reverse)
{
    move_cmd.mode = MOVE_RASTER;
    move_cmd.reverse = reverse;
    move_cmd.scanline_index = index;
    OCR1A = rate;
    OCR1B = rate - 10;
    move_cmd.total_steps = steps;
    
    // First pixel PWM value and step counter
    if (reverse)
    {
        OCR2A = scanline[index + steps - 1];
        move_cmd.steps = steps - 1;
    }
    else
    {
        OCR2A = scanline[index];
        move_cmd.steps = 0;
    }
    running = 1;
    enable_laser_pwm();
    timer1_start();
    timer2_start();
    
    while(running)
    {
    }
    disable_laser_pwm();    
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
    move_cmd.mode = MOVE_FROM_TABLE;
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

void y_advance(uint8_t steps)
{
    while (steps-- > 0)
    {
        PORTD |= _BV(PORTD3);
        delay(1);
        PORTD &= ~_BV(PORTD3);                
        delay(1);
    }
}

void test_pattern()
{
    int i;
    for (i = 0; i < 1024; i++)
    {
        if ((i % 256) < 4 || (i % 256) > 251)
            scanline[i] = 255;
        else
            scanline[i] = i % 256;
    }
}

void vertical_line()
{
    int i;
    for (i = 0; i < 1024; i++)
    {
        scanline[i] = 255;
    }
}

inline void test()
{
    // Time to settle
    delay(100);
    
    serial_send("$ rasterduino 0\r\n");
    
    // echo everything back
    while (1)
    {
        uint8_t data = serial_receive();
        serial_sendchar(data);
        delay(200);
    }

    // Enable stepper motors
    stepper_enable();
    delay(100);
    
    sei();

    uint16_t velocity = 2500;
    uint16_t steps_per_line = 5;
    uint16_t lines = 20;
    
    // Positive Y direction
    PORTD |= _BV(PORTD6);
    
    while (lines-- > 0)
    {
            if (lines < 2 || lines > 17)
                vertical_line();
            else
                test_pattern();
    
            // Set direction to 1 (rightwards)
            PORTD |= _BV(PORTD5);
            accel(velocity, 0, 1000); // speed up
            raster_move(velocity, 1024, 0, 0);
            accel(velocity, 1, 1000); // slow down

            // step Y+
            y_advance(steps_per_line);

            // Set direction to 0 (leftwards)
            PORTD &= ~_BV(PORTD5);
            accel(velocity, 0, 1000); // speed up
            raster_move(velocity, 1024, 0, 1);
            accel(velocity, 1, 1000); // slow down

            // step Y+
            y_advance(steps_per_line);
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
