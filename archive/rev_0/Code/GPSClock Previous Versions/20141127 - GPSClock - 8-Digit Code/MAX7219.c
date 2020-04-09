


#include <avr/io.h>
#include <util/delay.h>
#include <SPI.h>
#include <MAX7219.h>

 
void SEV_SEG_writeByte(uint8_t address, uint8_t data)	//Writes a byte to an address in the MAX7219
{
	SEV_SEG_LOAD_LOW;			//Drop the level of the LOAD pin
	SPI_tradeByte(0);
	SPI_tradeByte(0);
	SPI_tradeByte(address);		//Send the register address where the data will be stored
	SPI_tradeByte(data);		//Send the data to be stored
	SEV_SEG_LOAD_HIGH;			//Raise the level of the LOAD pin - this triggers latching of the sent bytes (last 16 bits of data are latched).
}

void init_SEV_SEG(void)
{
	SPI_DDR |= (1 << SEV_SEG_LOAD); 				//Set LOAD pin as an output
	PORTB |= (1 << SEV_SEG_LOAD);					//Set LOAD pin to high at start (data latching occurs on LOAD rising edge).
	SEV_SEG_writeByte(SEV_SEG_SCAN_LIMIT, 7);		//Set number of digits in the display to 8 (Data=number of digits - 1)
	SEV_SEG_writeByte(SEV_SEG_INTENSITY, 0x08);		//Set brightness (duty cycle) to about half-way.
	SEV_SEG_writeByte(SEV_SEG_SHUTDOWN, 1);			//Enter normal operation (exit shutdown mode).
    SEV_SEG_writeByte(SEV_SEG_DECODE_MODE, 0xFF);	//Set all digits to be set by Code B data input.
}

void SEV_SEG_allClear(void)
{
	uint8_t i;
	for (i=8; i>0; i--)	//Counts down through the digits from digit 7 (8) to digit 0 (1).
	{
		SEV_SEG_writeByte(i, SEV_SEG_CODEB_BLANK);	//Clears the digit.
	}
}

void SEV_SEG_startupAni(void)	//Simple startup animation scans the decimal point (DP) right to left then back a few times.
{
	SEV_SEG_allClear();			//Start by clearing all digits.
	uint8_t i = 3;				//Animation repeats 3 times.
	while(i)
	{
		uint8_t j=4;		//The two while loops write the DP, pause, clear the DP then move to the next digit in this order: D3,D2,D1,D0,D1,D2, repeat.
		while(j>0)
		{
			SEV_SEG_writeByte(j,0x8F);		//Clear the digit except turn on the DP
			_delay_ms(50);
			SEV_SEG_writeByte(j,0x0F);		//Clear the digit inc. the DP
			j--;
		}
		while(j<2)
		{
			SEV_SEG_writeByte(j+2,0x8F);	//Clear the digit except turn on the DP
			_delay_ms(50);
			SEV_SEG_writeByte(j+2,0x0F);	//Clear the digit inc. the DP
			j++;		
		}
		i--;
	}
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