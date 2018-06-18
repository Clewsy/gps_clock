
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
	
	//sei();			//Global enable interrupts (from avr/interrupt.h)
	initUSART();	//Initialises the USART for receive/transmit data, 8-bit with 1 stop bit.
	printString("\r\nGame over man.\r\n\n");

	initSPI(SPI_POL, SPI_PHA);	//Initialise SPI for RTC - mode 3 (compatible mode 1 or 3) - Polarity=0, Phase=1 and slave select on PB2

	init_RTC();

	RTC_SPI_writeByte(RTC_SECR_WA, INT_TO_BCD(55)); //Set Seconds
	RTC_SPI_writeByte(RTC_MINR_WA, INT_TO_BCD(59)); //Set minutes
	RTC_SPI_writeByte(RTC_HRR_WA, INT_TO_BCD(12)); //Set hours

	//initSPI(SEV_SEG_LOAD, SEV_SEG_POL, SEV_SEG_PHA); //Initialise SPI settings for use with MAX7219 seven segment displays driver.
	init_SEV_SEG();
	SEV_SEG_startupAni();
	SEV_SEG_allClear();
	

	
	
	struct RTC_Data RTC_time;
	
	while (1)
	{	

		RTC_getTime(&RTC_time);	//Refresh time data

		printString("\r");
		printByte(BCD_TO_INT(RTC_time.hours));
		printString(":");
		printByte(BCD_TO_INT(RTC_time.minutes));
		printString(":");
		printByte(BCD_TO_INT(RTC_time.seconds));
		printString(" ");





	SEV_SEG_writeByte(SEV_SEG_DIGIT_7, ((RTC_time.day) >> 4));
	SEV_SEG_writeByte(SEV_SEG_DIGIT_6, ((RTC_time.day) & 0b00001111));
	SEV_SEG_writeByte(SEV_SEG_DIGIT_5, ((RTC_time.hours) >> 4));
	SEV_SEG_writeByte(SEV_SEG_DIGIT_4, ((RTC_time.hours) & 0b00001111));
	
	SEV_SEG_writeByte(SEV_SEG_DIGIT_3, ((RTC_time.minutes) >> 4));
	SEV_SEG_writeByte(SEV_SEG_DIGIT_2, ((RTC_time.minutes) & 0b00001111));
	SEV_SEG_writeByte(SEV_SEG_DIGIT_1, ((RTC_time.seconds) >> 4));
	SEV_SEG_writeByte(SEV_SEG_DIGIT_0, ((RTC_time.seconds) & 0b00001111));
	//_delay_ms(500);
	//SEV_SEG_allClear();
	_delay_ms(50);





	}
								/* End event loop */
	return(0);
}

