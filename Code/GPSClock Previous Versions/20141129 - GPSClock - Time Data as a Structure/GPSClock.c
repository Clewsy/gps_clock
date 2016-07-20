
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

ISR(USART_RX_vect)	{				//Interrupt subroutine triggered when the USART receives a byte.
	
	transmitByte(receiveByte());	//Echos received byte.

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
	


	//The Following commands can be un-commented to manually set the RTC time on start-up.
	//RTC_SPI_writeByte(RTC_SECR_WA, INT_TO_BCD(30));		//Set Seconds
	//RTC_SPI_writeByte(RTC_MINR_WA, INT_TO_BCD(44));		//Set minutes
	//RTC_SPI_writeByte(RTC_HRR_WA, INT_TO_BCD(01));		//Set hours
	//RTC_SPI_writeByte(RTC_DAYR_WA, INT_TO_BCD(7));		//Set Day
	//RTC_SPI_writeByte(RTC_DATER_WA, INT_TO_BCD(29));		//Set Date
	//RTC_SPI_writeByte(RTC_MCR_WA, 0b10010001);			//Set Month/Century
	//RTC_SPI_writeByte(RTC_YRR_WA, INT_TO_BCD(14));		//Set Year

	
	struct RTC_Data RTC_time;	//Define a structure callede "RTC_time" that will contain all the used date and time data retrieved from the RTC.

	while (1)
	{	

		RTC_getTime(&RTC_time);	//Refresh time data
		

		printString("\r");
		printByte(BCD_TO_INT(RTC_time.century));
		printByte(BCD_TO_INT(RTC_time.year));
		printString("-");
		printByte(BCD_TO_INT(RTC_time.month));
		printString("-");
		printByte(BCD_TO_INT(RTC_time.date));
		printString(" ");
		printByte(BCD_TO_INT(RTC_time.hours));
		printString(":");
		printByte(BCD_TO_INT(RTC_time.minutes));
		printString(":");
		printByte(BCD_TO_INT(RTC_time.seconds));
		printString(" ");



	//SEV_SEG_writeByte(SEV_SEG_DIGIT_15, SEV_SEG_CODEB_DASH);
	SEV_SEG_writeByte(SEV_SEG_DIGIT_14, ((RTC_time.century) >> 4));
	SEV_SEG_writeByte(SEV_SEG_DIGIT_13, ((RTC_time.century) & 0b00001111));
	SEV_SEG_writeByte(SEV_SEG_DIGIT_12, ((RTC_time.year) >> 4));
	SEV_SEG_writeByte(SEV_SEG_DIGIT_11, ((RTC_time.year) & 0b00001111) | SEV_SEG_DP);
	SEV_SEG_writeByte(SEV_SEG_DIGIT_10, ((RTC_time.month) >> 4));
	SEV_SEG_writeByte(SEV_SEG_DIGIT_9, ((RTC_time.month) & 0b00001111) | SEV_SEG_DP);
	SEV_SEG_writeByte(SEV_SEG_DIGIT_8, ((RTC_time.date) >> 4));
	SEV_SEG_writeByte(SEV_SEG_DIGIT_7, ((RTC_time.date) & 0b00001111) | SEV_SEG_DP);
	SEV_SEG_writeByte(SEV_SEG_DIGIT_6, ((RTC_time.hours) >> 4));
	SEV_SEG_writeByte(SEV_SEG_DIGIT_5, ((RTC_time.hours) & 0b00001111) | SEV_SEG_DP);
	SEV_SEG_writeByte(SEV_SEG_DIGIT_4, ((RTC_time.minutes) >> 4));
	SEV_SEG_writeByte(SEV_SEG_DIGIT_3, ((RTC_time.minutes) & 0b00001111) | SEV_SEG_DP);
	SEV_SEG_writeByte(SEV_SEG_DIGIT_2, ((RTC_time.seconds) >> 4));
	SEV_SEG_writeByte(SEV_SEG_DIGIT_1, ((RTC_time.seconds) & 0b00001111));
	//SEV_SEG_writeByte(SEV_SEG_DIGIT_0, SEV_SEG_CODEB_DASH);
	
	}	


	
								/* End event loop */
	return(0);
}

