MCU = avr32da28
F_CPU = 4000000UL
CC = avr-gcc
OBJCOPY = avr-objcopy

CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -std=gnu11
SRC_DIR = src
BUILD_DIR = build

COMMON_SRC = $(SRC_DIR)/nokia5110_hspi.c
LAB_SRCS = $(wildcard $(SRC_DIR)/labb_*.c)
ELFS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.elf, $(LAB_SRCS))
HEXS = $(patsubst %.elf, %.hex, $(ELFS))

all: $(HEXS)

$(BUILD_DIR)/%.elf: $(SRC_DIR)/%.c $(COMMON_SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@

%.hex: %.elf
	$(OBJCOPY) -O ihex $< $@

clean:
	rm -rf $(BUILD_DIR)

# Generate per-target upload rules
UPLOAD_TARGETS = $(patsubst $(BUILD_DIR)/%.hex, upload_%, $(HEXS))

$(UPLOAD_TARGETS): upload_%: $(BUILD_DIR)/%.hex
	avrdude \
		-c serialupdi \
		-p 32da28 \
		-P /dev/ttyACM0 \
		-U flash:w:$<:i

monitor:
	screen /dev/ttyACM1 19200

.PHONY: all clean monitor

