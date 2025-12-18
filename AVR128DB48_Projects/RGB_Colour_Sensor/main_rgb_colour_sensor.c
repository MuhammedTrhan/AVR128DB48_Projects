/*
 * Praktikum_8.1.c
 *
 * RGB color sensor
 *
 * Created: 12/14/2025 3:59:37 PM
 * Author : mami4
 */ 

#define F_CPU 4000000UL															// Set Clock Speed to 4MHz
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdbool.h>
#include "I2C_LCD.h"

// **TCS34725 register definitions start
#define TCS34725_ADDRESS          0x29
#define TCS34725_COMMAND_BIT      0x80
#define TCS34725_ENABLE           0x00
#define TCS34725_ATIME            0x01
#define TCS34725_CONTROL          0x0F
#define TCS34725_ID               0x12
#define TCS34725_CDATAL           0x14
#define TCS34725_RDATAL           0x16
#define TCS34725_GDATAL           0x18
#define TCS34725_BDATAL           0x1A

#define TCS34725_PON              0x01											// Power ON
#define TCS34725_AEN              0x02											// ADC Enable
#define TCS34725_CMD_AUTO_INC     0x20											// Auto-increment protocol
// TCS34725 register definitions end**

/*
 * @brief Initializes the TCS34725 Sensor
 */
void sensor_init(void) {
    uint8_t Data[2];

    // Power ON
    // Select ENABLE register (0x00) with Command Bit (0x80)
    Data[0] = TCS34725_COMMAND_BIT | TCS34725_ENABLE;
    Data[1] = TCS34725_PON; 
    i2c_write(TCS34725_ADDRESS, Data, 2);

    _delay_ms(3);																// Wait 2.4ms for oscillator to start (per datasheet)

    // Enable ADC
    // Write PON (0x01) | AEN (0x02) = 0x03 to ENABLE register
    Data[0] = TCS34725_COMMAND_BIT | TCS34725_ENABLE;
    Data[1] = TCS34725_PON | TCS34725_AEN;
    i2c_write(TCS34725_ADDRESS, Data, 2);

    _delay_ms(10);																// Wait for first integration cycle to complete (default ~2.4ms)
}

/*
 * @brief Reads RGB data from the sensor
 */
void sensor_read_RGB(uint16_t *r, uint16_t *g, uint16_t *b) {
    // Read 6 bytes starting from red low (0x16)
    // Use the Auto-Increment bit (0x20) so the sensor moves to the next register automatically
    uint8_t cmd = TCS34725_COMMAND_BIT | TCS34725_CMD_AUTO_INC | TCS34725_RDATAL;

    i2c_write_byte(TCS34725_ADDRESS, cmd);										// Write Command Byte

    // Read 6 bytes (RedL, RedH, GreenL, GreenH, BlueL, BlueH)
    uint8_t Buffer[6];
    i2c_read(TCS34725_ADDRESS, Buffer, 6);

    // Combine bytes
    *r = (uint16_t)Buffer[1] << 8 | Buffer[0];
    *g = (uint16_t)Buffer[3] << 8 | Buffer[2];
    *b = (uint16_t)Buffer[5] << 8 | Buffer[4];
}

int main(void)
{
    // Variables
    char Display_Buffer[17];
    uint16_t Red, Green, Blue;
	
    // Initialize previous values to a value that forces an update on the first run (e.g. max value)
    uint16_t Last_Red = 0xFFFF, Last_Green = 0xFFFF, Last_Blue = 0xFFFF;

    i2c_init();																	// Initialize I2C Bus
    lcd_init();																	// Initialize LCD
    lcd_backlight(true);
    sensor_init();																// Initialize Color Sensor

    while (1) 
    {
        sensor_read_RGB(&Red, &Green, &Blue);									// Read sensor data

        if (Red != Last_Red || Green != Last_Green || Blue != Last_Blue)		// Check if values have changed
        {
            lcd_moveCursor(0, 0);
            sprintf(Display_Buffer, "R:%04X G:%04X", Red, Green);				// %04X prints at least 4 hex digits, padded with zeros if needed
            lcd_putString(Display_Buffer);										// Display red and green on line 1 (Hexadecimal)

            lcd_moveCursor(0, 1);
            sprintf(Display_Buffer, "B:%04X", Blue);
            lcd_putString(Display_Buffer);										// Display blue on line 2 (Hexadecimal)
            
            // Update history
            Last_Red = Red;
            Last_Green = Green;
            Last_Blue = Blue;
        }

        _delay_ms(100);															// Wait before next reading
    }
}