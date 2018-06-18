
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>	//Used to utilise interrups, defines sei/90; to enable global interrupts

#include <USART.h>			//Used for serial communications via USART 0
#include <SPI.h>			//Used for serial communications via SPI
#include <DS3234RTC.h>		//Used for SPI comms with DS3234 RTC (Real Time Clock) module
#include <MAX7219.h>

#define BCD_TO_INT(X) ((X) & 0x0F) + (10*((X) >> 4))			//Short cut operation to convert binary-coded decimal to an integer.
#define INT_TO_BCD(X) ((X) - ((X)/10)*10) | (((X) / 10) << 4)	//Short cut operation to convert integer to a binary-coded decimal.

//Set polarity and phase for SPI communications.  Fortunately RTC and Seven Seg display driver both work in mode 3 so do not have to switch between modes, only have to initialise SPI once..
#define	SPI_POL	1	//Define the polarity setting when initialising SPI
#define SPI_PHA	1	//Define the phase setting when initialising SPI

#define DEBOUNCE_DURATION 1000	//Used in external interrupt routines for buttons at INT0 and INT1.  In microseconts.

ISR(USART_RX_vect)	//Interrupt subroutine triggered when the USART receives a byte.
{				
	
	transmitByte(receiveByte());	//Echos received byte.

}



volatile uint8_t tick;

	volatile uint8_t time[SIZE_OF_TIME_ARRAY];		//Initialise array "time" which shall include all the data pulled from the RTC.  Bytes will be binary-coded deciaml (BCD) and in the following format:
											//time[0]: Seconds,	0  to 59
											//time[1]: Minutes,	0  to 59
											//time[2]: Hours,	0  to 23
											//time[4]: Date,	1  to 31
											//time[5]: Month,	1  to 12
											//time[6]: Year,	0  to 99
											//time[7]: Century,	19 to 20

void refresh_display(void)
{
	//SEV_SEG_writeByte(SEV_SEG_DIGIT_15, SEV_SEG_CODEB_DASH);
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
	SEV_SEG_writeByte(SEV_SEG_DIGIT_0, tick);
}




int main(void) {

	//CLKPR: Clock Prescale Register.  Pre-scale the system clock.  The following setup will disable prescaling so the system clock will be the full 8MHz internal frequency.
	CLKPR = (1 << CLKPCE);		//CLKPCE: Clock Prescaler Change Enable.  Must set to one to enable changes to clock prescaler bits.
	CLKPR = 0;					//Once CLKPCE is enabled, Clock Prescaler Select Bits can be changed if done within 4 cycles.  In this case, clear all for prescaler value 1 (i.e. 8MHz system).
	
	//sei();								//Global enable interrupts (from avr/interrupt.h)
	initUSART();							//Initialises the USART for receive/transmit data, 8-bit with 1 stop bit.
	printString("\r\n\n\nISO 8601\r\n");	//Initialisation test for USART.
	initSPI(SPI_POL, SPI_PHA);				//Initialise SPI for RTC - mode 3 (compatible mode 1 or 3) - Polarity=0, Phase=1 and slave select on PB2
	init_RTC();								//Initialise for comms with DS3234 real-time clock.
	init_SEV_SEG();							//Initialise for comms with 2x MAX7219 8x7-seg LED drivers.
	SEV_SEG_startupAni();					//Initialisation test for the 16 7-seg LED display.
	SEV_SEG_allClear();						//Clear all 7-seg digits.
	
	DDRD &= ~(1 << PD2);	//DDRD: Data Direction Register for PORTD.
							//PD2: Clear PD2 to make it an input.  By default all pins are inputs at start-up so this isn't exactly neccessary.
	PORTD |= (1 << PD2);	//'Set' PD2 to activate the internal pull-up resistor.
/*	EIMSK |= (1 << INT0);	//EIMSK: External Interrupt Mask Register
							//INT0: Set to enable External Interrupt 0 (On pin 4/PD2)
	EICRA |= (1 << ISC01);	//EICRA: External Interrupt Control Register A
							//ISC01:0: Interrupt Sense Control 0.  Set to 0b10 so INT0 ISR is triggered on a falling edge (when button is pressed/grounded). 
	sei();*/

//PCICR |= (1 << PCIE2);
//PCMSK2 |= (1 << PCINT18);
sei();					//Global enable interrupts (from avr/interrupt.h)

	//The Following commands can be un-commented to manually set the RTC time on start-up.
	//RTC_SPI_writeByte(RTC_SECR_WA, INT_TO_BCD(30));		//Set Seconds
	//RTC_SPI_writeByte(RTC_MINR_WA, INT_TO_BCD(44));		//Set minutes
	//RTC_SPI_writeByte(RTC_HRR_WA, INT_TO_BCD(01));		//Set hours
	//RTC_SPI_writeByte(RTC_DAYR_WA, INT_TO_BCD(7));		//Set Day
	//RTC_SPI_writeByte(RTC_DATER_WA, INT_TO_BCD(29));		//Set Date
	//RTC_SPI_writeByte(RTC_MCR_WA, 0b10010001);			//Set Month/Century
	//RTC_SPI_writeByte(RTC_YRR_WA, INT_TO_BCD(14));		//Set Year

	


	while (1)
	{	

		RTC_getTime(&time[0]);	//Refresh time data.  Pass to the function the address of the first element.
		

		//Following block prints the dime and date data pulled from the time array in ISO 8601 format via the USART.
		printString("\r");
		printByte(BCD_TO_INT(time[CEN]));
		printByte(BCD_TO_INT(time[YEA]));
		printString("-");
		printByte(BCD_TO_INT(time[MON]));
		printString("-");
		printByte(BCD_TO_INT(time[DAT]));
		printString(" ");
		printByte(BCD_TO_INT(time[HOU]));
		printString(":");
		printByte(BCD_TO_INT(time[MIN]));
		printString(":");
		printByte(BCD_TO_INT(time[SEC]));
		printString(" ");
		printByte(tick);
		printString(" ");


refresh_display();
	
	if(bit_is_clear(PIND, PD2))
	{
		_delay_ms(10);
		if(bit_is_clear(PIND, PD2))
		{

			tick++;
			while(bit_is_clear(PIND, PD2))
			{
			RTC_getTime(&time[0]);
				refresh_display();
			}
		}
	}
	
	}	


	
								/* End event loop */
	return(0);
}

