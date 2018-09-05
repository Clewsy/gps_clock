#include <avr/io.h>		//Defines standard pins, ports, etc
#include <avr/power.h>
#include <avr/interrupt.h>	//Requires to use interrupt macros such ase sei();
#include <util/delay.h>		//From the standard AVR libraries - used to call delay_ms() and delay_us() functions.
#include "usart.h"
#include "spi.h"
#include "max7219.h"
#include "ds3234.h"

#define TRUE	1	//True/false used to determine succeful sync of time from GPS.
#define FALSE	0

//Following definitions will be used to initialise and read the buttons.
#define BUTTON_DDR			DDRC		//DDRC: Data Direction Register C - For setting IO to input or output.
#define BUTTON_PORT			PORTC		//PORTC: Port C - For writing to IO.
#define BUTTON_PINS			PINC		//PINC: Pin C - For reading from IO.
#define BUTTON_PCIE			PCIE1		//PCIE1: Pin Change Interrupt Enable 1 - For enabling interrupts on IO level change.
#define BUTTON_PCMSK			PCMSK1		//PCMSK1: Pin Change Mask Register 1 - For masking which IO lines to trigger interrupt.
#define BUTTON_MODE			PC0		//PC0: PCINT8
#define BUTTON_SYNC			PC1		//PC1: PCINT9
#define BUTTON_PCI_VECTOR		PCINT1_vect	//PCINT1: Pin-Change Interrupt 1 - Define the interrupt sub-routine vector function name.
#define BUTTON_DEBOUNCE_DURATION	10		//Define the duration in ms to wait to avoide button "bounce".

//Define the display modes
//Mode 1 shows the date on the left and the time on the right with two blank segments between them.
//Mode 2 shows the date and time together with no gap but a blank digit either side of the full date/time.
//Sub-modes A/B determine if the decimal points will be shown between individual components of the date/time.
#define MODE_1A	0b000	//	|Y Y Y Y M M D D     H H M M S S |	ISO-8601 with date/time separation, no delimiters.
#define MODE_1B	0b001	//	|Y Y Y Y.M M.D D.    H H.M M.S S.|	ISO-8601 with date/time separation with delimiters (decimal points).
#define MODE_2A	0b010	//	|  Y Y Y Y M M D D H H M M S S   |	ISO-8601 centered, no delimiters.
#define MODE_2B	0b011	//	|  Y Y Y Y.M M.D D.H H.M M.S S.  |	ISO-8601 centered with delimiters (decimal points).
#define MODE_3	0b100	//	|E P O C H   S S S S S S S S S S |	Epoch time (seconds elapsed since midnight, Jan 1st, 1970).  Aka UNIX time.

//Define array elements for easier identification when pulled from time array.
#define CEN_TENS 0
#define CEN_ONES 1
#define YEA_TENS 2
#define YEA_ONES 3
#define MON_TENS 4
#define MON_ONES 5
#define DAT_TENS 6
#define DAT_ONES 7
#define HOU_TENS 8
#define HOU_ONES 9
#define MIN_TENS 10
#define MIN_ONES 11
#define SEC_TENS 12
#define SEC_ONES 13
#define SIZE_OF_TIME_ARRAY 14

//These definitions are used to make the calculation of epoch time much more readable.
//Epoch time (or unix epoch time) is the number of seconds elapsed since 0hrs, January first, 1970.
#define DAYS_IN_4_YEARS		1461		// = ((365days * 4years) + 1leap-day)
#define SECONDS_IN_A_DAY	86400		// = 60seconds * 60minutes * 24hours
#define SECONDS_IN_AN_HOUR	3600		// = 60seconds * 60minutes
#define SECONDS_IN_A_MINUTE	60
#define EPOCH_SECONDS_TO_2000	946684800	// = Seconds elapsed from epoch (midnight, Jan 1st, 1970) until midnight, Jan 1st, 2000.
#define EPOCH_YEAR		(uint32_t) ((10*time[YEA_TENS]) + time[YEA_ONES])	// 0-99		= Years since 2000.
#define EPOCH_MONTH		(uint32_t) ((10*time[MON_TENS]) + time[MON_ONES])	// 1-12		= Months since start of current year.
#define EPOCH_DAY		(uint32_t) ((10*time[DAT_TENS]) + time[DAT_ONES])	// 1-31		= Days since start of current month.
#define EPOCH_HOUR		(uint32_t) ((10*time[HOU_TENS]) + time[HOU_ONES])	// 0-23		= Hours since start of current day.
#define EPOCH_MINUTE		(uint32_t) ((10*time[MIN_TENS]) + time[MIN_ONES])	// 0-59		= Minutes since start of current hour.
#define EPOCH_SECOND		(uint32_t) ((10*time[SEC_TENS]) + time[SEC_ONES])	// 0-59		= Seconds since start of current minute.

//The following 2-dimensional array serves as a look-up table to determine the number of days elapsed within the current 4-year block.
//This is required so that leap-days are included when calculating epoch time.
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
uint8_t mode = MODE_1B;

//Initialise global array "time" which shall include all the time and date data pulled from the RTC or GPS.  Bytes will be binary-coded deciaml (BCD):
uint8_t time[SIZE_OF_TIME_ARRAY];	//time[0]  = time[CEN_TENS] : Century tens,	1 or 2
					//time[1]  = time[CEN_ONES] : Century ones,	9 or 0
					//time[2]  = time[YEA_TENS] : Year tens,	0 to 9
					//time[3]  = time[YEA_ONES] : Year ones,	0 to 9
					//time[4]  = time[MON_TENS] : Month tens,	1 to 2
					//time[6]  = time[MON_ONES] : Month ones,	0 to 9
					//time[5]  = time[DAT_TENS] : DAT tens,		0 to 3
					//time[7]  = time[DAT_ONES] : DAT ones,		1 to 9
					//time[8]  = time[HOU_TENS] : Hour tens,	0 to 2
					//time[9]  = time[HOU_ONES] : Hour ones,	0 to 9
					//time[10] = time[MIN_TENS] : Minute tens,	0 to 5
					//time[11] = time[MIN_ONES] : Minute ones,	0 to 9
					//time[12] = time[SEC_TENS] : Second tens,	0 to 5
					//time[13] = time[SEC_ONES] : Second ones,	0 to 9

//Initialise global array "disp_buffer" which will consist of 16 data bytes that will be sent to the seven-segment display drivers.
//I.e. each element is the data to be used for each of the 16 7-seg digits.
uint8_t disp_buffer[16];		//disp_buffer[0] = digit 0	<--Least-Significant Digit (Right)
					//disp_buffer[1] = digit 1
					//disp_buffer[2] = digit 2
					//	...	 =  ...
					//disp_buffer[14] = digit 14
					//disp_buffer[15] = digit 15	<--Least-Significant Digit (Right)


//Global static array declarations for using manual decode mode to print pseudo "text" to the seven-seg displays (i.e. using sev_seg_display_word()).
static uint8_t syncing[16] = {SEV_SEG_MANUAL_S, SEV_SEG_MANUAL_Y, SEV_SEG_MANUAL_N, SEV_SEG_MANUAL_C, SEV_SEG_MANUAL_I, SEV_SEG_MANUAL_N, SEV_SEG_MANUAL_G,
	0, 0, 0, 0, 0, 0, 0, 0, 0};	//"SynCIng"
static uint8_t no_sync[16] = {SEV_SEG_MANUAL_N, SEV_SEG_MANUAL_O, 0, SEV_SEG_MANUAL_S, SEV_SEG_MANUAL_Y, SEV_SEG_MANUAL_N, SEV_SEG_MANUAL_C,
	0, 0, 0, 0, 0, 0, 0, 0, 0};	//"nO SynC"
static uint8_t success[16] = {SEV_SEG_MANUAL_S, SEV_SEG_MANUAL_U, SEV_SEG_MANUAL_C, SEV_SEG_MANUAL_C, SEV_SEG_MANUAL_E, SEV_SEG_MANUAL_S, SEV_SEG_MANUAL_S,
	0, 0, 0, 0, 0, 0, 0, 0, 0};	//"SynCIng"
static uint8_t epoch_text[6] = {SEV_SEG_MANUAL_E, SEV_SEG_MANUAL_P, SEV_SEG_MANUAL_O, SEV_SEG_MANUAL_C, SEV_SEG_MANUAL_H, SEV_SEG_MANUAL_DASH};
//static uint8_t adjust[16] = {SEV_SEG_MANUAL_A, SEV_SEG_MANUAL_D, SEV_SEG_MANUAL_J, SEV_SEG_MANUAL_U, SEV_SEG_MANUAL_S, SEV_SEG_MANUAL_T,
//	0, 0, 0, 0, 0, 0, 0, 0, 0, 0};	//"AdJUSt"
//static uint8_t iso[16] = {0, 0, 0, 0, SEV_SEG_MANUAL_I, SEV_SEG_MANUAL_S, SEV_SEG_MANUAL_O, SEV_SEG_MANUAL_DASH,
//	SEV_SEG_MANUAL_8, SEV_SEG_MANUAL_6, SEV_SEG_MANUAL_0, SEV_SEG_MANUAL_1, 0, 0, 0, 0};	//"ISO-8601"


/////////////////////////
//Function Declarations//
/////////////////////////
void hardware_init(void);			//Initialise the peripherals.
void display_time(uint8_t *time, uint8_t mode);	//Show the current time in accordance with the selected display mode.
void clear_disp_buffer(void);			//Write data to the display buffer that corresponds to a blank digit (all segments off) for all 16.
void refresh_display(void);			//Take the contents of the display buffer array and send it to the seven segment display drivers.
uint8_t sync_time (uint8_t *time);		//Update the time array by parsing the date and time from the GPS module.  Returns FALSE if data is invalid.
void attempt_sync(void);			//Attempt to sync the RTC time with GPS data.  Display status with pseudo-text.
void sev_seg_display_word(uint8_t *word, uint16_t duration_ms);		//Use the seven-segment digits to display pseudo "text".
void sev_seg_startup_ani(void);			////Simple startup animation scans the decimal point (DP) right to left then back a few times.
