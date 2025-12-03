#define F_CPU 4000000UL
#include <avr/io.h> 
#include <util/delay.h>  

int main(void) {  
    PORTA.DIR = 255;  
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1024_gc | TCA_SINGLE_ENABLE_bm;
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
    while (1) {  
        PORTA.OUT = (TCA0.SINGLE.CNT >> 8); 
    } 
}
