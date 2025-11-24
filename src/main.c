#define F_CPU 4000000UL
#define BAUD_RATE 115200

// Calculate Baud Register
#define USART0_BAUD_VAL ((float)(4.0 * F_CPU / (BAUD_RATE)) + 0.5)

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

void USART0_init(void) {
    PORTA.DIR |= PIN0_bm;       // TX Output
    PORTA.DIR &= ~PIN1_bm;      // RX Input
    USART0.BAUD = (uint16_t)USART0_BAUD_VAL;
    USART0.CTRLB |= USART_TXEN_bm | USART_RXEN_bm;
}

void LED_init(void) {
    PORTA.DIR |= PIN7_bm;
}

void USART0_sendChar(const char c) {
    while (!(USART0.STATUS & USART_DREIF_bm)) {}
    USART0.TXDATAL = c;
}

int USART0_printChar(const char c, FILE *stream) {
    USART0_sendChar(c);
    return 0;
}

FILE USART_stream = FDEV_SETUP_STREAM(USART0_printChar, NULL, _FDEV_SETUP_WRITE);

int main(void) {
    USART0_init();
    LED_init();

    stdout = &USART_stream;

    uint16_t count = 0;

    while (1) {
        printf("Counter: %d\n", count++);

        PORTA.OUT |= PIN7_bm;
        _delay_ms(500);

        PORTA.OUT &= ~PIN7_bm;
        _delay_ms(500);
    }
}
