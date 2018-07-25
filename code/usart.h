//Definitions and declarations used for serial communications via USART

#ifndef BAUD		//If BAUD isn't already defined,
#define BAUD  9600	//Set the BAUD rate (bits/second).
#endif

#include <avr/io.h>		//Needed to identify AVR registers and bits.
#include <util/setbaud.h>	//Used to caluculate Usart Baud Rate Register (High and Low) values as a function of F_CPU and BAUD

//Function declarations
void usart_init(void);				//Initialise the USART peripheral.
uint8_t usart_receive_byte(void);		//Returns a byte as received by the USART.
void usart_transmit_byte(uint8_t data);		//Transmits a byte from the USART.
void usart_print_string(const char string[]);	//Transmits a string of characters.
void usart_print_byte(uint8_t byte);		//Takes an integer and transmits the characters.
void usart_print_binary_byte(uint8_t byte);	//Takes an integer and prints the binary equivalent.
