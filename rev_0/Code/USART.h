//Definitions and declarations used for serial communications via USART

#ifndef BAUD		//If BAUD isn't already defined,
#define BAUD  4800	//Set BAUD to 4800
#endif

#include <avr/io.h>			//Needed to identify AVR registers and bits.
#include <util/setbaud.h>	//Used to caluculate Usart Baud Rate Register (High and Low) values as a function of F_CPU defined in the makefile abd BAUD defined

//Function declarations
void initUSART(void);					//Initialise the USART peripheral
uint8_t receiveByte(void);				//Sets a variable to whatever was received by the USART
void transmitByte(uint8_t data);		//Transmits a byte from the USART
void printString(const char string[]);	//Transmits a string of characters.
void printByte(uint8_t byte);			//Takes an integer and transmits the characters.