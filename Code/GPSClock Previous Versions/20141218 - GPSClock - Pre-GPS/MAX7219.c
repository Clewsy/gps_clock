


#include <avr/io.h>
#include <util/delay.h>
#include <SPI.h>
#include <MAX7219.h>

 
void SEV_SEG_writeByte(uint8_t address, uint8_t data)	//Writes a byte to an address in both of the MAX7219s although one driver will receive data, the other will receive no-op command.
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

void init_SEV_SEG(void)
{
	SPI_DDR |= (1 << SEV_SEG_LOAD); 				//Set LOAD pin as an output
	PORTB |= (1 << SEV_SEG_LOAD);					//Set LOAD pin to high at start (data latching occurs on LOAD rising edge).
	SEV_SEG_writeByte(SEV_SEG_SCAN_LIMIT_A, 7);		//Set number of digits in the display to 8 (Data=number of digits - 1)
	SEV_SEG_writeByte(SEV_SEG_INTENSITY_A, 0x08);	//Set brightness (duty cycle) to about half-way.
	SEV_SEG_writeByte(SEV_SEG_SHUTDOWN_A, 1);		//Enter normal operation (exit shutdown mode).
    SEV_SEG_writeByte(SEV_SEG_DECODE_MODE_A, 0xFF);	//Set all digits to be set by Code B data input.
	
	SEV_SEG_writeByte(SEV_SEG_SCAN_LIMIT_B, 7);		//Set number of digits in the display to 8 (Data=number of digits - 1)
	SEV_SEG_writeByte(SEV_SEG_INTENSITY_B, 0x08);	//Set brightness (duty cycle) to about half-way.
	SEV_SEG_writeByte(SEV_SEG_SHUTDOWN_B, 1);		//Enter normal operation (exit shutdown mode).
    SEV_SEG_writeByte(SEV_SEG_DECODE_MODE_B, 0xFF);	//Set all digits to be set by Code B data input.
}

void SEV_SEG_allClear(void)	//Clears all digits.  Bypasses function "SEV_SEG_writeByte" and clears equivalent digits on both drivers simultaneously (~halves clear time).
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

void SEV_SEG_startupAni(void)	//Simple startup animation scans the decimal point (DP) right to left then back a few times.
{
	SEV_SEG_allClear();			//Start by clearing all digits.
	uint8_t i = 3;				//Animation repeats 3 times.
	while(i)					//The two while loops using j write the DP, pause, clear the DP then move to the next digit from left to right.
	{
		uint8_t j = SEV_SEG_DIGIT_15;		//Loop for driver B (DIG_15 to DIG_8).
		do
		{
			SEV_SEG_writeByte(j,0x8F);		//Clear the digit except turn on the DP
			_delay_ms(50);
			SEV_SEG_writeByte(j,0x0F);		//Clear the digit inc. the DP
			j--;
		}while(j>=SEV_SEG_DIGIT_8);
		
		j = SEV_SEG_DIGIT_7;		//Loop for driver A (DIG_7 to DIG_0).
		do
		{
			SEV_SEG_writeByte(j,0x8F);		//Clear the digit except turn on the DP
			_delay_ms(50);
			SEV_SEG_writeByte(j,0x0F);		//Clear the digit inc. the DP
			j--;
		}while(j>=SEV_SEG_DIGIT_0);
		i--;
	}
	//Following block prints "ISO-8601" to the seven-segment displays.
	SEV_SEG_writeByte(SEV_SEG_DIGIT_11, 1);
	SEV_SEG_writeByte(SEV_SEG_DIGIT_10, 5);
	SEV_SEG_writeByte(SEV_SEG_DIGIT_9, 0);
	SEV_SEG_writeByte(SEV_SEG_DIGIT_8, SEV_SEG_CODEB_DASH);
	SEV_SEG_writeByte(SEV_SEG_DIGIT_7, 8);
	SEV_SEG_writeByte(SEV_SEG_DIGIT_6, 6);
	SEV_SEG_writeByte(SEV_SEG_DIGIT_5, 0);
	SEV_SEG_writeByte(SEV_SEG_DIGIT_4, 1);
		
	_delay_ms(2000);
	SEV_SEG_allClear();			//Clear all 7-seg digits.
}

 
/*void MAX7219_displayNumber(volatile long number) 
{
    char negative = 0;
 
    // Convert negative to positive.
    // Keep a record that it was negative so we can
    // sign it again on the display.
    if (number < 0) {
        negative = 1;
        number *= -1;
    }
 
    MAX7219_clearDisplay();
 
    // If number = 0, only show one zero then exit
    if (number == 0) {
        SEV_SEG_writeByte(SEV_SEG_DIGIT_0, 0);
        return;
    }
    
    // Initialization to 0 required in this case,
    // does not work without it. Not sure why.
    char i = 0; 
    
    // Loop until number is 0.
    do {
        SEV_SEG_writeByte(++i, number % 10);
        // Actually divide by 10 now.
        number /= 10;
    } while (number);
 
    // Bear in mind that if you only have three digits, and
    // try to display something like "-256" all that will display
    // will be "256" because it needs an extra fourth digit to
    // display the sign.
    if (negative) {
        SEV_SEG_writeByte(i, MAX7219_CHAR_NEGATIVE);
    }
}*/