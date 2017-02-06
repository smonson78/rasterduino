#ifndef __TIMER2_H
#define __TIMER2_H

#include <avr/io.h>

//#define TIMER2_MODE0
#define TIMER2_MODE3 // FAST PWM

// Enable interrupts
//#define TIMER2_ENABLE_INT_OCR2

// Clock divider
//#define TIMER2_CLK_DIV_1
#define TIMER2_CLK_DIV_8
//#define TIMER2_CLK_DIV_32
//#define TIMER2_CLK_DIV_64
//#define TIMER2_CLK_DIV_128
//#define TIMER2_CLK_DIV_256
//#define TIMER2_CLK_DIV_1024

#define TIMER2_ENABLE_OC2A
//#define TIMER2_ENABLE_OC2A_INVERTED

inline void timer2_init();
inline void timer2_start();
inline void timer2_stop();

#endif
