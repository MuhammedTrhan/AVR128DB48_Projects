/*
 * Praktikum_6.2.c
 *
 * Created: 11/23/2025 3:57:29 PM
 * Author : mami4
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define F_CPU 4000000UL 

// Configuration Constants
#define PWM_PERIOD      255U									// 8-bit resolution for PWM (0-255)
#define MAX_BRIGHTNESS  255U

// TCB0 configuration for 50 ms delay
// F_CPU = 4,000,000 Hz
// Timer Frequency = 4,000,000 / 8 = 500,000 Hz. (prescaler is 8)
// 500,000 cycles/sec. For 50 ms (0.05 s), we need: 500,000 * 0.05 = 25,000 cycles.
#define TCB_PER_VALUE   25000U									// TCB0 CCMP register value
#define HUE_STEP_SIZE   5										// How fast to ramp the colors (0-255)

// Global State Variables
volatile uint8_t Red_Val = MAX_BRIGHTNESS;
volatile uint8_t Green_Val = 0;
volatile uint8_t Blue_Val = 0;
volatile uint8_t Phase = 0;										// Tracks which color transition we are in (0-2)

/**
 * @brief Sets the duty cycles for the RGB LED using TCA0 Compare Registers.
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 */
void set_rgb_brightness(uint8_t r, uint8_t g, uint8_t b) {
    // TCA0 WO0 -> PE0 (Red)
    TCA0.SINGLE.CMP0 = r;
    // TCA0 WO1 -> PE1 (Green)
    TCA0.SINGLE.CMP1 = g;
    // TCA0 WO2 -> PE2 (Blue)
    TCA0.SINGLE.CMP2 = b;
}

// Initializes the Timer for interrupt every 50 ms.
void initialize_timer() {
    // Set the Period for 50 ms (25000 cycles)
    TCB0.CCMP = TCB_PER_VALUE;

    // Enable interrupt for Capture/Compare (CMP)
    TCB0.INTCTRL = TCB_CAPT_bm;

    // Configure TCB0 to run in Periodic Interrupt Timer mode
    // 0x2 corresponds to CLKDIV8
    TCB0.CTRLA = (TCB_CLKSEL_gp & 0x2) | TCB_ENABLE_bm;
}

// Initializes the three-channel PWM output for RGB
void initialize_pwm() {
    // Configure Port Multiplexer (PORTMUX)
    // TCA0[2:0] (bits 2:0) = 0x4 routes TCA0 outputs to PORTE (WO0->PE0, WO1->PE1, WO2->PE2).
    PORTMUX.TCAROUTEA &= ~PORTMUX_TCA0_gm;
    PORTMUX.TCAROUTEA |= PORTMUX_TCA0_PORTE_gc; 

    PORTE.DIRSET = PIN0_bm | PIN1_bm | PIN2_bm;					// Set PE0, PE1, and PE2 as outputs	

    // Configure TCA0 Timer/Counter for Single-Slope PWM
    TCA0.SINGLE.CTRLA = 0;										// Disable TCA0
    TCA0.SINGLE.CTRLESET = TCA_SINGLE_CMD_RESET_gc;				// Software reset
    
    // Set the Period for 8-bit resolution
    TCA0.SINGLE.PER = PWM_PERIOD; 

    // Set initial values (Red=100%, Green=0%, Blue=0%)
    set_rgb_brightness(Red_Val, Green_Val, Blue_Val);

    TCA0.SINGLE.CTRLB = TCA_SINGLE_CMP0EN_bm |					// Enable Red output (WO0/PE0)
                        TCA_SINGLE_CMP1EN_bm |					// Enable Green output (WO1/PE1)
                        TCA_SINGLE_CMP2EN_bm |					// Enable Blue output (WO2/PE2)
                        TCA_SINGLE_WGMODE_DSBOTTOM_gc;			// Single-Slope PWM mode

    // Enable the Timer
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc |				// Select clock (No prescaler)
                        TCA_SINGLE_ENABLE_bm;					// Enable TCA0
}

/**
 * @brief Timer/Counter Type B0 Interrupt Service Routine (ISR).
 * This ISR runs every 50 ms (25000 cycles) and handles the color transition logic.
 */
ISR(TCB0_INT_vect) {
    // Clear the interrupt flag (W1C - Write 1 to Clear)
    TCB0.INTFLAGS = TCB_CAPT_bm;

    // The logic follows the standard Hue color wheel transitions (6 Phases):
    // 0: Red -> Green (G goes up & R goes down)
    // 1: Green -> Blue (B goes up & G goes down)
    // 2: Blue -> Red (R goes up & B goes down)
    
    switch (Phase) {
        case 0: // Red -> Green (G goes up & R goes down)
            if (Green_Val < MAX_BRIGHTNESS && Red_Val >= HUE_STEP_SIZE) {
                Green_Val += HUE_STEP_SIZE;
				Red_Val -= HUE_STEP_SIZE;
            } else {
                Phase = 1;
            }
            break;
        case 1: // Green -> Blue (B goes up & G goes down)
            if (Blue_Val < MAX_BRIGHTNESS && Green_Val >= HUE_STEP_SIZE) {
                Blue_Val += HUE_STEP_SIZE;
				Green_Val -= HUE_STEP_SIZE;
            } else {
                Phase = 2;
            }
            break;
        case 2: // Blue -> Red (R goes up & B goes down)
            if (Red_Val < MAX_BRIGHTNESS && Blue_Val >= HUE_STEP_SIZE) {
                Red_Val += HUE_STEP_SIZE;
				Blue_Val -= HUE_STEP_SIZE;
            } else {
                Phase = 0;
            }
            break;
    }
    
    // Update the hardware registers with the new values
    set_rgb_brightness(Red_Val, Green_Val, Blue_Val);
}


int main(void) {
    // Initialize the three PWM channels for the RGB LED
    initialize_pwm();

    // Initialize the TCB0 for interrupts
    initialize_timer();

    // Enable Global Interrupts
    sei();

    while (1) { }
}