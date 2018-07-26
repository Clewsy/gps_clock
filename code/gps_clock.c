#include "gps_clock.h"

//Interrupt subroutine triggered whenever the AVR receives a byte on the serial line (USART).  This was used for debugging.
//ISR(USART_RX_vect)	//Interrupt subroutine triggered when the USART receives a byte.
//{
//	usart_transmit_byte(usart_receive_byte());	//Echos received byte.
//}

void hardware_init(void)
{
	clock_prescale_set(clock_div_1);	//needs #include <avr/power.h>, prescaler 1 gives clock speed 8MHz
	power_adc_disable();			//Since it's not needed, disable the ADC and save a bit of power

	usart_init();				//Initialise the USART to enable serial communications.
	spi_init(SEV_SEG_POL, SEV_SEG_PHA);	//Initialise the SPI to enable comms with SPI devices.
						//Note, both spi devices (RTC and max7219) operate in spi mode 3, so spi_init only needs to be called once.

	sev_seg_init();

//	sei();				//Global enable interrups (requires avr/interrupt.h)
}


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
			usart_receive_byte();
			time[HOU] = (10 * (usart_receive_byte() - '0'));	//Hour tens	"- '0'" converts ascii char to the corresponding digit.
			time[HOU] += (usart_receive_byte() - '0');		//Hour ones
			time[MIN] = (10 * (usart_receive_byte() - '0'));	//Minute tens
			time[MIN] += (usart_receive_byte() - '0');		//Minute ones
			time[SEC] = (10 * (usart_receive_byte() - '0'));	//Second tens
			time[SEC] += (usart_receive_byte() - '0');		//Second ones

			for(uint8_t i = 0; i < 8; i++)				//Loop 8 times
			{
				do {}
				while (usart_receive_byte() != ',');		//Skip through 8 bytes (ignored) from the RMC sentence to get to the date data.
			}

			time[DAY] = (10 * (usart_receive_byte() - '0'));	//Date tens
			time[DAY] += (usart_receive_byte() - '0');		//Date ones
			time[MON] = (10 * (usart_receive_byte() - '0'));	//Month tens
			time[MON] += (usart_receive_byte() - '0');		//Month ones
			time[YEA] = (10 * (usart_receive_byte() - '0'));	//Year tens
			time[YEA] += (usart_receive_byte() - '0');		//Year ones

			//We now have the UTC date/time array in the format [YY,MM,DD,HH,MM,SS]
		}
	}

	//debugging: print date to usart
	usart_print_string("yyyymmdd:20");
	usart_print_byte(time[YEA]);
	usart_print_byte(time[MON]);
	usart_print_byte(time[DAY]);

	//debugging: print time to usart
	usart_print_string(" hhmmss:");
	usart_print_byte(time[HOU]);
	usart_print_byte(time[MIN]);
	usart_print_byte(time[SEC]);
	usart_print_string("\r\n");

	//debugging: print date and time to 16-digit seven segment display
	sev_seg_writeByte(SEV_SEG_DIGIT_0, 2);
	sev_seg_writeByte(SEV_SEG_DIGIT_1, 0);
	sev_seg_writeByte(SEV_SEG_DIGIT_2, ((time[YEA] / 10) % 10));
	sev_seg_writeByte(SEV_SEG_DIGIT_3, (time[YEA] % 10) | SEV_SEG_DP);
	sev_seg_writeByte(SEV_SEG_DIGIT_4, ((time[MON] / 10) % 10));
	sev_seg_writeByte(SEV_SEG_DIGIT_5, (time[MON] % 10) | SEV_SEG_DP);
	sev_seg_writeByte(SEV_SEG_DIGIT_6, ((time[DAY] / 10) % 10));
	sev_seg_writeByte(SEV_SEG_DIGIT_7, (time[DAY] % 10) | SEV_SEG_DP);
	//leave digits 8 & 9 blank
	sev_seg_writeByte(SEV_SEG_DIGIT_10, ((time[HOU] / 10) % 10));
	sev_seg_writeByte(SEV_SEG_DIGIT_11, (time[HOU] % 10) | SEV_SEG_DP);
	sev_seg_writeByte(SEV_SEG_DIGIT_12, ((time[MIN] / 10) % 10));
	sev_seg_writeByte(SEV_SEG_DIGIT_13, (time[MIN] % 10) | SEV_SEG_DP);
	sev_seg_writeByte(SEV_SEG_DIGIT_14, ((time[SEC] / 10) % 10));
	sev_seg_writeByte(SEV_SEG_DIGIT_15, (time[SEC] % 10));

}

int main(void)
{
	hardware_init();	//initialise the hardware peripherals

	usart_print_string("clock(get_time);");
	usart_transmit_byte(0x0A);
	usart_transmit_byte(0x0D);

	sev_seg_startupAni();

	while (1)
	{
		sync_time(time);
	}

	return 0;	//Never reached.
}
