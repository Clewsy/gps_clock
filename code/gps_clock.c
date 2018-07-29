#include "gps_clock.h"

//Interrupt subroutine triggered whenever the AVR receives a byte on the serial line (USART).  This was used for debugging.
//ISR(USART_RX_vect)	//Interrupt subroutine triggered when the USART receives a byte.
//{
//	usart_transmit_byte(usart_receive_byte());	//Echos received byte.
//}

//Initialise the peripherals.
void hardware_init(void)
{
	clock_prescale_set(clock_div_1);	//needs #include <avr/power.h>, prescaler 1 gives clock speed 8MHz
	power_adc_disable();			//Since it's not needed, disable the ADC and save a bit of power

	usart_init();				//Initialise the USART to enable serial communications.
	spi_init(SEV_SEG_POL, SEV_SEG_PHA);	//Initialise the SPI to enable comms with SPI devices.
						//Note, both spi devices (RTC and max7219) operate in spi mode 3, so spi_init only needs to be called once.

	sev_seg_init();				//Initialise the two cascaded max7219 chips that will drive the 16 7-segment digits.

//	sei();				//Global enable interrups (requires avr/interrupt.h)
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
			delimiters = 0;		//Do NOT show decimal points between years, months, days, hours, mionutes, seconds.
			break;
		case (1) :			//i.e. MODE_1B or MODE_2B
			delimiters = SEV_SEG_DP;//DO show decimal points between years, months, days, hours, mionutes, seconds.
			break;
	}

	int i;	//General use integer for running for loops.

	//This loop will apply the decimal point flag to the last digit of each date/time component IF "delimiters" is set my the operating mode.
	for (i=3;i<14;i+=2)
	{
		time[i] |= delimiters;	//"delimiters" is either 0 (no effect) or SEV_SEG_DP which will set the decimal point flag for the sev-seg drivers.
	}

	//This loop will clear every digit in the display buffer in case the mode has changed.
	for (i=0;i<16;i++)
	{
		disp_buffer[i] = SEV_SEG_CODEB_BLANK;
	}

	//This loop will set the display buffer digits for the date components (CEN, YEA, MON, DAY).
	for (i=0;i<8;i++)
	{
		disp_buffer[i + date_offset] = time[i];			//"date_offset" determined by current mode.
	}
	//This loop will set the display buffer digits for the time components (HOU, MIN, SEC).
	for (i=8;i<14;i++)
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
	int i;
	for (i=0;i<8;i++)	//Each iteration will send a digit to driver A and a digit to driver B so 8 iterations sends all 16 digits.
	{
		sev_seg_write_byte(SEV_SEG_DIGIT_0 + i, disp_buffer[i]);	//Send the digit for driver A.
		sev_seg_write_byte(SEV_SEG_DIGIT_8 + i, disp_buffer[i+8]);	//Send the digit for driver B.
	}
}

//This function will update the time array by parsing the date and time from the GPS module.
void sync_time (uint8_t *time)
{
	usart_print_string("Syncing\r\n");	//For debugging; indicates entering sync loop

	//RMC sentence format (sections of data separated by commas):
	//Example: $GPRMC,161229.487,A,3723.2475,N,12158.3416,W,0.13,309.62,120598, ,*10
	//$GPRMC,utc time,data valid/invalid,latitude,N/S,longitude,E/W,ground speed(knots),course(deg),date,magnetic variation(deg),checksum
	//Nested if statements to check characters 'R', 'M' & 'C' are received in order - this confirms beginning of RMC sentence.
	while(usart_receive_byte() != 'R'){};		//Ignore all received bytes until the 'R' character is received.
	if(usart_receive_byte() == 'M')			//If the next character is 'M'...
	{
		if(usart_receive_byte() == 'C')		//And the third character is 'C'...
		{
			usart_receive_byte();		//Ignore the next received byte (delimiter ",")

			int i;	//Initialise an integer to use in for loops

			//The next 6 bytes received represent the time in BCD -> H,H,M,M,S,S
			for (i = HOU_TENS; i <= SEC_ONES; i++)
			{
				time[i] = (usart_receive_byte() - '0');		//Copy the BCD value to the relevant element of the time array.
			}

			//The next 8 elements of the NMEA data aren't needed and so are ignored.
			for(i = 0; i < 8; i++)			//Loop 8 times
			{
				do {}
				while (usart_receive_byte() != ',');	//Skip through 8 delimiters (",") from the RMC sentence to get to the date data.
			}

			//The next 6 bytes received represent the date but are received in an inconvenient order -> D,D,M,M,Y,Y
			time[DAY_TENS] = (usart_receive_byte() - '0');	//Date tens
			time[DAY_ONES] = (usart_receive_byte() - '0');	//Date ones
			time[MON_TENS] = (usart_receive_byte() - '0');	//Month tens
			time[MON_ONES] = (usart_receive_byte() - '0');	//Month ones
			time[YEA_TENS] = (usart_receive_byte() - '0');	//Year tens
			time[YEA_ONES] = (usart_receive_byte() - '0');	//Year ones

			//The final two bytes needed for the time array represent the century.  Safe to set this to "20".
			time[CEN_TENS] = 2;				//Century (Year hundreds & thousands)
			time[CEN_ONES] = 0;				//Century (Year hundreds & thousands)
			//Note, this will not work as of midnight January 1st, 2100.

			//We now have the UTC date/time array in BCD format [Y,Y,Y,Y,M,M,D,D,H,H,M,M,S,S]
		}
	}

	//debugging: print date to usart
	usart_print_string("yyyy-mm-dd : ");
	usart_transmit_byte(time[CEN_TENS] + '0');
	usart_transmit_byte(time[CEN_ONES] + '0');
	usart_transmit_byte(time[YEA_TENS] + '0');
	usart_transmit_byte(time[YEA_ONES] + '0');
	usart_print_string("-");
	usart_transmit_byte(time[MON_TENS] + '0');
	usart_transmit_byte(time[MON_ONES] + '0');
	usart_print_string("-");
	usart_transmit_byte(time[DAY_TENS] + '0');
	usart_transmit_byte(time[DAY_ONES] + '0');

	//debugging: print time to usart
	usart_print_string("\r\n  hh-mm-ss : ");
	usart_transmit_byte(time[HOU_TENS] + '0');
	usart_transmit_byte(time[HOU_ONES] + '0');
	usart_print_string(":");
	usart_transmit_byte(time[MIN_TENS] + '0');
	usart_transmit_byte(time[MIN_ONES] + '0');
	usart_print_string(":");
	usart_transmit_byte(time[SEC_TENS] + '0');
	usart_transmit_byte(time[SEC_ONES] + '0');
	usart_print_string("\r\n");

}

//Simple startup animation scans the decimal point (DP) right to left then back a few times.
void sev_seg_startupAni(void)
{
	clear_disp_buffer();		//Start by setting all 16 digits in the buffer to blank (off).
	refresh_display();		//Refresh the display in accordance with the buffer (i.e. clear all digits);

	uint8_t i, j;			//Initialise a couple of integers for loops.

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

	clear_disp_buffer();		//End by setting all 16 digits in the buffer to blank (off).
	refresh_display();		//Refresh the display in accordance with the buffer (i.e. clear all digits);
}

int main(void)
{
	hardware_init();	//initialise the hardware peripherals

	usart_print_string("clock(get_time);\r\n");

	sev_seg_startupAni();

	//for debugging - display an integer for 2 seconds
	//sev_seg_display_int(1234567890180085);
	//_delay_ms(2000);
	//sev_seg_all_clear();

	while (1)
	{
		sync_time(time);
		display_time(time, MODE_1B);
	}

	return 0;	//Never reached.
}
