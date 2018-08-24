#include "gps_clock.h"

//The following interrupt sub-routine will be triggered every time there is a change in state of either button.
ISR(BUTTON_PCI_VECTOR)
{
	_delay_ms(BUTTON_DEBOUNCE_DURATION);	//wait for DEBOUNCE_DURATION milliseconds to mitigate effect of switch bounce.

	if(BUTTON_PINS & (1 << BUTTON_MODE))
	{
		mode++;				//Increment the mode.
		if(mode > MODE_2B)		//If largest mode value reached.
		{
			mode = MODE_1A;		//Cycle back to first mode.
		}
	}
	while(BUTTON_PINS & (1 << BUTTON_MODE))	//Keep refreshing the display until the button is released.
	{
		rtc_get_time(time);		//Update the current time from the rtc.
		display_time(time, mode);	//Display the current time.
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
	BUTTON_PCMSK |= ((1 << BUTTON_MODE) | (1 << BUTTON_SELECT));
						//Enable Pin-Change Interrupt on PCINT[x] for PCINT pins to which the 5 buttons are connected.
						//PCMSK0: Pin-Change Mask Register 0 (For PCINT[7-0] i.e. PB[7-0])
						//PCMSK1: Pin-Change Mask Register 1 (For PCINT[14-8] i.e. PC[6-0])
						//PCMSK2: Pin-Change Mask Register 2 (For PCINT[23-16] i.e. PD[7-0])
						//PCINT[0-7]: Pin-Change Interrupt [0-7]. Note PCINT[7-0]: PB[7-0]
						//PCINT[14-8]: Pin-Change Interrupt [14-8]. Note PCINT[7-0]: PC[6-0]
						//PCINT[23-16]: Pin-Change Interrupt [23-16]. Note PCINT[7-0]: PD[7-0]
	BUTTON_PORT |= ((1 << BUTTON_MODE) | (1 << BUTTON_SELECT));
						//Enable pull-up resistors for the buttons.

	sei();			//Global enable interrups (requires avr/interrupt.h)
}

//This function sets the display buffer array in accordance with the current time and the selected display mode,
// then requests a display update in accordance with the display buffer.
void display_time(uint8_t *time, uint8_t mode)
{
	uint8_t date_offset = 0;	//This offset will determine which digit the date will start at.
	uint8_t time_offset = 0;	//This offset will determine how many digits after the date that the tim ewill start.
	uint8_t delimiters = 0;		//This flag will be used to activate the decimal point if the current mode requires it.

	//First switch differentiates between primary modes 1 & 2 (sub-modes A & B are ignored).
	switch (mode >> 1)			//Second lsb of the mode byte determines primary mode 1 or 2.
	{
		case (0) :			//i.e. MODE_1A or MODE_1B
			date_offset = 0;	//Start date display at the first digit (digit 0).
			time_offset = 2;	//Leave two digits blank after the date and before the time.
			break;
		case (1) :			//i.e. MODE_2A or MODE_2B
			date_offset = 1;	//Start date display at the second digit (digit 1).
			time_offset = 0;	//Start time display immediately after date display.
			break;
	}

	//Second switch differentiates between sub-modes A & B (primary modes 1 or 2 are ignored).
	switch (mode & 1)			//Lsb of the mode byte determines sub-bode A or B.
	{
		case (0) :			//i.e. MODE_1A or MODE_2A
			delimiters = 0;		//Do NOT show decimal points between years, months, DATs, hours, mionutes, seconds.
			break;
		case (1) :			//i.e. MODE_1B or MODE_2B
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

			//The next 6 bytes received represent the date but are received in an inconvenient order -> D,D,M,M,Y,Y
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

//Simple startup animation that displays "ISO-8601" then scans the decimal point (DP) right to left then back a few times.
void sev_seg_startupAni(void)
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

	usart_print_string("\r\nclock(get_time);\r\n");

	sev_seg_startupAni();

	//Attempt to sync rtc time with gps time.
	if (!sync_time(time))	//If the sync is unsuccessful
	{
		sev_seg_display_int(5318008);	//Show a specified integer...
		_delay_ms(5000);		//...for 5 seconds.
	}

	while (1)
	{


		rtc_get_time(time);		//Update the current time from the rtc.
		display_time(time, mode);	//Display the current time.


	}
	return 0;	//Never reached.
}
