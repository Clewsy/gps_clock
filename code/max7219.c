//Functions to control two cascaded MAX7219 8x7-segment display drivers (total 16x 70segment digits).
#include <max7219.h>

//Writes a byte to an address in both of the MAX7219s.  Note, one driver will receive data, the other will receive no-op command.
void sev_seg_writeByte(uint8_t address, uint8_t data)
{
	if(address & 0x80)		//Check driver flag.  If set, address is for driver B (DIG_8 to DIG_15).
	{
		SEV_SEG_LOAD_LOW;	//Drop the level of the LOAD pin
		spi_tradeByte(address);	//Send the register address where the data will be stored
		spi_tradeByte(data);	//Send the data to be stored
		spi_tradeByte(0);	//Roll no-op data through to push real data out of driver A and into driver B (address byte)
		spi_tradeByte(0);	//Roll no-op data through to push real data out of driver A and into driver B (data byte)
		SEV_SEG_LOAD_HIGH;	//Raise the level of the LOAD pin - this triggers latching of the sent bytes (last 16 bits of data are latched).
	}
	else				//Not set, therefore address is for driver A (DIG_0 to DIG_7).
	{
		SEV_SEG_LOAD_LOW;	//Drop the level of the LOAD pin
		spi_tradeByte(0);	//Roll no-op data in first for driver B when real data for driver A pushes it through (address byte)
		spi_tradeByte(0);	//Roll no-op data in first for driver B when real data for driver A pushes it through (data byte)
		spi_tradeByte(address);	//Send the register address where the data will be stored
		spi_tradeByte(data);	//Send the data to be stored
		SEV_SEG_LOAD_HIGH;	//Raise the level of the LOAD pin - this triggers latching of the sent bytes (last 16 bits of data are latched).
	}
}

//Initialise both the display drivers.
void sev_seg_init(void)
{
	SPI_DDR |= (1 << SEV_SEG_LOAD); 		//Set LOAD pin as an output
	PORTB |= (1 << SEV_SEG_LOAD);			//Set LOAD pin to high at start (data latching occurs on LOAD rising edge).

	//First setup max7219 driver A (digits 0-7)
	sev_seg_writeByte(SEV_SEG_SCAN_LIMIT_A, 7);		//Set number of digits in the display to 8 (Data=number of digits - 1)
	sev_seg_writeByte(SEV_SEG_INTENSITY_A, 0x08);		//Set brightness (duty cycle) to about half-way.
	sev_seg_writeByte(SEV_SEG_SHUTDOWN_A, 1);		//Enter normal operation (exit shutdown mode).
	sev_seg_writeByte(SEV_SEG_DECODE_MODE_A, 0xFF);		//Set all digits to be set by Code B data input.
	sev_seg_writeByte(SEV_SEG_DISPLAY_TEST_A, 0x00);	//Ensures display test mode is set to normal (sometimes set to test mode on re-programme).

	//Second setup max7219 driver B (digits 8-15)
	sev_seg_writeByte(SEV_SEG_SCAN_LIMIT_B, 7);		//Set number of digits in the display to 8 (Data=number of digits - 1)
	sev_seg_writeByte(SEV_SEG_INTENSITY_B, 0x08);		//Set brightness (duty cycle) to about half-way.
	sev_seg_writeByte(SEV_SEG_SHUTDOWN_B, 1);		//Enter normal operation (exit shutdown mode).
	sev_seg_writeByte(SEV_SEG_DECODE_MODE_B, 0xFF);		//Set all digits to be set by Code B data input.
	sev_seg_writeByte(SEV_SEG_DISPLAY_TEST_B, 0x00);	//Ensures display test mode is set to normal (sometimes set to test mode on re-programme).
}

//Clears all digits.  Bypasses function "sev_seg_writeByte" and clears equivalent digits on both drivers simultaneously (~halves clear time).
void sev_seg_allClear(void)
{
	uint8_t i;
	for (i=8; i>0; i--)	//Counts down through the digits from digit 7 (8) to digit 0 (1).
	{
		SEV_SEG_LOAD_LOW;				//Drop the level of the LOAD pin
		spi_tradeByte(i);				//Push in digit address i (will be in driver B at latch)
		spi_tradeByte(SEV_SEG_CODEB_BLANK);		//Push in data to clear digit at i (will be in driver B at latch)
		spi_tradeByte(i);				//Push in digit address i (will be in driver A at latch)
		spi_tradeByte(SEV_SEG_CODEB_BLANK);		//Push in data to clear digit at i (will be in driver A at latch)
		SEV_SEG_LOAD_HIGH;				//Raise the level of the LOAD pin - triggers latching of the sent bytes (last 16 bits latched).
	}
}

//Simple startup animation scans the decimal point (DP) right to left then back a few times.
void sev_seg_startupAni(void)
{
	sev_seg_allClear();		//Start by clearing all digits.
	uint8_t i = 3;			//Animation repeats 3 times.
	while(i)			//The two while loops using j write the DP, pause, clear the DP then move to the next digit from left to right.
	{
		uint8_t j = SEV_SEG_DIGIT_15;		//Loop for driver B (DIG_15 to DIG_8).
		do
		{
			sev_seg_writeByte(j,(SEV_SEG_CODEB_BLANK | SEV_SEG_DP));	//Clear the digit except turn on the DP
			_delay_ms(50);							//Pause for 50 milliseconds.
			sev_seg_writeByte(j,SEV_SEG_CODEB_BLANK);			//Clear the digit inc. the DP
			j--;
		}while(j>=SEV_SEG_DIGIT_8);		//Exits loop when j counts down to digit 7

		j = SEV_SEG_DIGIT_7;			//Loop for driver A (DIG_7 to DIG_0).
		do
		{
			sev_seg_writeByte(j,(SEV_SEG_CODEB_BLANK | SEV_SEG_DP));	//Clear the digit except turn on the DP
			_delay_ms(50);							//Pause for 50 milliseconds.
			sev_seg_writeByte(j,SEV_SEG_CODEB_BLANK);			//Clear the digit inc. the DP
			j--;
		}while(j>=SEV_SEG_DIGIT_0);		//Exits the loop when j counts down past digit 0.
		i--;	//Exits the loop once i counts down to 0.
	}

	//Following block prints "ISO-8601" to the seven-segment displays for 2 seconds.
	sev_seg_writeByte(SEV_SEG_DIGIT_4, 1);				//I
	sev_seg_writeByte(SEV_SEG_DIGIT_5, 5);				//S
	sev_seg_writeByte(SEV_SEG_DIGIT_6, 0);				//O
	sev_seg_writeByte(SEV_SEG_DIGIT_7, SEV_SEG_CODEB_DASH);		//-
	sev_seg_writeByte(SEV_SEG_DIGIT_8, 8);				//8
	sev_seg_writeByte(SEV_SEG_DIGIT_9, 6);				//6
	sev_seg_writeByte(SEV_SEG_DIGIT_10, 0);				//0
	sev_seg_writeByte(SEV_SEG_DIGIT_11, 1);				//1

	_delay_ms(1000);		//Wait for milliseconds
	sev_seg_allClear();		//Clear all 7-seg digits.
}

void sev_seg_display_int(uint64_t num)
{
	sev_seg_writeByte(SEV_SEG_DIGIT_15, (num % 10));
	sev_seg_writeByte(SEV_SEG_DIGIT_14, ((num / 10) % 10));
	sev_seg_writeByte(SEV_SEG_DIGIT_13, ((num / 100) % 10));
	sev_seg_writeByte(SEV_SEG_DIGIT_12, ((num / 1000) % 10));
	sev_seg_writeByte(SEV_SEG_DIGIT_11, ((num / 10000) % 10));
	sev_seg_writeByte(SEV_SEG_DIGIT_10, ((num / 100000) % 10));
	sev_seg_writeByte(SEV_SEG_DIGIT_9,  ((num / 1000000) % 10));
	sev_seg_writeByte(SEV_SEG_DIGIT_8,  ((num / 10000000) % 10));
	sev_seg_writeByte(SEV_SEG_DIGIT_7,  ((num / 100000000) % 10));
	sev_seg_writeByte(SEV_SEG_DIGIT_6,  ((num / 1000000000) % 10));
	sev_seg_writeByte(SEV_SEG_DIGIT_5,  ((num / 10000000000) % 10));
	sev_seg_writeByte(SEV_SEG_DIGIT_4,  ((num / 100000000000) % 10));
	sev_seg_writeByte(SEV_SEG_DIGIT_3,  ((num / 1000000000000) % 10));
	sev_seg_writeByte(SEV_SEG_DIGIT_2,  ((num / 10000000000000) % 10));
	sev_seg_writeByte(SEV_SEG_DIGIT_1,  ((num / 100000000000000) % 10));
	sev_seg_writeByte(SEV_SEG_DIGIT_0,  ((num / 1000000000000000) % 10));

}
