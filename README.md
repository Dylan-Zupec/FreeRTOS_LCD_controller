# FreeRTOS_LCD_controller
 
This project is my first venture into the world of real-time operating systems.
It is in the same vein as my previous project which used an FPGA. The target of
this application is a STM32F446 MCU with an ARM Cortex-M4 processor.

My motivation for this project was to create an LCD interface that I could
easily employ in future projects. With minimal setup--only a call to 
LCD_ControllerInit() being necessitated, the user is able to write text, 
reposition the cursor, and more, all in single function calls. 

The controller employs two tasks, one to load the init sequence queue, and
another to process queued data and write it to the LCD pins. One additional
task is present which calls the controller interface as a demo application.

As the time it would take to check the busy flag of the LCD is limited to a 
minimum of 2 ms (HAL_Delay() is 1 ms minimum and a read operation requires two
seperate delays), it is equally fast to instead use a 2 ms delay. In the
future, I intend to implement shorter possible delays using the hardware timers
of the microcontroller.                 
