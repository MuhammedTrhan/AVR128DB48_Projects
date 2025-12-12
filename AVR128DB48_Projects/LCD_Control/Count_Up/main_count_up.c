/*
 * Praktikum_4.2.c
 *
 * Created: 11/9/2025 8:17:20 PM
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

void to_str(uint32_t num, char *str) {
	int i = 0;

	// Handle zero explicitly
	if (num == 0) {
		str[i++] = '0';
		str[i] = '\0';
		return;
	}

	// Extract digits in reverse order
	while (num > 0) {
		uint8_t digit = num % 10;
		str[i++] = '0' + digit;
		num /= 10;
	}

	// Add string terminator
	str[i] = '\0';

	// Reverse the string
	int start = 0;
	int end = i - 1;
	while (start < end) {
		char temp = str[start];
		str[start] = str[end];
		str[end] = temp;
		start++;
		end--;
	}
}

int str_length(const char *str) {
	int len = 0;
	while (str[len] != '\0') {  // count until we hit the null terminator
		len++;
	}
	return len;
}

int main(void)
{
    uint32_t Counter = 0;
	
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
	
	char Str[12];
	
	while (1)
	{
		if (status == SUCCESS)
		{
			to_str(Counter, Str);
			
			int Len = str_length(Str);
			
			switch (Len) {
			
				case 1:
					// Move the cursor to the 1. line, 4. row
					lcd_moveCursor(3, 0);
					break;
				
				case 2:
					// Move the cursor to the 1. line, 3. row
					lcd_moveCursor(2, 0);
					break;
			
				case 3:
					// Move the cursor to the 1. line, 2. row
					lcd_moveCursor(1, 0);
					break;
			
				case 4:
					// Move the cursor to the 1. line, 1. row
					lcd_moveCursor(0, 0);
					break;
				
			}
			
			status = lcd_putString(Str);
		}
		
		busy_waiting(WAIT_TIME);
		
		Counter++;
		status = lcd_clear();
	}
}

