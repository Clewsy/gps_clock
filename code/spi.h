//Definitions and declarations used for serial communications via SPI

#include <avr/io.h>

#define SPI_PORT	PORTB	//Port B utilised for SPI comms
#define SPI_DDR		DDRB	//Data Direction Register B used to set SPI comms input/output pins.
#define SPI_MOSI	PB3	//Pin B3 used for SPI MOSI (Master Out, Slave In)
#define SPI_MISO	PB4	//Pin B4 used for SPI MOSI (Master In, Slave Out)
#define SPI_SCK		PB5	//Pin B5 used for SPI SCK (Synchronous Clock)
//Note, SS Pin (slave select) not defined to allow compatibility with multiple SPI devices on the bus.  Should be defined in device header.

void spi_init(uint8_t polarity, uint8_t phase);	//Will initialise SPI hardware as master device and frequency/16 then enable.
void spi_tradeByte(uint8_t byte);		//The basic function used to send and receive a byte via SPI
