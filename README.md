# FreeRTOS_LCD_controller
 
This project is my first venture into the world of real-time operating systems.
It is in the same vein as my previous project which used an FPGA. The target of
this application is a STM32F446 MCU with an ARM Cortex-M4 processor.

My motivation for this project is to create an LCD interface that I can
easily employ in future projects with minimal setup. Only a call to 
LCD_ControllerInit() is necessitated. The user is then capable of writing text, 
moving the cursor, changing display settings, and more, all in single function 
calls.

This application can be used in either 8-bit or 4-bit mode. With the addition of 
a little hardware, it is also compatible with Serial Peripheral Interface (SPI).
The demo application uses SPI and interfaces with the LCD using a circuit 
consisting of two 8-bit shift registers, a 4-bit counter, and a hex inverter.

(./demo-picture.jpg)
 
      
