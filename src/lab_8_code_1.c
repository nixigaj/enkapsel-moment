#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>
#include <util/delay.h>

// servo on PA0 / TCA0-WO0
// potentiometer on PD0 / AIN0

int main(void) {
  uint16_t j = 100;
  uint8_t reached = 0;
  PORTA.DIRSET = PIN0_bm;

  while (1) {
    PORTA.OUTSET = PIN0_bm;
    for (uint16_t t = 0; t < j; t++) {
      _delay_us(10);
    }
    PORTA.OUTCLR = PIN0_bm;
    if (reached == 0) {
      j += 1;
    } else {
      j -= 1;
    }

    if (j == 200) {
      reached = 1;
    }
    if (j == 100) {
      reached = 0;
    }
    /*if (j>200)*/
    /*{*/
    /*  j=100;*/
    /*}*/
    _delay_ms(10);
  }
}
