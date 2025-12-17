#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdio.h>
#include "nokia5110_hspi.h"

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
  while (!(USART2.STATUS & USART_DREIF_bm)); // wait if still transmitting

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
  // configure the outputs of PORTA: MOSI, SCK, PA0 and PA1 for NOKIA, 
  // PA3 for an LED
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

  // configure the SPI for the NOKIA display
  PORTMUX.SPIROUTEA = PORTMUX_SPI0_DEFAULT_gc;
  SPI0.CTRLA = (0 << SPI_DORD_bp)  // MSB first
             | SPI_MASTER_bm
             | (0 << SPI_CLK2X_bp)
             | SPI_PRESC_DIV4_gc
             | SPI_ENABLE_bm;
  SPI0.CTRLB = SPI_MODE_0_gc;

  // configure the ADC
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

  // configure the pushbutton inputs: pull-up and interrupt on falling edge
  PORTC.PIN0CTRL = PORT_PULLUPEN_bm
    | PORT_ISC_FALLING_gc; 
  PORTC.PIN1CTRL = PORT_PULLUPEN_bm
    | PORT_ISC_FALLING_gc;
  
  // enable all interrupts
  sei();

  // start the NOKIA display
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
  char textbuffer[20];

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

  while (1)
  {
    ADC0.COMMAND = ADC_STCONV_bm;
    while (ADC0.COMMAND & ADC_STCONV_bm) {}

    sprintf(textbuffer, "i=%3d ADC=%4d", i, ADC0.RES);
    printf(textbuffer); printf("\n");
    NOKIA_scroll(8);
    NOKIA_print(0,40,textbuffer,NOKIA_NORMAL);

    if (keypressed)
    {
      if (keypressed & PIN0_bm)
      {
        i --;
      }
      if (keypressed & PIN1_bm)
      {
        i ++;
      }
      keypressed=0;

      sprintf(textbuffer, "key = 0x%02x", keypressed);
      printf(textbuffer); printf("\n");

      NOKIA_scroll(8);
      NOKIA_print(0,40,textbuffer,NOKIA_NORMAL);
    }

    NOKIA_update();
    _delay_ms(100);
  }
}
