/*******Header file used for serial communications via SPI**********/

#define SPI_PORT	PORTB	//Port B utilised for SPI comms
#define SPI_DDR		DDRB	//Data Direction Register B used to set SPI comms input/output pins.
#define SPI_MOSI	PB3		//Pin B3 used for SPI MOSI (Master Out, Slave In)
#define SPI_MISO	PB4		//Pin B3 used for SPI MOSI (Master In, Slave Out)
#define SPI_SCK		PB5		//Pin B3 used for SPI SCK (Synchronous Clock)
//Note, SS Pin not defined to allow compatibility with multiple SPI devices on the bus.  Should be defined in device header.


void initSPI(uint8_t polarity, uint8_t phase);	//Will initioalise SPI hardware as master device and frequency/16 then enable.
void SPI_tradeByte(uint8_t byte);

