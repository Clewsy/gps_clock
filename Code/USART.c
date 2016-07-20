//Functions used for serial communications via USART

#include <USART.h>

//Initialise the USART peripheral
void initUSART(void)
{
	//Utilising USART0
	UBRR0H = UBRRH_VALUE;	//USART Baud Rate Register High -Value defined in util/setbaud.h
	UBRR0L = UBRRL_VALUE;	//USART Baud Rate Register Low  -Value defined in util/setbaud.h

	#if USE_2X						//Double-Speed detemined in util/setbaud.h.  Needed is defined BAUD not achieavable without U2X0 
		UCSR0A |= (1 << U2X0);		//UCSR0A = USART 0 Control and Status Register A
	#else							//U2X0 = Double USART 0 Transmission Speed Enable
		UCSR0A &= ~(1 << U2X0);
	#endif
	
	UCSR0B = (1 << RXCIE0) | (1 << TXEN0) | (1 << RXEN0);		//UCSR0B = USART 0 Control and Status Register B
																//RXCIE0 = USART 0 RX Complete Interrupt Enable
																//TXEN0 = Transmit Enable USART0
																//RXEN0 = Receive Enable USART0
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);					//UCSR0C = USART 0 Control and Status Register C
																//UCSZ02:0 = Usart Character Size, Set to 0b011 for 8-bit.
																//(USBS = Usart Stop Bit Select, Stays at 0b0 for 1 stop bit)
}

//Sets a variable to whatever was received by the USART
uint8_t receiveByte(void)
{

	while (!(UCSR0A & (1 << RXC0))) {}		//Wait until the USART 0 receive complete flag is set 
	return UDR0;							//Returns received data.  UDR0 = USART 0 Data Register
}

//Transmits a byte from the USART
void transmitByte(uint8_t data)
{
	while (!(UCSR0A & (1 << UDRE0))) {}	//Wait until the USART 0 data register is empty (ready to transmit). Otherwise operates too fast and drops characters.
	UDR0 = data;							//UDR0 = USART 0 Data Register
}

//Transmits a string of characters.
void printString(const char string[])
{			
	uint8_t i = 0;								//Counter to increment for every character in the string.
	while ((string[i]) != '\0')				//Until null character (end of string).
	{
		transmitByte(string[i]);				//UCSR0A = USART 0 Control and Status Register A
		i++;									//UDRE0 = USART 0 Data Register Empty Flag
	}
}

//Takes an integer and transmits the characters
void printByte(uint8_t byte)
{
	//(modified to only print last 2 digits (tens & ones)).
	//while (!(UCSR0A & (1 << UDRE0))) {}	//Wait until the USART 0 data register is empty (ready to transmit).
	//transmitByte('0'+ (byte/100));		//Hundreds
	while (!(UCSR0A & (1 << UDRE0))) {}	//Wait until the USART 0 data register is empty (ready to transmit).
	transmitByte('0'+ ((byte/10) % 10));	//Tens
	while (!(UCSR0A & (1 << UDRE0))) {}	//Wait until the USART 0 data register is empty (ready to transmit).
	transmitByte('0'+ (byte % 10));		//Ones
}