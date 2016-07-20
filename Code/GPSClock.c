//GPS clock main programme function.

#include <GPSClock_Support.h>

int main(void)
{
	initClock();							//Initialise the system clock.
	initUSART();							//Initialises the USART for receive/transmit data, 8-bit with 1 stop bit.
	printString("\r\n\n\nISO 8601\r\n");	//Initialisation test for USART.
	initSPI(SPI_POL, SPI_PHA);				//Initialise SPI for RTC - mode 3 (compatible mode 1 or 3) - Polarity=0, Phase=1 and slave select on PB2
	init_RTC();								//Initialise for comms with DS3234 real-time clock.
	init_SEV_SEG();							//Initialise for comms with 2x MAX7219 8x7-seg LED drivers.
	SEV_SEG_startupAni();					//Initialisation test for the 16 7-seg LED display.
	init_buttons();							//Initialise pins connected to two control buttons.
	init_eeprom_offset();					//Initialise UTC offset value stored in eeprom (actually just check it is valid, otherwise set to 0).

	sei();									//Global enable interrupts (from avr/interrupt.h)

	//The Following commands can be un-commented to manually set the RTC time on start-up.
	//RTC_SPI_writeByte(RTC_SECR_WA, INT_TO_BCD(53));		//Set Seconds
	//RTC_SPI_writeByte(RTC_MINR_WA, INT_TO_BCD(59));		//Set minutes
	//RTC_SPI_writeByte(RTC_HRR_WA, INT_TO_BCD(23));		//Set hours
	//RTC_SPI_writeByte(RTC_DAYR_WA, INT_TO_BCD(5));		//Set Day
	//RTC_SPI_writeByte(RTC_DATER_WA, INT_TO_BCD(18));		//Set Date
	//RTC_SPI_writeByte(RTC_MCR_WA, 0b10010010);			//Set Month/Century
	//RTC_SPI_writeByte(RTC_YRR_WA, INT_TO_BCD(14));		//Set Year
	
	//Initialise array "time" which shall include all the time and date data pulled from the RTC or GPS.  Bytes will be binary-coded deciaml (BCD):
	uint8_t time[SIZE_OF_TIME_ARRAY];	//time[0] = time[SEC]: Seconds,	0  to 59
										//time[1] = time[MIN]: Minutes,	0  to 59
										//time[2] = time[HOU]: Hours,	0  to 23
										//time[4] = time[DAT]: Date,	1  to 31
										//time[5] = time[MON]: Month,	1  to 12
										//time[6] = time[YEA]: Year,	0  to 99
										//time[7] = time[CEN]: Century,	19 to 20
	
	uint8_t mode = MODE_NO_DP;											//Initialise "mode" setting.  Modes to be cycled via button press.
	int8_t offset = (int8_t) eeprom_read_byte(OFFSET_EEPROM_ADDRESS);	//Initialise UTC offset value - recall from eeprom.

	while (1)
	{	
		RTC_getTime(time);	//Refresh time data (pull from RTC).  Pass to the function the address of the first element of the time array.

		//Following block prints the time and date data pulled from the time array in ISO 8601 format via the USART.
		/*	printString("\r");
			printByte(BCD_TO_INT(time[CEN]));
			printByte(BCD_TO_INT(time[YEA]));
			printString("-");
			printByte(BCD_TO_INT(time[MON]));
			printString("-");
			printByte(BCD_TO_INT(time[DAT]));
			printString(" ");
			printByte(BCD_TO_INT(time[HOU]));
			printString(":");
			printByte(BCD_TO_INT(time[MIN]));
			printString(":");
			printByte(BCD_TO_INT(time[SEC]));
			printString(" ");					*/

		refresh_display(time, mode);					//Refresh the seven segment displays with the time in accordance with the current display mode.
		
		//If the mode button is pressed
		if(!(BUTTON_PINS & (1 << MODE_BUTTON)))
		{
			_delay_us(DEBOUNCE_DURATION);				//Brief wait to de-bounce.
			if(!(BUTTON_PINS & (1 << MODE_BUTTON)))	//Re-check mode button is still pressed.
			{
				mode++;									//Cycle to next mode.
				if(mode==MODE_ADJUST_OFFSET)			//Special case, if offset adjustment mode is entered
				{
					get_utc_offset();												//Run the function that allows operator to adjust the UTC offset.  Adjusted offset will be stored in eeprom.
					offset = (int8_t) eeprom_read_byte(OFFSET_EEPROM_ADDRESS);		//Reset main-loop offset in-case eeprom has been updated.
					printString("\r\nEEPROM Offset: ");								//For debugging
					printByte((int8_t) eeprom_read_byte(OFFSET_EEPROM_ADDRESS));	//For debugging
					mode = 0;														//Once offset adjustment mode is exited, set mode to 0.  
					//(Mode rollover "if" statement not needed is offset adjust mode is last in cycle).
				}
				
				while(!(BUTTON_PINS & (1 << MODE_BUTTON)))	//While loop to deal with holding the mode button in.
				{
					RTC_getTime(time);						//While button is pressed keep refreshing display so mode isn't continuously incremented.
					refresh_display(time, mode);			//Refresh the seven segment displays with the time in accordance with the current display mode.
				}
			}
		}

		//If the sync button is pressed
		if(!(BUTTON_PINS & (1 << SYNC_BUTTON)))
		{
			_delay_us(DEBOUNCE_DURATION);				//Brief wait to de-bounce.
			if(!(BUTTON_PINS & (1 << SYNC_BUTTON)))	//Re-check sync button is still pressed.
			{
				GPS_getTime(time);						//Pull the time/date data from the GPS unit.  Pass to the function the address of the first element of the time array.	
				utc_offset_adjust(time, offset);		//Adjust the UTC pulled from the GPS in accordance with the stored UTC offset.

				//Following block takes the time/data data synced from GPS and adjusted for the UTC then uses it to update the RTC.
				RTC_SPI_writeByte(RTC_YRR_WA, INT_TO_BCD(time[YEA]));					//Update year
				RTC_SPI_writeByte(RTC_MCR_WA, (0b10000000 | (INT_TO_BCD(time[MON]))));	//"OR" with 0x80 to set the century bit high (i.e. 20xx not 19xx)
				RTC_SPI_writeByte(RTC_DATER_WA, INT_TO_BCD(time[DAT]));				//Update
				RTC_SPI_writeByte(RTC_HRR_WA, INT_TO_BCD(time[HOU]));					//Update hour
				RTC_SPI_writeByte(RTC_MINR_WA, INT_TO_BCD(time[MIN]));					//Update minute
				RTC_SPI_writeByte(RTC_SECR_WA, INT_TO_BCD(time[SEC]));					//Update second
			}
		}
	}

	return(0);
}

;