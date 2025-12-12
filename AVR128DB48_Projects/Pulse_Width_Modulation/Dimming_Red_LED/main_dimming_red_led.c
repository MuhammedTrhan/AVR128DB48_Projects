/*
 * Praktikum_6.1.c
 *
 * Created: 11/23/2025 2:41:40 PM
 * Author : mami4
 */ 

#include <avr/io.h>
#include <stdint.h>
#include <math.h>											// math.h library is used for floating point.

// Define the CPU frequency. AVR128DB48 defaults to 4MHz.
#define F_CPU 4000000UL 

// Setting brightness percentage (0 to 100).
#define TARGET_BRIGHTNESS 25U 


/**
 * @brief Sets the brightness of the Red LED (PD0) using TCA0 PWM.
 * Function to adjust brightness.
 * @param percentage The desired brightness level (0 to 100).
 */
void set_red_led_brightness(uint8_t percentage) {
    // Ensuring it is between 0-100%
    if (percentage > 100)
    {
	    percentage = 100;
    }
    
	double compare_val;
	    
    // Calculate the Compare Value (CMP0).
    // (Percentage / 100) * 256 - 1
    compare_val = ( (double)percentage / 100.0 ) * 256.0 - 1;
	
	// Safety check
	if (percentage == 0) {
		compare_val = 0;
	}
    
    // Update the Compare register
    TCA0.SINGLE.CMP0 = (uint8_t)compare_val;
}


/**
 * @brief Initializes the Red LED on PD0 for PWM control using TCA0.
 * The Red LED is connected to PD0, which is the WO0 output of TCA0.
 */
void initialize_pwm() {
    // Configure Port Multiplexer (PORTMUX)
    // TCA0 outputs to PORTD (WO0 -> PD0)
    PORTMUX.TCAROUTEA &= ~PORTMUX_TCA0_gm; 
    PORTMUX.TCAROUTEA |= PORTMUX_TCA0_PORTD_gc; 

    PORTD.DIRSET = PIN0_bm;									// Set PD0 as output

    // Configure TCA0 Timer/Counter for Single-Slope PWM
    TCA0.SINGLE.CTRLA = 0;									// Disable TCA0 before configuration
    TCA0.SINGLE.CTRLESET = TCA_SINGLE_CMD_RESET_gc;			// Software reset
    
    // Set the Period for 8-bit resolution (0 to 255)
    TCA0.SINGLE.PER = 255; 

    // Set the brightness
    set_red_led_brightness(TARGET_BRIGHTNESS);

    // Enable the Compare Channel 0 (CMP0) Output (WO0) and set Waveform Generation Mode
    // DSBOTTOM is the reliable Single-Slope PWM mode for variable output.
    TCA0.SINGLE.CTRLB = TCA_SINGLE_CMP0EN_bm | TCA_SINGLE_WGMODE_DSBOTTOM_gc;  

    // Enable the Timer
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc |			// Set clock source (F_CPU / 1)
                        TCA_SINGLE_ENABLE_bm;				// enable TCA0
}


int main(void) {
    // Initialize the PWM hardware
    initialize_pwm();

    while (1) {
        // PWM runs in the background.
    }
}