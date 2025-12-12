/*
 * Praktikum_7.3.c
 *
 * Pressed buttons w/ USART
 *
 * Created: 12/7/2025 3:17:44 PM
 * Author : mami4
 */ 

#define MS_DEBOUNCE_MAX 10															// 10ms for stable button reading
#define F_CPU 4000000UL																// CPU Frequency 4MHz
#define BAUD_RATE 9600																// Target Baud Rate

// Ring buffer
#define TX_BUFFER_SIZE 128															// Size of the queue (Must be power of 2 for efficiency)
#define TX_BUFFER_MASK (TX_BUFFER_SIZE - 1)											// When tail hit 128 -> 1000 0000 & 0111 1111 = 0000 0000. This creates a circle

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

volatile uint8_t Button_Pressed_Flag = 0;											// Global variable to track which pin is pressed on port C.

// **Button debounce variables
volatile bool G_PC4_Stable_State = false;
volatile uint8_t G_PC4_Debounce_Counter = 0;

volatile bool G_PC5_Stable_State = false;
volatile uint8_t G_PC5_Debounce_Counter = 0;

volatile bool G_PC6_Stable_State = false;
volatile uint8_t G_PC6_Debounce_Counter = 0;

volatile bool G_PC7_Stable_State = false;
volatile uint8_t G_PC7_Debounce_Counter = 0;
// End of button debounce variables**

// UART Ring Buffer Variables
volatile char Tx_Buffer[TX_BUFFER_SIZE];
volatile uint8_t Tx_Head = 0;														// Index where we write/receive data
volatile uint8_t Tx_Tail = 0;														// Index where we read/send data

/**
 * @brief Initializes Timer/Counter B0 (TCB0) for 1ms periodic interrupts.
 */
void timer_init(void)
{
	TCB0.CCMP = 3999; 
	TCB0.CTRLA = TCB_CLKSEL_DIV1_gc; 
	TCB0.CTRLB = TCB_CNTMODE_INT_gc; 
	TCB0.INTCTRL = TCB_CAPT_bm;      
	TCB0.CTRLA |= TCB_ENABLE_bm; 
}

/**
 * @brief Generic debounce logic. It assigns the Button_Pressed_Flag with the button presssed.
 *
 * @param Pin_Value helps us to indicate if the button that calls the function pressed or not.
 * @param Stable_State is the boolean indicates the current stable state of the button that calls the function.
 * @param Counter is the debounce counter of the button that calls the function.
 * @param Button_Id is the bit position of the button that calls the function.
 */
static void handle_button_debounce(uint8_t Pin_Value, volatile bool *Stable_State, volatile uint8_t *Counter, uint8_t Button_Id)
{
	bool Current_Raw_State = (Pin_Value != 0);

	if (Current_Raw_State != *Stable_State)
	{
		if (*Counter < MS_DEBOUNCE_MAX) (*Counter)++;
		
		if (*Counter >= MS_DEBOUNCE_MAX)
		{
			if (Current_Raw_State == true)
			{
				Button_Pressed_Flag = Button_Id;
			}
			*Stable_State = Current_Raw_State;
			*Counter = 0;
		}
	}
	else
	{
		*Counter = 0;
	}
}

// Interrupt Service Routine
ISR(TCB0_INT_vect)
{
	TCB0.INTFLAGS = TCB_CAPT_bm; 
	uint8_t Port_In = PORTC.IN;														// Read the port just once
	
	handle_button_debounce((Port_In & PIN4_bm), &G_PC4_Stable_State, &G_PC4_Debounce_Counter, 4);
	handle_button_debounce((Port_In & PIN5_bm), &G_PC5_Stable_State, &G_PC5_Debounce_Counter, 5);
	handle_button_debounce((Port_In & PIN6_bm), &G_PC6_Stable_State, &G_PC6_Debounce_Counter, 6);
	handle_button_debounce((Port_In & PIN7_bm), &G_PC7_Stable_State, &G_PC7_Debounce_Counter, 7);
}

/*
 * UART Data Register Empty Interrupt (Consumer)
 * This fires whenever the hardware is ready to accept a new byte.
 */
ISR(USART3_DRE_vect)
{
    // Check if there is data in the buffer
    if (Tx_Head != Tx_Tail)
    {
        // 1. Take byte from the Tail (Oldest data)
        // 2. Use Mask logic to wrap around (e.g. 127 -> 0)
        Tx_Tail = (Tx_Tail + 1) & TX_BUFFER_MASK;
        
        // 3. Send the byte to hardware
        USART3.TXDATAL = Tx_Buffer[Tx_Tail];
    }
    else
    {
        // Buffer is empty. Disable interrupt to stop firing.
        USART3.CTRLA &= ~USART_DREIE_bm;
    }
}

void USART3_init(void)
{
    USART3.BAUD = (uint16_t)((float)(4UL * F_CPU) / BAUD_RATE);
    PORTB.DIR |= PIN0_bm;
    USART3.CTRLB = USART_TXEN_bm;
}

void buttons_init(void)
{
    PORTC.PINCONFIG = PORT_ISC_INTDISABLE_gc; 
    PORTC.PINCTRLUPD = 0xF0;
}

/*
 * UART Send String (Producer)
 * Adds the string to the Ring Buffer and enables the interrupt.
 */
void USART3_send_string(const char *str)
{
    // Loop through every character in the string
    while (*str)																	// Check if the character is Null
    {
        // Calculate where the next byte would go
        uint8_t next_head = (Tx_Head + 1) & TX_BUFFER_MASK;

        // Check if Buffer is Full
        if (next_head != Tx_Tail)
        {
            // There is room!
            Tx_Head = next_head;
            Tx_Buffer[Tx_Head] = *str++;											// Copy char to buffer
        }
        else
        {
            // Buffer is full! Drop the data (since busy waiting is not an option)
            break; 
        }
    }
    
    // Trigger the ISR to start sending (if it wasn't already)
    USART3.CTRLA |= USART_DREIE_bm;
}

int main(void)
{
    USART3_init();
    buttons_init();
    timer_init();
    sei();

    while (1) 
    {
        if (Button_Pressed_Flag != 0)
        {
            // Messages are being added to the queue!
            switch (Button_Pressed_Flag)
            {
                case 4:
                    USART3_send_string("Button C4 Pressed!\r\n");
                    break;
                case 5:
                    USART3_send_string("Button C5 Pressed!\r\n");
                    break;
                case 6:
                    USART3_send_string("Button C6 Pressed!\r\n");
                    break;
                case 7:
                    USART3_send_string("Button C7 Pressed!\r\n");
                    break;
            }
            Button_Pressed_Flag = 0;
        }
    }
}