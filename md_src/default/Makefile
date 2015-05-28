###############################################################################
# Makefile for the project Donkey
###############################################################################

## General Flags
PROJECT = Donkey
MCU = atmega128
TARGET = Donkey.elf
CC = avr-gcc

CPP = avr-g++

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -gdwarf-2                                                                          -DF_CPU=16000000UL -Os -fsigned-char
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d 

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS += 


## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom -R .fuse -R .lock -R .signature

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings


## Include Directories
INCLUDES = -I"C:\MD\md_src\..\md_inc" 

## Libraries
LIBS = -lc -lm 

## Objects that must be built in order to link
OBJECTS = timer.o twi.o vchar.o demolcd.o lcd.o graph.o varedit.o curves.o led.o menu.o adc.o trakball.o uart.o md.o md_term.o kbd.o calc.o servo1.o servo3.o 

## Objects explicitly added by the user
LINKONLYOBJECTS = 

## Build
all: $(TARGET) Donkey.hex Donkey.eep## Compile
timer.o: ../timer.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

twi.o: ../twi.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

vchar.o: ../vchar.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

demolcd.o: ../demolcd.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

lcd.o: ../lcd.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

graph.o: ../graph.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

varedit.o: ../varedit.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

curves.o: ../curves.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

led.o: ../led.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

menu.o: ../menu.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

adc.o: ../adc.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

trakball.o: ../trakball.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

uart.o: ../uart.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

md.o: ../md.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

md_term.o: ../md_term.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

kbd.o: ../kbd.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

calc.o: ../calc.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

servo1.o: ../servo1.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

servo3.o: ../servo3.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

##Link
$(TARGET): $(OBJECTS)
	 $(CC) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

%.hex: $(TARGET)
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@

%.eep: $(TARGET)
	-avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lss: $(TARGET)
	avr-objdump -h -S $< > $@

## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECTS) Donkey.elf dep/* Donkey.hex Donkey.eep

## Other dependencies
-include $(shell mkdir dep 2>NUL) $(wildcard dep/*)

