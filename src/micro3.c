#define F_CPU 4000000UL
#include <avr/io.h> 
#include <util/delay.h>  
#include <avr/interrupt.h>
#include <util/atomic.h>

uint8_t font [10] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f};
volatile uint8_t framebuffer[4] = {0, 0, 0, 0};
volatile uint8_t current_digit = 0;

ISR(RTC_PIT_vect) {
    PORTA.OUT = 5;
    current_digit += 1;
    current_digit %= 4;
    PORTD.OUT = 0xff;
    PORTC.OUT = (1 << current_digit);
    PORTD.OUT = ~font[framebuffer[current_digit]];
    RTC.PITINTFLAGS = RTC_PI_bm;
}

ISR(TCA0_OVF_vect) {
    framebuffer[3] = (framebuffer[3] + 1) % 10;
    if (framebuffer[3] == 0) {
        framebuffer[2] = (framebuffer[2] + 1) % 10;
    }
    if (framebuffer[2] == 0) {
        framebuffer[1] = (framebuffer[1] + 1) % 10;
    }
    if (framebuffer[3] == 0) {
        framebuffer[0] = (framebuffer[0] + 1) % 10;
    }
    /*TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1024_gc | TCA_SINGLE_ENABLE_bm;*/
}

int main(void) {  
    PORTA.DIR = 0xff;
    PORTD.DIR = 0xff;
    PORTC.DIR = 0x0f;

    PORTA.OUT = 5;
    PORTC.OUT = (1 << current_digit);
    PORTD.OUT = 0xff;

    // RTC timer setup
    RTC.CTRLA = RTC_PRESCALER_DIV1_gc; // set the prescaler to 1:1 
    RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc; // select the internal 32 kHz oscillator 
    RTC.PITCTRLA = RTC_PERIOD_CYC16_gc | RTC_PITEN_bm; // enable PIT and set interrupt period 
    RTC.PITINTCTRL = RTC_PI_bm; // enable PIT interrupts

    // TCA0 timer setup
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1024_gc | TCA_SINGLE_ENABLE_bm;
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
    sei();
    while (1) {  
        /*PORTA.OUT = RTC.CNT;*/
    } 
}
