//Functions used for serial communications via SPI

#include <spi.h>

//Will initialise SPI hardware as master device and frequency/16 then enable.
void spi_init(uint8_t polarity, uint8_t phase)
{
	//Set data directions (inputs/outputs) and pull-ups as required
	SPI_DDR |= (1 << SPI_MOSI);		//MOSI - Output on MOSI
	SPI_PORT |= (1 << SPI_MISO);	 	//MISO - Left set as an input but pullup activated
	SPI_DDR |= (1 << SPI_SCK);		//SCK - Output on SCK
	SPI_DDR |= (1 << PB2);			//PB2 on AVR is designated SS but any I/O pin can be used, the designation is really for when AVR
						// SPI acts in slave mode.  HOWEVER must always set PB2 to output even if alternate I/O pin is in
						// use, otherwise AVR defers to other uCUs as per multimaster setup.

	//SPCR = SPI Control Register
	SPCR |= (1 << SPR1);	//Div 16, safer for breadboards (Slower SPI frequency to reduce chance of interference)
	SPCR |= (1 << MSTR);	//Clockmaster
	SPCR |= (1 << SPE);	//Enable SPI

	//Set Polarity/Phase Mode as required for the connected SPI device.
	SPCR |= (polarity << CPOL);	//CPOL = SPI Clock Polarity, 0 for Clock Idles Low, 1 for clock idles high
	SPCR |= (phase << CPHA);	//CPHA = SPI Clock Phase, 1 for Data Sampled on Falling Edge, 0 for Rising Edge
}

//The basic function used to send and receive a byte via SPI (as a byte rolls out, one rolls in).
void spi_trade_byte(uint8_t byte)
{
	SPDR = byte; 				//SPI starts sending immediately.  SPDR=SPI Data Register
	while(!(SPSR & (1 << SPIF))) {}		//Wait until SPIF (SPI Interrupt Flag) in the SPSR (SPI Status Register) is set indication transfer is complete
	//SPDR now contains the received byte
}
