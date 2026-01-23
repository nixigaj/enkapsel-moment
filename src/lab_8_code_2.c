#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>
#include <util/delay.h>

// servo on PA0 / TCA0-WO0
// potentiometer on PD0 / AIN0

int main(void) {
  uint16_t j = 1000;

  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV4_gc | TCA_SINGLE_ENABLE_bm;
  TCA0.SINGLE.CTRLB = 0b00010000 | TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
  TCA0.SINGLE.PER = 19999;
  TCA0.SINGLE.CMP0 = 2000;

  PORTA.DIRSET = PIN0_bm;

  while (1) {
    TCA0.SINGLE.CMP0 = j;

    j += 10;
    if (j > 2000) {
      j = 1000;
    }
    _delay_ms(100);
  }
}
