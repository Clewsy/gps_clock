//Definitions and declarations to control two cascaded MAX7219 8x7-segment display drivers (total 16x 70segment digits).

//These max7219.h and .c files will drive two cascaded MAX7219 drivers to control a total 16 digits.
//The digits are numbered 0 to 15 from left to right: DIG_0-> |8.8.8.8.8.8.8.8.8.8.8.8.8.8.8.8.| <-DIG_15
//When latched, the drivers expect 16 bits of data.  8 MSBs are the address byte, 8 LSBs are the data byte.
//The 4 MSBs of the address are "Don't cares" as far as the drivers are concerned, but this code will use the MSB to identify which driver is being used ie:
//Driver A (DIG_0 to DIG_7) will have the address byte MSB cleared (0)
//Driver B (DIG_8 to DIG_15) will have the address byte MSB set (1)

#include <avr/io.h>		//From standard AVR libraries - used for calling standard registers etc.
#include <util/delay.h>		//From the standard AVR libraries - used to call delay_ms() and delay_us() functions.
#include <spi.h>		//Used for serial communications via SPI
#include <avr/interrupt.h>	//Required to use sei(); and cli(); so interrupts can be controlled to avoid corrupting spi transmissions.

//Definitions for use by SPI.c functions
#define SEV_SEG_LOAD	PB1	//Although MAX7219 is not true SPI, LOAD pin triggers latching (on rising edge) and acts similarly to SS/CS.
#define SEV_SEG_PORT	PORTB	//Port on which the load pin is found.
#define SEV_SEG_POL	1	//Set polarity for SPI comms.
#define SEV_SEG_PHA	1	//Set phase for SPI comms

//Definitions for setting shutdown mode (address SEV_SEG_SHUTDOWN_X)
#define ON	1
#define OFF	0

//Definitions for setting the decode mode for all digits (address SEV_SEG_DECODE_MODE_X)
#define DECODE_CODE_B	0xFF
#define DECODE_MANUAL	0x00

//Macros for setting the LOAD pin on the MAX7219 low or high.  Serially shifted data is latched into the MAX7219 on a rising edge of LOAD pin.
#define SEV_SEG_LOAD_HIGH	SEV_SEG_PORT |= (1 << SEV_SEG_LOAD)	//Set LOAD pin high
#define SEV_SEG_LOAD_LOW	SEV_SEG_PORT &= ~(1 << SEV_SEG_LOAD)	//Set LOAD pin low

//Driver A Register Addresses (MSB cleared to identify as driver A in code (MSB ignored my driver))
#define	SEV_SEG_NO_OP_A		0x00	//No operation (ignore lower byte).
#define	SEV_SEG_DIGIT_0		0x01
#define	SEV_SEG_DIGIT_1		0x02
#define	SEV_SEG_DIGIT_2		0x03
#define	SEV_SEG_DIGIT_3		0x04
#define	SEV_SEG_DIGIT_4		0x05
#define	SEV_SEG_DIGIT_5		0x06
#define	SEV_SEG_DIGIT_6		0x07
#define	SEV_SEG_DIGIT_7		0x08
#define	SEV_SEG_DECODE_MODE_A	0x09	//Controls Code B Font for each digit. Simple codes to display 0-9,-,H,E,L,P. 0=off, 1=on for each bit.
#define	SEV_SEG_INTENSITY_A	0x0A	//Controls intensity (brightness) by setting duty cycle - range from 0x00(min) to 0x0F(max)
#define	SEV_SEG_SCAN_LIMIT_A	0x0B	//Controls number of digits in the display (0-8) - 0=One digit, 1=Two digits...7=Eight digits.
#define	SEV_SEG_SHUTDOWN_A	0x0C	//Controls shutdown mode. 0=Shutdown, 1=Normal Operation
#define	SEV_SEG_DISPLAY_TEST_A	0x0F

//Driver B Register Addresses (MSB set to identify as driver B in code (MSB ignored my driver))
#define	SEV_SEG_NO_OP_B		0x80	//No operation (ignore lower byte).
#define	SEV_SEG_DIGIT_8		0x81
#define	SEV_SEG_DIGIT_9		0x82
#define	SEV_SEG_DIGIT_10	0x83
#define	SEV_SEG_DIGIT_11	0x84
#define	SEV_SEG_DIGIT_12	0x85
#define	SEV_SEG_DIGIT_13	0x86
#define	SEV_SEG_DIGIT_14	0x87
#define	SEV_SEG_DIGIT_15	0x88
#define	SEV_SEG_DECODE_MODE_B	0x89	//Controls Code B Font for each digit. Simple codes to display 0-9,-,H,E,L,P. 0=off, 1=on for each bit.
#define	SEV_SEG_INTENSITY_B	0x8A	//Controls intensity (brightness) by setting duty cycle - range from 0x00(min) to 0x0F(max)
#define	SEV_SEG_SCAN_LIMIT_B	0x8B	//Controls number of digits in the display (0-8) - 0=One digit, 1=Two digits...7=Eight digits.
#define	SEV_SEG_SHUTDOWN_B	0x8C	//Controls shutdown mode. 0=Shutdown, 1=Normal Operation
#define	SEV_SEG_DISPLAY_TEST_B	0x8F

//Non-code B Data (i.e. manual config of 7 segments)
//	Segments represented by bits:
//	MSb-> DP A B C D E F G <-LSb
//	  _A_
//	F|_G_|B
//	E|___|C .DP
//	   D
#define SEV_SEG_MANUAL_0	0b01111110
#define SEV_SEG_MANUAL_1	0b00110000
#define SEV_SEG_MANUAL_6	0b01011111
#define SEV_SEG_MANUAL_8	0b01111111
#define SEV_SEG_MANUAL_A	0b01110111
#define SEV_SEG_MANUAL_B	0b00011111
#define SEV_SEG_MANUAL_C	0b01001110
#define SEV_SEG_MANUAL_D	0b00111101
#define SEV_SEG_MANUAL_E	0b01001111
#define SEV_SEG_MANUAL_F	0b01000111
#define SEV_SEG_MANUAL_G	0b01111011
#define SEV_SEG_MANUAL_H	0b00110111
#define SEV_SEG_MANUAL_I	0b00110000
#define SEV_SEG_MANUAL_J	0b00111100
#define SEV_SEG_MANUAL_L	0b00001110
#define SEV_SEG_MANUAL_N	0b01110110
#define SEV_SEG_MANUAL_O	0b01111110
#define SEV_SEG_MANUAL_P	0b01100111
#define SEV_SEG_MANUAL_R	0b00000101
#define SEV_SEG_MANUAL_S	0b01011011
#define SEV_SEG_MANUAL_T	0b00001111
#define SEV_SEG_MANUAL_U	0b00111110
#define SEV_SEG_MANUAL_Y	0b00111011
#define SEV_SEG_MANUAL_BLANK	0b00000000
#define SEV_SEG_MANUAL_DASH	0b00000001

//Code B Data - pass to digit registers if code B font is enabled.  Note, DP set by MSB.
#define SEV_SEG_CODEB_DASH	0x0A	//G segment only (i.e. a "-") (note, DP off)
#define SEV_SEG_CODEB_E		0x0B	//Code B encoded for displaying character 'E'
#define SEV_SEG_CODEB_H		0x0C	//Code B encoded for displaying character 'H'
#define SEV_SEG_CODEB_L		0x0D	//Code B encoded for displaying character 'L'
#define SEV_SEG_CODEB_P		0x0E	//Code B encoded for displaying character 'P'
#define SEV_SEG_CODEB_BLANK	0x0F	//All segments off (inc. DP).

//OR with data sent to digit to turn on the decimal point (i.e. MSb toggles the DP).
#define SEV_SEG_DP	0x80

//MAX7219 control function declarations.
void sev_seg_write_byte(uint8_t address, uint8_t data);	//Writes a byte to an address the MAX7219s.  One driver will receive data, the other will receive no-op.
void sev_seg_init(void);				//Initialise both the display drivers.
void sev_seg_all_clear(void);				//Clears all digits (needs to be in CODE-B mode).
void sev_seg_power(uint8_t on_or_off);			//Turns display on or off without changing any other registers.
void sev_seg_decode_mode(uint8_t decode_mode);		//Sets all digits on both drivers to decode mode "manual" or "code B".
void sev_seg_display_int(uint64_t num);			//Takse any 64-bit integer and displays the decimal value using the 16 7-seg digits.
