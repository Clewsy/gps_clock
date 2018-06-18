//GPS clock support functions.

#include <GPSClock_Support.h>

//Interrupt subroutine triggered whenever the AVR receives a byte on the serial line (USART).  This was used for debugging.
ISR(USART_RX_vect)	//Interrupt subroutine triggered when the USART receives a byte.
{					
	//transmitByte(receiveByte());	//Echos received byte.
}

//The following function initialises the system clock
void initClock(void)
{
	//CLKPR: Clock Prescale Register.  Pre-scale the system clock.  The following setup will disable prescaling so the system clock will be the full 8MHz internal frequency (default is 1MHz).
	CLKPR = (1 << CLKPCE);		//CLKPCE: Clock Prescaler Change Enable.  Must set to one to enable changes to clock prescaler bits.
	CLKPR = 0;					//Once CLKPCE is enabled, Clock Prescaler Select Bits can be changed if done within 4 cycles.  In this case, clear all for prescaler value 1 (i.e. 8MHz system).
}

//The following function initialises the two input control buttons.
void init_buttons(void)
{
	DDR_BUTTONS &= ~(1 << MODE_BUTTON);	//Clear DDR bit for MODE_BUTTON to make it an input.  By default all pins are inputs at start-up so this isn't exactly neccessary.
	PORT_BUTTONS |= (1 << MODE_BUTTON);	//'Set' bit for MODE_BUTTON to activate the internal pull-up resistor.

	DDR_BUTTONS &= ~(1 << SYNC_BUTTON);	//Clear DDR bit for MODE_BUTTON to make it an input.  By default all pins are inputs at start-up so this isn't exactly neccessary.
	PORT_BUTTONS |= (1 << SYNC_BUTTON);	//'Set' bit for MODE_BUTTON to activate the internal pull-up resistor.	
}

//This function checks the UTC offset value stored in eeprom is valid.  If not, sets it to 0.
void init_eeprom_offset(void)
{
	//The offset is stored in eeprom so it is retained when power is lost (also when AVR is reprogrammed provided the save_eeprom fuse is set).
	//This if statement will ensure it is a valid value (multiple of 5 from -115 to 120).  If not, set it to 0.  On a new chip eeprom memory would be defaulted to 0xFF).
	//Have to typecast byte as a signed int as only unsigned integers are stored in eeprom.
	if(((int8_t)eeprom_read_byte(OFFSET_EEPROM_ADDRESS) % 5) || ((int8_t)eeprom_read_byte(OFFSET_EEPROM_ADDRESS) > 120) || ((int8_t)eeprom_read_byte(OFFSET_EEPROM_ADDRESS) < -115))
	{
		eeprom_update_byte((uint8_t *) OFFSET_EEPROM_ADDRESS, 0);	//Write the value 0 to the offset in eeprom.
	}
}

//The following function is called frequently to refresh the display while the clock is "syncing" the time from the GPS unit.
void sync_display (uint8_t sec)
{
		//The following block will display "SYnCInG-" on the display.
		SEV_SEG_writeByte(SEV_SEG_DECODE_MODE_A, 0x00);				//Set digits 0-7 to manual encode (not code-b)
		SEV_SEG_writeByte(SEV_SEG_DECODE_MODE_B, 0x00);				//Set digits 8-15 to manual encode (not code-b)
		SEV_SEG_writeByte(SEV_SEG_DIGIT_15, SEV_SEG_MANUAL_S);
		SEV_SEG_writeByte(SEV_SEG_DIGIT_14, SEV_SEG_MANUAL_Y);
		SEV_SEG_writeByte(SEV_SEG_DIGIT_13, SEV_SEG_MANUAL_N);
		SEV_SEG_writeByte(SEV_SEG_DIGIT_12, SEV_SEG_MANUAL_C);
		SEV_SEG_writeByte(SEV_SEG_DIGIT_11, SEV_SEG_MANUAL_I);
		SEV_SEG_writeByte(SEV_SEG_DIGIT_10, SEV_SEG_MANUAL_N);
		SEV_SEG_writeByte(SEV_SEG_DIGIT_9, SEV_SEG_MANUAL_G);
		SEV_SEG_writeByte(SEV_SEG_DIGIT_8, SEV_SEG_MANUAL_DASH);
		SEV_SEG_writeByte(SEV_SEG_DIGIT_7, SEV_SEG_MANUAL_BLANK);
		SEV_SEG_writeByte(SEV_SEG_DIGIT_6, SEV_SEG_MANUAL_BLANK);
		SEV_SEG_writeByte(SEV_SEG_DIGIT_5, SEV_SEG_MANUAL_BLANK);
		SEV_SEG_writeByte(SEV_SEG_DIGIT_4, SEV_SEG_MANUAL_BLANK);
		SEV_SEG_writeByte(SEV_SEG_DIGIT_3, SEV_SEG_MANUAL_BLANK);
		SEV_SEG_writeByte(SEV_SEG_DIGIT_2, SEV_SEG_MANUAL_BLANK);
		SEV_SEG_writeByte(SEV_SEG_DIGIT_1, SEV_SEG_MANUAL_BLANK);
		SEV_SEG_writeByte(SEV_SEG_DIGIT_0, SEV_SEG_MANUAL_BLANK);
		
		//Following if statement is used to scroll the DP across the right-most 8 digits to indicate the clock is attempting to sync time from a satelite.
		if((sec != 0) && (sec != 9))
		{
			SEV_SEG_writeByte(sec, (SEV_SEG_MANUAL_BLANK | SEV_SEG_DP));	//Writes to the displays to turn on the DP on the digit corrsponding to 'sec' (i.e. digit 0 to 7).
		}
}

//The following function is used to obtain the time and date from the GPS unit over the UART.
void GPS_getTime (uint8_t *time)
{		
	//While loop to deal with holding the button in.
	while(!(BUTTON_PINS & (1 << SYNC_BUTTON)))
	{
		sync_display(0);	//Just display "Syncing-"
	}
	
	printString("Syncing\r\n");	//For debugging; indicates entering sync loop
	
	uint8_t syncing = 1;	//Setting 'syncing' to 0 will allow the while loop to be exited.
	while(syncing)
	{	
		//RMC sentence format (sections of data separated by commas):
		//Example: $GPRMC,161229.487,A,3723.2475,N,12158.3416,W,0.13,309.62,120598, ,*10
		//$GPRMC,utc time,data valid/invalid,latitude,N/S,longitude,E/W,ground speed(knots),course(deg),date,magnetic variation(deg),checksum
		//Nested if statements to check characters 'R', 'M' & 'C' are received in order - this confirms beginning of RMC sentence.
		if(receiveByte() == 'R')
		{
			if(receiveByte() == 'M')
			{
				if(receiveByte() == 'C')
				{
					receiveByte();									//Receive and ignore ','
					time[HOU] = (10 * (receiveByte() - '0'));		//Hour tens		minusing the char '0' converts the int from a ascii char to the corresponding digit.
					time[HOU] += (receiveByte() - '0');			//Hour ones
					time[MIN] = (10 * (receiveByte() - '0'));		//Minute tens
					time[MIN] += (receiveByte() - '0');			//Minute ones
					time[SEC] = (10 * (receiveByte() - '0'));		//Second tens
					time[SEC] += (receiveByte() - '0');			//Second ones

					for(uint8_t i = 0; i < 8; i++)					//Loop 8 times
					{
						do {}
						while (receiveByte() != ',');				//Skip through 8 lots of data (ignored) from the RMC sentence to get to the date data.
					}

					time[DAT] = (10 * (receiveByte() - '0'));		//Date tens
					time[DAT] += (receiveByte() - '0');			//Date ones
					time[MON] = (10 * (receiveByte() - '0'));		//Month tens
					time[MON] += (receiveByte() - '0');			//Month ones
					time[YEA] = (10 * (receiveByte() - '0'));		//Year tens
					time[YEA] += (receiveByte() - '0');			//Year ones			

					//Following block for debugging; prints out the time/date data parsed from the GPS RMC sentence.
					printString(" ddmmyy:");
					printByte(time[DAT]);
					printByte(time[MON]);
					printByte(time[YEA]);
					printString(" hhmmss:");
					printByte(time[HOU]);
					printByte(time[MIN]);
					printByte(time[SEC]);
					printString("\r\n");
	
					sync_display(time[SEC] % 10);	//Display "Syncing-" with a scrolling DP effect across the right 8 digits.
			
					//If the "year" is greater than or equal to 14 (i.e. 2014+) then the time has been updated from a satellite. (Default year at gps unit startup is '03)
					if(time[YEA] >= 14)	
					{
						printString("\r\nSync Successful\r\n");			//For debugging; indicates successfully exited sync loop
						syncing = 0;									//Syncing complete, exit while loop.
					}
					
					//If the mode button is pressed, exit the sync loop.  I.e. allow exiting the sync loop manually - time will be set incorrectly to un-synced time from GPS unit.
					if(!(BUTTON_PINS & (1 << MODE_BUTTON)))			//If the sync button is pressed
					{
						_delay_us(DEBOUNCE_DURATION);					//Brief wait to de-bounce.
						if(!(BUTTON_PINS & (1 << MODE_BUTTON)))		//Re-check mode button is still pressed.
						{
							printString("\r\nSync Unsuccessful\r\n");	//For debugging; indicates exited sync loop without sync
							syncing = 0;								//Syncing not complete, manual exit.
						}
					}
				}
			}
		}
	}
}

//The following function is used to obtained and programme the UTC offset.
//The offset is a signed value in multiples of 5 from -115 to 120 which represents 10x the hours to be offset from the UTC (e.g. "offset = 95" means offset is + 09:30)
void get_utc_offset(void)
{
	int8_t offset = (int8_t) (eeprom_read_byte(OFFSET_EEPROM_ADDRESS));

	//While loop to deal with holding the button in.
	while(!(BUTTON_PINS & (1 << MODE_BUTTON)))
	{
		refresh_display(0, MODE_ADJUST_OFFSET);	//Display "OFF AdJ-"
	}

	//While loop for adjusting the offset.  Loop is exited when the mode button is pressed again.
	while((!(BUTTON_PINS & (1 << MODE_BUTTON)))==0)
	{
		if(offset < 0)
		{
			SEV_SEG_writeByte(SEV_SEG_DIGIT_4, SEV_SEG_CODEB_DASH);			//Display a dash (negative value)
			SEV_SEG_writeByte(SEV_SEG_DIGIT_3, (-offset / 100));			//Hour tens
			SEV_SEG_writeByte(SEV_SEG_DIGIT_2, ((-offset / 10) % 10));		//Hour ones
			if(-offset % 10)												//Check if remainder is 0 (even hour) or 5 (half an hour)
			{
				SEV_SEG_writeByte(SEV_SEG_DIGIT_1, 3);						//Minute tens - always either zero or half hour (in this case display 3).
			}
			else
			{
				SEV_SEG_writeByte(SEV_SEG_DIGIT_1, 0);						//Minute tens - always either zero or half hour (in this case display 0).
			}	
			SEV_SEG_writeByte(SEV_SEG_DIGIT_0, 0);							//Minute ones - always zero.
		}
		else
		{
			SEV_SEG_writeByte(SEV_SEG_DIGIT_4, SEV_SEG_CODEB_BLANK);		//Display no dash (positive value)
			SEV_SEG_writeByte(SEV_SEG_DIGIT_3, (offset / 100));			//Hour tens
			SEV_SEG_writeByte(SEV_SEG_DIGIT_2, ((offset / 10) % 10));		//Hour ones
			if(offset % 10)													//Check if remainder is 0 (even hour) or 5 (half an hour)
			{
				SEV_SEG_writeByte(SEV_SEG_DIGIT_1, 3);						//Minute tens - always either zero or half hour (in this case display 3).
			}
			else
			{
				SEV_SEG_writeByte(SEV_SEG_DIGIT_1, 0);						//Minute tens - always either zero or half hour (in this case display 0).
			}
			SEV_SEG_writeByte(SEV_SEG_DIGIT_0, 0);							//Minute ones - always zero.
		}


		if(!(BUTTON_PINS & (1 << SYNC_BUTTON)))		//If the sync button is pressed
		{
			_delay_us(DEBOUNCE_DURATION);				//Brief wait to de-bounce.
			if(!(BUTTON_PINS & (1 << SYNC_BUTTON)))	//Re-check mode button is still pressed.
			{
				offset += 5;
				while(!(BUTTON_PINS & (1 << SYNC_BUTTON))){}	//Wait until button is released.
				_delay_us(DEBOUNCE_DURATION);					//Brief wait to de-bounce.
				if(offset >= 125)								//If the offset exceeds the maximum allowable value (120)
				{
					offset = -115;								//Reset to the minimum allowable value.
				}
			}
		}

	}
	eeprom_update_byte((uint8_t *) OFFSET_EEPROM_ADDRESS, offset);	//Will re-write the offset to eeprom if it has changed.
}

//The following function adjusts the time/date in accordance with the programmed offset.
//To avoid using floats, value of offset is always 10x the actual desired offset.  E.g. for positive 10.5hr adjustment, offset=105
//Offset value can only be a multiple of 5.  A 5 at the end therefore indicates offset of Xhrs, 30min.
void utc_offset_adjust (uint8_t *time, int8_t offset)
{
	if (offset > 0)						//Positive Offset
	{
		if(offset % 10)					//Check if there is a .5hr adjustment - returns true if there is a remainder after dividing offset by 10.
		{
			time[MIN] += 30;			//Increment minutes by 30.
			if(time[MIN] > 59)			//Check to see if hour needs to be rolled over.
			{
				time[MIN] -= 60;		//Reduce minutes to a value between 0 and 59.
				rollover_hour(time);	//Increment hour then check for date, month and year rollover.
			}
		}
	
		time[HOU] += (offset/10);		//Increment hours by desired amount.

		if (time[HOU] > 23)				//If the increment has pushed the time into the next day
		{
			time[HOU] -= 24;			//Reduce hour to a value between 0 and 23
			rollover_date(time);		//Increment date then check for month and year rollover.
		}
	}
	
	else if (offset < 0)
	{
		if((-offset) % 10)				//Check if there is a .5hr adjustment - returns true if there is a remainder after dividing -offset by 10.
		{
			time[MIN] -= 30;			//Decrement minutes by 30.
			if(time[MIN] > 200)			//Check to see if hour needs to be rolled under (If time[MIN] (uint) is less than 30, will back from 0 to 255)
			{
				time[MIN] += 60;		//Increase minutes to a value between 0 and 59.
				rollunder_hour(time);	//Decrement hour then check for date, month and year rollunder.
			}
		}
	
		time[HOU] -= ((-offset)/10);	//Decrement hours by desired amount.

		if (time[HOU] > 200)			//If the decrement has pulled the time into the previous day
		{
			time[HOU] += 24;			//Increase hour to a value between 0 and 23
			rollunder_date(time);		//Decrement date then check for month and year rollunder.
		}
	}
}

//The following function is called to increment the date if required by the offset from the UTC.  Also checks to see if the increment requires rollover of date, month or year.
void rollover_hour(uint8_t *time)
{
	time[HOU]++;				//Increment hour.
	if(time[HOU] == 24)		//Check for day rollover.
	{
		time[HOU] = 0;			//Reset hour to 0000.
		rollover_date(time);	//Increment date.
	}
}

//The following function is called to increment the date if required by the offset from the UTC.  Also checks to see if the increment requires rollover of month or year.
void rollover_date(uint8_t *time)
{
	time[DAT]++;	//Increment date.
	
	//Switch statement to check the current month then check to see if it has rolled over
	switch(time[MON])
	{
		//If the month has 31 days
		case JAN:
		case MAR:
		case MAY:
		case JUL:
		case AUG:
		case OCT:
			if(time[DAT] == 32)	//If the month has rolled over
			{
				time[DAT] = 1;		//Reset date to the 1st
				time[MON]++;		//Increment the month
			}
		break;

		//If the month has 30 days
		case APR:
		case JUN:
		case SEP:
		case NOV:
			if(time[DAT] == 31)	//If the month has rolled over
			{
				time[DAT] = 1;		//Reset date to the 1st
				time[MON]++;		//Increment the month
			}
		break;
			
		//If the month has 28 days
		case FEB:
			if(time[DAT] == 29)	//If the month has rolled over
			{
				time[DAT] = 1;		//Reset date to the 1st
				time[MON]++;		//Increment the month
			}
		break;
		
		//If the month is December
		case DEC:
			if(time[DAT] == 32)	//If the month has rolled over
			{
				time[DAT] = 1;		//Reset date to the 1st
				time[MON] = JAN;	//Reset month to JAN
				time[YEA]++;		//Increment the year
			}
		break;
	}
}

//The following function is called to decrement the hour if required by the offset from the UTC.  Also checks to see if the decrement requires rollback of date, month or year.
void rollunder_hour(uint8_t *time)
{
	time[HOU]--;				//Decrement hour.
	if(time[HOU] == 255)		//Check for day rollunder.
	{
		time[HOU] = 23;			//Reset hour to 2300.
		rollunder_date(time);	//Decrement date.
	}
}

//The following function is called to decrement the date if required by the offset from the UTC.  Also checks to see if the decrement requires rollback of month or year.
void rollunder_date(uint8_t *time)
{
	time[DAT]--;			//Decrement date.
	
	if(time[DAT] == 0)		//Check if the month has rolled under
	{
	
		//Switch statement to check the current month so it is known what month to roll back into
		switch(time[MON])
		{
			//If the previous month has 31 days
			case FEB:
			case APR:
			case JUN:
			case AUG:
			case SEP:
			case NOV:
				time[DAT] = 31;		//Reset date to the 31st
				time[MON]--;		//Decrement the month
			break;

			//If the previous month has 30 days
			case MAY:
			case JUL:
			case OCT:
			case DEC:
				time[DAT] = 30;		//Reset date to the 30th
				time[MON]--;		//Decrement the month
			break;
			
			//If the previous month has 28 days
			case MAR:
				time[DAT] = 28;		//Reset date to the 28th
				time[MON]--;		//Decrement the month
			break;
		
			//If the month is January
			case JAN:
				time[DAT] = 31;		//Reset date to the 31st
				time[MON] = DEC;	//Reset month to DEC
				time[YEA]--;		//Decrement the year
			break;
		}
	}
}

//The following function is called frequently to refresh the display in accordance with the current mode.
void refresh_display(uint8_t *time, uint8_t mode)
{
	switch(mode)
	{
		case MODE_NO_DP:  //Time is displayed centered with no break between date and time and with no DPs between parts.
			SEV_SEG_writeByte(SEV_SEG_DECODE_MODE_A, 0xFF);				//Set digits  0-7 to code-b encode
			SEV_SEG_writeByte(SEV_SEG_DECODE_MODE_B, 0xFF);				//Set digits 8-15 to code-b encode
			SEV_SEG_writeByte(SEV_SEG_DIGIT_15, SEV_SEG_CODEB_BLANK);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_14, ((time[CEN]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_13, ((time[CEN]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_12, ((time[YEA]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_11, ((time[YEA]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_10, ((time[MON]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_9, ((time[MON]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_8, ((time[DAT]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_7, ((time[DAT]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_6, ((time[HOU]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_5, ((time[HOU]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_4, ((time[MIN]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_3, ((time[MIN]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_2, ((time[SEC]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_1, ((time[SEC]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_0, SEV_SEG_CODEB_BLANK);
		break;
			
		case MODE_DP: //Time is displayed centered with no break between date and time and with DPs between parts.
			SEV_SEG_writeByte(SEV_SEG_DECODE_MODE_A, 0xFF);				//Set digits  0-7 to code-b encode
			SEV_SEG_writeByte(SEV_SEG_DECODE_MODE_B, 0xFF);				//Set digits 8-15 to code-b encode
			SEV_SEG_writeByte(SEV_SEG_DIGIT_15, SEV_SEG_CODEB_BLANK);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_14, ((time[CEN]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_13, ((time[CEN]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_12, ((time[YEA]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_11, ((time[YEA]) & 0b00001111) | SEV_SEG_DP);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_10, ((time[MON]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_9, ((time[MON]) & 0b00001111) | SEV_SEG_DP);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_8, ((time[DAT]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_7, ((time[DAT]) & 0b00001111) | SEV_SEG_DP);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_6, ((time[HOU]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_5, ((time[HOU]) & 0b00001111) | SEV_SEG_DP);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_4, ((time[MIN]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_3, ((time[MIN]) & 0b00001111) | SEV_SEG_DP);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_2, ((time[SEC]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_1, ((time[SEC]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_0, SEV_SEG_CODEB_BLANK);
		break;

		case MODE_CENTER_GAP_NO_DP: //Time is displayed with a break between date and time and with no DPs between parts.
			SEV_SEG_writeByte(SEV_SEG_DECODE_MODE_A, 0xFF);				//Set digits  0-7 to code-b encode
			SEV_SEG_writeByte(SEV_SEG_DECODE_MODE_B, 0xFF);				//Set digits 8-15 to code-b encode
			SEV_SEG_writeByte(SEV_SEG_DIGIT_15, ((time[CEN]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_14, ((time[CEN]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_13, ((time[YEA]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_12, ((time[YEA]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_11, ((time[MON]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_10, ((time[MON]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_9, ((time[DAT]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_8, ((time[DAT]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_7, SEV_SEG_CODEB_BLANK);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_6, SEV_SEG_CODEB_BLANK);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_5, ((time[HOU]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_4, ((time[HOU]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_3, ((time[MIN]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_2, ((time[MIN]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_1, ((time[SEC]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_0, ((time[SEC]) & 0b00001111));
		break;
		
		case MODE_CENTER_GAP_DP: //Time is displayed with a break between date and time and with DPs between parts.
			SEV_SEG_writeByte(SEV_SEG_DECODE_MODE_A, 0xFF);				//Set digits  0-7 to code-b encode
			SEV_SEG_writeByte(SEV_SEG_DECODE_MODE_B, 0xFF);				//Set digits 8-15 to code-b encode
			SEV_SEG_writeByte(SEV_SEG_DIGIT_15, ((time[CEN]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_14, ((time[CEN]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_13, ((time[YEA]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_12, ((time[YEA]) & 0b00001111) | SEV_SEG_DP);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_11, ((time[MON]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_10, ((time[MON]) & 0b00001111) | SEV_SEG_DP);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_9, ((time[DAT]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_8, ((time[DAT]) & 0b00001111) | SEV_SEG_DP);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_7, SEV_SEG_CODEB_BLANK);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_6, SEV_SEG_CODEB_BLANK);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_5, ((time[HOU]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_4, ((time[HOU]) & 0b00001111) | SEV_SEG_DP);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_3, ((time[MIN]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_2, ((time[MIN]) & 0b00001111) | SEV_SEG_DP);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_1, ((time[SEC]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_0, ((time[SEC]) & 0b00001111));
		break;
			
		case MODE_ADJUST_OFFSET: //Following block to make the display read: "OFF AdJ -    "
			SEV_SEG_writeByte(SEV_SEG_DECODE_MODE_A, 0xFF);				//Set digits  0-7 to code-b encode
			SEV_SEG_writeByte(SEV_SEG_DECODE_MODE_B, 0x00);				//Set digits 8-15 to manual encode (not code-b)
			SEV_SEG_writeByte(SEV_SEG_DIGIT_15, SEV_SEG_MANUAL_O);		//Manual
			SEV_SEG_writeByte(SEV_SEG_DIGIT_14, SEV_SEG_MANUAL_F);		//Manual
			SEV_SEG_writeByte(SEV_SEG_DIGIT_13, SEV_SEG_MANUAL_F);		//Manual
			SEV_SEG_writeByte(SEV_SEG_DIGIT_12, SEV_SEG_MANUAL_BLANK);	//Manual
			SEV_SEG_writeByte(SEV_SEG_DIGIT_11, SEV_SEG_MANUAL_A);		//Manual
			SEV_SEG_writeByte(SEV_SEG_DIGIT_10, SEV_SEG_MANUAL_D);		//Manual
			SEV_SEG_writeByte(SEV_SEG_DIGIT_9, SEV_SEG_MANUAL_J);		//Manual
			SEV_SEG_writeByte(SEV_SEG_DIGIT_8, SEV_SEG_MANUAL_BLANK);	//Manual
			SEV_SEG_writeByte(SEV_SEG_DIGIT_7, SEV_SEG_CODEB_BLANK);	//Code-B
			SEV_SEG_writeByte(SEV_SEG_DIGIT_6, SEV_SEG_CODEB_BLANK);	//Code-B
			SEV_SEG_writeByte(SEV_SEG_DIGIT_5, SEV_SEG_CODEB_BLANK);	//Code-B
			SEV_SEG_writeByte(SEV_SEG_DIGIT_4, SEV_SEG_CODEB_BLANK);	//Code-B
			SEV_SEG_writeByte(SEV_SEG_DIGIT_3, SEV_SEG_CODEB_BLANK);	//Code-B
			SEV_SEG_writeByte(SEV_SEG_DIGIT_2, SEV_SEG_CODEB_BLANK);	//Code-B
			SEV_SEG_writeByte(SEV_SEG_DIGIT_1, SEV_SEG_CODEB_BLANK);	//Code-B
			SEV_SEG_writeByte(SEV_SEG_DIGIT_0, SEV_SEG_CODEB_BLANK);	//Code-B
		break;	
	}
}
