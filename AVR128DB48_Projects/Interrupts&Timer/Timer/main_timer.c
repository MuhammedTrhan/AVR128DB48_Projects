/*
 * Praktikum_5.2.c
 *
 * Created: 11/12/2025 4:17:19 PM
 * Author : mami4
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h> // For the boolean flag
#include <stdint.h>  // For uint16_t and uint32_t
#include "I2C_LCD.h" // For LCD functions

// Define the clock frequency (4 MHz as per your I2C files)
#define F_CPU 4000000UL 

// The TCB is configured to overflow every 1ms (1kHz).
// We need 1000 such events to make 1 second.
#define MS_PER_SECOND 1000

// --- Global State Variables ---
// Flag set by the ISR when 1ms has elapsed
volatile bool G_Second_Changed = false;

// Variable to store the running count of seconds
volatile uint32_t G_Second_Counter = 0;

// Variable to store the running count of milliseconds (0 to 999)
volatile uint16_t G_ms_Accumulator = 0;


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

/**
 * @brief Initializes Timer/Counter B0 (TCB0) for 1ms periodic interrupts.
 * * TCB0 is configured in Periodic Interrupt Mode to interrupt at a 1kHz frequency (1ms).
 * CLK_PER = 4 MHz. We need a count value (CCMP) such that:
 * TCB0_Frequency = CLK_PER / (CCMP + 1)
 * 1000 Hz = 4,000,000 Hz / (CCMP + 1)
 * CCMP + 1 = 4000
 * CCMP = 3999
 */
void timer_init(void)
{
	// 1. Set the compare value for 1ms
	TCB0.CCMP = 3999;
	
	// 2. Configure TCB in Periodic Interrupt Mode (CLKSEL = CLK_PER, MODE = Periodic)
	TCB0.CTRLA = TCB_CLKSEL_DIV1_gc; // No prescaler, use CLK_PER
	TCB0.CTRLB = TCB_CNTMODE_INT_gc; // Set to Periodic Interrupt Mode (INT)
	
	// 3. Enable the interrupt
	TCB0.INTCTRL = TCB_CAPT_bm; // Enable Capture/Compare interrupt
	
	// 4. Enable the timer
	TCB0.CTRLA |= TCB_ENABLE_bm; 
}


/**
 * @brief Timer/Counter B0 (TCB0) Interrupt Service Routine.
 * * This ISR executes exactly every 1ms. Its only job is to update the
 * millisecond accumulator and second counter, and set a flag for the main loop.
 */
ISR(TCB0_INT_vect)
{
	// 1. Clear the interrupt flag (mandatory)
	TCB0.INTFLAGS = TCB_CAPT_bm;
	
	// 2. Increment the millisecond accumulator
	G_ms_Accumulator++;
	
	// 3. Check if a full second has elapsed (1000ms)
	if (G_ms_Accumulator >= MS_PER_SECOND)
	{
		G_ms_Accumulator = 0;
		G_Second_Counter++;
		
		// Set flag for the main loop to know a second has passed
		G_Second_Changed = true;
	}
}


int main(void)
{
	i2c_status status;

	// Initialize the I2C LCD Module
	status = lcd_init();
	if (status != SUCCESS) {
		// Basic error handling: if I2C fails, halt and blink an error indication
		PORTE.DIRSET = PIN0_bm; // Set PE0 as output for error indication
		while(1) { 
			PORTE.OUTTGL = PIN0_bm;
		} 
	}

	timer_init();
    sei(); // Enable global interrupts
	
    while (1) 
    {
		// Second Update Check
		// Check the flag set by the ISR every time a full second (1000ms) has passed.
		if (G_Second_Changed)
		{
			// Reset the flag immediately
			G_Second_Changed = false;

			cli();
			sei();
				
			char Str[12];
				
			to_str(G_Second_Counter, Str);
				
			lcd_clear();
			// Move cursor to the second line and display the string
			lcd_moveCursor(0, 1);
			lcd_putString(Str);
		}
		
		// Idle loop continues to run here, waiting for G_1ms_Flag to be set
    }
}