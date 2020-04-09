/*******Functions used for communications with DS3234 RC device**********/

#include <avr/io.h>
#include <DS3234RTC.h>
#include <SPI.h>

void init_RTC (void)
{
	SPI_DDR |= (1 << RTC_SS);	//Set slave select pin on uMC as an output.
	PORTB |= (1 << RTC_SS);	//Start SS off not selected (i.e. high as SS is inverted)
}

uint8_t RTC_SPI_readByte(uint8_t address) //Reads and returns a byte at the desired address
{
	SLAVE_SELECT_RTC;			//Enable the RTC comms
	SPI_tradeByte(address);		//Send the address to be read from
	SPI_tradeByte(0);			//Send dummy byte to load SPDR with byte at address
	SLAVE_DESELECT_RTC;			//Disable the RTC comms
	return(SPDR);				//Return the byte.  SPDR=SPI Data Register
}


void RTC_SPI_writeByte(uint8_t address, uint8_t data) //Writes a byte to the desired address
{
	SLAVE_SELECT_RTC;			//Enable the RTC comms
	SPI_tradeByte(address);		//Send the address to be written to
	SPI_tradeByte(data);		//Send the data to be written to address
	SLAVE_DESELECT_RTC;			//Disable the RTC comms
}

void RTC_getTime (uint8_t *time)	//Fill in all structure fields from data in the RTC.  All bytes are BCD representations of data.
{
	time[SEC] = (RTC_SPI_readByte(RTC_SECR_RA));								//B7=0, B6-B4=(10 seconds), B3-B0=(seconds)
	time[MIN] = (RTC_SPI_readByte(RTC_MINR_RA));								//B7=0, B6-B4=(10 minutes), B3-B0=(minutes)
	time[HOU] = ((RTC_SPI_readByte(RTC_HRR_RA)) & 0b00111111);					//B7-B6=0, B5=(20 hours), B4=(10 hours), B3-B0=(hours)
	time[DAT] = (RTC_SPI_readByte(RTC_DATER_RA));									//B7-B6=0, B5-B4=(10 date), B3-B0=(date)
	time[MON] = ((RTC_SPI_readByte(RTC_MCR_RA)) & 0b00011111);					//B7-B5=0, B4=(10 month), B3-B0=(month)
	time[YEA] = (RTC_SPI_readByte(RTC_YRR_RA));									//B7-B4=(10 year), B3-B0=(year)
	if(RTC_SPI_readByte(RTC_MCR_RA) & 0b10000000)									//B7= Century flag. Note, century is either 19 or 20.  I.e. if bit is clear, year is 19XX.
	{
		time[CEN] = 0b00100000;	//BCD 20		
	}
	else
	{

		time[CEN] = 0b00011001; //BCD 19
	}
					
}