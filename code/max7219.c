//Functions to control two cascaded MAX7219 8x7-segment display drivers (total 16x 70segment digits).
#include <max7219.h>

//Writes a byte to an address in both of the MAX7219s.  Note, one driver will receive data, the other will receive no-op command.
void sev_seg_write_byte(uint8_t address, uint8_t data)
{
	if(address & 0x80)		//Check driver flag.  If set, address is for driver B (DIG_8 to DIG_15).
	{
		SEV_SEG_LOAD_LOW;		//Drop the level of the LOAD pin
		spi_trade_byte(address);	//Send the register address where the data will be stored
		spi_trade_byte(data);		//Send the data to be stored
		spi_trade_byte(0);		//Roll no-op data through to push real data out of driver A and into driver B (address byte)
		spi_trade_byte(0);		//Roll no-op data through to push real data out of driver A and into driver B (data byte)
		SEV_SEG_LOAD_HIGH;		//Raise the level of the LOAD pin - this triggers latching of the sent bytes (last 16 bits of data are latched).
	}
	else					//Not set, therefore address is for driver A (DIG_0 to DIG_7).
	{
		SEV_SEG_LOAD_LOW;		//Drop the level of the LOAD pin
		spi_trade_byte(0);		//Roll no-op data in first for driver B when real data for driver A pushes it through (address byte)
		spi_trade_byte(0);		//Roll no-op data in first for driver B when real data for driver A pushes it through (data byte)
		spi_trade_byte(address);	//Send the register address where the data will be stored
		spi_trade_byte(data);		//Send the data to be stored
		SEV_SEG_LOAD_HIGH;		//Raise the level of the LOAD pin - this triggers latching of the sent bytes (last 16 bits of data are latched).
	}
}

//Initialise both the display drivers.
void sev_seg_init(void)
{
	SPI_DDR |= (1 << SEV_SEG_LOAD); 			//Set LOAD pin as an output
	SEV_SEG_PORT |= (1 << SEV_SEG_LOAD);			//Set LOAD pin to high at start (data latching occurs on LOAD rising edge).

	//First setup max7219 driver A (digits 0-7)
	sev_seg_write_byte(SEV_SEG_SCAN_LIMIT_A, 7);		//Set number of digits in the display to 8 (Data=number of digits - 1)
	sev_seg_write_byte(SEV_SEG_INTENSITY_A, 0x08);		//Set brightness (duty cycle) to about half-way.
	sev_seg_write_byte(SEV_SEG_SHUTDOWN_A, 1);		//Enter normal operation (exit shutdown mode).
	sev_seg_write_byte(SEV_SEG_DECODE_MODE_A, 0xFF);	//Set all digits to be set by Code B data input.
	sev_seg_write_byte(SEV_SEG_DISPLAY_TEST_A, 0x00);	//Ensures display test mode is set to normal (sometimes set to test mode on re-programme).

	//Second setup max7219 driver B (digits 8-15)
	sev_seg_write_byte(SEV_SEG_SCAN_LIMIT_B, 7);		//Set number of digits in the display to 8 (Data=number of digits - 1)
	sev_seg_write_byte(SEV_SEG_INTENSITY_B, 0x08);		//Set brightness (duty cycle) to about half-way.
	sev_seg_write_byte(SEV_SEG_SHUTDOWN_B, 1);		//Enter normal operation (exit shutdown mode).
	sev_seg_write_byte(SEV_SEG_DECODE_MODE_B, 0xFF);	//Set all digits to be set by Code B data input.
	sev_seg_write_byte(SEV_SEG_DISPLAY_TEST_B, 0x00);	//Ensures display test mode is set to normal (sometimes set to test mode on re-programme).
}

//Clears all digits.  Bypasses function "sev_seg_writeByte" and clears equivalent digits on both drivers simultaneously (~halves clear time).
//Note the digits bust be in CODE-B mode.
void sev_seg_all_clear(void)
{
	for (uint8_t i = 8; i > 0; i--)		//Counts down through the digits from digit 7 (8) to digit 0 (1).
	{
		SEV_SEG_LOAD_LOW;				//Drop the level of the LOAD pin.
		spi_trade_byte(i);				//Push in digit address i (will be in driver B at latch).
		spi_trade_byte(SEV_SEG_CODEB_BLANK);		//Push in data to clear digit at i (will be in driver B at latch).
		spi_trade_byte(i);				//Push in digit address i (will be in driver A at latch).
		spi_trade_byte(SEV_SEG_CODEB_BLANK);		//Push in data to clear digit at i (will be in driver A at latch).
		SEV_SEG_LOAD_HIGH;				//Raise the level of the LOAD pin - triggers latching of the sent bytes (last 16 bits latched).
	}
}

//Turns display on or off without changing any other registers.  This is useful to prevent display artifacts when switching between manual and code-b modes.
void sev_seg_power(uint8_t on_or_off)
{
	SEV_SEG_LOAD_LOW;				//Drop the level of the LOAD pin.
	spi_trade_byte(SEV_SEG_SHUTDOWN_B);		//Push in address for shutdown mode (will be in driver B at latch).
	spi_trade_byte(on_or_off);			//Push in data to set shutdown mode : 1=on, 2=off (will be in driver B at latch).
	spi_trade_byte(SEV_SEG_SHUTDOWN_A);		//Push in address for shutdown mode (will be in driver A at latch).
	spi_trade_byte(on_or_off);			//Push in data to set shutdown mode : 1=on, 2=off (will be in driver A at latch).
	SEV_SEG_LOAD_HIGH;				//Raise the level of the LOAD pin - triggers latching of the sent bytes (last 16 bits latched).
}

//Sets all digits on both drivers to decode mode "manual" or "code B".
//At init, both drivers are set to code B, this is considered the default for this application.
//Therefore, when manual is used in a function, the function should reset to code B prior to exit.
void sev_seg_decode_mode(uint8_t decode_mode)
{
	SEV_SEG_LOAD_LOW;				//Drop the level of the LOAD pin.
	spi_trade_byte(SEV_SEG_DECODE_MODE_B);		//Push in address for decode mode (will be in driver B at latch).
	spi_trade_byte(decode_mode);			//Push in data to set decode mode : 0x00=all manual, OxFF=all Code B (will be in driver B at latch).
	spi_trade_byte(SEV_SEG_DECODE_MODE_A);		//Push in address for decode mode (will be in driver A at latch).
	spi_trade_byte(decode_mode);			//Push in data to set decode mode : 0x00=all manual, OxFF=all Code B (will be in driver A at latch).
	SEV_SEG_LOAD_HIGH;				//Raise the level of the LOAD pin - triggers latching of the sent bytes (last 16 bits latched).
}

//Takse any 64-bit integer and displays the decimal value using the 16 7-seg digits.  Least-significant digit will be displayed to the far right (digit 15).
void sev_seg_display_int(uint64_t num)
{
	int i = SEV_SEG_DIGIT_15;			//First digit (least-sig) will be displayed on 7-seg digit 15 (far right).
	while(num>0)					//Loop until the 64-bit integer has been divided to zero.
	{
		sev_seg_write_byte(i, (num % 10));	//Write the digit (num modulus 10 gives remainder i.e. 'ones' of the integer)
		num /= 10;				//Divide num by 10 so that next iteration of loop will determine the next digit.  I.e. remove LS digit.
		i--;					//Decrement the 7-seg display digit address so that next iteration displays digit to the left.
		if(i == SEV_SEG_NO_OP_B)		//Decrement from address of dig 8 (driver B) requires resetting the address to dig 7 (driver A).
		{
			i = SEV_SEG_DIGIT_7;
		}
	}
}
