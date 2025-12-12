#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdio.h>
#include "nokia5110_hspi.h"

// hardware I2C address
#define DS1307_ADDR  0xD0

// definition of the DS1307 hardware registers - eight 8-bit registers
#define DSREGS 8
#define dsSEC 0
#define dsMIN 1
#define dsHOUR 2
#define dsDOW 3
#define dsDAY 4
#define dsMONTH 5
#define dsYEAR 6
#define dsSTATUS 7

// predefined values to set the real-time clock: seconds minutes hours day date  month  year control
//  20:26:30 on Sunday (7) 7 December 2025
static const uint8_t DS1307_const[DSREGS] = { 0x30, 0x26, 0x20, 7, 0x07, 0x12, 0x25,  0b00000000};

// memory copy of the DS1307 registers
uint8_t DS1307_regs[DSREGS];

//
// TWI/I2C helper functions (by Microchip)
//
#define TWI_BAUD(F_SCL, T_RISE)   ((((((float)F_CPU / (float)F_SCL)) - 10 - ((float)F_CPU * T_RISE))) / 2)
#define TWI0_SLAVE_RESPONSE_ACKED (!(TWI_RXACK_bm & TWI0.MSTATUS))
#define TWI0_DATA_RECEIVED        (TWI_RIF_bm & TWI0.MSTATUS)
#define TWI0_IS_CLOCKHELD()       TWI0.MSTATUS & TWI_CLKHOLD_bm
#define TWI0_IS_BUSERR()          TWI0.MSTATUS & TWI_BUSERR_bm
#define TWI0_IS_ARBLOST()         TWI0.MSTATUS & TWI_ARBLOST_bm
#define TWI0_IS_BUSBUSY()         ((TWI0.MSTATUS & TWI_BUSSTATE_BUSY_gc) == TWI_BUSSTATE_BUSY_gc)
#define TWI0_WAIT() while (!((TWI0_IS_CLOCKHELD()) || (TWI0_IS_BUSERR()) || (TWI0_IS_ARBLOST()) || (TWI0_IS_BUSBUSY())))

void i2c_init(void)
{
  PORTMUX.TWIROUTEA = PORTMUX_TWI0_DEFAULT_gc;
  TWI0.CTRLA        = TWI_SDAHOLD_50NS_gc;
  TWI0.DUALCTRL     = 0b00000000;
  TWI0.MBAUD        = TWI_BAUD(100000, 0);
  TWI0.MCTRLA       = TWI_TIMEOUT_200US_gc | TWI_ENABLE_bm;
  TWI0.MSTATUS      = TWI_RIF_bm | TWI_WIF_bm | TWI_CLKHOLD_bm
                    | TWI_RXACK_bm | TWI_ARBLOST_bm
                    | TWI_BUSERR_bm | TWI_BUSSTATE_IDLE_gc;
}

static void TWI0_sendMasterCommand(uint8_t newCommand)
{
    TWI0.MCTRLB |=  newCommand;
}

static void TWI0_setACKAction(void)
{
    TWI0.MCTRLB &= !TWI_ACKACT_bm;
}

static void TWI0_setNACKAction(void)
{
    TWI0.MCTRLB |= TWI_ACKACT_bm;
}

//
// Dallas DS1307 RTC Hardware routines
//

// read the eight registers from the DS1307 into the memory
void DS1307read(void)
{
  uint8_t i;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    TWI0.MADDR = DS1307_ADDR | 0;   // I2C write
    TWI0_WAIT();

    TWI0.MDATA = 0x00;              // read starting at reg 0
    TWI0_WAIT();

    _delay_us(200);

    TWI0.MADDR = DS1307_ADDR | 1;   // I2C read
    TWI0_WAIT();

    for (i=0; i<(DSREGS-1); i++)
    {
      _delay_us(10);
      TWI0_WAIT();
      DS1307_regs[i]  = TWI0.MDATA;
      TWI0_setACKAction();
      TWI0_sendMasterCommand(TWI_MCMD_RECVTRANS_gc);
    }
    _delay_us(10);
    TWI0_WAIT();
    DS1307_regs[DSREGS-1] = TWI0.MDATA;
    TWI0_setNACKAction();
    TWI0_sendMasterCommand(TWI_MCMD_STOP_gc);
  }
}

// write the contents of the memory into the eight hardware registers of the DS1307
void DS1307write(void)
{
  uint8_t i;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    TWI0.MADDR = DS1307_ADDR | 0;   // I2C write
    TWI0_WAIT();

    TWI0.MDATA = 0x00;              // write starting at reg 0
    TWI0_WAIT();

    for (i=0; i<DSREGS; i++)
    {
      TWI0.MDATA = DS1307_regs[i];  // write to DS1307
      TWI0_WAIT();
    }
    TWI0.MCTRLB = TWI_MCMD_STOP_gc; // stop condition
    TWI0_sendMasterCommand(TWI_MCMD_STOP_gc);
  }
}

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

void init(void)
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
    0xb5,   //vop,
    NOKIA_ORIENTATION_180
  );
  NOKIA_update();
  _delay_ms(1000);
  NOKIA_clear();
  NOKIA_print(0,40,"Hello World!",NOKIA_NORMAL);
  NOKIA_update();

  UARTInit(19200);

  i2c_init();
}

int main(void)
{
  char textbuffer[20];

  init();

  DS1307read();
  sprintf(textbuffer, "%02x:%02x:%02x %02x", DS1307_regs[2],DS1307_regs[1],DS1307_regs[0],DS1307_regs[7]);

  NOKIA_scroll(8);
  NOKIA_print(0, 40, textbuffer, NOKIA_NORMAL);
  NOKIA_update();
  _delay_ms(2000);
  NOKIA_scroll(4);

  for (uint8_t i = 0; i < sizeof(DS1307_const); i++)
  {
    DS1307_regs[i] = DS1307_const[i];
  }

  DS1307write();
  _delay_ms(500);

  while (1)
  {
    DS1307read();
    sprintf(textbuffer, "%02x:%02x:%02x %02x", DS1307_regs[2],DS1307_regs[1],DS1307_regs[0],DS1307_regs[7]);

    NOKIA_scroll(8);
    NOKIA_print(0, 40, textbuffer, NOKIA_NORMAL);
    NOKIA_update();
    _delay_ms(900);
  }
}
