#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>

#define UP 0
#define DOWN 1

// servo on PA0 / TCA0-WO0
// potentiometer on PD0 / AIN0

uint16_t map(
  uint16_t min_out, uint16_t max_out, // output range
  uint16_t min_in,  uint16_t max_in,  // input range
  uint16_t value)
{
  float x;
  x = 1.0*(value - min_in) / (max_in - min_in);
  return (uint16_t)(min_out + x * (max_out - min_out));
}

int main(void)
{
  uint16_t result;

  VREF.ADC0REF = VREF_REFSEL_VDD_gc;
  ADC0.CTRLA = 0b0 | ADC_ENABLE_bm | ADC_CONVMODE_SINGLEENDED_gc | ADC_RESSEL_12BIT_gc | ADC_FREERUN_bm;
  ADC0.CTRLB = ADC_SAMPNUM_NONE_gc;
  ADC0.CTRLC = ADC_PRESC_DIV20_gc;
  ADC0.CTRLD = 0;
  ADC0.CTRLE = 0;
  ADC0.MUXPOS = ADC_MUXPOS_AIN0_gc;
  ADC0.MUXNEG = ADC_MUXNEG_GND_gc;
  ADC0.COMMAND = ADC_STCONV_bm;

  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV4_gc | TCA_SINGLE_ENABLE_bm;
  TCA0.SINGLE.CTRLB = 0b00010000 | TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
  TCA0.SINGLE.PER = 19999;
  TCA0.SINGLE.CMP0 = 1000;

  PORTA.DIRSET = PIN0_bm;

  while (1)
  {
    _delay_ms(50);
    result = map(1000, 2000, 0, 4095, ADC0.RES);
    TCA0.SINGLE.CMP0 = result;
  }
}
