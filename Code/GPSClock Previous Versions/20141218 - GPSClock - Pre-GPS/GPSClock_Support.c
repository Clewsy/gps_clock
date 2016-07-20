
#include <GPSClock_Support.h>

void initClock(void)
{
	//CLKPR: Clock Prescale Register.  Pre-scale the system clock.  The following setup will disable prescaling so the system clock will be the full 8MHz internal frequency.
	CLKPR = (1 << CLKPCE);		//CLKPCE: Clock Prescaler Change Enable.  Must set to one to enable changes to clock prescaler bits.
	CLKPR = 0;					//Once CLKPCE is enabled, Clock Prescaler Select Bits can be changed if done within 4 cycles.  In this case, clear all for prescaler value 1 (i.e. 8MHz system).
}

void init_buttons(void)
{
	DDR_BUTTONS &= ~(1 << MODE_BUTTON);	//Data Direction Register for port to which buttons are connected.
											//Clear DDR bits for buttons to make them inputs.  By default all pins are inputs at start-up so this isn't exactly neccessary.
	PORT_BUTTONS |= (1 << MODE_BUTTON);	//'Set' buttons port bit for buttons to activate the internal pull-up resistors.
}

void refresh_display(uint8_t *time, uint8_t mode)
{
	switch(mode)
	{
		case MODE_NO_DP:
			SEV_SEG_writeByte(SEV_SEG_DIGIT_15, SEV_SEG_CODEB_BLANK);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_14, ((time[CEN]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_13, ((time[CEN]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_12, ((time[YEA]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_11, ((time[YEA]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_10, ((time[MON]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_9, ((time[MON]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_8, ((time[DAT]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_7, ((time[DAT]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_6, ((time[HOU]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_5, ((time[HOU]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_4, ((time[MIN]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_3, ((time[MIN]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_2, ((time[SEC]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_1, ((time[SEC]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_0, SEV_SEG_CODEB_BLANK);
			break;
			
		case MODE_DP:
			SEV_SEG_writeByte(SEV_SEG_DIGIT_15, SEV_SEG_CODEB_BLANK);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_14, ((time[CEN]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_13, ((time[CEN]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_12, ((time[YEA]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_11, ((time[YEA]) & 0b00001111) | SEV_SEG_DP);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_10, ((time[MON]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_9, ((time[MON]) & 0b00001111) | SEV_SEG_DP);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_8, ((time[DAT]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_7, ((time[DAT]) & 0b00001111) | SEV_SEG_DP);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_6, ((time[HOU]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_5, ((time[HOU]) & 0b00001111) | SEV_SEG_DP);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_4, ((time[MIN]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_3, ((time[MIN]) & 0b00001111) | SEV_SEG_DP);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_2, ((time[SEC]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_1, ((time[SEC]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_0, SEV_SEG_CODEB_BLANK);
			break;

		case MODE_CENTER_GAP_NO_DP:
			SEV_SEG_writeByte(SEV_SEG_DIGIT_15, ((time[CEN]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_14, ((time[CEN]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_13, ((time[YEA]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_12, ((time[YEA]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_11, ((time[MON]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_10, ((time[MON]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_9, ((time[DAT]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_8, ((time[DAT]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_7, SEV_SEG_CODEB_BLANK);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_6, SEV_SEG_CODEB_BLANK);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_5, ((time[HOU]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_4, ((time[HOU]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_3, ((time[MIN]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_2, ((time[MIN]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_1, ((time[SEC]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_0, ((time[SEC]) & 0b00001111));
			break;
		
		case MODE_CENTER_GAP_DP:
			SEV_SEG_writeByte(SEV_SEG_DIGIT_15, ((time[CEN]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_14, ((time[CEN]) & 0b00001111));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_13, ((time[YEA]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_12, ((time[YEA]) & 0b00001111) | SEV_SEG_DP);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_11, ((time[MON]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_10, ((time[MON]) & 0b00001111) | SEV_SEG_DP);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_9, ((time[DAT]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_8, ((time[DAT]) & 0b00001111) | SEV_SEG_DP);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_7, SEV_SEG_CODEB_BLANK);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_6, SEV_SEG_CODEB_BLANK);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_5, ((time[HOU]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_4, ((time[HOU]) & 0b00001111) | SEV_SEG_DP);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_3, ((time[MIN]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_2, ((time[MIN]) & 0b00001111) | SEV_SEG_DP);
			SEV_SEG_writeByte(SEV_SEG_DIGIT_1, ((time[SEC]) >> 4));
			SEV_SEG_writeByte(SEV_SEG_DIGIT_0, ((time[SEC]) & 0b00001111));
			break;
	}
}
