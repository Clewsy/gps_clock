#include <avr/io.h>		//Defines standard pins, ports, etc
#include <avr/power.h>
#include "usart.h"
#include "spi.h"
#include "max7219.h"

void hardware_init(void)
{
	clock_prescale_set(clock_div_1);	//needs #include <avr/power.h>, prescaler 1 gives clock speed 8MHz
	power_adc_disable();			//Since it's not needed, disable the ADC and save a bit of power

	usart_init();				//Initialise the USART to enable serial communications.
	spi_init(SEV_SEG_POL, SEV_SEG_PHA);	//Initialise the SPI to enable comms with SPI devices.
	sev_seg_init();
}

int main(void)
{
	hardware_init();	//initialise the hardware peripherals

	usart_print_string("clock(get_time);");
	usart_transmit_byte(0x0A);
	usart_transmit_byte(0x0D);

	sev_seg_startupAni();
//	sev_seg_startupAni();

	uint64_t i = 0;
	while (1)
	{
		//Everything is interrupt driven.
		sev_seg_display_int(i);
		i++;
	}

	return 0;	//Never reached.
}
