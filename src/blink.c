#define F_CPU 4000000UL
#include <avr/io.h>
#include <util/delay.h>

int main() {

  uint8_t number = 0b0;
  while (1) {
    PORTA.DIR = number;
    PORTA.OUT = number;
    number++;
    _delay_ms(500);
  }
  return 0;
}
