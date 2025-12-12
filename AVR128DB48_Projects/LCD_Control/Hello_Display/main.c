/*
 * Praktikum_4.1.c
 *
 * Created: 11/9/2025 8:05:32 PM
 * Author : mami4
 */

#include <avr/io.h>
#include "I2C_LCD.h"

#define MESSAGE "Hello Display"
#define WAIT_TIME 100000

void busy_waiting(uint32_t number)
{
	volatile uint32_t i;
	for(i = 0; i < number; i++);
}


int main(void)
{
	// 1. Initialize the I2C Bus and the LCD
	// The lcd_init() function internally calls i2c_init().
	i2c_status status = lcd_init();
	
	if (status != SUCCESS)
	{
		while (1)
		{
			busy_waiting(WAIT_TIME);
		}
	}
	
	// Move the cursor to the 2. line
	lcd_moveCursor(0, 1);
	
	if (status == SUCCESS)
	{
		status = lcd_putString(MESSAGE);
	}
	
	while (1)
	{
		//Do nothing
		busy_waiting(WAIT_TIME);
	}
	
}



