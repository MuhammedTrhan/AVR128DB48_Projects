/*
 * Praktikum_7.1.c
 *
 * ADC /w potentiometer
 *
 * Created: 12/5/2025 5:09:17 PM
 * Author : mami4
 */ 

#define F_CPU 4000000UL													// Set CPU Frequency to 4MHz

#include <avr/io.h>
#include <avr/interrupt.h>												// Required for ISR usage
#include <stdio.h>														// Used for handling the strings
#include <stdbool.h>													// Used for bool variables
#include "I2C_LCD.h"

// Global Variables
volatile uint16_t Adc_Result = 0;										// Stores the latest ADC value
volatile bool Result_Ready = false;										// Flag to tell Main that data is ready

void ADC0_init(void)
{
	// Configure Pin PF3
	// Disable digital input buffer
	PORTF.PIN3CTRL &= ~PORT_ISC_gm;										// Clear all ISC (Input/Sense Configuration) bits
	PORTF.PIN3CTRL |= PORT_ISC_INPUT_DISABLE_gc;						// Disable the digital input buffer
	
	// Configure VREF
	VREF.ADC0REF = VREF_REFSEL_VDD_gc;									// Set ADC0 reference to VDD (3.3V for us)
	
	// Configure ADC Control C (Prescaler)
	ADC0.CTRLC = ADC_PRESC_DIV4_gc;										// ADC Clock = 1MHz
	
	// Configure MUX (Input Selection)
	ADC0.MUXPOS = ADC_MUXPOS_AIN19_gc;									// Select AIN19 (which corresponds to Pin PF3)
	
    // Enable ADC Interrupt
    ADC0.INTCTRL = ADC_RESRDY_bm;										// Set the Result Ready Interrupt Enable bit

	ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_12BIT_gc;					// Enable ADC and set 12-bit Resolution
}

// Interrupt Service Routine
ISR(ADC0_RESRDY_vect)													// It does not make any sense since we don't do anything if the result is not ready yet
{																		// but it is forbidden to use blocking delays
    Adc_Result = ADC0.RES;												// Read the RES register. It automatically clears the interrupt flag
    
    Result_Ready = true;												// Set the flag to let main know we have new data
}

int main(void)
{
    char Text[17];														// Text for LCD text
	uint16_t Adc_Prev_Result = 0xFFFF;									// Initialize with a value that ensures first update happens
    uint32_t Voltage_Mv = 0;											// Voltage in millivolts
    uint16_t Percentage = 0;											// Percentage value
    
    ADC0_init();														// 1. Initialize Peripherals
    lcd_init();															// 2. Initialize LCD
    lcd_clear();
    
    sei();																// Enable Global Interrupts
    
    // Start the FIRST conversion manually
    ADC0.COMMAND = ADC_STCONV_bm;
    
    while (1) 
    {
        // Check if the ISR has given us new data
        if (Result_Ready)
        {
            // Reset the flag immediately
            Result_Ready = false;
            
			if (Adc_Result != Adc_Prev_Result)							// Handle the hard job if the result has been changed
			{
                Adc_Prev_Result = Adc_Result;                           // Update previous result
                
				// Vref = 3300mV, Resolution = 4096 (12-bit)
				Voltage_Mv = ((uint32_t)Adc_Result * 3300) / 4095;		// V = (ADC * Vref) / Resolution
				
				Percentage = ((uint32_t)Adc_Result * 100) / 4095;		// Perc = (ADC * 100) / Resolution
				

				// Edit the first line text
				sprintf(Text, "Volt: %lu.%lu V   ", Voltage_Mv / 1000, (Voltage_Mv % 1000) / 100);
					
				lcd_moveCursor(0, 0);									// Move the cursor to line 1
				lcd_putString(Text);									// Print Voltage: "Volt: x.y V"

				// Edit the second line text
				sprintf(Text, "Perc: %u %%    ", Percentage);
					
				lcd_moveCursor(0, 1);									// Move the cursor to line 2
				lcd_putString(Text);									// Print Percentage: "Perc: x %"
			}
            
            // Start the NEXT conversion
            ADC0.COMMAND = ADC_STCONV_bm;
        }
    }
}