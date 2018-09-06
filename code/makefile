## Note: Use http://www.engbedded.com/fusecalc/ for fuse setting info
##########------------------------------------------------------##########
##########              Project-specific Details                ##########
##########    Check these every time you start a new project    ##########
##########------------------------------------------------------##########

MCU   = atmega328p
F_CPU = 8000000UL
BAUD  = 9600UL
## Also try BAUD = 19200 or 38400 if you're feeling lucky.

## A directory for common include files and the simple USART library.
## If you move either the current folder or the Library folder, you'll
##  need to change this path to match.
#!#LIBDIR = ../../AVR-Programming-Library

##########------------------------------------------------------##########
##########                 Programmer Defaults                  ##########
##########          Set up once, then forget about it           ##########
##########        (Can override.  See bottom of file.)          ##########
##########------------------------------------------------------##########

PROGRAMMER_TYPE = usbtiny
# extra arguments to avrdude: baud rate, chip type, -F flag, etc.
PROGRAMMER_ARGS =

##########------------------------------------------------------##########
##########                  Program Locations                   ##########
##########     Won't need to change if they're in your PATH     ##########
##########------------------------------------------------------##########

CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
AVRSIZE = avr-size
AVRDUDE = avrdude

##########------------------------------------------------------##########
##########                   Makefile Magic!                    ##########
##########         Summary:                                     ##########
##########             We want a .hex file                      ##########
##########        Compile source files into .elf                ##########
##########        Convert .elf file into .hex                   ##########
##########        You shouldn't need to edit below.             ##########
##########------------------------------------------------------##########

## The name of your project (without the .c)
TARGET = gps_clock
## Or name it automatically after the enclosing directory
#!#TARGET = $(lastword $(subst /, ,$(CURDIR)))

# Object files: will find all .c/.h files in current directory
#  and in LIBDIR.  If you have any other (sub-)directories with code,
#  you can add them in to SOURCES below in the wildcard statement.
#!#SOURCES=$(wildcard *.c $(LIBDIR)/*.c)

##
##SOURCES=$(TARGET).c usart.c i2c.c ssd1306.c rtc.c
##
SOURCES=$(TARGET).c usart.c spi.c max7219.c ds3234.c
OBJECTS=$(SOURCES:.c=.o)
HEADERS=$(SOURCES:.c=.h)


## C++ options
CPPFLAGS = -DF_CPU=$(F_CPU) -BAUD=$(BAUD) -I.
#### notes
###### -DF_CPU=$(FCPU) defines the CPU frequency for use in some libraries (e.g. _delayms()).  Needed if F_CPU not #defined in code.
###### -BAUD=$(BAUD) defines the serial comms baud rate for setting USART registers.  Needed if not #defined in code.
###### -I. (or I<dir>adds the current directory (.) to the head of the list of directories to be searched for header files.

## Compiler options
CFLAGS = -Os -g -std=gnu99 -Wall
#### notes
###### -Os used to optimise compilation for size
###### -g  used to produce debugging information in the operating system's native format
###### -std=gnu99 used to define the standard language (-std) as gnu99
###### -Wall used to enable (all) (W)arnings

## Linker options
#!#LDFLAGS = -Wl,-Map,$(TARGET).map
#### notes
###### -Wl: -Wl,<option> to pass an options to the linker.  Or multiple options separated by commas.
###### -Map,$(TARGET).map creates a $(TARGET).map file. Useful for monitoring the sizes of your code and data.

TARGET_ARCH = -mmcu=$(MCU)

## Explicit pattern rules:
#### Notes:
###### $@ evaluates to the file name of the target of the rule.
###### $< evaluates to the name of the first prerequisite (usually the source file)
###### $% evaluates to the target member name, when the target is an archive member.
###### $^ evaluates to the names of all the prerequisites, with spaces between them.

##  Make .o (object) files from .c files
%.o: %.c $(HEADERS) makefile
	 $(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<;
#### Notes:
###### -c: Compile or assemble the source files, but do not link.
###### -o <file>: Place output in file <file>.


## To make .elf (executable and linkable format) file from .o files
$(TARGET).elf: $(OBJECTS)
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ -o $@

## Make a .hex file from the .elf file.
%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

all: $(TARGET).hex

# Optionally show how big the resulting program is
size:  $(TARGET).elf
	$(AVRSIZE) -C --mcu=$(MCU) $(TARGET).elf

# Delete all the $(TARGET).* files
clean:
	rm -f $(TARGET).elf $(TARGET).hex $(TARGET).obj \
	$(TARGET).o $(TARGET).d $(TARGET).eep $(TARGET).lst \
	$(TARGET).lss $(TARGET).sym $(TARGET).map $(TARGET)~ \
	$(TARGET).eeprom

# Delete all the files generated by compile operations
squeaky_clean:
	rm -f *.elf *.hex *.obj *.o *.d *.eep *.lst *.lss *.sym *.map *~ *.eeprom

##########------------------------------------------------------##########
##########              Programmer-specific details             ##########
##########           Flashing code to AVR using avrdude         ##########
##########------------------------------------------------------##########
program: $(TARGET).hex
	$(AVRDUDE) -c $(PROGRAMMER_TYPE) -p $(MCU) $(PROGRAMMER_ARGS) -U flash:w:$<

avr_terminal:
	$(AVRDUDE) -c $(PROGRAMMER_TYPE) -p $(MCU) -nt
