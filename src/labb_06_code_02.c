#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdio.h>
#include "nokia5110_hspi.h"

static int uart_putchar(char c, FILE *stream);
static int uart_getchar(FILE *stream);

static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);

/**
 * @param baudrate - UART baudrate
 * @brief initializes the UART module
 * @note internal use only
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

static int uart_putchar(char c, FILE *stream)
{
  while (!(USART2.STATUS & USART_DREIF_bm)); // wait if still transmitting

  USART2.TXDATAL = c; // next byte into the send register
  return 0;
}

static int uart_getchar(FILE *stream)
{
  uint16_t timeout = 1000;
  uint8_t data_l, data_h;
  int result;
  while ((timeout) || !(USART2.STATUS & USART_RXCIF_bm))
  {
    timeout --;
    _delay_ms(1);
  }
  data_h = USART2.RXDATAH;
  data_l = USART2.RXDATAL;
  if ((data_h & USART_BUFOVF_bm) || (data_h & USART_FERR_bm) || (data_h & USART_PERR_bm))
  {
    result = _FDEV_ERR;
  }
  else
  {
    if (timeout==0)
    {
      result = _FDEV_EOF;
    }
    else
    {
      result = data_l;
    }
  }
}

int main(void)
{
  _delay_ms(500);
  PORTA.DIR = 0b01010011;
  PORTMUX.SPIROUTEA = PORTMUX_SPI0_DEFAULT_gc;
  SPI0.CTRLA = (0 << SPI_DORD_bp)  // MSB first
             | SPI_MASTER_bm
             | (0 << SPI_CLK2X_bp)
             | SPI_PRESC_DIV4_gc
             | SPI_ENABLE_bm;
  SPI0.CTRLB = SPI_MODE_0_gc;

  NOKIA_init(
    &SPI0,
    &PORTA, 0, //rst_pin,
    &PORTA, 1, //dc_pin,
    0xbc,      //vop,
    NOKIA_ORIENTATION_180
  );
  NOKIA_update();
  _delay_ms(1000);
  NOKIA_clear();
  NOKIA_print(0,40,"Hello World!",NOKIA_NORMAL);
  NOKIA_update();

  UARTInit(19200);
  uint8_t x=0;
  uint8_t data_l, data_h;
  char textbuffer[20];
  
  while (1)
  {
    while (!(USART2.STATUS & USART_RXCIF_bm))
    {
      _delay_ms(.1);
    }
    data_h = USART2.RXDATAH;
    data_l = USART2.RXDATAL;
    NOKIA_putchar(x, 40, data_l, NOKIA_NORMAL);
    x+=6;
    if ((x>=84) || (data_l=='\n') || (data_l=='\r'))
    {
      x = 0;
      NOKIA_scroll(8);
    }
    NOKIA_update();
  }
}
