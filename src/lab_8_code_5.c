#include <avr/io.h>
#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>

uint8_t buffer[24] = {
  255,0,0,
  0,255,0,
  0,0,255,
  100,100,0,
  100,0,100,
  0,100,100,
  50,25,50,
  50,50,50
};

void sendbyte(uint8_t data)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (data & 0b10000000)
    {  // 0.5µs extra delay results in about 500ns long pulses
      PORTA.OUTSET = PIN4_bm;
      _delay_us(0.5);
      PORTA.OUTCLR = PIN4_bm;
      _delay_us(0.25);
    }
    else
    {  // no delay at 16 MHz gives about 300ns long pulses
      PORTA.OUTSET = PIN4_bm;
      PORTA.OUTCLR = PIN4_bm;
      _delay_us(0.75);
    }
    data <<= 1;
  }
}

int main(void)
{
  uint8_t offset = 0;
  _PROTECTED_WRITE(CLKCTRL.OSCHFCTRLA, CLKCTRL_FRQSEL_16M_gc);
  PORTA.DIRSET = PIN4_bm;
  while (1)
  {
    for (uint8_t i = 0; i < 24; i++)
    {
      sendbyte(buffer[(i+offset)%24]);
    }
    _delay_ms(500);
    offset = (offset+3) % 24;
  }
}
