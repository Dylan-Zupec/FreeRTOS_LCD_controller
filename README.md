# FreeRTOS_LCD_controller
 
This project is my first venture into the world of real-time operating systems.
It is in the same vein as my previous project which used an FPGA. The target of
this application is a STM32F446 MCU with an ARM Cortex-M4 processor.

My motivation for this project is to create an LCD interface that I can
easily employ in future projects with minimal setup. Only a call to 
LCD_ControllerInit() is necessitated. The user is then capable of writing text, 
moving the cursor, and more, all in single function calls.

This application works in 4-bit as well as 8-bit mode with the intent of adding 
compatability with SPI.

As the time it would take to check the busy flag of the LCD is limited to a 
minimum of 2 ms (HAL_Delay() is 1 ms minimum and a read operation requires two
seperate delays), it is equally fast to instead use a 2 ms delay. In the
future, I intend to implement shorter possible delays using the hardware timers
of the microcontroller.                 
