#include <avr/io.h>		//Defines standard pins, ports, etc
#include <avr/power.h>
#include <avr/interrupt.h>	//Requires to use interrupt macros such ase sei();
#include "usart.h"
#include "spi.h"
#include "max7219.h"

//Define array elements for easier identification when pulled from time array.
#define SEC 0
#define MIN 1
#define HOU 2
#define DAY 3
#define MON 4
#define YEA 5
#define CEN 6
#define SIZE_OF_TIME_ARRAY 7
//Initialise array "time" which shall include all the time and date data pulled from the RTC or GPS.  Bytes will be binary-coded deciaml (BCD):
uint8_t time[SIZE_OF_TIME_ARRAY];	//time[0] = time[SEC]: Seconds,	0  to 59
					//time[1] = time[MIN]: Minutes,	0  to 59
					//time[2] = time[HOU]: Hours,	0  to 23
					//time[4] = time[DAT]: Date,	1  to 31
					//time[5] = time[MON]: Month,	1  to 12
					//time[6] = time[YEA]: Year,	0  to 99
					//time[7] = time[CEN]: Century,	19 to 20
