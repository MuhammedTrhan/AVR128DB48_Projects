/*
 * Praktikum_7.4.c
 *
 * Internal temperature w/ USART (Strictly Non-Blocking)
 *
 * Created: 12/7/2025 6:47:59 PM
 * Author : mami4
 */

#define F_CPU 4000000UL                                                                 // CPU Frequency 4MHz
#define BAUD_RATE 9600                                                                  // UART Baud Rate
#define ONE_SECOND_MS 1000                                                              // 1000ms = 1 Second

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// **Global flags & data
// Timer Flags
volatile uint16_t G_Time_Remaining_ms = ONE_SECOND_MS;
volatile bool G_Start_Adc_Flag = false;												// Flag to trigger a new measurement
volatile uint32_t G_Total_Time_s = 0;

// ADC Data
volatile uint16_t G_Adc_Raw_Result = 0;												// Stores the result from ISR
volatile bool G_New_Data_Available = false;											// Flag to tell main "Result is ready"
// End of global flags & data**

// Factory Calibration Value
uint16_t Sigrow_ADC_Cal_Val = 0;

// UART Variables
volatile const char *Tx_Ptr = NULL;
volatile bool Tx_Busy = false;

// **Initialization functions
void ADC0_init(void)
{
    // Vref: Internal 1.024V
    VREF.ADC0REF = VREF_REFSEL_1V024_gc;
    
    // Prescaler: 4MHz / 4 = 1MHz ADC Clock
    ADC0.CTRLC = ADC_PRESC_DIV4_gc;
    
    // MUX: Select Internal Temperature Sensor
    ADC0.MUXPOS = ADC_MUXPOS_TEMPSENSE_gc;
    
    // Sample Control (High Impedance Sensor needs longer time)
    ADC0.SAMPCTRL = 31;
    
    // Init Delay
    ADC0.CTRLD = ADC_INITDLY_DLY32_gc;
    
    // Enable Interrupts for "Result Ready"
    // This allows us to remove the blocking while loop!
    ADC0.INTCTRL = ADC_RESRDY_bm;
    
    // Enable ADC (12-bit)
    ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_12BIT_gc;
}

void Timer_init(void)
{
    // TCB0 setup for 1ms interrupt
    TCB0.CCMP = 3999;
    TCB0.CTRLA = TCB_CLKSEL_DIV1_gc | TCB_ENABLE_bm;
    TCB0.CTRLB = TCB_CNTMODE_INT_gc;
    TCB0.INTCTRL = TCB_CAPT_bm;
}

void USART3_init(void)
{
    USART3.BAUD = (uint16_t)((float)(4UL * F_CPU) / BAUD_RATE);
    PORTB.DIR |= PIN0_bm;															// TX Pin Output
    USART3.CTRLB = USART_TXEN_bm;
}
// End of initialization functions**

void USART3_send_string(const char *str)
{
    if (Tx_Busy) return;
    Tx_Ptr = str;
    Tx_Busy = true;
    USART3.CTRLA |= USART_DREIE_bm;
}

// **Interrupt Service Routines
/*
 * Timer TCB0 Interrupt - Fires every 1ms
 * Logic: Handles the countdown. When 1s hits, it tells Main to START the ADC.
 */
ISR(TCB0_INT_vect)
{
    TCB0.INTFLAGS = TCB_CAPT_bm;
    
    if (G_Time_Remaining_ms > 0)
    {
        G_Time_Remaining_ms--;
    }
    else
    {
        // 1 Second has passed!
        G_Time_Remaining_ms = ONE_SECOND_MS;
        G_Total_Time_s++;
        
        // Signal Main to START conversion.
        // We don't start it here to keep ISR short, 
        // and to avoid accessing ADC registers from two ISRs if we ever expanded logic.
        G_Start_Adc_Flag = true; 
    }
}

/*
 * ADC Result Ready Interrupt
 * Logic: Fires automatically when the ADC finishes measuring (~35us after start).
 * This replaces the blocking "while" loop.
 */
ISR(ADC0_RESRDY_vect)
{
    // Read the result immediately
    // Reading .RES automatically clears the Interrupt Flag
    G_Adc_Raw_Result = ADC0.RES;
    
    // Tell Main loop: "I have data for you to calculate"
    G_New_Data_Available = true;
}
// End of Interrupt Service Routines**

/*
 * UART Data Register Empty Interrupt
 */
ISR(USART3_DRE_vect)
{
    if (Tx_Ptr != NULL && *Tx_Ptr != '\0')
    {
        USART3.TXDATAL = *Tx_Ptr;
        Tx_Ptr++;
    }
    else
    {
        USART3.CTRLA &= ~USART_DREIE_bm;
        Tx_Busy = false;
        Tx_Ptr = NULL;
    }
}

int main(void)
{
    char Msg_Buffer[64];
    uint32_t Temp_K = 0;
    int32_t Temp_C = 0;
    
    USART3_init();
    Timer_init();
    ADC0_init();
    
    // Read Calibration Data
    Sigrow_ADC_Cal_Val = SIGROW.TEMPSENSE0 | (SIGROW.TEMPSENSE1 << 8);
    
    sei();
    
    while (1) 
    {
        // EVENT 1: Time to Measure
        if (G_Start_Adc_Flag)
        {
            G_Start_Adc_Flag = false;
            
            // FIRE AND FORGET!
            ADC0.COMMAND = ADC_STCONV_bm;
        }
        
        // EVENT 2: Measurement Finished
        if (G_New_Data_Available)
        {
            G_New_Data_Available = false;
            
            if (Sigrow_ADC_Cal_Val != 0)
            {
                Temp_K = ((uint32_t)G_Adc_Raw_Result * 358) / Sigrow_ADC_Cal_Val;
                Temp_C = (int32_t)Temp_K - 273;
            }
            
            // Send Message (Only if UART is free)
            if (!Tx_Busy)
            {
                sprintf(Msg_Buffer, "T: %lu s | %lu K | %ld C\r\n", G_Total_Time_s, Temp_K, Temp_C);
                USART3_send_string(Msg_Buffer);
            }
        }
    }
}