/*
 * Praktikum_4.3.c
 *
 * Created: 11/9/2025 9:02:02 PM
 * Author : mami4
 */ 

#include <avr/io.h>
#include "I2C_LCD.h"

#define WAIT_TIME 100000

void busy_waiting(uint32_t number)
{
	volatile uint32_t i;
	for(i = 0; i < number; i++);
}

//fills display with '0'
void lcd_fill()
{
	for (uint8_t i = 0; i < 2; i++)			// Making all the chars '0'
	{
		for (uint8_t j = 0; j < 16; j++)
		{
			lcd_moveCursor(j, i);
			lcd_putChar('0');
		}
	}
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
	
	lcd_fill();
		
	while (1)
	{
		// Move left to right (row 0)
		for (int8_t Column = 0; Column < 16; Column++)
		{
			lcd_moveCursor(Column, 0);
			lcd_putChar('1');
				
			if (Column != 0)
			{
				lcd_moveCursor((Column-1), 0);
				lcd_putChar('0');
			}
			busy_waiting(WAIT_TIME);
		}
			
		// make 16. column '0' before moving on to the next line
		lcd_moveCursor(15, 0);
		lcd_putChar('0');
			
		// Move right to left (row 1)
		for (int8_t Column = 15; Column >= 0; Column--)
		{
			lcd_moveCursor(Column, 1);
			lcd_putChar('1');
				
			if (Column != 15)
			{
				lcd_moveCursor((Column +1), 1);
				lcd_putChar('0');
			}
			busy_waiting(WAIT_TIME);
		}
			
		// make 0. column '0' before moving on to the next line
		lcd_moveCursor(0, 1);
		lcd_putChar('0');
	}
}

