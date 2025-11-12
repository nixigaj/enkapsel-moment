PROGRAM_NAME = blink
SRC_DIR = src
BUILD_DIR = build
TARGET := avr32da28
PORT := ttyACM0
all: $(PROGRAM_NAME)


# Create the build directory
$(BUILD_DIR):
	mkdir -pv $(BUILD_DIR)
# Compile and build the program for Atmega328P
$(PROGRAM_NAME): $(BUILD_DIR)
	avr-gcc -mmcu=$(TARGET) -Wall -Os -o $(BUILD_DIR)/$(PROGRAM_NAME).elf $(SRC_DIR)/$(PROGRAM_NAME).c
	avr-objcopy -j .text -j .data -O ihex $(BUILD_DIR)/$(PROGRAM_NAME).elf $(BUILD_DIR)/$(PROGRAM_NAME).hex

asm: $(BUILD_DIR)
	avr-gcc -mmcu=$(TARGET) -Wall -Os -g -S $(SRC_DIR)/$(PROGRAM_NAME).c -o $(BUILD_DIR)/$(PROGRAM_NAME).S

# Upload the built program (hex file) to Atmega168 using USBasp
upload: $(PROGRAM_NAME)
	avrdude -c serialupdi -p avr32da28 -P /dev/ttyACM0 -U flash:w:$(BUILD_DIR)/$(PROGRAM_NAME).hex

# Remove build directory with all built files
clean:
	rm -rf $(BUILD_DIR) 


