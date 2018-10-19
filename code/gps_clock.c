#include "gps_clock.h"

//The following interrupt sub-routine will be triggered every time there is a change in state of either button.
ISR(BUTTON_PCI_VECTOR)
{
	cli();	//Disable interrupts until this ISR is complete.

	_delay_ms(BUTTON_DEBOUNCE_DURATION);	//wait for DEBOUNCE_DURATION milliseconds to mitigate effect of switch bounce.
	//If statement captures press of the "Mode" button.  Cycles through the various display modes.
	if(!(BUTTON_PINS & (1 << BUTTON_MODE)))
	{
		sev_seg_power(OFF);			//Blank the display while settings are changed.
		sev_seg_decode_mode(DECODE_CODE_B);	//Whenever the mode is changed, return all digits to default Code B decode mode.
		sev_seg_all_clear();			//Clear all digits to avoid artifacts between display modes.
		sev_seg_power(ON);			//Reactivate display now that settings are changed.

		mode++;					//Increment the mode.
		if(mode > MODE_5_INTENSITY)		//If largest mode value reached.
		{
			mode = MODE_1A_ISO;		//Cycle back to first mode.
		}

		while(BUTTON_PINS & (1 << BUTTON_MODE))	//Keep refreshing the display until the button is released.
		{
			rtc_get_time(time);		//Update the current time from the rtc.
			poll();				//Display the current time/mode.
		}
	}

	//If statement captures press of the "Sync" button.  Initiates at attempt at syncing RTC clock to GPS data.
	if(!(BUTTON_PINS & (1 << BUTTON_SYNC)))
	{
		if (mode == MODE_4_OFFSET)		//If mode 4 is running, the Sync button cycles the UTC offset.
		{
			cycle_offset();			//Cycle the UTC offset value (-11.5hrs to +12.0hrs).
		}
		else if (mode == MODE_5_INTENSITY)	//If mode 5 is running, the sync button cycles the intensity value (brightness).
		{
			cycle_intensity();		//Cycle the intensity value that corresponds to display brightness.
		}
		else					//All other modes, the Sync button attempts to sync the clock.
		{
			attempt_sync();			//Attempt to sync the RTC time with GPS data.
		}
	}

	sei();	//Re-enable interrupts.
}

//Initialise the peripherals.
void hardware_init(void)
{
	clock_prescale_set(clock_div_1);	//needs #include <avr/power.h>, prescaler 1 gives clock speed 8MHz.
	power_adc_disable();			//Since it's not needed, disable the ADC and save a bit of power.

	usart_init();				//Initialise the USART to enable serial communications.
	spi_init(SEV_SEG_POL, SEV_SEG_PHA);	//Initialise the SPI to enable comms with SPI devices.
						//Note, both spi devices (RTC and max7219) operate in spi mode 3, so spi_init only needs to be called once.

	sev_seg_init();				//Initialise the two cascaded max7219 chips that will drive the 16 7-segment digits (spi comms).
	rtc_init();				//Initialise the hardware for spi comms with the DS3234 RTC.

	//Disabled the following setting of PCICR until macro "BUTTONS_ENABLED" is called thus disabling buttons during start-up until after first sync attempt.
	//PCICR |= (1 << BUTTON_PCIE);		//Enable Pin-Change Interrupt for pin-change int pins PCINT[8-14].  This includes both buttons.
						//PCICR: Pin-Change Interrupt Control Register
	BUTTON_PCMSK |= ((1 << BUTTON_MODE) | (1 << BUTTON_SYNC));
						//Enable Pin-Change Interrupt on PCINT[x] for PCINT pins to which the 5 buttons are connected.
						//PCMSK0: Pin-Change Mask Register 0 (For PCINT[7-0] i.e. PB[7-0])
	BUTTON_PORT |= ((1 << BUTTON_MODE) | (1 << BUTTON_SYNC));
						//Enable pull-up resistors for the buttons.

	sei();					//Global enable interrups (requires avr/interrupt.h).
}

//Initialise (validate and set) settings that are stored in eeprom.
void settings_init(void)
{
	validate_eeprom_offset();					//Confirm the UTC time offset value stored in eeprom is valid.
	offset = (int8_t) eeprom_read_byte(OFFSET_EEPROM_ADDRESS);	//set the "offset" variable to what is stored in eeprom.

	validate_eeprom_intensity();					//Confirm the intensity (brightness) value stored in eeprom is valid.
	intensity = eeprom_read_byte(INTENSITY_EEPROM_ADDRESS);		//Initialise the global "intensity" variable to value in eeprom.
	sev_seg_set_intensity(intensity);				//Set the display intensity accordingly.
}

//This function validates the GMT time offset value saved in eeprom.
//The offset is stored in eeprom so it is retained when power is lost (also when AVR is reprogrammed provided the save_eeprom fuse is set).
//This if statement will ensure it is a valid value (multiple of 5 from -120 to 120).
//If the value is invalid, set it to 0.  On a new chip eeprom memory would be defaulted to 0xFF.
//Have to typecast byte as a signed int as only unsigned integers are stored in eeprom.
void validate_eeprom_offset(void)
{
	if
	(
		((int8_t)eeprom_read_byte(OFFSET_EEPROM_ADDRESS) % 5) ||		//Returns true if there is a remainder after dividing by 5, OR
		((int8_t)eeprom_read_byte(OFFSET_EEPROM_ADDRESS) > 120) ||		//Returns true if the value is larger than 120, OR
		((int8_t)eeprom_read_byte(OFFSET_EEPROM_ADDRESS) < -120)		//Returns true if the value is less than -120.
	)
	{
		eeprom_update_byte((uint8_t *) OFFSET_EEPROM_ADDRESS, 0);	//set the offset value stored in eeprom to 0.
	}
}

//This function validates the intensity (brightness) value saved in eeprom.
//The intensity is stored in eeprom so it is retained when power is lost (also when AVR is reprogrammed provided the save_eeprom fuse is set).
//This if statement will ensure it is a valid value (integer range 0 to 15 inclusive).
//If the value is invalid, set it to 8.  On a new chip eeprom memory would be defaulted to 0xFF.
void validate_eeprom_intensity(void)
{
	if (eeprom_read_byte(INTENSITY_EEPROM_ADDRESS) > 15)		//Returns true if the value is greaterss than 15.
	{
		eeprom_update_byte((uint8_t *) INTENSITY_EEPROM_ADDRESS, 8);	//set the offset value stored in eeprom to 8.
	}
}

//This function will call other functions depending on the currently selected display mode.
void poll(void)
{
	switch (mode)	//Thhis switch is used to setup the display depending on which mode is selected.
	{
		//Modes 1A, 1B, 2A and 2B are all variations on the ISO-8601 date/time display.
		case (MODE_1A_ISO) :		//	|Y Y Y Y M M D D     H H M M S S |
		case (MODE_1B_ISO) :		//	|Y Y Y Y.M M.D D.    H H.M M.S S.|
		case (MODE_2A_ISO) :		//	|  Y Y Y Y M M D D H H M M S S   |
		case (MODE_2B_ISO) :		//	|  Y Y Y Y.M M.D D.H H.M M.S S.  |
			display_iso_time();
		break;

		//Mode 3 displays Epoch time (unix time) which is the number of seconds since midnight, January first, 1970 (i.e. 19700101000000).
		case (MODE_3_EPOCH) :		//	|E P O C H - S S S S S S S S S S |
			display_epoch_time();
		break;

		//Mode 4 allows setting the UTC time offset (-11.5hrs to +12.0hrs).
		case (MODE_4_OFFSET) :		//	|O F F S E t             ± # #.# |
			get_offset();
		break;

		//Mode 5 allows setting the LED brightness level for all digits (level 0 to 10).
		case (MODE_5_INTENSITY) :	//	|I n t E n S I t y           # # |
			get_intensity();
		break;
	}
}

//Display the time in a standard ISO-8601 format.
void display_iso_time(void)
{
	uint8_t date_offset = 0;	//This offset will determine which digit the date will start at.
	uint8_t time_offset = 0;	//This offset will determine how many digits after the date that the time will start.
	uint8_t delimiters = 0;		//This flag will be used to activate the decimal point if the current mode requires it.
	uint8_t current_mode = mode;	//This variable will be used to check for mode change that will exit the while loop.
	uint8_t i;			//General use integer for running for loops.

	//Initialise array "buffer" which will consist of 16 data void settings_init(void)bytes that will be sent to the seven-segment display drivers.
	//I.e. each element is the data to be used for each of the 16 7-seg digits.
	uint8_t buffer[16];		//buffer[0] = digit 0	<--Least-Significant Digit (Right)
					//buffer[1] = digit 1
					//buffer[2] = digit 2
					//	...	 =  ...
					//buffer[14] = digit 14
					//buffer[15] = digit 15	<--Least-Significant Digit (Right)

	//This switch used to set the positions of the date and time within the 16-digit display.
	switch (mode)
	{
		case (MODE_1A_ISO) :
		case (MODE_1B_ISO) :
			date_offset = 0;	//Start date display at the first digit (digit 0).
			time_offset = 2;	//Leave two digits blank after the date and before the time.
		break;

		case (MODE_2A_ISO) :
		case (MODE_2B_ISO) :
			date_offset = 1;	//Start date display at the second digit (digit 1).
			time_offset = 0;	//Start time display immediately after date display.
		break;
	}

	//This switch used to determine whether or not delimiters are to be used (decimal point between time/date components).
	switch (mode)
	{
		case (MODE_1A_ISO) :
		case (MODE_2A_ISO) :
			delimiters = 0;		//Do NOT show decimal points between years, months, DATs, hours, mionutes, seconds.
		break;

		case (MODE_1B_ISO) :
		case (MODE_2B_ISO) :
			delimiters = SEV_SEG_DP;//DO show decimal points between years, months, DATs, hours, mionutes, seconds.
		break;
	}

	//This loop will initialise all 16 elements in the buffer array to represent a blank digit.
	for (i = 0; i < 16; i++)
	{
		buffer[i] = SEV_SEG_CODEB_BLANK;
	}

	//All static date required to display in accordance with the selected mode is set,
	//so enter a loop that will continuously update the dynamic data and refresh the display.
	while (mode == current_mode)	//This loop will exit when the mode changes.
	{
		rtc_get_time(time);		//Update the current time from the rtc.

		//This loop will apply the decimal point flag to the last digit of each date/time component IF "delimiters" is set my the operating mode.
		for (i = 3; i < 14; i += 2)
		{
			time[i] |= delimiters;	//"delimiters" is either 0 (no effect) or SEV_SEG_DP which will set the decimal point flag for the sev-seg drivers.
		}

		//This loop will set the display buffer digits for the date components (CEN, YEA, MON, DAT).
		for (i = 0; i < 8; i++)
		{
			buffer[i + date_offset] = time[i];			//"date_offset" determined by current mode.
		}

		//This loop will set the display buffer digits for the time components (HOU, MIN, SEC).
		for (i = 8; i < 14; i++)
		{
			buffer[i + date_offset + time_offset] = time[i];	//"time_offset" determined by current mode.
		}

		//Take the contents of the buffer array and send it to the seven segment display drivers
		for (uint8_t i = 0; i < 8; i++)	//Each iteration will send a digit to driver A and a digit to driver B so 8 iterations sends all 16 digits.
		{
			sev_seg_write_byte(SEV_SEG_DIGIT_0 + i, buffer[i]);	//Send the digit for driver A (digits 0 to 7).
			sev_seg_write_byte(SEV_SEG_DIGIT_8 + i, buffer[i+8]);	//Send the digit for driver B (digits 8 to 15).
		}
	}
}

void display_epoch_time(void)
{
	//Initialisation of the epoch variable (number of seconds elapsed since 1970.01.01.00.00.00.
	uint64_t epoch = 0;

	//Enter a loop that will continuously update the dynamic data and refresh the display.
	while (mode == MODE_3_EPOCH)	//This loop will exit when the mode changes.
	{
		sev_seg_write_byte(SEV_SEG_DECODE_MODE_A, 0b11000000);		//Set the first 5 digits (0-4) to manual decode to display text.
		for (uint8_t i = 0; i < 6; i++)					//For loop runs through the first 6 digits (0-5).
		{
			sev_seg_write_byte(SEV_SEG_DIGIT_0 + i, epoch_text[i]);	//Write the pseudo-text "EPOCH-"
		}

		rtc_get_time(time);	//Update the current time from the rtc.

		//Time to calculate the epoch time.  This will work for any date time from Jan 1st 2000 until Dec 31st 2100.
		epoch =  EPOCH_SECONDS_TO_2000;						//Seconds elapsed from 19700101000000 to 20000101000000.
		epoch += (EPOCH_YEAR/4) * DAYS_IN_4_YEARS * SECONDS_IN_A_DAY;		//Seconds elapsed in full 4-year blocks since 2000.
		epoch += days_table[EPOCH_YEAR % 4][EPOCH_MONTH - 1] * SECONDS_IN_A_DAY;//Seconds elapsed in current 4-year block to start of month.
		epoch += (EPOCH_DAY -1) * SECONDS_IN_A_DAY;				//Seconds elapsed since start of month to start of day.
		epoch += EPOCH_HOUR * SECONDS_IN_AN_HOUR;				//Seconds elapsed since start of day to start of hour.
		epoch += EPOCH_MINUTE * SECONDS_IN_A_MINUTE;				//Seconds elapsed since start of hour to to start of minute.
		epoch += EPOCH_SECOND;							//Seconds elapsed since start of minute.

		sev_seg_display_int(epoch);	//We have the epoch value so display it (next to "EPOCH-" text).
	}
}

//Allow the UTC offset to be adjusted by pressing the "Sync" button.  Valid UTC offsets are in half-hour increments from -11.5 to 12.0.
//(Note, to avoid using floats, the actual offset value is an integer from -120 to 120)
void get_offset(void)
{
	sev_seg_all_clear();						//Clear all digits to prevent artifacts from previous mode.

	//Enter a loop that will continuously refresh the display and allow adjustment of the offset.
	while (mode == MODE_4_OFFSET)						//Run the following loop until the mode is changed.
	{
		sev_seg_set_word(offset_text, sizeof(offset_text));		//Display "OFFSEt" on the left-most digits.
		display_offset();						//Display the current offset value using the last 4 digits (12-15).
	}

	sev_seg_decode_mode(DECODE_CODE_B);					//Return all digits to Code B decode mode.

	if (offset != (int8_t) eeprom_read_byte(OFFSET_EEPROM_ADDRESS))		//If the mode has changed and the offset is different to what's in eeprom...
	{
		eeprom_update_byte((uint8_t *) OFFSET_EEPROM_ADDRESS, offset);	//Record the new value to eeprom.
		attempt_sync();							//Attempt a re-sync with the new offset.
	}
}

//Display the current offset value using the last 4 digits (12-15).
void display_offset(void)
{
	if (offset < 0)								//If the current offset is less than zero...
	{
		sev_seg_write_byte(SEV_SEG_DIGIT_12, SEV_SEG_CODEB_DASH);	//Print a minus sign (dash) before the offset value.
	}
	else
	{
		sev_seg_write_byte(SEV_SEG_DIGIT_12, SEV_SEG_CODEB_BLANK);	//Otherwise keep digit 12 blank for positive offsets.
	}
	sev_seg_write_byte(SEV_SEG_DIGIT_13, abs(offset) / 100);			//Display the tens of the offset value.
	sev_seg_write_byte(SEV_SEG_DIGIT_14, ((abs(offset) / 10) % 10) | SEV_SEG_DP);	//Display the ones of the offset value.
	sev_seg_write_byte(SEV_SEG_DIGIT_15, (abs(offset) % 10));			//Display the .0 or .5 of the offset value.
}

//Increment the offset value and rollover when maximum value is exceeded.
//This function is called when the "Sync" button is pressed only when running Mode 4.
void cycle_offset(void)
{
	offset += 5;			//Increment offset by 5.
	if (offset > 120)		//If maximum valid value (120) is exceeded...
	{
		offset = -120;		//Rollover to minimum valid value (-120).
	}
}

//Attempt to sync thr RTC time to GPS data.  Display status on-screen using pseudo-text.
void attempt_sync(void)
{
	BUTTONS_DISABLE;	//Disable pin-change interrupts effectively disabling the buttons so that the sync function is less likely to produce an error.

	//Attempt to sync rtc time with gps time.
	sev_seg_flash_word(syncing, sizeof(syncing), 2000);		//Display "SynCIng" for 2 seconds.
	if (!sync_time(time))						//If the sync is unsuccessful...
	{
		sev_seg_flash_word(no_sync, sizeof(no_sync), 1000);	//Display "nO SynC" for 1 seconds.
	}
	else								//If the sync was successful...
	{
		sev_seg_flash_word(success, sizeof(success), 1000);	//Display "SUCCESS" for 1 seconds.
	}

	BUTTONS_ENABLE;		//Re-enable the buttons just prior to exiting the function.
}

//This function will update the time array by parsing the date and time from the GPS module.
uint8_t sync_time (uint8_t *time)
{

	usart_print_string("\r\nSyncing...");	//For debugging; indicates entering sync loop

	uint8_t i;	//Initialise an integer to use in for loops

	//RMC sentence format (sections of data separated by commas):
	//Example: $GPRMC,161229.487,A,3723.2475,N,12158.3416,W,0.13,309.62,120598, ,*10
	//$GPRMC,utc time,data valid/invalid,latitude,N/S,longitude,E/W,ground speed(knots),course(deg),date,magnetic variation(deg),checksum
	//Nested if statements to check characters 'R', 'M' & 'C' are received in order - this confirms beginning of RMC sentence.
	while(usart_receive_byte() != 'R'){};		//Ignore all received bytes until the 'R' character is received.
	if(usart_receive_byte() == 'M')			//If the next character is 'M'...
	{
		if(usart_receive_byte() == 'C')		//And the third character is 'C'...  Verified that we have reached the RMC NMEA string.
		{
			usart_receive_byte();		//Ignore the next received byte (delimiter ",")

			//The next 6 bytes received represent the time in BCD -> H,H,M,M,S,S
			for (i = HOU_TENS; i <= SEC_ONES; i++)
			{
				time[i] = (usart_receive_byte() - '0');		//Copy the BCD value to the relevant element of the time array.
			}

			//The next 8 elements of the NMEA data aren't needed and so are ignored.
			for(i = 0; i < 8; i++)			//Loop 8 times
			{
				while (usart_receive_byte() != ','){};	//Skip through 8 delimiters (",") from the RMC sentence to get to the date data.
			}

			//The next 6 bytes received represent the date but are received in an inconvenient (not ISO-8601) order -> D,D,M,M,Y,Y
			time[DAY_TENS] = (usart_receive_byte() - '0');	//Date tens
			time[DAY_ONES] = (usart_receive_byte() - '0');	//Date ones
			time[MON_TENS] = (usart_receive_byte() - '0');	//Month tens
			time[MON_ONES] = (usart_receive_byte() - '0');	//Month ones
			time[YEA_TENS] = (usart_receive_byte() - '0');	//Year tens
			time[YEA_ONES] = (usart_receive_byte() - '0');	//Year ones

			//The final two bytes needed for the time array represent the century.  Safe to set this to "20".
			time[CEN_TENS] = 2;				//Century (Year thousands)
			time[CEN_ONES] = 0;				//Century (Year hundreds)
			//Note, this will cease working as of midnight, January 1st, 2100.

			//We now have the UTC date/time array in BCD format [Y,Y,Y,Y,M,M,D,D,H,H,M,M,S,S]
		}
	}

	//Basic error checking.  All digits in the date/time array "time[]" should be 0-9.  If anything else is found, exit the function and return FALSE.
	for(i = YEA_TENS; i <= SEC_ONES; i++)	//Loop through all elements of the "time" array (excluding manually set century).
	{
		if(time[i] > 9)			//If any element is an integer larger than 9, then the data is no good.
		{
			return(FALSE);		//If the time data is no good, exit the function and return FALSE.
		}
		else
		{
			apply_offset();		//Since the time appears valid, apply the UTC offset.
			rtc_set_time(time);	//Valid time from GPS so update the real-time clock module.
			return(TRUE);		//This will only be reached if the function received valid time data from the GPS module, so return TRUE.
		}
	}

	return(FALSE);		//Should not reach this.
}

//Offset range is -120 to +120 whereby actual offset in half-hour increments correspond to values of 5 (e.g. -120:-12.0hrs, +65:+6.5hrs)
void apply_offset(void)
{
	if (offset < 0)
	{
		//If statement returns true if (absolute value of offset)/10 is non-zero.  Only possible for offset values ending in 5.
		//Therefore ~if(half-hour increment/decrement is required).
		if (abs(offset) % 10)				//Need to decrement minutes by 30.
		{
			time[MIN_TENS] -= 3;			//Decrement minute tens by 3.
			if (time[MIN_TENS] > 5)			//If minute tens has rolled under zero (unsigned integer means it actually rolls to 255)
			{
				time[MIN_TENS] += 6;		//Correct by adding 6...
				time[HOU_ONES] --;		//And decrementing hour ones.
			}
		}

		time[HOU_ONES] -= (abs(offset) / 10) % 10;	//Decrement hour ones by the ones value represented by "offset".
		if (time[HOU_ONES] > 9)				//If the hour ones has rolled under zero (unsigned integer means it actually rolls to 255)
		{
			time[HOU_ONES] += 10;			//Correct by adding 10...
			time[HOU_TENS] --;			//And decrementing hour tens.
		}

		time[HOU_TENS] -= abs(offset) / 100;		//Decrement hour tens by the tens value represented by "offset".

		if (HOU > 23)					//Finally, assess the offset corrected hours value.  If it has rolled under zero...
		{
			rollunder_hours();			//Correct by running the rollunder hours function (offset indicates local time is previous day).
		}
	}
	else if (offset > 0)
	{
		//If statement returns true if (absolute value of offset)/10 is non-zero.  Only possible for offset values ending in 5.
		//Therefore ~if(half-hour increment/decrement is required).
		if (offset % 10)				//Need to increment minutes by 30.
		{
			time[MIN_TENS] += 3;			//Increment minute tens by 3.
			if (time[MIN_TENS] > 5)			//If minute tens has rolled over max valid value.
			{
				time[MIN_TENS] -= 6;		//Correct by minusing 6...
				time[HOU_ONES] ++;		//And incrementing hour ones.
			}
		}

		time[HOU_ONES] += (offset / 10) % 10;		//Increment hour ones by the ones value represented by "offset".
		if (time[HOU_ONES] > 9)				//If the hour ones has rolled over max valid value.
		{
			time[HOU_ONES] -= 10;			//Correct by minusing 10...
			time[HOU_TENS] ++;			//And incrementing hour tens.
		}

		time[HOU_TENS] += offset / 100;			//Increment hour tens by the tens value represented by "offset".

		if (HOU > 23)					//Finally, assess the offset corrected hours value.  If it has rolled over max valid value...
		{
			rollover_hours();			//Correct by running the rollunder hours function (offset indicates local time is previous day).
		}
	}
}

//This function is called when offset corrected time requires the local day to be the day prior to UTC day.
//For correct date/time, Need to add 24 hours and minus one day.
void rollunder_hours(void)
{
	uint8_t hours = HOU + 24;		//"hours" is the desired hours (need to add 24 to the decremented total hours).
	time[HOU_ONES] = hours % 10;		//Set the required value for hour ones.
	time[HOU_TENS] = hours / 10;		//Set the required value for hour tens.
	time[DAY_ONES] --;			//Decrement the day ones.

	if (time[DAY_ONES] > 9)			//Adjust day tens if ones have dropped below zero (e.g. day: 10->09)
	{
		time[DAY_ONES] = 9;		//Set the day ones to 9.
		time[DAY_TENS] --;		//Decrement the day tens.
	}

	if (DAY < 1)				//Asses the offset corrected day.  If it has rolled below minimum valid value of 1...
	{
		rollunder_days();		//Correct by running the rollunder days function (offset indicates local time is previous month).
	}
}

//This function is called when offset corrected time requires the local day to be the day after the UTC day.
//For correct date/time, Need to minus 24 hours and add one day.
void rollover_hours(void)
{
	uint8_t hours = HOU - 24;		//"hours" is the desired hours (need to minus 24 from the incremented total hours).
	time[HOU_ONES] = hours % 10;		//Set the required value for hour ones.
	time[HOU_TENS] = hours / 10;		//Set the required value for hour tens.
	time[DAY_ONES] ++;			//Increment the day ones.

	if (time[DAY_ONES] > 9)			//Adjust day tens if ones have incremented above 9 (e.g. day: 09->10)
	{
		time[DAY_ONES] = 0;		//Set the day ones to zero.
		time[DAY_TENS] ++;		//Increment the day tens.
	}

	if (DAY > 27)				//Asses the offset corrected day.  If it is possible that the month has rolled over...
	{
		rollover_days();		//Check and correct (if required) by running the rollover days.
	}
}

//This function is called when offset corrected time requires the local month to be the month prior to UTC month.
void rollunder_days(void)
{
	//Corrected day value depends on the month being rolled down to.
	switch (MON)
	{
		//In cases where the previous month has 31 days.
		case FEB:
		case APR:
		case JUN:
		case AUG:
		case SEP:
		case NOV:
			time[DAY_TENS] = 3;
			time[DAY_ONES] = 1;
		break;
		//In cases where the previous month has 30 days.
		case MAY:
		case JUL:
		case OCT:
		case DEC:
			time[DAY_TENS] = 3;
			time[DAY_ONES] = 0;
		break;
		//In case of March where the previous month has 28 or 29 days.
		case MAR:
			time[DAY_TENS] = 2;
			time[DAY_ONES] = 9;		//Assume leap year.
			if (YEA % 4)			//If it's not a leap year...
			{
				time[DAY_ONES] --;	//Correct for no leap day.
			}
		break;
		//In case of January where the previous month requires a year rollunder.
		case JAN:
			time[DAY_TENS] = 3;
			time[DAY_ONES] = 1;
			time[YEA_ONES] --;		//Decrement year ones.
			if (time[YEA_ONES] > 9)		//If the year ones has rolled under zero (unsigned integer actually rolls over to 255)...
			{
				time[YEA_ONES] = 9;	//Set the year ones to 9.
				time[YEA_TENS] --;	//Decrement the year tens.
			}
		break;
	}
}

//This function is called when offset corrected time may possibly require the local month to be the month after the UTC month.
void rollover_days(void)
{
	//Corrected day value depends on the month being rolled down to.
	switch (MON)
	{
		//In cases where the month has 31 days.
		case JAN:
		case MAR:
		case MAY:
		case JUL:
		case AUG:
		case OCT:
			if (DAY == 32)
			{
				time[DAY_TENS] = 0;
				time[DAY_ONES] = 1;
				time[MON_ONES] ++;
			}
		break;
		//In cases where the month has 30 days.
		case APR:
		case JUN:
		case NOV:
			if (DAY == 31)
			{
				time[DAY_TENS] = 0;
				time[DAY_ONES] = 1;
				time[MON_ONES] ++;
			}
		break;
		//In case of September with 30 days and the following month requires incrementing month tens (SEP->OCT:09->10)
		case SEP:
			if (DAY == 31)
			{
				time[DAY_TENS] = 0;
				time[DAY_ONES] = 1;
				time[MON_TENS] = 1;
				time[MON_ONES] = 0;
			}
		break;
		//In case of February where the month has 28 or 29 days.
		case FEB:
			if ( (DAY > 28) && (YEA % 4) )	//If days > 28 and it's not a leap year.
			{
				time[DAY_TENS] = 0;
				time[DAY_ONES] = 1;
				time[MON_ONES] ++;
			}
			else if (DAY > 29)		//It must be a leap year.
			{
				time[DAY_TENS] = 0;
				time[DAY_ONES] = 1;
				time[MON_ONES] ++;
			}
		break;
		//In case of December where the next  month requires a year rollover.
		case DEC:
			time[DAY_TENS] = 0;
			time[DAY_ONES] = 1;
			time[YEA_ONES] ++;		//Increment year ones.
			if (time[YEA_ONES] > 9)		//If the year ones has rolled over max valid value...
			{
				time[YEA_ONES] = 0;	//Set the year ones to 0.
				time[YEA_TENS] ++;	//Increment the year tens.
			}
		break;
	}
}

//Allow the UTC offset to be adjusted by pressing the "Sync" button.  Valid UTC offsets are in half-hour increments from -11.5 to 12.0.
//(Note, to avoid using floats, the actual offset value is an integer from -120 to 120)
void get_intensity(void)
{
	sev_seg_all_clear();			//Clear all digits to prevent artifacts from previous mode.

	//Enter a loop that will continuously refresh the display and allow adjustment of the offset.
	while (mode == MODE_5_INTENSITY)					//Run the following loop until the mode is changed.
	{
		sev_seg_set_word(intensity_text, sizeof(intensity_text));	//Display the psuedo-test "IntEnSIty" on the left.
		sev_seg_display_int(intensity);					//Display the current intensity integer value om the right.
		//PLACEHOLDER - Call function to set the brightness level.
	}

	sev_seg_decode_mode(DECODE_CODE_B);	//Return all digits to Code B decode mode.

	if (intensity != eeprom_read_byte(INTENSITY_EEPROM_ADDRESS))		//If the mode has changed and the intensity is different to what's in eeprom...
	{
		eeprom_update_byte((uint8_t *) INTENSITY_EEPROM_ADDRESS, intensity);	//Record the new value to eeprom.
	}
}

//Increment the offset value and rollover when maximum value is exceeded.
//This function is called when the "Sync" button is pressed only when running Mode 4.
void cycle_intensity(void)
{
	intensity ++;								//Increment intensity.
	if (intensity > 15)							//If maximum valid value (9) is exceeded...
	{
		intensity = 0;							//Rollover to minimum valid value (1).
		sev_seg_write_byte(SEV_SEG_DIGIT_14, SEV_SEG_CODEB_BLANK);	//Changing the intensity from 15 to zero requires the tens digit to be cleared.
	}

	sev_seg_set_intensity(intensity);					//Every time the intensity value changes, actually re-set the intensity.
}


//This function will be passed a word (encoded as required by the MAX7219 drivers) and the number of characters in that word.
//Starting from the left most digit (0) all digits required to display the word will be set to manual decode mode.
//All other digits remain in code B decode mode (assume all digits in code B mode when function is called).
//Note, if required, code B mode must be restored manually after this function is complete ("sev_seg_decode_mode(DECODE_CODE_B)").
void sev_seg_set_word(uint8_t *word, uint8_t word_length)
{
	//Set the required number of digits to manual decode mode (0) leaving unrequired digits in code B mode (1).
	sev_seg_write_byte(SEV_SEG_DECODE_MODE_A, 0xFF << (word_length));	//Set the decode mode bits corresponding to digits 0-7 (driver A).
	sev_seg_write_byte(SEV_SEG_DECODE_MODE_B, 0xFF << (word_length - 8));	//Set the decode mode bits corresponding to digits 8-15 (driver B).

	//Note, to increment through all digits, going from DIGIT_7 (0x08) to DIGIT_8 (0x81) requires an addition of 0x78.
	//Therefore, adding (0x78 multiplied by i/8) only applies the addition of 0x78 for i greater than 8 (i is an integer so remainders are ignored).
	for (uint8_t i = 0; i < word_length; i++)
	{
		sev_seg_write_byte(SEV_SEG_DIGIT_0 + i + (0x78 * (i/8)), word[i]);	//Display the character.
	}
}

//Display "pseudo-text" on the seven-segment display.
//Assume all digits currently operating in Code B decode mode when this function is entered.
void sev_seg_flash_word(uint8_t *word, uint8_t word_length, uint16_t duration_ms)
{
	sev_seg_all_clear();			//Clear all digits (only works while in code b decode mode).
	sev_seg_power(OFF);			//Turn off both display drivers (prevents artifacts when changing to manual decode).
	sev_seg_set_word(word, word_length);	//Set up the drivers to display the pseudo-text.
	sev_seg_power(ON);			//Turn off both display drivers (prevents artifacts when changing to manual decode).

	while(duration_ms--)			//Keep the "text" displayed for "duration_ms" milliseconds (can't pass variables directly to _delay_ms()).
	{
		_delay_ms(1);
	}

	sev_seg_power(OFF);			//Turn off both display drivers (prevents artifacts when changing back to Code B decode).
	sev_seg_decode_mode(DECODE_CODE_B);	//Switch back into Code B decode mode for all digits.
	sev_seg_all_clear();			//Clear all digits (note this function only works in Code B mode).
	sev_seg_power(ON);			//Switch the display back on (will be blank).
}

//Simple startup animation that displays "ISO-8601" then scans the decimal point (DP) right to left then back a few times.
void sev_seg_startup_ani(void)
{
	uint8_t i, j;			//Initialise a couple of integers for loops.

	sev_seg_flash_word(splash, sizeof(splash), 3000);	//Display the "Splash-Screen" (pseudo text).

	//The following nested loops scans the decimal point (DP) right to left then back a few times.
	for (j = 0; j < 5; j++)		//Repeat main animation sequence 5 times.
	{
		//Note, to increment through all digits, going from DIGIT_7 (0x08) to DIGIT_8 (0x81) requires an addition of 0x78.
		//Therefore, adding (0x78 multiplied by i/8) only applies the addition of 0x78 for i greater than 8 (i is an integer so remainders are ignored).
		for (i = 0; i < 16; i++)	//Increment the following through all 16 digits 0 to 15 (left to right).
		{
			//Turn on the decimal point for the current digit.
			sev_seg_write_byte(SEV_SEG_DIGIT_0 + i + (0x78 * (i/8)), SEV_SEG_CODEB_BLANK | SEV_SEG_DP);
			_delay_ms(20);			//Pause for milliseconds.
			sev_seg_all_clear();		//Clear the DP (and all digits).
		}
		for (i = 15; i < 255; i--)	//Decrement the following through all 16 digits 15 to 0 (right to left).
		{
			sev_seg_write_byte(SEV_SEG_DIGIT_0 + i + (0x78 * (i/8)), SEV_SEG_CODEB_BLANK | SEV_SEG_DP);
			_delay_ms(20);			//Pause for milliseconds.
			sev_seg_all_clear();		//Clear the DP (and all digits).
		}
	}
}

int main(void)
{
	hardware_init();	//initialise the hardware peripherals.

	settings_init();	//Initialise (validate and set) system settings stored in eeprom.

	sev_seg_startup_ani();	//Run through the start-up animation.

	attempt_sync();		//Attempt to sync the RTC time with GPS data.

	while (1)		//Main infinite loop.
	{

		poll();		//Poll the selected mode and take the appropriate action..

	}
	return 0;		//Never reached.
}
