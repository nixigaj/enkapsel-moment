.PHONY: upload run clean

all: upload

upload:
	pio run -t upload -e AVR32DA28

run:
	pio run -t upload -t monitor -e AVR32DA28

clean:
	rm -rf .pio
