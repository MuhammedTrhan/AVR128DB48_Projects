/*
 * Praktikum_5.3.c
 *
 * Created: 11/16/2025 1:41:26 PM
 * Author : mami4
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdint.h>

// Define the clock frequency (4 MHz)
#define F_CPU 4000000UL 

// --- Configuration Constants ---
#define MS_PER_SECOND 1000
#define MS_DEBOUNCE_MAX 10 // 10ms for stable button reading

// Traffic Light Phase Durations (in milliseconds)
#define RED_TIME_MS     4000 // 4 seconds
#define YELLOW_TIME_MS  2000 // 2 seconds
#define GREEN_TIME_MS   3000 // 3 seconds

// --- State Definitions ---
typedef enum {
    STATE_RED,
    STATE_RED_TO_GREEN_YELLOW, // Red -> Yellow phase
    STATE_GREEN,
    STATE_GREEN_TO_RED_YELLOW  // Green -> Yellow phase
} traffic_state_t;

// --- Global Volatile Variables ---

// **State Variables**
volatile traffic_state_t G_Traffic_State = STATE_RED; // Initial state
volatile uint32_t G_Time_Remaining_ms = RED_TIME_MS; // Time remaining in current phase

// **Button Debounce Variables (PC4)**
volatile bool G_PC4_Stable_State = false; 
volatile uint8_t G_PC4_Debounce_Counter = 0;

// **Button Debounce Variables (PC5)**
volatile bool G_PC5_Stable_State = false;
volatile uint8_t G_PC5_Debounce_Counter = 0;

// **Request Flags**
volatile bool G_Request_Green = false; // Set when PC4 is pressed
volatile bool G_Request_Red = false;   // Set when PC5 is pressed

// --- Hardware Control Macros ---

// Note: A real traffic light uses R, Y, G. We simulate Yellow (Y) using R+G (PE0+PE1).
#define LED_OFF()        (PORTE.OUTCLR = PIN0_bm | PIN1_bm)
#define LED_RED_ON()     (LED_OFF(), PORTE.OUTSET = PIN0_bm)        // PE0 (Red)
#define LED_YELLOW_ON()  (LED_OFF(), PORTE.OUTSET = PIN0_bm | PIN1_bm) // PE0 (Red) + PE1 (Green) = Yellow
#define LED_GREEN_ON()   (LED_OFF(), PORTE.OUTSET = PIN1_bm)        // PE1 (Green)

// --- Function Declarations ---
static void update_traffic_light_output(void);
static void handle_button_debounce(uint8_t pin_value, volatile bool *stable_state, volatile uint8_t *counter, bool is_pc4);

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

// --- Interrupt Service Routines ---

/**
 * @brief Timer/Counter B0 (TCB0) Interrupt Service Routine (1ms tick).
 * * Handles all timing and button debouncing logic.
 */
ISR(TCB0_INT_vect)
{
	TCB0.INTFLAGS = TCB_CAPT_bm; // Clear the interrupt flag
	
	// Handle PC4 (Request GREEN)
	handle_button_debounce((PORTC.IN & PIN4_bm), 
	                       &G_PC4_Stable_State, 
	                       &G_PC4_Debounce_Counter, 
	                       true); // is_pc4 = true
	
	// Handle PC5 (Request RED)
	handle_button_debounce((PORTC.IN & PIN5_bm), 
	                       &G_PC5_Stable_State, 
	                       &G_PC5_Debounce_Counter, 
	                       false); // is_pc4 = false
	
	// --- 2. Timing Logic (Executed every 1ms) ---
	if (G_Time_Remaining_ms > 0)
	{
		G_Time_Remaining_ms--;
	}
	else // Time for current phase has expired
	{
		// Update state machine
		switch (G_Traffic_State)
		{
			case STATE_RED:
				// Should only transition if requested
				break; 
				
			case STATE_RED_TO_GREEN_YELLOW: // R -> Y -> G
				G_Traffic_State = STATE_GREEN;
				G_Time_Remaining_ms = GREEN_TIME_MS;
				break;
				
			case STATE_GREEN:
				// Should only transition if requested
				break;
				
			case STATE_GREEN_TO_RED_YELLOW: // G -> Y -> R
				G_Traffic_State = STATE_RED;
				G_Time_Remaining_ms = RED_TIME_MS;
				break;
		}
		
		// Update the LED output immediately after state change
		update_traffic_light_output();
	}
}

/**
 * @brief Generic debounce and rising edge detection logic for one button.
 * The ISR sets the appropriate G_Request_X flag upon a stable rising edge.
 */
static void handle_button_debounce(uint8_t pin_value, volatile bool *stable_state, volatile uint8_t *counter, bool is_pc4)
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
			
			// 1. Check for RISING EDGE (Transition from stable low to stable high)
			if (current_raw_state == true)
			{
				if (is_pc4)
				{
					G_Request_Green = true;
				}
				else // PC5
				{
					G_Request_Red = true;
				}
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

// --- Hardware and State Functions ---

/**
 * @brief Sets the RGB LED output based on the current traffic state.
 */
static void update_traffic_light_output(void)
{
    switch (G_Traffic_State)
    {
        case STATE_RED:
            LED_RED_ON();
            break;
            
        case STATE_RED_TO_GREEN_YELLOW:
            LED_YELLOW_ON(); // R+G
            break;
            
        case STATE_GREEN:
            LED_GREEN_ON();
            break;
            
        case STATE_GREEN_TO_RED_YELLOW:
            LED_YELLOW_ON(); // R+G
            break;
    }
}

/**
 * @brief Handles the application logic for button requests.
 */
static void handle_requests(void)
{
    // --- Request to go GREEN (PC4) ---
    if (G_Request_Green)
    {
        G_Request_Green = false; // Consume the request
        
        // Only start transition if currently RED
        if (G_Traffic_State == STATE_RED && G_Time_Remaining_ms == 0)
        {
            // Transition: RED -> RED_TO_GREEN_YELLOW
            G_Traffic_State = STATE_RED_TO_GREEN_YELLOW;
            G_Time_Remaining_ms = YELLOW_TIME_MS;
            update_traffic_light_output();
        }
    }
    
    // --- Request to go RED (PC5) ---
    if (G_Request_Red)
    {
        G_Request_Red = false; // Consume the request
        
        // Only start transition if currently GREEN
        if (G_Traffic_State == STATE_GREEN && G_Time_Remaining_ms == 0)
        {
            // Transition: GREEN -> GREEN_TO_RED_YELLOW
            G_Traffic_State = STATE_GREEN_TO_RED_YELLOW;
            G_Time_Remaining_ms = YELLOW_TIME_MS;
            update_traffic_light_output();
        }
    }
}


int main(void)
{
	// --- Port Configuration ---
	PORTC.DIRCLR = PIN4_bm | PIN5_bm; // Set PC4 and PC5 as inputs (buttons)
	PORTC.PIN4CTRL = 0x00;           // No internal pull-ups (assuming external pull-downs)
	PORTC.PIN5CTRL = 0x00;
	
	// Set PE0 (R), PE1 (G) as output (RGB LED)
	PORTE.DIRSET = PIN0_bm | PIN1_bm;
	
	// Start in RED state
	LED_RED_ON();
	
	timer_init();
	sei(); // Enable global interrupts
	
    while (1) 
    {
        // The main loop continuously checks and processes button requests.
        // This keeps the ISR minimal and handles the state transitions safely.
        handle_requests();
		
		// The MCU spends most of its time in the idle loop, waiting for the 1ms TCB0 interrupt.
    }
}