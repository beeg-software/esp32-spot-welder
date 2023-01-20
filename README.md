# Spot Welder Controller

This project is a spot welder controller that utilizes an ESP32 development board, OLED display, rotary encoder, a foot switch, a Solid State Relay (SSR) and a modified microwave transformer. The program is written in C++ using the Arduino framework and FreeRTOS, a real-time operating system. PlatformIO is used to build and upload the code to the ESP32.

## Features
- Control the pre-impulse, pause, and impulse timings of the spot welder
- OLED display shows the current timings and allows for easy editing
- Rotary encoder allows for quick and precise adjustments to the timings
- Foot switch to trigger the welding
- SSR to switch the high current to the modified microwave transformer
- Serial commands allow for remote control and monitoring of the spot welder

## Hardware Used
- ESP32 Development Board
- OLED Display (SH1106)
- Rotary Encoder
- Foot switch
- SSR
- Modified microwave transformer

## Libraries Used
- Arduino
- FreeRTOS
- Adafruit_SH1106G
- Adafruit_GFX

## Usage
1. Connect the OLED display, rotary encoder, foot switch, SSR to the ESP32 according to the pin definitions in the code.
2. Connect the SSR to the modified microwave transformer according to the instructions.
3. Upload the code to the ESP32 using PlatformIO.
4. Open the serial monitor to interact with the spot welder using commands or adjust the timings using the rotary encoder and OLED display.

## Tasks
- TaskImpulse: handles the actual welding
- TaskDrawMenu: displays the menu on the OLED display
- TaskSerialCommands: handles commands received from the serial connection
- TaskEncoder: handles rotary encoder inputs

## Serial Commands
- `i`: Increment the current selected value by 10
- `d`: Decrement the current selected value by 10
- `!`: Triggers the welding
- `e`: Enters the editing mode (only for encoder debug purposes)
- `1-3`: Select the corresponding menu row
