#include <util/delay.h>
#define F_CPU 4000000UL
#include <avr/io.h>
#include <avr/delay.h>


int main() {
    PORTD.DIR = 0b00000001;
    while (1) {
        PORTD.OUT = 0b00000001;
        _delay_ms(500);
        PORTD.OUT = 0b00000000;
        _delay_ms(500);
    }
    return 0;
}
