#include "gps_clock.h"

//The following interrupt sub-routine will be triggered every time there is a change in state of either button.
ISR(BUTTON_PCI_VECTOR)
{
	_delay_ms(BUTTON_DEBOUNCE_DURATION);	//wait for DEBOUNCE_DURATION milliseconds to mitigate effect of switch bounce.

	//If statement captures press of the "Mode" button.  Cycles through the various display modes.
	if(!(BUTTON_PINS & (1 << BUTTON_MODE)))
	{
		sev_seg_decode_mode(DECODE_CODE_B);	//Whenever the mode is changed, return all digits to default Code B decode mode.
		sev_seg_all_clear();			//Clear all digits to avoid artifacts between display modes.
		mode++;					//Increment the mode.
		if(mode > MODE_4)			//If largest mode value reached.
		{
			mode = MODE_1A;			//Cycle back to first mode.
		}
		while(BUTTON_PINS & (1 << BUTTON_MODE))	//Keep refreshing the display until the button is released.
		{
			rtc_get_time(time);		//Update the current time from the rtc.
			poll();				//Display the current time.
		}
	}

	//If statement captures press of the "Sync" button.  Initiates at attempt at syncing RTC clock to GPS data.
	if(!(BUTTON_PINS & (1 << BUTTON_SYNC)))
	{
		if (mode == MODE_4)		//Special case: if mode 4 is running, the Sync button cycles the UTC offset.
		{
			cycle_offset();		//Cycle the UTC offset value (-11.5hrs to +12.0hrs).
		}
		else				//All other modes, the Sync button attempts to sync the clock.
		{
			attempt_sync();		//Attempt to sync the RTC time with GPS data.
		}
	}
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


	PCICR |= (1 << BUTTON_PCIE);		//Enable Pin-Change Interrupt for pin-change int pins PCINT[8-14].  This includes both buttons.
						//PCICR: Pin-Change Interrupt Control Register
						//PCIE0: Pin-Change Interrupt Enable 0 (Enables PCINT[7-0] i.e. PB[7-0])
						//PCIE1: Pin-Change Interrupt Enable 1 (Enables PCINT[14-8] i.e. PC[6-0])
						//PCIE2: Pin-Change Interrupt Enable 2 (Enables PCINT[23-16] i.e. PD[7-0])
	BUTTON_PCMSK |= ((1 << BUTTON_MODE) | (1 << BUTTON_SYNC));
						//Enable Pin-Change Interrupt on PCINT[x] for PCINT pins to which the 5 buttons are connected.
						//PCMSK0: Pin-Change Mask Register 0 (For PCINT[7-0] i.e. PB[7-0])
						//PCMSK1: Pin-Change Mask Register 1 (For PCINT[14-8] i.e. PC[6-0])
						//PCMSK2: Pin-Change Mask Register 2 (For PCINT[23-16] i.e. PD[7-0])
						//PCINT[0-7]: Pin-Change Interrupt [0-7]. Note PCINT[7-0]: PB[7-0]
						//PCINT[14-8]: Pin-Change Interrupt [14-8]. Note PCINT[7-0]: PC[6-0]
						//PCINT[23-16]: Pin-Change Interrupt [23-16]. Note PCINT[7-0]: PD[7-0]
	BUTTON_PORT |= ((1 << BUTTON_MODE) | (1 << BUTTON_SYNC));
						//Enable pull-up resistors for the buttons.

	sei();			//Global enable interrups (requires avr/interrupt.h)
}

//This function will call other functions depending on the currently selected display mode.
void poll(void)
{
	switch (mode)	//Thhis switch is used to setup the display depending on which mode is selected.
	{
		//Modes 1A, 1B, 2A and 2B are all variations on the ISO-8601 date/time display.
		case (MODE_1A) :	//	|Y Y Y Y M M D D     H H M M S S |
		case (MODE_1B) :	//	|Y Y Y Y.M M.D D.    H H.M M.S S.|
		case (MODE_2A) :	//	|  Y Y Y Y M M D D H H M M S S   |
		case (MODE_2B) :	//	|  Y Y Y Y.M M.D D.H H.M M.S S.  |
			display_iso_time();
		break;

		//Mode 3 displays Epoch time (unix time) which is the number of seconds since midnight, January first, 1970 (i.e. 19700101000000).
		case (MODE_3) :		//	|E P O C H   S S S S S S S S S S |
			display_epoch_time();
		break;

		//Mode 4 allows setting the UTC time offset (-11.5hrs to +12.0hrs).
		case (MODE_4) :		//	|A d J U S t             ± # #.# |
			get_offset();
		break;
	}
}

//Display the time in a standard ISO-8601 format.
void display_iso_time(void)
{
	uint8_t date_offset = 0;	//This offset will determine which digit the date will start at.
	uint8_t time_offset = 0;	//This offset will determine how many digits after the date that the time will start.
	uint8_t delimiters = 0;		//This flag will be used to activate the decimal point if the current mode requires it.

	//This switch used to set the positions of the date and time within the 16-digit display.
	switch (mode)
	{
		case (MODE_1A) :
		case (MODE_1B) :
			date_offset = 0;	//Start date display at the first digit (digit 0).
			time_offset = 2;	//Leave two digits blank after the date and before the time.
		break;

		case (MODE_2A) :
		case (MODE_2B) :
			date_offset = 1;	//Start date display at the second digit (digit 1).
			time_offset = 0;	//Start time display immediately after date display.
		break;
	}

	//This switch used to determine whether or not delimiters are to be used (decimal point between time/date components).
	switch (mode)
	{
		case (MODE_1A) :
		case (MODE_2A) :
			delimiters = 0;		//Do NOT show decimal points between years, months, DATs, hours, mionutes, seconds.
		break;

		case (MODE_1B) :
		case (MODE_2B) :
			delimiters = SEV_SEG_DP;//DO show decimal points between years, months, DATs, hours, mionutes, seconds.
		break;
	}

	uint8_t i;	//General use integer for running for loops.

	//This loop will apply the decimal point flag to the last digit of each date/time component IF "delimiters" is set my the operating mode.
	for (i = 3; i < 14; i += 2)
	{
		time[i] |= delimiters;	//"delimiters" is either 0 (no effect) or SEV_SEG_DP which will set the decimal point flag for the sev-seg drivers.
	}

	//This loop will clear every digit in the display buffer in case the mode has changed.
	for (i = 0; i < 16; i++)
	{
		disp_buffer[i] = SEV_SEG_CODEB_BLANK;
	}

	//This loop will set the display buffer digits for the date components (CEN, YEA, MON, DAT).
	for (i = 0; i < 8; i++)
	{
		disp_buffer[i + date_offset] = time[i];			//"date_offset" determined by current mode.
	}

	//This loop will set the display buffer digits for the time components (HOU, MIN, SEC).
	for (i = 8; i < 14; i++)
	{
		disp_buffer[i + date_offset + time_offset] = time[i];	//"time_offset" determined by current mode.
	}

	refresh_display();	//send the display buffer data to the sev-seg display drivers.
}

//This function will write data to the display buffer that corresponds to a blank digit (all segments off) for all 16.
void clear_disp_buffer(void)
{
	for (uint8_t i = 0; i < 16; i++)		//Repeat for all 16 digits (0 to 15).
	{
		disp_buffer[i] = SEV_SEG_CODEB_BLANK;	//Set the element to the data that corresponds to a blank digit.
	}
}

//This function will take the contents of the display buffer array and send it to the seven segment display drivers
void refresh_display(void)
{
	for (uint8_t i = 0; i < 8; i++)	//Each iteration will send a digit to driver A and a digit to driver B so 8 iterations sends all 16 digits.
	{
		sev_seg_write_byte(SEV_SEG_DIGIT_0 + i, disp_buffer[i]);	//Send the digit for driver A (digits 0 to 7).
		sev_seg_write_byte(SEV_SEG_DIGIT_8 + i, disp_buffer[i+8]);	//Send the digit for driver B (digits 8 to 15).
	}
}

void display_epoch_time(void)
{
	//Initialisation of the epoch variable (number of seconds elapsed since 1970.01.01.00.00.00.
	uint64_t epoch = 0;

	sev_seg_write_byte(SEV_SEG_DECODE_MODE_A, 0b11000000);		//Set the first 5 digits (0-4) to manual decode to display text.
	for (uint8_t i = 0; i < 6; i++)					//For loop runs through the first 6 digits (0-5).
	{
		sev_seg_write_byte(SEV_SEG_DIGIT_0 + i, epoch_text[i]);	//Write the pseudo-text "EPOCH-"
	}

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

//Allow the UTC offset to be adjusted by pressing the "Sync" button.  Valid UTC offsets are in half-hour increments from -11.5 to 12.0.
//(Note, to avoid using floats, the actual offset value is an integer from -115 to 120)
void get_offset(void)
{
	sev_seg_all_clear();						//Clear all digits to prevent artifacts from previous mode.
	sev_seg_write_byte(SEV_SEG_DECODE_MODE_A, 0x00);		//Set the first 8 digits (0-7) to manual decode to display text.
	while (mode == MODE_4)						//Run the following loop until the mode is changed.
	{
		for (uint8_t i = 0; i < 8; i++)					//For loop runs through the first 8 digits (0-7).
		{
			sev_seg_write_byte(SEV_SEG_DIGIT_0 + i, adjust[i]);	//Write the pseudo-text "AdJUSt  "
		}
		display_offset();						//Display the current offset value using the last 4 digits (12-15).
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
		offset = -115;		//Rollover to minimum valid value (-115).
	}
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
			time[DAT_TENS] = (usart_receive_byte() - '0');	//Date tens
			time[DAT_ONES] = (usart_receive_byte() - '0');	//Date ones
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

	//Error checking.  All digits in the date/time array "time[]" should be 0-9.  If anything else is found, exit the function and return FALSE.
	for(i = YEA_TENS; i <= SEC_ONES; i++)		//Loop through all elements of the "time" array (excluding manually set century).
	{
		if(time[i] > 9)				//If any element is an integer larger than 9, then the data is no good.
		{
			//usart_print_string("No good.");	//Print to usart for debugging.
			return(FALSE);			//If the time data is no good, exit the function and return FALSE.
		}
		else
		{
			rtc_set_time(time);	//Valid time from GPS so update the real-time clock module.
			return(TRUE);		//This will only be reached if the function received valid time data from the GPS module, so return TRUE.
		}
	}

	return(FALSE);		//Should not reach this.
}

//Attempt to sync thr RTC time to GPS data.  Display status on-screen using pseudo-text.
void attempt_sync(void)
{
	//Attempt to sync rtc time with gps time.
	sev_seg_display_word(syncing, 2000);		//Display "SynCIng" for 3 seconds.
	if (!sync_time(time))				//If the sync is unsuccessful...
	{
		sev_seg_display_word(no_sync, 1000);	//Display "nO SynC" for 3 seconds.
	}
	else						//If the sync was successful...
	{
		sev_seg_display_word(success, 1000);	//Display "SUCCESS" for 3 seconds.
	}
}

//Display "pseudo-text" on the seven-segment display.
void sev_seg_display_word(uint8_t *word, uint16_t duration_ms)
{
	//Note, display running in "Code B" decode mode when function is entered.  This needs to be changed to manual decode to print text-like characters.
	sev_seg_all_clear();			//Clear all digits.
	sev_seg_power(OFF);			//Turn off both display drivers (prevents artifacts when changing to manual decode).
	sev_seg_decode_mode(DECODE_MANUAL);	//Switch into manual decode mode for all digits.

	for (uint8_t i = 0; i < 16; i++)	//Run a loop to set all 16 digits in the display buffer.
	{
		disp_buffer[i] = word[i];	//Copy the word array to the disp_buffer array.
	}
	refresh_display();			//Refresh the display in accordance with the buffer.
	sev_seg_power(ON);			//Switch the display drivers back on.

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
	clear_disp_buffer();		//Start by setting all 16 digits in the buffer to blank (off).
	refresh_display();		//Refresh the display in accordance with the buffer (i.e. clear all digits);

	uint8_t i, j;			//Initialise a couple of integers for loops.

	//Following block prints "ISO-8601" to the seven-segment displays for 2 seconds.
	uint8_t splash_string[16] = 	{SEV_SEG_CODEB_BLANK, SEV_SEG_CODEB_BLANK, SEV_SEG_CODEB_BLANK, SEV_SEG_CODEB_BLANK,
		 			1, 5, 0, SEV_SEG_CODEB_DASH, 8, 6, 0, 1,
					SEV_SEG_CODEB_BLANK, SEV_SEG_CODEB_BLANK, SEV_SEG_CODEB_BLANK, SEV_SEG_CODEB_BLANK};
	for (i = 0; i < 16; i++)
	{
		disp_buffer[i] = splash_string[i];	//Copy the splash_string array to the disp_buffer array.
	}
	refresh_display();		//Refresh the display in accordance with the buffer.
	_delay_ms(2000);		//Wait for milliseconds.

	//The following nested loops scans the decimal point (DP) right to left then back a few times.
	for (j = 0; j < 5; j++)		//Repeat main animation sequence 5 times.
	{
		for (i = 0; i < 16; i++)	//Increment the following through all 16 digits 0 to 15 (left to right).
		{
			disp_buffer[i] = (SEV_SEG_CODEB_BLANK | SEV_SEG_DP);	//Turn on the decimal point for the current digit.
			refresh_display();					//Refresh the display in accordance with the buffer.
			_delay_ms(20);						//Short pause.
			clear_disp_buffer();					//Clear the decimal point from the buffer.
		}
		for (i = 15; i < 250; i--)	//Decrement the following through all 16 digits 0 to 15 (right to left).
		{
			disp_buffer[i] = (SEV_SEG_CODEB_BLANK | SEV_SEG_DP);	//Turn on the decimal point for the current digit.
			refresh_display();					//Refresh the display in accordance with the buffer.
			_delay_ms(20);						//Short pause.
			clear_disp_buffer();					//Clear the decimal point from the buffer.
		}
	}

	clear_disp_buffer();		//End by setting all 16 digits in the buffer to blank (off).
	refresh_display();		//Refresh the display in accordance with the buffer (i.e. clear all digits);
}


int main(void)
{
	hardware_init();	//initialise the hardware peripherals

	//usart_print_string("\r\nclock(get_time);\r\n");	//Used for debugging.

	sev_seg_startup_ani();		//Run through the start-up animation.

	attempt_sync();			//Attempt to sync the RTC time with GPS data.

	while (1)
	{

		rtc_get_time(time);		//Update the current time from the rtc.
		poll();	//Display the current time.


	}
	return 0;	//Never reached.
}
