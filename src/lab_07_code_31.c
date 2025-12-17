#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdio.h>
#include "nokia5110_hspi.h"

/*--------------------------------------------------------------------------------------------------
  int16_t sine table - stored in the flash memory
--------------------------------------------------------------------------------------------------*/
static const int16_t sinetab[64] PROGMEM =
{
     402, //  0: sin(0.01) = 0.0123
    1206, //  1: sin(0.04) = 0.0368
    2009, //  2: sin(0.06) = 0.0613
    2811, //  3: sin(0.09) = 0.0858
    3612, //  4: sin(0.11) = 0.1102
    4410, //  5: sin(0.13) = 0.1346
    5205, //  6: sin(0.16) = 0.1589
    5998, //  7: sin(0.18) = 0.1830
    6786, //  8: sin(0.21) = 0.2071
    7571, //  9: sin(0.23) = 0.2311
    8351, // 10: sin(0.26) = 0.2549
    9126, // 11: sin(0.28) = 0.2785
    9896, // 12: sin(0.31) = 0.3020
   10659, // 13: sin(0.33) = 0.3253
   11417, // 14: sin(0.36) = 0.3484
   12167, // 15: sin(0.38) = 0.3713
   12910, // 16: sin(0.40) = 0.3940
   13645, // 17: sin(0.43) = 0.4164
   14372, // 18: sin(0.45) = 0.4386
   15090, // 19: sin(0.48) = 0.4605
   15800, // 20: sin(0.50) = 0.4822
   16499, // 21: sin(0.53) = 0.5035
   17189, // 22: sin(0.55) = 0.5246
   17869, // 23: sin(0.58) = 0.5453
   18537, // 24: sin(0.60) = 0.5657
   19195, // 25: sin(0.63) = 0.5858
   19841, // 26: sin(0.65) = 0.6055
   20475, // 27: sin(0.67) = 0.6249
   21096, // 28: sin(0.70) = 0.6438
   21705, // 29: sin(0.72) = 0.6624
   22301, // 30: sin(0.75) = 0.6806
   22884, // 31: sin(0.77) = 0.6984
   23452, // 32: sin(0.80) = 0.7157
   24007, // 33: sin(0.82) = 0.7327
   24547, // 34: sin(0.85) = 0.7491
   25072, // 35: sin(0.87) = 0.7652
   25582, // 36: sin(0.90) = 0.7807
   26077, // 37: sin(0.92) = 0.7958
   26556, // 38: sin(0.94) = 0.8105
   27019, // 39: sin(0.97) = 0.8246
   27466, // 40: sin(0.99) = 0.8382
   27896, // 41: sin(1.02) = 0.8514
   28310, // 42: sin(1.04) = 0.8640
   28706, // 43: sin(1.07) = 0.8761
   29085, // 44: sin(1.09) = 0.8876
   29447, // 45: sin(1.12) = 0.8987
   29791, // 46: sin(1.14) = 0.9092
   30117, // 47: sin(1.17) = 0.9191
   30424, // 48: sin(1.19) = 0.9285
   30714, // 49: sin(1.21) = 0.9373
   30985, // 50: sin(1.24) = 0.9456
   31237, // 51: sin(1.26) = 0.9533
   31470, // 52: sin(1.29) = 0.9604
   31685, // 53: sin(1.31) = 0.9670
   31880, // 54: sin(1.34) = 0.9729
   32057, // 55: sin(1.36) = 0.9783
   32213, // 56: sin(1.39) = 0.9831
   32351, // 57: sin(1.41) = 0.9873
   32469, // 58: sin(1.44) = 0.9909
   32567, // 59: sin(1.46) = 0.9939
   32646, // 60: sin(1.48) = 0.9963
   32705, // 61: sin(1.51) = 0.9981
   32745, // 62: sin(1.53) = 0.9993
   32765  // 63: sin(1.56) = 0.9999
};

int16_t isin(uint8_t alpha)
{
  int16_t  result;
  if (alpha & 0b01000000)
  {
    result = pgm_read_word(&sinetab[63 - (alpha & 63)]);
  }
  else
  {
    result = pgm_read_word(&sinetab[(alpha & 63)]);
  }
  if (alpha & 0b10000000)
  {
    result = -result;
  }
  return result;
}

int16_t icos(uint8_t alpha)
{
  return isin(alpha+64);
}


/**
 * prepare the re-routing of stdout to UART2
 */
static int uart_putchar(char c, FILE *stream);
static int uart_getchar(FILE *stream);
static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);

/**
 * @param baudrate - UART baudrate
 * @brief initializes the UART module
 */
void UARTInit(uint32_t baudrate)
{
  uint16_t baud;
  baud = ((float) ( F_CPU * 64 /  ( 16 * (float)baudrate )) + 0.5 );
  PORTMUX.USARTROUTEA = PORTMUX_USART2_DEFAULT_gc; // TxD PF0, RxD PF1
  PORTF.DIRSET = PIN0_bm;
  USART2.BAUD  = baud;
  USART2.CTRLB = USART_TXEN_bm | USART_RXEN_bm;
  USART2.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc | USART_SBMODE_1BIT_gc | USART_CHSIZE_8BIT_gc;
  USART2.CTRLA = 0;
  stdout = &mystdout;
}
/**
 * @param c - char/byte to send
 * @brief send a byte to USART2, used for stdout
 */
static int uart_putchar(char c, FILE *stream)
{
  uint16_t timeout=1000;
  while (!(USART2.STATUS & USART_DREIF_bm) && (timeout>0)) // wait if still transmitting
  {
    timeout --;
    _delay_us(100);
  }
  USART2.TXDATAL = c; // next byte into the send register
  return 0;
}

/**
 * @brief interrupt service routine for PORTC
 */
volatile uint8_t keypressed=0;
ISR(PORTC_PORT_vect)
{
  keypressed |= PORTC.INTFLAGS;
  PORTC.INTFLAGS = 0xff;  // clear interrupt sources
}

/**
 * @brief init function - hardware initialization
 */
void init(void)
{
  PORTA.DIR = 0b01011011;

  UARTInit(19200);

  // create some additional startup delay
  for (uint8_t i = 0; i < 10; i++)
  {
    PORTA.OUTSET = PIN3_bm;
    _delay_ms(500);
    PORTA.OUTCLR = PIN3_bm;
    _delay_ms(500);
  }

  PORTMUX.SPIROUTEA = PORTMUX_SPI0_DEFAULT_gc;
  SPI0.CTRLA = (0 << SPI_DORD_bp)  // MSB first
             | SPI_MASTER_bm
             | (0 << SPI_CLK2X_bp)
             | SPI_PRESC_DIV4_gc
             | SPI_ENABLE_bm;
  SPI0.CTRLB = SPI_MODE_0_gc;

  VREF.ADC0REF = VREF_REFSEL_VDD_gc;
  PORTD.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;
  ADC0.CTRLA = (0 << ADC_CONVMODE_bp)
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

  PORTC.PIN0CTRL = PORT_PULLUPEN_bm
    | PORT_ISC_FALLING_gc; 
  PORTC.PIN1CTRL = PORT_PULLUPEN_bm
    | PORT_ISC_FALLING_gc;
  /*PORTC.PIN0CTRL = 0b...;*/
  /*PORTC.PIN1CTRL = 0b...;*/

  sei();

  NOKIA_init(
    &SPI0,
    &PORTA, 0, //rst_pin,
    &PORTA, 1, //dc_pin,
    0x00,      //vop,
    NOKIA_ORIENTATION_180
  );
  NOKIA_update();
}

/**
 * @brief main function
 */
int main(void)
{
  uint8_t vop=0;
  uint8_t i=0;
  char textbuffer[30];

  init();

  while (keypressed==0)
  {
    ADC0.COMMAND = ADC_STCONV_bm;
    while (ADC0.COMMAND & ADC_STCONV_bm) {}
    vop = ADC0.RES >> 5; // reduce to 7 bit, 0...127
    sprintf(textbuffer, "vop = 0x%02x", vop);
    printf(textbuffer); printf("\n");
    NOKIA_print(0,40,textbuffer,NOKIA_NORMAL);
    NOKIA_setVop(vop);
    NOKIA_update();
    _delay_ms(100);
  }
  keypressed = 0;

  uint8_t x, y;

  while (1)
  {

    NOKIA_clear();

    for (uint16_t i = 0; i < 84*4; i++)
    {
      PORTA.OUTSET = PIN3_bm;
      x =(uint8_t)(i/4);//(uint8_t)(42 - isin(i)/1638);
      y =(uint8_t)(24 - icos(i*2)/1638);
      NOKIA_setpixel(x, y);
      PORTA.OUTCLR = PIN3_bm;
      NOKIA_update();
    }

    _delay_ms(500);
  }
}
