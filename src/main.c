#include <avr/io.h>
#include <util/delay.h>

int main(void) {
	PORTD.DIR = 0b11111111;
	PORTC.DIR = 0b11111111;

	//PORTD.PINCONFIG = PORT_PULLUPEN_bm;
	//PORTD.PINCTRLUPD = 0b11111111;

	uint8_t font[10] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f};
	uint8_t nums[4]  = {6, 7, 6, 7};
	uint8_t number = 0;
	for (;;) {
		//PORTA.OUT = PORTD.IN;
		for (int i = 0; i < 10; ++i) {

			for (uint8_t j = 0; j < 4; ++j) {
				PORTD.OUT = 255;
				PORTC.OUT = (1 << j);
				PORTD.OUT = ~font[nums[j]];
				_delay_us(10);
			}

			//PORTD.OUT = ~0x4f;
			//number = (number + 1) % 4;

			//_delay_ms(1);
		}
	}
}
