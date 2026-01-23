#include <avr/io.h>
#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>
#include "sinetable_full_16bit_256.h"

typedef struct
{
  uint8_t R;
  uint8_t G;
  uint8_t B;
} RGB_t;

RGB_t buffer[8];

void sendbyte(uint8_t data)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (data & 0b10000000)
    {   // 0.5µs extra delay results in about 500ns long pulses
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

void update_neopixels(void)
{
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    for (uint8_t i = 0; i < 8; i++)
    {
      sendbyte(buffer[i].G);
      sendbyte(buffer[i].R);
      sendbyte(buffer[i].B);
    }
  }
}

/*
 * borrowed from https://github.com/Makuna/NeoPixelBus
 * Written by Michael C. Miller
 * -> plain C by Uwe Zimmermann, 2025
*/
void HsvToRgb(RGB_t *rgb, float h, float s, float v)
{
  float r,g,b;
  if (s == 0.0f)
  {
    r = v;
    g = v;
    b = v;
  }
  else
  {
    if (h < 0.0f)
    {
        h += 1.0f;
    }
    else if (h >= 1.0f)
    {
        h -= 1.0f;
    }
    h *= 6.0f;
    int i = (int)h;
    float f = h - i;
    float q = v * (1.0f - s * f);
    float p = v * (1.0f - s);
    float t = v * (1.0f - s * (1.0f - f));
    switch (i)
    {
      case 0:
        r = v;
        g = t;
        b = p;
        break;
      case 1:
        r = q;
        g = v;
        b = p;
        break;
      case 2:
        r = p;
        g = v;
        b = t;
        break;
      case 3:
        r = p;
        g = q;
        b = v;
        break;
      case 4:
        r = t;
        g = p;
        b = v;
        break;
      default:
        r = v;
        g = p;
        b = q;
        break;
    }
  }
  rgb->R = (uint8_t)(r*255);
  rgb->G = (uint8_t)(g*255);
  rgb->B = (uint8_t)(b*255);
}

uint16_t map(
  uint16_t min_out, uint16_t max_out, // output range
  uint16_t min_in,  uint16_t max_in,  // input range
  uint16_t value)
{
  float x;
  x = 1.0*(value - min_in) / (max_in - min_in);
  return (uint16_t)(min_out + x * (max_out - min_out));
}

void init(void)
{
  _PROTECTED_WRITE(CLKCTRL.OSCHFCTRLA, CLKCTRL_FRQSEL_16M_gc);

  VREF.ADC0REF = VREF_REFSEL_VDD_gc;
  ADC0.CTRLA = 0b00000011;
  ADC0.CTRLB = 0b00000000;
  ADC0.CTRLC = ADC_PRESC_DIV20_gc;
  ADC0.CTRLD = 0;
  ADC0.CTRLE = 0;
  ADC0.MUXPOS = ADC_MUXPOS_AIN0_gc;
  ADC0.MUXNEG = ADC_MUXNEG_GND_gc;
  ADC0.COMMAND = ADC_STCONV_bm;

  TCA0.SINGLE.CTRLA = 0b00001001;
  TCA0.SINGLE.CTRLB = 0b00010011;
  TCA0.SINGLE.PER   = 19999;
  TCA0.SINGLE.CMP0  = 1000;

  PORTA.DIRSET = PIN0_bm;
  PORTA.DIRSET = PIN4_bm;
}

int main(void)
{
  uint16_t adc;

  init();

  while (1)
  {
    adc = ADC0.RES;
    for (uint8_t i = 0; i < 8; i++)
    {
      //HsvToRgb(&buffer[i], 1.0*i/8, 1.0, adc/4096.0);
      //HsvToRgb(&buffer[i], 1.0*((i+adc/512)%8)/8, 1.0, 0.15);
      HsvToRgb(&buffer[i], 1.0*i/8, adc/4096.0, 0.15);
    }
    update_neopixels();
    TCA0.SINGLE.CMP0 = map(800,2200,0,4096,adc);
  }
}
