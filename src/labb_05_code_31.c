#include <avr/io.h>
#include <util/delay.h>

int main(void)
{
  VREF.DAC0REF   = VREF_REFSEL_..._gc;
  PORTD.PIN6CTRL = PORT_ISC_INPUT_DISABLE_gc;
  DAC0.CTRLA     = DAC_..._bm | DAC_..._bm;
  
  uint16_t i;
  while (1)
  {
    DAC0.DATA = i << 6;
    i = (i + 1) %= 1024;
    _delay_ms(1);
  }
}