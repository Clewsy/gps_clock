//Functions to control two cascaded MAX7219 8x7-segment display drivers (total 16x 70segment digits).
#include <MAX7219.h>

//Writes a byte to an address in both of the MAX7219s.  Note, one driver will receive data, the other will receive no-op command.
void SEV_SEG_writeByte(uint8_t address, uint8_t data)	
{
	if(address & 0x80)	//Check driver flag.  If set, address is for driver B (DIG_8 to DIG_15).
	{
		SEV_SEG_LOAD_LOW;			//Drop the level of the LOAD pin
		SPI_tradeByte(address);		//Send the register address where the data will be stored
		SPI_tradeByte(data);		//Send the data to be stored
		SPI_tradeByte(0);			//Roll no-op data through to push real data out of driver A and into driver B (address byte)
		SPI_tradeByte(0);			//Roll no-op data through to push real data out of driver A and into driver B (data byte)
		SEV_SEG_LOAD_HIGH;			//Raise the level of the LOAD pin - this triggers latching of the sent bytes (last 16 bits of data are latched).
	}
	else				//Not set, therefore address is for driver A (DIG_0 to DIG_7).
	{
		SEV_SEG_LOAD_LOW;			//Drop the level of the LOAD pin
		SPI_tradeByte(0);			//Roll no-op data in first for driver B when real data for driver A pushes it through (address byte)
		SPI_tradeByte(0);			//Roll no-op data in first for driver B when real data for driver A pushes it through (data byte)
		SPI_tradeByte(address);		//Send the register address where the data will be stored
		SPI_tradeByte(data);		//Send the data to be stored
		SEV_SEG_LOAD_HIGH;			//Raise the level of the LOAD pin - this triggers latching of the sent bytes (last 16 bits of data are latched).
	}
}

//Initialise both the display drivers.
void init_SEV_SEG(void)
{
	SPI_DDR |= (1 << SEV_SEG_LOAD); 					//Set LOAD pin as an output
	PORTB |= (1 << SEV_SEG_LOAD);						//Set LOAD pin to high at start (data latching occurs on LOAD rising edge).
	
	SEV_SEG_writeByte(SEV_SEG_SCAN_LIMIT_A, 7);			//Set number of digits in the display to 8 (Data=number of digits - 1)
	SEV_SEG_writeByte(SEV_SEG_INTENSITY_A, 0x08);		//Set brightness (duty cycle) to about half-way.
	SEV_SEG_writeByte(SEV_SEG_SHUTDOWN_A, 1);			//Enter normal operation (exit shutdown mode).
    SEV_SEG_writeByte(SEV_SEG_DECODE_MODE_A, 0xFF);		//Set all digits to be set by Code B data input.
	SEV_SEG_writeByte(SEV_SEG_DISPLAY_TEST_A, 0x00);	//Ensures display test mode is set to normal (sometimes set to test mode on re-programme).
	
	SEV_SEG_writeByte(SEV_SEG_SCAN_LIMIT_B, 7);			//Set number of digits in the display to 8 (Data=number of digits - 1)
	SEV_SEG_writeByte(SEV_SEG_INTENSITY_B, 0x08);		//Set brightness (duty cycle) to about half-way.
	SEV_SEG_writeByte(SEV_SEG_SHUTDOWN_B, 1);			//Enter normal operation (exit shutdown mode).
    SEV_SEG_writeByte(SEV_SEG_DECODE_MODE_B, 0xFF);		//Set all digits to be set by Code B data input.
	SEV_SEG_writeByte(SEV_SEG_DISPLAY_TEST_B, 0x00);	//Ensures display test mode is set to normal (sometimes set to test mode on re-programme).
}

//Clears all digits.  Bypasses function "SEV_SEG_writeByte" and clears equivalent digits on both drivers simultaneously (~halves clear time).
void SEV_SEG_allClear(void)
{
	uint8_t i;
	for (i=8; i>0; i--)	//Counts down through the digits from digit 7 (8) to digit 0 (1).
	{
		SEV_SEG_LOAD_LOW;						//Drop the level of the LOAD pin
		SPI_tradeByte(i);						//Push in digit address i (will be in driver B at latch)
		SPI_tradeByte(SEV_SEG_CODEB_BLANK);		//Push in data to clear digit at i (will be in driver B at latch)
		SPI_tradeByte(i);						//Push in digit address i (will be in driver A at latch)
		SPI_tradeByte(SEV_SEG_CODEB_BLANK);		//Push in data to clear digit at i (will be in driver A at latch)
		SEV_SEG_LOAD_HIGH;						//Raise the level of the LOAD pin - this triggers latching of the sent bytes (last 16 bits of data in driver are latched).
	}
}

//Simple startup animation scans the decimal point (DP) right to left then back a few times.
void SEV_SEG_startupAni(void)
{
	SEV_SEG_allClear();						//Start by clearing all digits.
	uint8_t i = 3;							//Animation repeats 3 times.
	while(i)								//The two while loops using j write the DP, pause, clear the DP then move to the next digit from left to right.
	{
		uint8_t j = SEV_SEG_DIGIT_15;		//Loop for driver B (DIG_15 to DIG_8).
		do
		{
			SEV_SEG_writeByte(j,0x8F);		//Clear the digit except turn on the DP
			_delay_ms(50);					//Pause for 50 milliseconds.
			SEV_SEG_writeByte(j,0x0F);		//Clear the digit inc. the DP
			j--;
		}while(j>=SEV_SEG_DIGIT_8);
		
		j = SEV_SEG_DIGIT_7;				//Loop for driver A (DIG_7 to DIG_0).
		do
		{
			SEV_SEG_writeByte(j,0x8F);		//Clear the digit except turn on the DP
			_delay_ms(50);					//Pause for 50 milliseconds.
			SEV_SEG_writeByte(j,0x0F);		//Clear the digit inc. the DP
			j--;
		}while(j>=SEV_SEG_DIGIT_0);
		i--;
	}
	
	//Following block prints "ISO-8601" to the seven-segment displays for 2 seconds.
	SEV_SEG_writeByte(SEV_SEG_DIGIT_11, 1);					//I
	SEV_SEG_writeByte(SEV_SEG_DIGIT_10, 5);					//S
	SEV_SEG_writeByte(SEV_SEG_DIGIT_9, 0);					//O
	SEV_SEG_writeByte(SEV_SEG_DIGIT_8, SEV_SEG_CODEB_DASH);	//-
	SEV_SEG_writeByte(SEV_SEG_DIGIT_7, 8);					//8
	SEV_SEG_writeByte(SEV_SEG_DIGIT_6, 6);					//6
	SEV_SEG_writeByte(SEV_SEG_DIGIT_5, 0);					//0
	SEV_SEG_writeByte(SEV_SEG_DIGIT_4, 1);					//1
	_delay_ms(2000);										//Wait 2 seconds
	SEV_SEG_allClear();										//Clear all 7-seg digits.
}