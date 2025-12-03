#define F_CPU 4000000UL
#include <avr/io.h> 
#include <util/delay.h>  
#include <avr/interrupt.h>
#include <util/atomic.h>


ISR(RTC_PIT_vect) {
    PORTA.OUT ++;
    RTC.PITINTFLAGS = RTC_PI_bm;
}

int main(void) {  
    PORTA.DIR = 255;  
    PORTA.OUT = 0;
    RTC.CTRLA = RTC_PRESCALER_DIV1_gc; // set the prescaler to 1:1 
    RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc; // select the internal 32 kHz oscillator 
    RTC.PITCTRLA = RTC_PERIOD_CYC16384_gc | RTC_PITEN_bm; // enable PIT and set interrupt period 
    RTC.PITINTCTRL = RTC_PI_bm; // enable PIT interrupts

    while (1) {  
        /*PORTA.OUT = (TCA0.SINGLE.CNT >> 8); */
    } 
}
