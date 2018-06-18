

void init_buttons(void);
ISR(USART_RX_vect):	//Interrupt subroutine triggered when the USART receives a byte.
void refresh_display(uint8_t *time, uint8_t mode);
