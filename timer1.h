#ifndef __TIMER1_H
#define __TIMER1_H

#include <avr/io.h>

//#define TIMER1_MODE0
#define TIMER1_MODE4 // Clear Timer on Compare OCR1A

// Enable interrupts
#define TIMER1_ENABLE_INT_OCR1A
#define TIMER1_ENABLE_INT_OCR1B

// Clock divider
//#define TIMER1_CLK_DIV_0
#define TIMER1_CLK_DIV_8
//#define TIMER1_CLK_DIV_64
//#define TIMER1_CLK_DIV_1024

inline void timer1_init();
inline void timer1_start();
inline void timer1_stop();

#endif
