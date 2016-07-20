/*******Header file used for communications with DS3234 RC device**********/

//Macros used to trigger SPI comms
#define SLAVE_SELECT_RTC 	PORTB &= ~(1<<PB2)	//Command to enable the RTC via the slave select pin.
#define SLAVE_DESELECT_RTC 	PORTB |= (1<<PB2)	//Command to disable the RTC via the slave select pin.

//Definitions for use by SPI.c functions
#define RTC_SS		PB2		//RTC SPI Slave Select Pin = Port B Pin 2
#define RTC_POL 	1		//RTC SPI Polarity = 1 (clock idles high) (DS2334 works in mode 1 or 3)
#define RTC_PHA 	1		//RTC SPI Phase = 1 (DS2334 works in mode 1 or 3)

//Device register addresses
#define RTC_CR_RA		0x0E	//RTC Control Register Read Address
#define RTC_CR_WA		0x8E	//RTC Control Register Write Address
#define RTC_CSR_RA		0x0F	//RTC Control/Status Register Read Address
#define RTC_CsR_WA		0x8F	//RTC Control/Status Register Write Address

#define RTC_CAOR_RA		0x10	//RTC Crystal Aging Offset Register Read Address
#define RTC_CAOR_WA		0x90	//RTC Crystal Aging Offset Register Write Address
#define RTC_TR_MSBRA	0x11	//RTC Temperature Register Most Significant Byte Read Address (Read Only)
#define RTC_TR_LSBRA	0x12	//RTC Temperature Register Least Significant Byte Read Address (Read Only)
#define RTC_TCR_RA		0x13	//RTC Temperature Control Register Read Address
#define RTC_TCR_WA		0x93	//RTC Temperature Control Register Write Address

#define RTC_SRAMAR_RA	0x18	//RTC SRAM Address Register Read Address
#define RTC_SRAMAR_WA	0x98	//RTC SRAM Address Register Write Address
#define RTC_SRAMDR_RA	0x19	//RTC SRAM Data Register Read Address
#define RTC_SRAMDR_WA	0x99	//RTC SRAM Data Register Write Address

#define RTC_SECR_RA		0x00	//RTC Seconds Register Read Address
#define RTC_SECR_WA		0x80	//RTC Seconds Register Write Address
#define RTC_MINR_RA		0x01	//RTC Minutes Register Read Address
#define RTC_MINR_WA		0x81	//RTC Minutes Register Write Address
#define RTC_HRR_RA		0x02	//RTC Hours Register Read Address
#define RTC_HRR_WA		0x82	//RTC Hours Register Write Address
#define RTC_DAYR_RA		0x03	//RTC Day Register Read Address
#define RTC_DAYR_WA		0x83	//RTC Day Register Write Address
#define RTC_DATER_RA	0x04	//RTC Date Register Read Address
#define RTC_DATER_WA	0x84	//RTC Date Register Write Address
#define RTC_MCR_RA		0x05	//RTC Month/Century Register Read Address
#define RTC_MCR_WA		0x85	//RTC Month/Century Register Write Address
#define RTC_YRR_RA		0x06	//RTC Year Register Read Address
#define RTC_YRR_WA		0x86	//RTC Year Register Write Address

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
#define RTC_EOSC	7		//RTC Enable Oscillator (Inverted)
#define RTC_BBSQW	6		//RTC Battery-Backed Square Wave Enable
#define RTC_CONV	5		//RTC Convert Temperature
#define RTC_RS2		4		//RTC Rate Select (Bit 1) - Contro Frequency of Square Wave
#define RTC_RS1		3		//RTC Rate Select (Bit 2) - Contro Frequency of Square Wave
#define RTC_INTCN	2		//RTC Interrupt Control - 0=Square Wave on !INT/SQW Pin, 1=Alarm-Match Interrupt on !INT/SQW Pin
#define RTC_A2IE	1		//RTC Alarm 2 Interrupt Enable
#define RTC_A1IE	0		//RTC Alarm 1 Interrupt Enable

//Control/Status Register Bits (Initialised to 0b11001000)
#define RTC_OSF		7		//RTC Oscillator Stop Flag - 1=Oscillator has stopped
#define RTC_BB32KHZ	6		//RTC Battery-Backed 32kHz Output - 1=Enabled, 0=Low
#define RTC_CRATE1	5		//RTC Conversion Rate Bit 1 - Controls the Sample Rate of TXCO
#define RTC_CRATE0	4		//RTC Conversion Rate Bit 2 - Controls the Sample Rate of TXCO
#define RTC_EN32KHZ	3		//RTC Enable 32kHz Output
#define RTC_BSY		2		//RTC Busy Executing TCXO functions - 1=Busy
#define RTC_A2F		1		//RTC Alarm 2 Flag
#define RTC_A1F		0		//RTC Alarm 1 Flag

struct RTC_Data				//Define a structure to hold all of the current time data from the RTC.
{
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t twelve_hr_flag;
	uint8_t day;
	uint8_t date;
	uint8_t month;
	uint8_t year;
	uint8_t century;
};

void init_RTC(void);
uint8_t RTC_SPI_readByte(uint8_t address);				//Read a byte from the RTC at address
void RTC_SPI_writeByte(uint8_t address, uint8_t data);	//Write a byte in RTC at address
void RTC_getTime (struct RTC_Data *time);				//Fill in all structure fields from data in the RTC.
