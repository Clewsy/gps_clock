


#include <GPSClock_Support.h>
#include <USART.h>			//Used for serial communications via USART 0
#include <SPI.h>			//Used for serial communications via SPI


#define BCD_TO_INT(X) ((X) & 0x0F) + (10*((X) >> 4))			//Short cut operation to convert binary-coded decimal to an integer.
#define INT_TO_BCD(X) ((X) - ((X)/10)*10) | (((X) / 10) << 4)	//Short cut operation to convert integer to a binary-coded decimal.




ISR(USART_RX_vect)	//Interrupt subroutine triggered when the USART receives a byte.
{				
	
	transmitByte(receiveByte());	//Echos received byte.

}



int main(void) {


	initClock();
	initUSART();							//Initialises the USART for receive/transmit data, 8-bit with 1 stop bit.
	printString("\r\n\n\nISO 8601\r\n");	//Initialisation test for USART.
	initSPI(SPI_POL, SPI_PHA);				//Initialise SPI for RTC - mode 3 (compatible mode 1 or 3) - Polarity=0, Phase=1 and slave select on PB2
	init_RTC();								//Initialise for comms with DS3234 real-time clock.
	init_SEV_SEG();							//Initialise for comms with 2x MAX7219 8x7-seg LED drivers.
	SEV_SEG_startupAni();					//Initialisation test for the 16 7-seg LED display.
	init_buttons();							//Initialise pins connected to two control buttons.

	sei();									//Global enable interrupts (from avr/interrupt.h)

	//The Following commands can be un-commented to manually set the RTC time on start-up.
	//RTC_SPI_writeByte(RTC_SECR_WA, INT_TO_BCD(30));		//Set Seconds
	//RTC_SPI_writeByte(RTC_MINR_WA, INT_TO_BCD(18));		//Set minutes
	//RTC_SPI_writeByte(RTC_HRR_WA, INT_TO_BCD(23));		//Set hours
	//RTC_SPI_writeByte(RTC_DAYR_WA, INT_TO_BCD(1));		//Set Day
	//RTC_SPI_writeByte(RTC_DATER_WA, INT_TO_BCD(30));		//Set Date
	//RTC_SPI_writeByte(RTC_MCR_WA, 0b10010001);			//Set Month/Century
	//RTC_SPI_writeByte(RTC_YRR_WA, INT_TO_BCD(14));		//Set Year

	
	uint8_t time[SIZE_OF_TIME_ARRAY];		//Initialise array "time" which shall include all the data pulled from the RTC.  Bytes will be binary-coded deciaml (BCD) and in the following format:
											//time[0]: Seconds,	0  to 59
											//time[1]: Minutes,	0  to 59
											//time[2]: Hours,	0  to 23
											//time[4]: Date,	1  to 31
											//time[5]: Month,	1  to 12
											//time[6]: Year,	0  to 99
											//time[7]: Century,	19 to 20
	
	uint8_t mode = MODE_NO_DP;				//Initialise "mode" setting.  Modes to be cycled via button press.

	while (1)
	{	

		RTC_getTime(time);	//Refresh time data.  Pass to the function the address of the first element.
		

		//Following block prints the time and date data pulled from the time array in ISO 8601 format via the USART.
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


		refresh_display(time, mode);
		
		if(!(BUTTON_PINS & (1 << MODE_BUTTON)))		//If the mode button is pressed
		{
			_delay_us(DEBOUNCE_DURATION);					//Brief wait to de-bounce.
			if(!(BUTTON_PINS & (1 << MODE_BUTTON)))	//Re-check mode button is still pressed.
			{
				mode++;									//Cycle to next mode.
				if(mode==NUMBER_OF_MODES)				//If last mode is passed...
				{
					mode = 0;							//Rollover to first mode.
				}
				while(!(BUTTON_PINS & (1 << MODE_BUTTON)))
				{
					RTC_getTime(time);					//While button is pressed keep refreshing display so mode isn't continuously incremented.
					refresh_display(time, mode);
				}
			}
		}
		
	}	
	return(0);
}

