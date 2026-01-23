#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>
#include "sinetable_full_16bit_256.h"

// servo on PA0 / TCA0-WO0
// potentiometer on PD0 / AIN0

uint16_t map(uint16_t min_out, uint16_t max_out, uint16_t min_in, uint16_t max_in, uint16_t value)
{
  float x;
  x = 1.0*(value - min_in) / (max_in - min_in);
  return (uint16_t)(min_out + x * (max_out - min_out));
}

ISR(RTC_PIT_vect)
{
  static uint8_t i=0;
  TCA0.SINGLE.CMP0 =  map(800, 2200, 0, 1023, pgm_read_word(&sine[i]));
  i++;
  RTC.PITINTFLAGS = 0b00000001;
}

int main(void)
{
  uint16_t result;

  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV4_gc | TCA_SINGLE_ENABLE_bm;
  TCA0.SINGLE.CTRLB = 0b00010000 | TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
  TCA0.SINGLE.PER = 19999;
  TCA0.SINGLE.CMP0 = 1000;

  PORTA.DIRSET = PIN0_bm;

  RTC.CTRLA      = 0b00000000;
  RTC.CLKSEL     = RTC_CLKSEL_OSC32K_gc;
  RTC.PITCTRLA   = RTC_PERIOD_CYC256_gc | RTC_PITEN_bm;
  RTC.PITINTCTRL = 0b00000001;
  sei();

  while (1)
  {
    _delay_ms(1000);
  }
}
