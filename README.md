# AVR128DB48 Microprocessor Projects

This repository contains a collection of laboratory exercises ("Praktikums") and projects developed for the **AVR128DB48** microprocessor. The codebase demonstrates the implementation of low-level drivers, interrupt-based systems, and communication protocols using C.

## 🛠 Hardware & Software

* **Microcontroller:** AVR128DB48
* **IDE:** Microchip Studio (formerly Atmel Studio)
* **Compiler:** AVR-GCC
* **Programmer:** MPLAB SNAP / Pickit 4
* **Key Peripherals:** TCA0 (PWM), TCB0 (Timers), ADC0 (Analog Input), USART3 (Serial), I2C (TWI).

## 📂 Repository Structure

The repository is organized into 4 main project folders. To keep the repository clean and lightweight, **only the source code (`main.c`) and necessary header files are included.** Build artifacts (like `.hex` or `.cproj` files) are excluded.

### 1. 🖥️ Project: LCD_Control
Focuses on I2C communication and string manipulation using a 16x2 LCD.
* **Hello_Display:** Basic initialization and string output.
* **Count_Up:** Timer-based integer display updates.
* **Ping_Pong:** Implements a visual animation where a character bounces left/right across the screen using custom cursor movement functions.
* **Binary_Calculator:** Displaying binary operations and results.

### 2. 🚦 Project: Interrupts & Timers
Focuses on non-blocking architecture using hardware timers and external interrupts.
* **Button_Interrupt:** Handling GPIO interrupts (PORT_ISC) for immediate response.
* **Timer:** Basic periodic interrupts.
* **Traffic_Light:** A complex State Machine implementation. It uses **TCB0** as a 1ms system tick to handle button debouncing and phase timing (Red -> Yellow -> Green) simultaneously without blocking the CPU.
* **Programmable_Timer:** User-configurable timer intervals.

### 3. 🌈 Project: Pulse_Width_Modulation (PWM)
Focuses on using Timer/Counter A (TCA0) for waveform generation.
* **Dimming_Red_LED:** Configures **TCA0** in Single-Slope PWM mode (`DSBOTTOM`) to control LED brightness via duty cycle.
* **Rainbow_Led:** Mixing RGB channels to create colors.
* **Waving_Servomotor:** Controlling servo angles by manipulating the pulse width (typically 1ms - 2ms).

### 4. 📡 Project: ADC & USART
The most advanced project, combining analog sensors with serial data logging.
* **ADC_Potantiometer:** Basic analog reading.
* **ADC_Photoresistor:** Reads voltage from a light sensor, calculates percentage (0-100%), and displays real-time stats on the LCD.
* **USART_Buttons:** Sending command strings based on input.
* **USART_Internal_Temperature:** Reads the chip's internal temperature sensor (`ADC_MUXPOS_TEMPSENSE_gc`), applies factory calibration data (`SIGROW`), and logs the temperature to a PC via UART every second.
* **USART_RGB-LED_Control:** Controlling hardware via serial commands.

### 5. 🎨 Project: RGB_Color_Sensor
**New Addition!** Focuses on interfacing advanced digital sensors via I2C.
* **RGB_Color_Sensor:** *[Featured Code]* A driver for the **TCS34725** Color Sensor.
    * **I2C Protocol:** Implements manual register writing/reading.
    * **Auto-Increment:** Uses the command bit (`0x20`) to efficiently read multi-byte RGBA data in a single I2C transaction.
    * **Hex Display:** Formats the 16-bit color data into Hexadecimal strings for the LCD.

## 🚀 How to Use

Since this repository contains source files only, you must create a project environment on your local machine to run them.

1.  **Create a New Project:**
    * Open **Microchip Studio**.
    * Go to `File` -> `New` -> `Project...`.
    * Select **GCC C Executable Project**.
    * Select the device: **AVR128DB48**.

2.  **Import the Code:**
    * Open the `main*.c` file from the specific exercise folder in this repository (e.g., `Project_2/Traffic_Light/main_traffic_light.c`).
    * Copy the entire content.
    * Paste it into the `main.c` of your newly created Microchip Studio project.

3.  **Add Libraries (If required):**
    * For projects using the LCD (like Project 1 or 4), ensure you download the files from the `Includes` folder (`I2C_LCD.h` and `I2C_LCD.c`).
    * Add them to your project folder.
    * In Solution Explorer, right-click your project -> `Add` -> `Existing Item...` and select the library files.
    * Add these paths to your project settings. To do this, navigate to:
      Project → Properties → Toolchain → AVR/GNU Compiler → Directories
      and add the paths “Includes/AVR128DB48 I2C” and “Includes/I2C LCD” within your project using the green plus button.

4.  **Build & Flash:**
    * Press `F7` to build the solution.
    * Connect your programmer and press `Ctrl + Alt + F5` to program the chip.

## 📝 Technical Notes

* **Non-Blocking Logic:** Most projects (especially the Traffic Light and Temperature Logger) rely on `volatile` flags and ISRs (Interrupt Service Routines) to keep the `main` loop free.
* **Clock Speed:** All projects assume a default clock speed of **4MHz** (`F_CPU 4000000UL`). If you change the fuse settings, remember to update the definition in the code.

---
*Created by Muhammed Turhan*
