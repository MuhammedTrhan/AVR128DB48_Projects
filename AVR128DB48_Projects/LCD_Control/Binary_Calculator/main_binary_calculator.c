/*
 * Praktikum_4.4.c
 *
 * Created: 11/9/2025 10:40:04 PM
 * Author : mami4
 */ 

#define F_CPU 16000000UL
#include <avr/io.h>
#include <stdbool.h>
#include <util/delay.h>
#include "I2C_LCD.h"

#define WAIT_TIME 5000

void busy_waiting(uint32_t number)
{
	volatile uint32_t i;
	for(i = 0; i < number; i++);
}

void to_str(int32_t num, char *str) {
	int i = 0;
	bool is_negative = false;
	uint32_t n;

	// Handle negative numbers
	if (num < 0) {
		is_negative = true;
		// Careful: -INT32_MIN would overflow, so cast to uint32_t
		n = (uint32_t)(-((int64_t)num));
		} else {
		n = (uint32_t)num;
	}

	// Handle zero explicitly
	if (n == 0) {
		str[i++] = '0';
		str[i] = '\0';
		return;
	}

	// Extract digits in reverse order
	while (n > 0) {
		uint8_t digit = n % 10;
		str[i++] = '0' + digit;
		n /= 10;
	}

	// Add negative sign if needed
	if (is_negative) {
		str[i++] = '-';
	}

	// Null terminator
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

// Returns the key pressed
static uint8_t single_button_debounce_filter(PORT_t port) {

	uint8_t buffer    = 0x01;
	uint8_t pin        = 0x00;
	const uint8_t button_pins = PIN7_bm | PIN6_bm | PIN5_bm | PIN4_bm;
	
	while (buffer != 0xFF) {
		
		buffer <<= 1;
		
		_delay_us(500);
		
		switch ((~port.IN) & button_pins) {
			
			case PIN4_bm:
			pin = PIN4_bp;
			buffer |= 0x01;
			break;
			
			case PIN5_bm:
			pin = PIN5_bp;
			buffer |= 0x01;
			break;
			
			case PIN6_bm:
			pin = PIN6_bp;
			buffer |= 0x01;
			break;
			
			case PIN7_bm:
			pin = PIN7_bp;
			buffer |= 0x01;
			break;
			
			case 0xFF:
			break;
			
			default:
			return 0xFF;
		}
	}
	return pin;
}

int main(void)
{
	PORTC.DIRCLR = PIN7_bm | PIN6_bm | PIN5_bm | PIN4_bm;	// Set PC7 ... PC4 as inputs (PC4 -> +1, PC5 -> -1, PC6 -> *2, PC7 -> /2)
		
	PORTC.PIN4CTRL = 0x00;
	PORTC.PIN5CTRL = 0x00;
	PORTC.PIN6CTRL = 0x00;
	PORTC.PIN7CTRL = 0x00;						// Configurations set to default
	
	int32_t Result = 0;							// Variable for result
	
	bool result_changed = true;					// Flag to track if we need to update the LCD (it is true to show the '0' at the first itteration)
	
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
	
	char Result_Str[13];							// Result can be max 11 digits including '-' + "\0"
	
	while (1)
	{
		uint8_t Button = single_button_debounce_filter(PORTC);

		switch (Button)
		{
			case 4:
				result_changed = true;
				Result++;
				break;
			
			case 5:
				result_changed = true;
				Result--;
				break;

			case 6:
				result_changed = true;
				Result <<= 1;					// Multiply by 2
				break;
			
			case 7:
				result_changed = true;
				Result >>= 1;					// Divide by 2
				break;
		}
		
		// Only update the LCD if the result changed
		if (result_changed)
		{
			to_str(Result, Result_Str);
			
			lcd_moveCursor(0, 0);
			// Clears any leftover characters from previous, longer numbers
			lcd_clear();
			
			lcd_moveCursor(0, 0);
			status = lcd_putString(Result_Str);
		}
		
		result_changed = false;
	}
}

