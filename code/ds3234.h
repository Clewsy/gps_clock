//Definitions and declarations used for spi communications and control with DS3234 RTC device.

#include <avr/io.h>
#include <spi.h>

//Definitions for use by SPI.c functions
#define RTC_SS		PB2	//RTC SPI Slave Select Pin = Port B Pin 2
#define RTC_POL 	1	//RTC SPI Polarity = 1 (clock idles high) (DS2334 works in mode 1 or 3)
#define RTC_PHA 	1	//RTC SPI Phase = 1 (DS2334 works in mode 1 or 3)

//Macros used to trigger SPI comms (SPI_PORT defined in spi.h)
#define RTC_ENABLE 	SPI_PORT &= ~(1<<RTC_SS)	//Command to enable the RTC via the slave select pin.
#define RTC_DISABLE 	SPI_PORT |= (1<<RTC_SS)		//Command to disable the RTC via the slave select pin.

//Device register addresses - copied from datasheet, not all used in this application.
#define RTC_CR_RA	0x0E	//RTC Control Register Read Address
#define RTC_CR_WA	0x8E	//RTC Control Register Write Address
#define RTC_CSR_RA	0x0F	//RTC Control/Status Register Read Address
#define RTC_CsR_WA	0x8F	//RTC Control/Status Register Write Address

#define RTC_CAOR_RA	0x10	//RTC Crystal Aging Offset Register Read Address
#define RTC_CAOR_WA	0x90	//RTC Crystal Aging Offset Register Write Address
#define RTC_TR_MSBRA	0x11	//RTC Temperature Register Most Significant Byte Read Address (Read Only)
#define RTC_TR_LSBRA	0x12	//RTC Temperature Register Least Significant Byte Read Address (Read Only)
#define RTC_TCR_RA	0x13	//RTC Temperature Control Register Read Address
#define RTC_TCR_WA	0x93	//RTC Temperature Control Register Write Address

#define RTC_SRAMAR_RA	0x18	//RTC SRAM Address Register Read Address
#define RTC_SRAMAR_WA	0x98	//RTC SRAM Address Register Write Address
#define RTC_SRAMDR_RA	0x19	//RTC SRAM Data Register Read Address
#define RTC_SRAMDR_WA	0x99	//RTC SRAM Data Register Write Address

#define RTC_SECR_RA	0x00	//RTC Seconds Register Read Address
#define RTC_SECR_WA	0x80	//RTC Seconds Register Write Address
#define RTC_MINR_RA	0x01	//RTC Minutes Register Read Address
#define RTC_MINR_WA	0x81	//RTC Minutes Register Write Address
#define RTC_HRR_RA	0x02	//RTC Hours Register Read Address
#define RTC_HRR_WA	0x82	//RTC Hours Register Write Address
#define RTC_DAYR_RA	0x03	//RTC Day Register Read Address
#define RTC_DAYR_WA	0x83	//RTC Day Register Write Address
#define RTC_DATER_RA	0x04	//RTC Date Register Read Address
#define RTC_DATER_WA	0x84	//RTC Date Register Write Address
#define RTC_MCR_RA	0x05	//RTC Month/Century Register Read Address
#define RTC_MCR_WA	0x85	//RTC Month/Century Register Write Address
#define RTC_YRR_RA	0x06	//RTC Year Register Read Address
#define RTC_YRR_WA	0x86	//RTC Year Register Write Address

#define RTC_A1SECR_RA	0x07	//RTC Alarm 1 Seconds Register Read Address
#define RTC_A1SECR_WA	0x87	//RTC Alarm 1 Seconds Register Write Address
#define RTC_A1MINR_RA	0x08	//RTC Alarm 1 Minutes Register Read Address
#define RTC_A1MINR_WA	0x88	//RTC Alarm 1 Minutes Register Write Address
#define RTC_A1HRR_RA	0x09	//RTC Alarm 1 Hours Register Read Address
#define RTC_A1HRR_WA	0x89	//RTC Alarm 1 Hours Register Write Address
#define RTC_A1DDR_RA	0x0A	//RTC Alarm 1 Day/Date Register Read Address
#define RTC_A1DDR_WA	0x8A	//RTC Alarm 1 Day/Date Register Write Address

#define RTC_A2MINR_RA	0x0B	//RTC Alarm 2 Minutes Register Read Address
#define RTC_A2MINR_WA	0x8B	//RTC Alarm 2 Minutes Register Write Address
#define RTC_A2HRR_RA	0x0C	//RTC Alarm 2 Hours Register Read Address
#define RTC_A2HRR_WA	0x8C	//RTC Alarm 2 Hours Register Write Address
#define RTC_A2DDR_RA	0x0D	//RTC Alarm 2 Day/Date Register Read Address
#define RTC_A2DDR_WA	0x8D	//RTC Alarm 2 Day/Date Register Write Address

//Control Register Bits (Initialised to 0b00011100)
#define RTC_EOSC	7	//RTC Enable Oscillator (Inverted)
#define RTC_BBSQW	6	//RTC Battery-Backed Square Wave Enable
#define RTC_CONV	5	//RTC Convert Temperature
#define RTC_RS2		4	//RTC Rate Select (Bit 1) - Contro Frequency of Square Wave
#define RTC_RS1		3	//RTC Rate Select (Bit 2) - Contro Frequency of Square Wave
#define RTC_INTCN	2	//RTC Interrupt Control - 0=Square Wave on !INT/SQW Pin, 1=Alarm-Match Interrupt on !INT/SQW Pin
#define RTC_A2IE	1	//RTC Alarm 2 Interrupt Enable
#define RTC_A1IE	0	//RTC Alarm 1 Interrupt Enable

//Control/Status Register Bits (Initialised to 0b11001000)
#define RTC_OSF		7	//RTC Oscillator Stop Flag - 1=Oscillator has stopped
#define RTC_BB32KHZ	6	//RTC Battery-Backed 32kHz Output - 1=Enabled, 0=Low
#define RTC_CRATE1	5	//RTC Conversion Rate Bit 1 - Controls the Sample Rate of TXCO
#define RTC_CRATE0	4	//RTC Conversion Rate Bit 2 - Controls the Sample Rate of TXCO
#define RTC_EN32KHZ	3	//RTC Enable 32kHz Output
#define RTC_BSY		2	//RTC Busy Executing TCXO functions - 1=Busy
#define RTC_A2F		1	//RTC Alarm 2 Flag
#define RTC_A1F		0	//RTC Alarm 1 Flag

//Define array elements for easier identification when pulled from time array.
#define CEN_TENS 0
#define CEN_ONES 1
#define YEA_TENS 2
#define YEA_ONES 3
#define MON_TENS 4
#define MON_ONES 5
#define DAT_TENS 6
#define DAT_ONES 7
#define HOU_TENS 8
#define HOU_ONES 9
#define MIN_TENS 10
#define MIN_ONES 11
#define SEC_TENS 12
#define SEC_ONES 13
#define SIZE_OF_TIME_ARRAY 14

//Define numerical values for each month
#define JAN  1
#define FEB  2
#define MAR  3
#define APR  4
#define MAY  5
#define JUN  6
#define JUL  7
#define AUG  8
#define SEP  9
#define OCT 10
#define NOV 11
#define DEC 12

//Function declarations
void rtc_init(void);					//Initialise the RTC (actually initialise the AVR to use SPI comms with the RTC).
uint8_t rtc_read_byte(uint8_t address);			//Reads and returns a byte at the desired address.
void rtc_write_byte(uint8_t address, uint8_t data);	//Writes a byte to the desired address.
void rtc_get_time(uint8_t *time);			//Fill in all array fields from data in the RTC.  All bytes are BCD representations of data..
void rtc_set_time(uint8_t *time);			//Set the clock to the time as per the current values in the "time" array.
