/*
 * Praktikum_6.2.c
 *
 * Created: 11/23/2025 6:12:14 PM
 * Author : mami4
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>


#define F_CPU 4000000UL											// Period is 0.25us

// Servo Position Definitions
#define NEUTRAL_PULSE_us	1500U								// 1500us or 1.5ms
#define LEFT_PULSE_us		1000U
#define RIGHT_PULSE_us		2000U

// Step size in microseconds per 20ms signal period.
// The higher the value, the faster the sweep.
#define STEP_SIZE_us		10U									// 10us step for 1000us range takes 2.0 seconds to traverse.

// PWM Timing
#define PWM_PERIOD_us		20000U								// The Servo expects a signal frame every 20ms (50Hz)

// Timer Calculation for TCB0 @ F_CPU/2 (2 MHz)
#define CYCLES_PER_us		2U									// 1 clock cycle = 0.5us (because of the prescaller of 2). So, cycles per us is 2.

#define CYCLES_PER_SIGNAL (PWM_PERIOD_us * CYCLES_PER_us)		// The clock makes 40,000 cycles per signal frame (20ms)

// Pin Definition for PF4
#define SERVO_PIN_bm      PIN4_bm
#define SERVO_PORT        PORTF

// Global State Variables
volatile uint32_t G_Current_Pulse_Cycles;						// Number of the cycles we need to set PF4 high
volatile bool G_State_Changed = true;							// Change every 20ms in ISR

/**
 * @brief Initializes the TCB0 Timer.
 */
void initialize_tcb0() {
    SERVO_PORT.DIRSET = SERVO_PIN_bm;							// Configure PF4 as Output
    SERVO_PORT.OUTCLR = SERVO_PIN_bm;							// Start LOW

    // Initial Pulse Setting (Start at neutral state)
     G_Current_Pulse_Cycles = NEUTRAL_PULSE_us * CYCLES_PER_us;

    // Configure TCB0 to Enable Capture/Timeout Interrupt
    TCB0.INTCTRL = TCB_CAPT_bm; 
    
    // Set Mode: Periodic Interrupt (Default)
    // Set Clock: CLK_PER / 2 (0x1)
    TCB0.CTRLA = (1 << TCB_CLKSEL_gp) | TCB_ENABLE_bm; 
    
    // Trigger the first interrupt immediately
    TCB0.CCMP = 100; 
    TCB0.CNT = 0;
}

/**
 * @brief Sets the servo position safely
 */
void set_servo_position(uint16_t Pulse_us) {
    uint32_t Cycles = (uint32_t)Pulse_us * CYCLES_PER_us;		// Calculate the number of cycles we need to keep PF4 high
    cli(); 
    G_Current_Pulse_Cycles = Cycles;
    sei();
}

/**
 * @brief ISR for TCB0. Generates Software PWM and counts system ticks.
 */
ISR(TCB0_INT_vect) {
    TCB0.INTFLAGS = TCB_CAPT_bm;								// Clear flag

    if (SERVO_PORT.OUT & SERVO_PIN_bm) {
        // CASE: End of Pulse (Pin was HIGH)
        SERVO_PORT.OUTCLR = SERVO_PIN_bm;						// Set PF4 LOW
        // Wait for remainder of 20ms period
        TCB0.CCMP = CYCLES_PER_SIGNAL - G_Current_Pulse_Cycles;
    } 
    else {
        // CASE: Start of Period (Pin was LOW)
        SERVO_PORT.OUTSET = SERVO_PIN_bm;						// Set PF4 HIGH
        
        // Count this as one "tick" (20ms passed)
        G_State_Changed = true;

        // Wait for Pulse Duration
        TCB0.CCMP = G_Current_Pulse_Cycles;
    }
}

/**
 * @brief Main function.
 * Executes an infinite, continuous sweep from Left to Right and back.
 */
int main(void) {
    initialize_tcb0();
    sei();

    // Start at the neutral position
    uint16_t Current_Position_us = NEUTRAL_PULSE_us;
    
    // Direction flag: 1 = Moving Right (Increasing), -1 = Moving Left (Decreasing)
    int8_t direction = 1; 

    while (1) {
        // Only update position if a new frame (tick) has occurred
        if (G_State_Changed) {
            G_State_Changed = false;							// Immediately change the flag

            // Calculate new position
            if (direction == 1) {
                Current_Position_us += STEP_SIZE_us;
                if (Current_Position_us >= RIGHT_PULSE_us) {
                    Current_Position_us = RIGHT_PULSE_us;
                    direction = -1; // Reverse direction to Left
                }
            } else {
                Current_Position_us -= STEP_SIZE_us;
                if (Current_Position_us <= LEFT_PULSE_us) {
                    Current_Position_us = LEFT_PULSE_us;
                    direction = 1; // Reverse direction to Right
                }
            }

            // Apply the new position to the servo
            set_servo_position(Current_Position_us);
        }
    }
}