/*
 * Praktikum_7.5.c
 *
 * Receive RGB w/ USART
 *
 * Created: 12/7/2025 7:51:21 PM
 * Author : mami4
 */ 
#define F_CPU 4000000UL																// 4MHz
#define BAUD_RATE 9600

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>																	// For strtol()
#include <ctype.h>																	// For isxdigit()

// Global variables
volatile char Rx_Buffer[8];															// Buffer to hold incoming Hex String (RRGGBB + null terminator)
volatile uint8_t Rx_Index = 0;

// RGB led controller
void set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
	TCA0.SINGLE.CMP0 = r;
	TCA0.SINGLE.CMP1 = g;
	TCA0.SINGLE.CMP2 = b;
}

void Process_Color_Command(void)
{
	char hex_str[4];
	long red, green, blue;
	
	// Parse Red (First 2 chars)
	hex_str[0] = Rx_Buffer[0];
	hex_str[1] = Rx_Buffer[1];
	hex_str[2] = '\0';
	red = strtol(hex_str, NULL, 16);
	
	// Parse Green (Middle 2 chars)
	hex_str[0] = Rx_Buffer[2];
	hex_str[1] = Rx_Buffer[3];
	green = strtol(hex_str, NULL, 16);
	
	// Parse Blue (Last 2 chars)
	hex_str[0] = Rx_Buffer[4];
	hex_str[1] = Rx_Buffer[5];
	blue = strtol(hex_str, NULL, 16);
	
	// Update LED
	set_rgb((uint8_t)red, (uint8_t)green, (uint8_t)blue);
}

void TCA0_init(void)
{
    // Configure Port Multiplexer (PORTMUX)
    // TCA0[2:0] (bits 2:0) = 0x4 routes TCA0 outputs to PORTE (WO0->PE0, WO1->PE1, WO2->PE2).
    PORTMUX.TCAROUTEA &= ~PORTMUX_TCA0_gm;
    PORTMUX.TCAROUTEA |= PORTMUX_TCA0_PORTE_gc;

    PORTE.DIRSET = PIN0_bm | PIN1_bm | PIN2_bm;										// Set PE0, PE1, and PE2 as outputs
	
	// Configure Timer for Single Slope PWM (8-bit)
	TCA0.SINGLE.CTRLA = 0;															// Disable first
	TCA0.SINGLE.CTRLESET = TCA_SINGLE_CMD_RESET_gc;
	
	TCA0.SINGLE.PER = 255;															// Period = 255 (Max value for 8-bit color)
	
	// Enable Output Channels (Compare 0, 1, 2)
	TCA0.SINGLE.CTRLB = TCA_SINGLE_CMP0EN_bm | TCA_SINGLE_CMP1EN_bm | TCA_SINGLE_CMP2EN_bm | TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
	
	set_rgb(0, 0, 0);																// Set Initial Color (Off)
	
	// Start Timer
	// Clock = System Clock (4MHz) / 1 = 4MHz
	// PWM Frequency = 4MHz / 256 ~= 15.6 kHz
	TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm;
}

void USART3_init(void)
{
	// Use default pins PB0/PB1
	PORTMUX.USARTROUTEA = 0;														//PB0=TX, PB1=RX
	
	// Baud Rate Calculation
	USART3.BAUD = (uint16_t)((float)(4UL * F_CPU) / BAUD_RATE);
	
	// Set Pins (PB1 = RX)
	PORTB.DIR &= ~PIN1_bm;															// RX is Input (Default, but good to be explicit)
	
	// Enable Receiver Only and RX Interrupt
	USART3.CTRLC = USART_CHSIZE_8BIT_gc | USART_PMODE_DISABLED_gc | USART_SBMODE_1BIT_gc;
	USART3.CTRLB |= USART_RXEN_bm;
	USART3.CTRLA |= USART_RXCIE_bm;													// enable RX interrupt
}

// Interrupt Service Routine
/*
 * UART Receive Complete Interrupt
 * Fires whenever a character arrives from the PC.
 */
ISR(USART3_RXC_vect)
{
    char Rx_Char = USART3.RXDATAL;

    if (isxdigit(Rx_Char))
    {
	    // Add valid hex char to buffer
	    Rx_Buffer[Rx_Index++] = Rx_Char;
	    
	    // AUTO-TRIGGER: If we have 6 digits, process immediately!
	    if (Rx_Index == 6)
	    {
		    Rx_Buffer[6] = '\0';
		    Process_Color_Command();
		    Rx_Index = 0;															// Reset for next color
	    }
    }
    else
    {
	    // Received something weird (Space, Newline, 'x', etc.)
	    // Treat this as a "Clear/Reset" command to sync up
	    Rx_Index = 0;
    }
}

int main(void)
{
    TCA0_init();
    USART3_init();
    
    sei();																			// Enable Interrupts
    
    while (1) 
    {
        // Everything is handled by Hardware PWM and RX Interrupts.
    }
}