#include <avr/io.h>		//Defines standard pins, ports, etc
#include <avr/power.h>
#include <avr/interrupt.h>	//Requires to use interrupt macros such ase sei();
#include <util/delay.h>		//From the standard AVR libraries - used to call delay_ms() and delay_us() functions.
#include "usart.h"
#include "spi.h"
#include "max7219.h"
#include "ds3234.h"

#define TRUE	1
#define FALSE	0

//Define the display modes
//Mode 1 shows the date on the left and the time on the right with two blank segments between them.
//Mode 2 shows the date and time together with no gap but a blank digit either side of the full date/time.
//Sub-modes A/B determine if the decimal points will be shown between individual components of the date/time.
#define MODE_1A	0b00	//	|Y Y Y Y M M D D     H H M M S S |
#define MODE_1B	0b01	//	|Y Y Y Y.M M.D D.    H H.M M.S S.|
#define MODE_2A	0b10	//	|  Y Y Y Y M M D D H H M M S S   |
#define MODE_2B	0b11	//	|  Y Y Y Y.M M.D D.H H.M M.S S.  |

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

//Initialise global array "time" which shall include all the time and date data pulled from the RTC or GPS.  Bytes will be binary-coded deciaml (BCD):
uint8_t time[SIZE_OF_TIME_ARRAY];	//time[0]  = time[CEN_TENS] : Century tens,	1 or 2
					//time[1]  = time[CEN_ONES] : Century ones,	9 or 0
					//time[2]  = time[YEA_TENS] : Year tens,	0 to 9
					//time[3]  = time[YEA_ONES] : Year ones,	0 to 9
					//time[4]  = time[MON_TENS] : Month tens,	0 to 1
					//time[6]  = time[MON_ONES] : Month ones,	0 to 9
					//time[5]  = time[DAT_TENS] : DAT tens,		0 to 3
					//time[7]  = time[DAT_ONES] : DAT ones,		0 to 9
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

//Function declarations
void hardware_init(void);			//Initialise the peripherals.
void display_time(uint8_t *time, uint8_t mode);	//Show the current time in accordance with the selected display mode.
void clear_disp_buffer(void);			//Write data to the display buffer that corresponds to a blank digit (all segments off) for all 16.
void refresh_display(void);			//Take the contents of the display buffer array and send it to the seven segment display drivers.
uint8_t sync_time (uint8_t *time);		//Update the time array by parsing the date and time from the GPS module.  Returns FALSE if data is invalid.
void sev_seg_startupAni(void);			////Simple startup animation scans the decimal point (DP) right to left then back a few times.
