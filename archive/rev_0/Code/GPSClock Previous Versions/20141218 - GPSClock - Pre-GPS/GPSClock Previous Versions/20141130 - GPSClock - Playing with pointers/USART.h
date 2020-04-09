/*******Header file used for serial communications via USART**********/


#ifndef BAUD		//If BAUD isn't already defined,
#define BAUD  9600	//Set BAUD to 9600
#endif


void initUSART(void);					//Initialise the USART peripheral
uint8_t receiveByte(void);				//Sets a variable to whatever was received by the USART
void transmitByte(uint8_t data);		//Transmits a byte from the USART
void printString(const char string[]);	//Transmits a string of characters.
void printByte(uint8_t byte);			//Takes an integer and transmits the characters - modified to only print last 2 digits (tens & ones).