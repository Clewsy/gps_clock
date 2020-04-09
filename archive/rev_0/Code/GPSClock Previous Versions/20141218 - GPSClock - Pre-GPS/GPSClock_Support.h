
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>	//Used to utilise interrups, defines sei/90; to enable global interrupts

#include <DS3234RTC.h>		//Used for SPI comms with DS3234 RTC (Real Time Clock) module
#include <MAX7219.h>

//Set polarity and phase for SPI communications.  Fortunately RTC and Seven Seg display driver both work in mode 3 so do not have to switch between modes, only have to initialise SPI once..
#define	SPI_POL	1	//Define the polarity setting when initialising SPI
#define SPI_PHA	1	//Define the phase setting when initialising SPI

#define NUMBER_OF_MODES			4	//Used to enable rollover when cycling through modes.
#define MODE_NO_DP				0	//Display mode ISO 8601 centered, no DPs.
#define MODE_DP					1	//Display mode ISO 8601 centered, with DPs.
#define MODE_CENTER_GAP_NO_DP	2	//Date to the left, time to the right, no DPs.
#define MODE_CENTER_GAP_DP		3	//Date to the left, time to the right, with DPs.

#define DDR_BUTTONS		DDRD
#define PORT_BUTTONS	PORTD
#define BUTTON_PINS		PIND	//Define the PINS (input port) to which the buttons are connected.
#define MODE_BUTTON		PD2		//Define which input is connected to the mode button

#define DEBOUNCE_DURATION 1000	//Used in external interrupt routines for buttons at INT0 and INT1.  In microseconds.

void initClock(void);
void init_buttons(void);
//ISR(USART_RX_vect):	//Interrupt subroutine triggered when the USART receives a byte.
void refresh_display(uint8_t *time, uint8_t mode);

