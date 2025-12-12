/*
 * Praktikum_5.4.c
 *
 * Created: 11/16/2025 2:27:19 PM
 * Author : mami4
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "I2C_LCD.h" // For LCD functions

// Define the clock frequency (4 MHz)
#define F_CPU 4000000UL 

// --- Configuration Constants ---
#define MS_PER_SECOND 1000
#define MS_DEBOUNCE_MAX 10 // 10ms for stable button reading
#define TIME_ADD_SECONDS 5 // Amount of time to add with PC4

// --- State Definitions ---

typedef enum {
    TIMER_PAUSED,
    TIMER_RUNNING,
    TIMER_EXPIRED
} timer_state_t;

// --- Global Volatile Variables (Shared with ISR) ---

// **Timer Variables**
volatile timer_state_t G_Timer_State = TIMER_EXPIRED;
volatile uint32_t G_Remaining_Seconds = 0;
volatile uint16_t G_ms_Accumulator = 0; // Counts 0 to 999 for 1 second

// Flag to determine if the string that will be printed changed
volatile bool G_Status_Changed = true;

// **Button Debounce Variables (PC4: Add Time)**
volatile bool G_PC4_Stable_State = false; 
volatile uint8_t G_PC4_Debounce_Counter = 0;
volatile bool G_PC4_Request = false; 

// **Button Debounce Variables (PC5: Start/Pause)**
volatile bool G_PC5_Stable_State = false;
volatile uint8_t G_PC5_Debounce_Counter = 0;
volatile bool G_PC5_Request = false;

// --- Hardware Control Macros ---

// PE0 (Red), PE1 (Green), PE2 (Blue)
#define LED_OFF()		(PORTE.OUTCLR = PIN0_bm | PIN1_bm | PIN2_bm)
#define LED_RED_ON()	(LED_OFF(), PORTE.OUTSET = PIN0_bm)        
#define LED_GREEN_ON()	(LED_OFF(), PORTE.OUTSET = PIN1_bm)        
#define LED_BLUE_ON()	(LED_OFF(), PORTE.OUTSET = PIN2_bm)

// --- Function Declarations ---
static void update_display(uint32_t seconds, timer_state_t state);
static void update_led(timer_state_t state);
static void handle_button_debounce(uint8_t pin_value, volatile bool *stable_state, volatile uint8_t *counter, volatile bool *request_flag);

// --- Timer Initialization ---

/**
 * @brief Initializes Timer/Counter B0 (TCB0) for 1ms periodic interrupts (4MHz / 4000 = 1kHz).
 */
void timer_init(void)
{
	TCB0.CCMP = 3999; 
	TCB0.CTRLA = TCB_CLKSEL_DIV1_gc; // No prescaler, use CLK_PER
	TCB0.CTRLB = TCB_CNTMODE_INT_gc; // Periodic Interrupt Mode
	TCB0.INTCTRL = TCB_CAPT_bm;      // Enable Capture/Compare interrupt
	TCB0.CTRLA |= TCB_ENABLE_bm; 
}


// --- Interrupt Service Routine ---

/**
 * @brief Timer/Counter B0 (TCB0) Interrupt Service Routine (1ms tick).
 * * Handles button debouncing and second-level timing logic.
 */
ISR(TCB0_INT_vect)
{
	TCB0.INTFLAGS = TCB_CAPT_bm; // Clear the interrupt flag
	
	// --- 1. Debounce Logic for PC4 (Add Time) and PC5 (Start/Pause) ---
	uint8_t current_portc_in = PORTC.IN;
	
	// Handle PC4 (Add Time)
	handle_button_debounce((current_portc_in & PIN4_bm), 
	                       &G_PC4_Stable_State, 
	                       &G_PC4_Debounce_Counter, 
	                       &G_PC4_Request);
	
	// Handle PC5 (Start/Pause)
	handle_button_debounce((current_portc_in & PIN5_bm), 
	                       &G_PC5_Stable_State, 
	                       &G_PC5_Debounce_Counter, 
	                       &G_PC5_Request);
	
	// --- 2. Timing Logic (Executed every 1ms) ---
	if (G_Timer_State == TIMER_RUNNING)
	{
		if (G_ms_Accumulator < MS_PER_SECOND - 1)
		{
			G_ms_Accumulator++;
		}
		else // A full second has elapsed
		{
			G_ms_Accumulator = 0; // Reset ms counter
			
			// Decrement G_Remaining_Seconds (32-bit access is atomic here, but cli/sei in main is still safer)
			if (G_Remaining_Seconds > 0)
			{
				G_Remaining_Seconds--;
				G_Status_Changed = true;						// Signal main loop for display update
			}
			
			// Check for timer expiration
			if (G_Remaining_Seconds == 0)
			{
				G_Timer_State = TIMER_EXPIRED;
				G_Status_Changed = true;						// Signal main loop for display update
			}
		}
	}
}

/**
 * @brief Generic debounce and rising edge detection logic.
 * Sets the request_flag upon a stable rising edge.
 */
static void handle_button_debounce(uint8_t pin_value, volatile bool *stable_state, volatile uint8_t *counter, volatile bool *request_flag)
{
    // Button is pressed if pin_value is non-zero
	bool current_raw_state = (pin_value != 0);

	if (current_raw_state != *stable_state)
	{
		if (*counter < MS_DEBOUNCE_MAX)
		{
			(*counter)++;
		}
		
		if (*counter >= MS_DEBOUNCE_MAX)
		{
			// Stable edge reached
			
			// 1. Check for RISING EDGE
			if (current_raw_state == true)
			{
				*request_flag = true; // **ISR sets button request flag**
			}
			
			// 2. Update the stable state and reset counter
			*stable_state = current_raw_state;
			*counter = 0;
		}
	}
	else
	{
		// State matches stable state, reset counter to wait for a change
		*counter = 0;
	}
}

// --- Display and LED Control Functions (Run in main) ---

static void update_led(timer_state_t state)
{
    LED_OFF();
    if (state == TIMER_EXPIRED) {
        LED_BLUE_ON();									// Blue is on when expired
    } else if (state == TIMER_PAUSED) {
        LED_RED_ON();									// Red is on when paused/ready
    } else if (state == TIMER_RUNNING) {
		LED_GREEN_ON();									// Green is on when running
    }
}

static void update_display(uint32_t seconds, timer_state_t state)
{
	lcd_clear();
    char line1_buffer[17];
    char line2_buffer[17];
    
    // Line 1: Status
    switch (state) {
        case TIMER_PAUSED:  strcpy(line1_buffer, "     PAUSED     "); break;
        case TIMER_RUNNING: strcpy(line1_buffer, "    RUNNING     "); break;
        case TIMER_EXPIRED: strcpy(line1_buffer, "    EXPIRED     "); break;
    }
    
    // Line 2: Remaining Time
    if (state == TIMER_EXPIRED) {
        sprintf(line2_buffer, "0s  GOOD JOB!   ");
    } else {
        sprintf(line2_buffer, "%lu seconds left", seconds);
    }
    
    // Write to LCD
    lcd_moveCursor(0, 0);
    lcd_putString(line1_buffer);
    lcd_moveCursor(0, 1);
    lcd_putString(line2_buffer);
}


int main(void)
{
	// --- Port Configuration ---
	PORTC.DIRCLR = PIN4_bm | PIN5_bm; // Set PC4 and PC5 as inputs (buttons)
	PORTC.PIN4CTRL = 0x00;           
	PORTC.PIN5CTRL = 0x00;
	
	// Set PE0 (R), PE1 (G), and PE2 (B) as output (RGB LED)
	PORTE.DIRSET = PIN0_bm | PIN1_bm | PIN2_bm;
	
	// --- Initialization ---
	i2c_status status;

	// Initialize the I2C LCD Module
	status = lcd_init();
	if (status != SUCCESS) {
		// Basic error handling: if I2C fails, halt and blink an error indication
		PORTE.DIRSET = PIN0_bm;									// Set PE0 as output for error indication
		while(1) {
			PORTE.OUTTGL = PIN0_bm;
		}
	}

	lcd_clear();
	
	update_led(G_Timer_State);									// Initialize LED to blue
	update_display(G_Remaining_Seconds, G_Timer_State);
	
	timer_init();
	sei();														// Enable global interrupts
	
    while (1) 
    {
        // --- 1. Handle START/PAUSE Request (PC5) ---
        if (G_PC5_Request)
        {
            G_PC5_Request = false;									// Consume request
            
            // Critical Section: Protect shared state variables
            cli();
            if (G_Timer_State == TIMER_RUNNING) {
                G_Timer_State = TIMER_PAUSED;						// Pause
            } else if (G_Remaining_Seconds > 0) {
                G_Timer_State = TIMER_RUNNING;						// Start
            } else if (G_Timer_State == TIMER_EXPIRED) {
                // If expired, pressing start/pause resets it to PAUSED
                G_Remaining_Seconds = 0; 
                G_Timer_State = TIMER_PAUSED;
            }
            sei();
            
            update_display(G_Remaining_Seconds, G_Timer_State);
            update_led(G_Timer_State);
        }
        
        // --- 2. Handle ADD 5s Request (PC4) ---
        if (G_PC4_Request)
        {
            G_PC4_Request = false; // Consume request
            
            // Critical Section: Protect shared seconds counter
            cli();
            G_Remaining_Seconds += TIME_ADD_SECONDS; // **Add time**
			G_Status_Changed = true;
            
            // If we add time while expired, we reset the state to PAUSED
            if (G_Timer_State == TIMER_EXPIRED) {
                G_Timer_State = TIMER_PAUSED;
                G_ms_Accumulator = 0; // Clear ms counter too
            }
            sei();
        }
		
        // --- 3. Handle Timer Expiration or Second Update (ISR Trigger) ---
		
        // Check for state changes set by the ISR
        if (G_Timer_State == TIMER_EXPIRED)
        {
			if (G_Status_Changed)
			{
				// Reading the time is not strictly atomic here since it's checked right after
				// the flag is set, but cli/sei on the read ensures safety.
				cli();
				G_Status_Changed = false;
				sei();
				
				update_display(0, G_Timer_State);
				update_led(G_Timer_State);
			}
        }
        else 
		{
			if (G_Status_Changed) 
			{
				cli();
				G_Status_Changed = false; 
				sei();

				update_display(G_Remaining_Seconds, G_Timer_State);
				update_led(G_Timer_State);
			}
		}
    }
}