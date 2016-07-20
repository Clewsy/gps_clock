
//Definitions and declarations for the GPS clock support functions.

#include <avr/io.h>			//From standard AVR libraries - used for calling standard registers etc.
#include <util/delay.h>		//From the standard AVR libraries - used to call delay_ms() and delay_us() functions.
#include <avr/eeprom.h>		//EEPROM used to store timezone/UTC-offset to survive power cycle.
#include <avr/interrupt.h>	//Used to utilise interrups, defines sei/90; to enable global interrupts

#define BAUD  4800			//This is the speed for the USART serial comms.  Set to 4800 for compatibility with GPS default speed.
#include <USART.h>			//Used for serial communications via USART 0
#include <SPI.h>			//Used for serial communications via SPI

#include <DS3234RTC.h>		//Used for SPI comms with DS3234 RTC (Real Time Clock) module
#include <MAX7219.h>		//Used for SPI comms with MAX7219 8x7-segment display controllers.

//Set polarity and phase for SPI communications.  Fortunately RTC and Seven Seg display driver both work in mode 3 so do not have to switch between modes, only have to initialise SPI once..
#define	SPI_POL	1	//Define the polarity setting when initialising SPI
#define SPI_PHA	1	//Define the phase setting when initialising SPI

#define NUMBER_OF_MODES			5	//Used to enable rollover when cycling through modes.
#define MODE_NO_DP				0	//Display mode ISO 8601 centered, no DPs.
#define MODE_DP					1	//Display mode ISO 8601 centered, with DPs.
#define MODE_CENTER_GAP_NO_DP	2	//Date to the left, time to the right, no DPs.
#define MODE_CENTER_GAP_DP		3	//Date to the left, time to the right, with DPs.
#define MODE_ADJUST_OFFSET		4	//Adjust the time offset from UTC - Cycle (using sync button) from -11:30 to +12:00 in 30min intervals.

#define DDR_BUTTONS		DDRD	//Define which data direction register is used for the two operational control buttons.
#define PORT_BUTTONS	PORTD	//Define which port the two operational control buttons are connected to.
#define BUTTON_PINS		PIND	//Define the PINS (input port) to which the buttons are connected.
#define MODE_BUTTON		PD2		//Define which input is connected to the mode button
#define SYNC_BUTTON		PD3		//Define which input is connected to the sync button

#define DEBOUNCE_DURATION 2000	//Used in external interrupt routines for buttons at INT0 and INT1.  In microseconds.

//Address in eeprom for storing the signed integer representing the UTC offset.
#define OFFSET_EEPROM_ADDRESS (uint8_t *) 0	//Note "(uint8_t *)" is required to typecast to a pointer to an integer as that is what function eeprom_read_byte() expects.
//Without typecast, warning will be generated for any address of non-zero value.

#define BCD_TO_INT(X) ((X) & 0x0F) + (10*((X) >> 4))			//Short cut operation to convert binary-coded decimal to an integer.
#define INT_TO_BCD(X) ((X) - ((X)/10)*10) | (((X) / 10) << 4)	//Short cut operation to convert integer to a binary-coded decimal.

//Main clock operational function declarations
void initClock(void);									//Initialises the system clock
void init_buttons(void);								//Initialises the two input control buttons.
void init_eeprom_offset(void);							//Checks the UTC offset value stored in eeprom is valid.  If not, sets it to 0.
void sync_display (uint8_t sec);						//Refreshes the display while the clock is "syncing" the time from the GPS unit.
void GPS_getTime(uint8_t *time);						//Used to obtain the time and date from the GPS unit over the UART.
void get_utc_offset(void);								//Obtain and programme (to eeprom) the UTC offset.
void utc_offset_adjust(uint8_t *time, int8_t offset);	//Adjusts the time/date in accordance with the programmed offset.
void rollover_hour(uint8_t *time);						//Increment the date if required by the offset from the UTC.
void rollover_date(uint8_t *time);						//Increment the date if required by the offset from the UTC.
void rollunder_hour(uint8_t *time);						//Decrement the hour if required by the offset from the UTC.
void rollunder_date(uint8_t *time);						//Decrement the date if required by the offset from the UTC.
void refresh_display(uint8_t *time, uint8_t mode);		//Called to refresh the display in accordance with the current mode.

