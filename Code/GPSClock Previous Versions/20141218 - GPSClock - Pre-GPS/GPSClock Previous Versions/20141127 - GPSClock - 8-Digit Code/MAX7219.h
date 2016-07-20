
// Outputs, pin definitions



//Definitions for use by SPI.c functions
#define SEV_SEG_LOAD	PB1		//Although MAX7219 is not true SPI, LOAD pin triggers latching (on rising edge) and acts similarly to SS/CS.
#define SEV_SEG_POL		1		//Set polarity for SPI comms.
#define SEV_SEG_PHA		1		//Set phase for SPI comms


//Macros for setting the LOAD pin on the MAX7219 low or high.  Serially shifted data is latched into the MAX7219 on a rising edge of LOAD pin.
#define SEV_SEG_LOAD_HIGH		PORTB |= (1 << SEV_SEG_LOAD)	//Set LOAD pin high
#define SEV_SEG_LOAD_LOW		PORTB &= ~(1 << SEV_SEG_LOAD)	//Set LOAD pin low

//Register Addresses
#define	SEV_SEG_NO_OP			0x00
#define	SEV_SEG_DIGIT_0			0x01
#define	SEV_SEG_DIGIT_1			0x02
#define	SEV_SEG_DIGIT_2			0x03
#define	SEV_SEG_DIGIT_3			0x04
#define	SEV_SEG_DIGIT_4			0x05
#define	SEV_SEG_DIGIT_5			0x06
#define	SEV_SEG_DIGIT_6			0x07
#define	SEV_SEG_DIGIT_7			0x08
#define	SEV_SEG_DECODE_MODE		0x09	//Controls Code B Font for each digit - Simple data codes for digit registers to display 0-9,-,H,E,L,P. 0=off, 1=on for each bit. 
#define	SEV_SEG_INTENSITY		0x0A	//Controls intensity (brightness) by setting duty cycle - range from 0x00(min) to 0x0F(max)
#define	SEV_SEG_SCAN_LIMIT		0x0B	//Controls number of digits in the display (0-8) - 0=One digit, 1=Two digits...7=Eight digits.
#define	SEV_SEG_SHUTDOWN		0x0C	//Controls shutdown mode. 0=Shutdown, 1=Normal Operation
#define	SEV_SEG_DISPLAY_TEST	0x0F



//Code B Data - pass to digit registers if code B font is enabled.  Note, DP set by MSB.
#define SEV_SEG_CODEB_BLANK		0x0F	//All segments off (inc. DP).
#define SEV_SEG_CODEB_DASH		0x0A	//G segment only (i.e. a "-") (note, DP off)

 
void SEV_SEG_writeByte(uint8_t address, uint8_t data);

void init_SEV_SEG(void);

void SEV_SEG_allClear(void);

void SEV_SEG_startupAni(void);

//void MAX7219_clearDisplay(void);
 
//void MAX7219_displayNumber(volatile long number);
