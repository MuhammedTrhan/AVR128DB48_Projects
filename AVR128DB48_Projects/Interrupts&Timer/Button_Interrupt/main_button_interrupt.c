/*
 * Praktikum_5.1.c
 *
 * Created: 11/12/2025 4:17:19 PM
 * Author : mami4
 *
 * Implements non-blocking debouncing and rising edge detection
 * using a Timer ISR to toggle an RGB LED state.
 *
 * Microcontroller: AVR128DB48
 * LED: RGB connected to PE0, PE1, PE2 (using PE2 for demonstration)
 * Button: Connected to PC4
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdint.h>							// Include for uint32_t used by Xorshift

// Debounce constants: 
// Timer runs at 1kHz (1ms period). 
// 10 cycles = 10ms stable time required.
#define DEBOUNCE_MAX 10 

// Tracks the last confirmed, stable state of the button (true = pressed, false = released)
volatile bool G_Last_Stable_State = false;

// Counter for debouncing
volatile uint8_t G_Debounce_Counter = 0;

// State variable (seed) for the Xorshift32 PRNG.
// Change this value to alter the sequence of random numbers.
volatile uint32_t G_Xorshift_State = 123456789;

// Controls the RGB values in least significant 3 bits
volatile uint8_t color_bits;



// Generates a 32-bit pseudo-random number.
uint32_t xorshift32(void) 
{
    // Use a temporary variable to hold the state
    uint32_t x = G_Xorshift_State; 
    
    // Xorshift operations (xorshift13, xorshift17, xorshift5)
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    
    // Store the new state and return the result
    G_Xorshift_State = x;
    return x;
}

// interrupt service routine for the button
ISR(TCA0_OVF_vect)
{
	// Clear interrupt flag
	TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
	
	// Read raw button state (PC4 is 1 when pressed, 0 when released due to pull-down)
	bool Raw_State = (PORTC.IN & PIN4_bm) != 0;

	// Check if the current raw reading differs from the last confirmed stable state
	if (Raw_State != G_Last_Stable_State)
	{
		// State change detected, start/continue counting
		if (G_Debounce_Counter < DEBOUNCE_MAX)
		{
			G_Debounce_Counter++;
		}
		
		// Stable edge reached
		else {
			// 1. Update the last confirmed stable state
			G_Last_Stable_State = Raw_State;
			
			// 2. Rising Edge Detection (only act if the new stable state is HIGH/Pressed)
			if (G_Last_Stable_State == true)
			{
				// Random Color Generation
				uint32_t random_val = xorshift32();
				
				// Extract 3 bits for the R, G, B pins (PE0, PE1, PE2)
				color_bits = (uint8_t)(random_val & 0x07); // Mask to keep only the lower 3 bits (0b111)
				
				// Do not turn the LED off: avoid color_bits == 0b000
				if (color_bits == 0) {
					color_bits = 1; // Force to Red (0b001) if 0 is generated
				}
			}
			
			// 3. Reset the counter (ready to count the next transition)
			G_Debounce_Counter = 0;
		}
	}
	else
	{
		// The button state matches the last stable state, so reset the counter.
		// Wait for the next transition.
		G_Debounce_Counter = 0;
	}
}

// Configuring timer
void timer_init(void)
{
	// Configure TCA0 for 1 ms overflow
	// Assuming CLK_PER = 4 MHz (default for AVR128DB48)
	// 4,000,000 Hz / 4000 = 1,000 Hz (1 ms period)
	TCA0.SINGLE.PER = 4000; 		   // Set period for 1ms overflow (FIXED: Removed stray '\240' chars)
	TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc; // No prescaler (use CLK_PER)
	TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm; // Enable overflow interrupt (FIXED: Removed stray '\240' chars)
	TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm; // Enable timer
}


int main(void)
{
	// --- Port Configuration ---
	PORTC.DIRCLR = PIN4_bm;								// Set PC4 as input (button)
	// Since you have external pull-down resistors, we don't need internal pull-ups.
	// We also don't use PORT_ISC_RISING_gc because the timer is doing the polling.
	PORTC.PIN4CTRL = 0x00; 
	
	// Set PE0, PE1, and PE2 as output (RGB LED)
	PORTE.DIRSET = PIN0_bm | PIN1_bm | PIN2_bm;
	// Initialize LED to OFF
	PORTE.OUTCLR = PIN0_bm | PIN1_bm | PIN2_bm;
	
	// Initialization
	timer_init();
    sei(); // Enable global interrupts (FIXED: Removed stray '\240' chars)
	
    while (1) 
    {
		// The main loop is the application of the stable state.
		// We use PIN2_bm (blue) for a stable output color.
		if (G_Last_Stable_State)
		{
			// Clear only the RGB pins (PE0, PE1, PE2) before setting the new color
			PORTE.OUTCLR = PIN0_bm | PIN1_bm | PIN2_bm;
			
			// Set the new color bits directly:
			// PE0 (Red) = Bit 0, PE1 (Green) = Bit 1, PE2 (Blue) = Bit 2
			if (color_bits & PIN0_bm) { PORTE.OUTSET = PIN0_bm; }
			if (color_bits & PIN1_bm) { PORTE.OUTSET = PIN1_bm; }
			if (color_bits & PIN2_bm) { PORTE.OUTSET = PIN2_bm; }
		}
    }
}