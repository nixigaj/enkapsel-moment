#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

static const uint8_t font[10] = // numbers 0 to 9
{0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};

static const uint8_t segments[3][8] = // cathode/anode
{ // A     B     C     D     E     F     G   dec.point
  {0x05, 0x03, 0x01, 0x20, 0x02, 0x10, 0x15, 0x00},
  {0x21, 0x13, 0x12, 0x23, 0x30, 0x32, 0x31, 0x54},
  {0x34, 0x24, 0x41, 0x04, 0x14, 0x43, 0x42, 0x53},
};

volatile uint8_t framebuffer[3];

ISR(RTC_PIT_vect)
{
  static uint8_t digit=0;
  static uint8_t segment=0;
  PORTA.DIR = 0;
  if (framebuffer[digit] & (1 << segment))
  {
    PORTA.OUT = 1 << (segments[digit][segment] & 0x0f);
    PORTA.DIR = (1 << (segments[digit][segment] & 0x0f))
              | (1 << (segments[digit][segment] >> 4));
  }
  segment ++;
  if (segment > 7)
  {
    segment = 0;
    digit ++;
    if (digit > 2)
    {
      digit = 0;
    }
  }
  RTC.PITINTFLAGS = RTC_PI_bm; // clear interrupt flag
}

void print_number(uint16_t number, uint8_t point)
{
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    for (uint8_t j=0; j<3; j++)
    {
      framebuffer[j] = font[number % 10];
      number = number / 10;
    }
    if (point < 3)
    {
      framebuffer[point] |= 0b10000000;
    }
  }
}

int main(void)
{

  RTC.CTRLA    = RTC_PRESCALER_DIV1_gc;
  RTC.CLKSEL   = RTC_CLKSEL_OSC32K_gc;
  RTC.PITCTRLA = RTC_PITEN_bm | RTC_PERIOD_CYC16_gc;
  RTC.PITINTCTRL = RTC_PI_bm;

  sei();

  VREF.ADC0REF   = VREF_REFSEL_VDD_gc;
  PORTD.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;
  ADC0.CTRLA = 0
             | ADC_RESSEL_12BIT_gc
             | ADC_ENABLE_bm;
  ADC0.CTRLB = ADC_SAMPNUM_NONE_gc;
  ADC0.CTRLC = ADC_PRESC_DIV20_gc;
  ADC0.CTRLD = ADC_INITDLY_DLY0_gc
             | ADC_SAMPDLY_DLY0_gc;
  ADC0.CTRLE = 0;
  ADC0.SAMPCTRL = 0;
  ADC0.MUXPOS = ADC_MUXPOS_AIN0_gc;
  ADC0.MUXNEG = ADC_MUXNEG_GND_gc;

  TCA0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm
                    | TCA_SINGLE_CLKSEL_DIV4_gc;
  TCA0.SINGLE.CTRLB = TCA_SINGLE_CMP0EN_bm
                    | TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
  TCA0.SINGLE.PER   = 4096;
  PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTC_gc; // UNSURE here
  PORTC.DIR         = 0b0001;

  uint16_t result;
  while (1)
  {
    ADC0.COMMAND = ADC_STCONV_bm;
    while (ADC0.COMMAND & ADC_STCONV_bm) {}
    TCA0.SINGLE.CMP0 = ADC0.RES; // <======
    result = ADC0.RES / 4;
    print_number(result, 0);
    _delay_ms(200);
  }
}
