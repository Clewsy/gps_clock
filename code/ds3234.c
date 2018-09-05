//Functions used for communications and control with DS3234 RTC device
//Note spi.h is required for functions and definitions.

#include <ds3234.h>

//Initialise the RTC (actually initialise the AVR to use SPI comms with the RTC).
void rtc_init(void)
{
	SPI_DDR |= (1 << RTC_SS);	//Set slave select pin as an output.
	SPI_PORT |= (1 << RTC_SS);	//Start SS off not selected (i.e. high as SS is inverted)
}

//Reads and returns a byte at the provided address.
uint8_t rtc_read_byte(uint8_t address)
{
	RTC_ENABLE;			//Enable the RTC comms
	spi_trade_byte(address);	//Send the address to be read from
	spi_trade_byte(0);		//Send dummy byte to load SPDR with byte at address
	RTC_DISABLE;			//Disable the RTC comms
	return(SPDR);			//Return the byte.  SPDR=SPI Data Register
}

//Writes a byte to the desired address.
void rtc_write_byte(uint8_t address, uint8_t data)
{
	RTC_ENABLE;			//Enable the RTC comms
	spi_trade_byte(address);	//Send the address to be written to
	spi_trade_byte(data);		//Send the data to be written to the provided address
	RTC_DISABLE;			//Disable the RTC comms
}

//Fill in all array fields from data in the RTC.  All bytes are BCD representations of data.
//Note, the century indicator is a flag contained in the byte shared with the BCD encoded month value (RTC_MCR_RA: Month/Century Register Read Address).
//Refer to the address map in the ds3234 datasheet.
void rtc_get_time(uint8_t *time)
{
	time[SEC_ONES] = (rtc_read_byte(RTC_SECR_RA) & 0b1111);		//B7=0, B6-B4=(10 seconds), B3-B0=(seconds)
	time[SEC_TENS] = (rtc_read_byte(RTC_SECR_RA) >> 4);
	time[MIN_ONES] = (rtc_read_byte(RTC_MINR_RA) & 0b1111);		//B7=0, B6-B4=(10 minutes), B3-B0=(minutes)
	time[MIN_TENS] = (rtc_read_byte(RTC_MINR_RA) >> 4);
	time[HOU_ONES] = (rtc_read_byte(RTC_HRR_RA) & 0b1111);		//B7-B6=0, B5=(20 hours), B4=(10 hours), B3-B0=(hours)
	time[HOU_TENS] = (rtc_read_byte(RTC_HRR_RA) >> 4);
	time[DAT_ONES] = (rtc_read_byte(RTC_DATER_RA) & 0b1111);	//B7-B6=0, B5-B4=(10 date), B3-B0=(date)
	time[DAT_TENS] = (rtc_read_byte(RTC_DATER_RA) >> 4);
	time[MON_ONES] = (rtc_read_byte(RTC_MCR_RA) & 0b1111);		//B7-B5=0, B4=(10 month), B3-B0=(month)
	time[MON_TENS] = ((rtc_read_byte(RTC_MCR_RA) >> 4) & 0b111);	//	(Note need to ignore century bit).
	time[YEA_ONES] = (rtc_read_byte(RTC_YRR_RA) & 0b1111);		//B7-B4=(10 year), B3-B0=(year)
	time[YEA_TENS] = (rtc_read_byte(RTC_YRR_RA) >> 4);
	if(rtc_read_byte(RTC_MCR_RA) & 0b10000000)			//B7= Century flag. Note, century is either 19 or 20, so if bit is clear, year is 19XX.
	{
		time[CEN_ONES] = 0;
		time[CEN_TENS] = 2;
	}
	else
	{
		time[CEN_ONES] = 9;
		time[CEN_TENS] = 1;
	}

}

//This function will take the current values of the time array, convert to equivalent BCD encoded values and write to the RTC effectively setting the clock.
//Refer to the address map in the ds3234 datasheet.
void rtc_set_time(uint8_t *time)
{
	rtc_write_byte(RTC_SECR_WA,	((time[SEC_TENS] << 4) | time[SEC_ONES]));			//Set the RTC seconds (00-59).
	rtc_write_byte(RTC_MINR_WA,	((time[MIN_TENS] << 4) | time[MIN_ONES]));			//Set the RTC minutes (00-59).
	rtc_write_byte(RTC_HRR_WA,	((time[HOU_TENS] << 4) | time[HOU_ONES]));			//Set the RTC hours (00-24).
	rtc_write_byte(RTC_DATER_WA,	((time[DAT_TENS] << 4) | time[DAT_ONES]));			//Set the RTC date (01-31).
	rtc_write_byte(RTC_MCR_WA,	(((time[MON_TENS] << 4) | time[MON_ONES])) | (0b10000000));	//Set the RTC month and century (01-12 & 19/20).
	rtc_write_byte(RTC_YRR_WA,	((time[YEA_TENS] << 4) | time[YEA_ONES]));			//Set the RTC year (00-99).

	//Note the century flag at address RTC_MCR_WA is always set to indicate year 20xx.
}
