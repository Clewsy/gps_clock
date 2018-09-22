#include <avr/io.h>		//Defines standard pins, ports, etc
#include <avr/power.h>		//Required to set the system clock by code (rather than fuse bits).
#include <avr/interrupt.h>	//Requires to use interrupt macros such ase sei();
#include <util/delay.h>		//From the standard AVR libraries - used to call delay_ms() and delay_us() functions.
#include <avr/eeprom.h>		//Required to easil utilise eeprom for variable storage that survives a power-cycle,
#include <stdlib.h>		//Included to utilise abs() function for easily converting a negative value to a positive (absolute value).
#include "usart.h"		//For USART serial communications.
#include "spi.h"		//For SPI communications.
#include "max7219.h"		//For max7219 (sev-seg driver) functions.
#include "ds3234.h"		//For ds3234 (real-time clock) functions.

//True/false used to determine succeful sync of time from GPS.
#define TRUE	1
#define FALSE	0

//Following definitions will be used to initialise and read the buttons.
#define BUTTON_DDR			DDRC		//DDRC: Data Direction Register C - For setting IO to input or output at port C.
#define BUTTON_PORT			PORTC		//PORTC: Port C - For writing to IO.
#define BUTTON_PINS			PINC		//PINC: Pin C - For reading from IO.
#define BUTTON_PCIE			PCIE1		//PCIE1: Pin Change Interrupt Enable 1 - For enabling interrupts on IO level change.
#define BUTTON_PCMSK			PCMSK1		//PCMSK1: Pin Change Mask Register 1 - For masking which IO lines to trigger interrupt.
#define BUTTON_MODE			PC0		//PC0: PCINT8
#define BUTTON_SYNC			PC1		//PC1: PCINT9
#define BUTTON_PCI_VECTOR		PCINT1_vect	//PCINT1: Pin-Change Interrupt 1 - Define the interrupt sub-routine vector function name.
#define BUTTON_DEBOUNCE_DURATION	100		//Define the duration in ms to wait to avoide button "bounce".
#define BUTTONS_ENABLE			PCICR |= (1 << BUTTON_PCIE);	//Enable Pin-Change Interrupt for pin-change int pins PCINT[8-14].
#define BUTTONS_DISABLE			PCICR &= !(1 << BUTTON_PCIE);	//Disable Pin-Change Interrupt for pin-change int pins PCINT[8-14].

//Allocate an address within the AVR's eeprom to store the GMT time offset value so that the offset is retained after a power-cycle.
//Valid value for the offset is a multiple of 5 from -120 to +120 which represents an offset range of -12.0 hrs to +12.0 hours in 0.5 hour increments.
//Note, "(uint8_t *)" is required to typecast an integer pointer as that is what's expected by the function eeprom_read_byte() expects.
//Without this typecast, a warning will be generated for any non-zero address.
#define OFFSET_EEPROM_ADDRESS (uint8_t *) 5	//Arbitrary value, just keep it different to INTENSITY_EEPROM_ADDRESS.

//Allocate an address within the AVR's eeprom to store the intensity (brightness) value so that the selected intensity is retained after a power-cycle.
//Valid value for the intensity is 0 to 15 as per the datasheet.
//Note, "(uint8_t *)" is required to typecast an integer pointer as that is what's expected by the function eeprom_read_byte() expects.
//Without this typecast, a warning will be generated for any non-zero address.
#define INTENSITY_EEPROM_ADDRESS (uint8_t *) 6	//Arbitrary value, just keep it different to OFFSET_EEPROM_ADDRESS.


//Define the display modes
//Mode 1 shows the date on the left and the time on the right with two blank segments between them.
//Mode 2 shows the date and time together with no gap but a blank digit either side of the full date/time.
//Sub-modes A/B determine if the decimal points will be shown between individual components of the date/time.
#define MODE_1A_ISO		0b000	//	|Y Y Y Y M M D D     H H M M S S |	ISO-8601 with date/time separation, no delimiters.
#define MODE_1B_ISO		0b001	//	|Y Y Y Y.M M.D D.    H H.M M.S S.|	ISO-8601 with date/time separation with delimiters (decimal points).
#define MODE_2A_ISO		0b010	//	|  Y Y Y Y M M D D H H M M S S   |	ISO-8601 centered, no delimiters.
#define MODE_2B_ISO		0b011	//	|  Y Y Y Y.M M.D D.H H.M M.S S.  |	ISO-8601 centered with delimiters (decimal points).
#define MODE_3_EPOCH		0b100	//	|E P O C H   S S S S S S S S S S |	UNIX Epoch time (seconds elapsed since midnight, Jan 1st, 1970).
#define MODE_4_OFFSET		0b101	//	|O F F S E t             ± # #.# |	Enable setting of the time offset from UTC.
#define MODE_5_INTENSITY	0b110	//	|I n t E n S I t y           # # |	Enable setting of the time offset from UTC.

//Define array elements for easier identification when pulled from time array.
#define CEN_TENS  0
#define CEN_ONES  1
#define YEA_TENS  2
#define YEA_ONES  3
#define MON_TENS  4
#define MON_ONES  5
#define DAY_TENS  6
#define DAY_ONES  7
#define HOU_TENS  8
#define HOU_ONES  9
#define MIN_TENS 10
#define MIN_ONES 11
#define SEC_TENS 12
#define SEC_ONES 13
#define SIZE_OF_TIME_ARRAY 14

//For readability define integer values determined from BCD values of time elements.
#define SEC (10*time[SEC_TENS] + time[SEC_ONES])
#define MIN (10*time[MIN_TENS] + time[MIN_ONES])
#define HOU (10*time[HOU_TENS] + time[HOU_ONES])
#define DAY (10*time[DAY_TENS] + time[DAY_ONES])
#define MON (10*time[MON_TENS] + time[MON_ONES])
#define YEA (10*time[YEA_TENS] + time[YEA_ONES])

//For readability, define month names in accordance with corrsponding numerical values
#define JAN  1
#define FEB  2
#define MAR  3
#define APR  4
#define MAY  5
#define JUN  6
#define JUL  7
#define AUG  8
#define SEP  9
#define OCT 10
#define NOV 11
#define DEC 12

//These definitions are used to make the calculation of epoch UNIX time much more readable.
//Epoch time (or unix epoch time) is the number of seconds elapsed since 0hrs, January first, 1970.
#define DAYS_IN_4_YEARS		1461		// = ((365days * 4years) + 1leap-day)
#define SECONDS_IN_A_DAY	86400		// = 60seconds * 60minutes * 24hours
#define SECONDS_IN_AN_HOUR	3600		// = 60seconds * 60minutes
#define SECONDS_IN_A_MINUTE	60
#define EPOCH_SECONDS_TO_2000	946684800	// = Seconds elapsed from epoch (midnight, Jan 1st, 1970) until midnight, Jan 1st, 2000.
#define EPOCH_YEAR		(uint32_t) ((10*time[YEA_TENS]) + time[YEA_ONES])	// 0-99		= Years since 2000.
#define EPOCH_MONTH		(uint32_t) ((10*time[MON_TENS]) + time[MON_ONES])	// 1-12		= Months since start of current year.
#define EPOCH_DAY		(uint32_t) ((10*time[DAY_TENS]) + time[DAY_ONES])	// 1-31		= Days since start of current month.
#define EPOCH_HOUR		(uint32_t) ((10*time[HOU_TENS]) + time[HOU_ONES])	// 0-23		= Hours since start of current day.
#define EPOCH_MINUTE		(uint32_t) ((10*time[MIN_TENS]) + time[MIN_ONES])	// 0-59		= Minutes since start of current hour.
#define EPOCH_SECOND		(uint32_t) ((10*time[SEC_TENS]) + time[SEC_ONES])	// 0-59		= Seconds since start of current minute.

//The following 2-dimensional array serves as a look-up table to determine the number of days elapsed within the current 4-year block.
//This is required so that leap-days are included when calculating epoch time.
//Each element represents the number of days elapsed from the start of the 4-year block to the start of the selected month.
static uint16_t days_table[4][12] =
{	//Jan  Feb  Mar  Apr  May  Jun  Jul  Aug  Sep  Oct  Nov  Dec
	{   0,  31,  60,  91, 121, 152, 182, 213, 244, 274, 305, 335},	//First year (leap year).
	{ 366, 397, 425, 456, 486, 517, 547, 578, 609, 639, 670, 700},	//Second year.
	{ 731, 762, 790, 821, 851, 882, 912, 943, 974,1004,1035,1065},	//Third year.
	{1096,1127,1155,1186,1216,1247,1277,1308,1339,1369,1400,1430},	//Fourth year.
};


////////////////////////////////////
//Global Variable Initialisations://
////////////////////////////////////

//"mode" is altered via interrupt (pin-change triggered by button press).  Initialisation here sets the display mode at boot.
uint8_t mode = MODE_1B_ISO;

//"offset" represents the time offset from UTC.  The GPS data always returns UTC so an offset is required to get local time and allow for DST.
//Valid offsets are half-hour increments from -120 hours to +12.0 hours.
//To avoid using floats, offset actually ranges from -120 to 120 and is adjusted in increments of 5 (representing half an hour).
int8_t offset;	//Value is initialised in the main function by reading the value stored in eeprom.

//"intensity" represents the brightness level of the seven-segment displays (valid range integer from 0 to 15).
//Needs to be global as the function for setting intensity is called from an interrupt sub-routine.
uint8_t intensity;

//Initialise global array "time" which shall include all the time and date data pulled from the RTC or GPS.  Bytes will be binary-coded deciaml (BCD):
uint8_t time[SIZE_OF_TIME_ARRAY];	//time[0]  = time[CEN_TENS] : Century tens,	1 or 2
					//time[1]  = time[CEN_ONES] : Century ones,	9 or 0
					//time[2]  = time[YEA_TENS] : Year tens,	0 to 9
					//time[3]  = time[YEA_ONES] : Year ones,	0 to 9
					//time[4]  = time[MON_TENS] : Month tens,	1 to 2
					//time[6]  = time[MON_ONES] : Month ones,	0 to 9
					//time[5]  = time[DAY_TENS] : DAT tens,		0 to 3
					//time[7]  = time[DAY_ONES] : DAT ones,		1 to 9
					//time[8]  = time[HOU_TENS] : Hour tens,	0 to 2
					//time[9]  = time[HOU_ONES] : Hour ones,	0 to 9
					//time[10] = time[MIN_TENS] : Minute tens,	0 to 5
					//time[11] = time[MIN_ONES] : Minute ones,	0 to 9
					//time[12] = time[SEC_TENS] : Second tens,	0 to 5
					//time[13] = time[SEC_ONES] : Second ones,	0 to 9

//Global static array declarations for using manual decode mode to print pseudo "text" to the seven-seg displays (i.e. using sev_seg_flash_word()).
static uint8_t splash[16] = {
	SEV_SEG_MANUAL_C, SEV_SEG_MANUAL_L, SEV_SEG_MANUAL_O, SEV_SEG_MANUAL_C, SEV_SEG_MANUAL_Y, SEV_SEG_MANUAL_DASH,
	SEV_SEG_MANUAL_D, SEV_SEG_MANUAL_O, SEV_SEG_MANUAL_O, SEV_SEG_MANUAL_D, SEV_SEG_MANUAL_L, SEV_SEG_MANUAL_E, SEV_SEG_MANUAL_DASH,
	SEV_SEG_MANUAL_D, SEV_SEG_MANUAL_O, SEV_SEG_MANUAL_O};	//"CLOCy-dOOdLE-dOO"
static uint8_t syncing[16] = {
	SEV_SEG_MANUAL_S, SEV_SEG_MANUAL_Y, SEV_SEG_MANUAL_N, SEV_SEG_MANUAL_C, SEV_SEG_MANUAL_I, SEV_SEG_MANUAL_N, SEV_SEG_MANUAL_G,
	0, 0, 0, 0, 0, 0, 0, 0, 0};	//"SynCIng"
static uint8_t no_sync[16] = {
	SEV_SEG_MANUAL_N, SEV_SEG_MANUAL_O, 0, SEV_SEG_MANUAL_S, SEV_SEG_MANUAL_Y, SEV_SEG_MANUAL_N, SEV_SEG_MANUAL_C,
	0, 0, 0, 0, 0, 0, 0, 0, 0};	//"nO SynC"
static uint8_t success[16] = {
	SEV_SEG_MANUAL_S, SEV_SEG_MANUAL_U, SEV_SEG_MANUAL_C, SEV_SEG_MANUAL_C, SEV_SEG_MANUAL_E, SEV_SEG_MANUAL_S, SEV_SEG_MANUAL_S,
	0, 0, 0, 0, 0, 0, 0, 0, 0};	//"SUCCESS"
static uint8_t intensity_text[9] = {
	SEV_SEG_MANUAL_I, SEV_SEG_MANUAL_N, SEV_SEG_MANUAL_T, SEV_SEG_MANUAL_E, SEV_SEG_MANUAL_N, SEV_SEG_MANUAL_S, SEV_SEG_MANUAL_I,
	SEV_SEG_MANUAL_T, SEV_SEG_MANUAL_Y};	//"IntEnSIty"
static uint8_t epoch_text[6] = {
	SEV_SEG_MANUAL_E, SEV_SEG_MANUAL_P, SEV_SEG_MANUAL_O, SEV_SEG_MANUAL_C, SEV_SEG_MANUAL_H, SEV_SEG_MANUAL_DASH};	//"EPOCH-"
static uint8_t offset_text[6] = {
	SEV_SEG_MANUAL_O, SEV_SEG_MANUAL_F, SEV_SEG_MANUAL_F, SEV_SEG_MANUAL_S, SEV_SEG_MANUAL_E, SEV_SEG_MANUAL_T};//"OFFSEt  "

/////////////////////////
//Function Declarations//
/////////////////////////
void hardware_init(void);			//Initialise the peripherals.
void settings_init(void);			//Initialise (validate and set) settings that are stored in eeprom (UTC time offset and intensity (brightness).
void validate_eeprom_offset(void);		//Read the UTC time offset value from eeprom and confirm that it is valid.
void validate_eeprom_intensity(void);		//Read the UTC intensity (brightness) value from eeprom and confirm that it is valid.
void poll(void);				//Poll the current selected mode and run the according function.
void display_iso_time(void);			//Display the time in a standard ISO-8601 format.
void display_epoch_time(void);			//Display UNIX Epoch time (seconds elapsed since 1970.01.01.00.00.00).
void get_offset(void);				//Enables alteration of the time offset from UTC (time data from the GPS module is always UTC).
void display_offset(void);			//Display the current offset value using the last 3 digits (-11.5 to 12.0).
void cycle_offset(void);			//Increment the offset value by half an hour and rollover when maximum valid value is exceeded.
void attempt_sync(void);			//Attempt to sync the RTC time with GPS data.  Display status with pseudo-text.
uint8_t sync_time (uint8_t *time);		//Update the time array by parsing the date and time from the GPS module.  Returns FALSE if data is invalid.
void apply_offset(void);			//Apply the set UTC time offset to the time received from the GPS.
void rollunder_hours(void);			//Adjust the date when offset has caused the hours to roll under 0.
void rollover_hours(void);			//Adjust the date when offset has caused the hours to roll over 23.
void rollunder_days(void);			//Adjust the month when offset has caused the date to roll under 1.
void rollover_days(void);			//Adjust the month when offset has caused the date to roll over the maximum for the current month.
void get_intensity(void);			//Allow adjustment of the display intensity (brightness).
void cycle_intensity(void);			//Cycle through the possible intensity levels.
void sev_seg_set_word(uint8_t *word, uint8_t word_length);				//Use the seven-segment digits to display "text".
void sev_seg_flash_word(uint8_t *word, uint8_t word_length, uint16_t duration_ms);	//Use the seven-segment digits to display "text" for a defined duration.
void sev_seg_startup_ani(void);			////Simple startup animation scans the decimal point (DP) right to left then back a few times.
