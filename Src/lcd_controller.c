/*lcd_controller.c*/

#include "lcd_controller_private.h"

void LCD_ControllerInit(uint32_t use_4bit_mode)
{
	init_task_done = init_sequence_done = pdFALSE;

	//create queue for init sequence instructions
	LCD_InitQueue = xQueueCreate(9, sizeof(LCD_Frame_t));
	//create queue for text data and regular instructions
	LCD_WriteQueue = xQueueCreate(40, sizeof(LCD_Frame_t));

	//create task to load init queue
	xTaskCreate(LCD_InitHandler, "LCD Init", 200, (void*)use_4bit_mode, 4, &LCD_InitTask);
	//create task to process both queues and write to LCD
	xTaskCreate(LCD_WriteHandler, "LCD Write", 200, (void*)use_4bit_mode, 2, &LCD_WriteTask);
}

void LCD_InitHandler(void* parameters)
{
	LCD_Frame_t frame;

	//first three instructions are function set
	frame.data = LCD_FUNC_SET | LCD_8_BIT_MODE;
	//all data is being written to instruction register
	frame.dest_reg = LCD_REG_INSTRUCTION;
	//busy flag not yet available
	frame.check_busyflag = pdFALSE;
	//only write upper nibble in 4-bit mode
	frame.mode_selected = pdFALSE;

	//wait 100 ms after power on
	vTaskDelay(pdMS_TO_TICKS(100));
	xQueueSend(LCD_InitQueue, &frame, portMAX_DELAY);

	//wait another 10 ms
	vTaskDelay(pdMS_TO_TICKS(10));
	xQueueSend(LCD_InitQueue, &frame, portMAX_DELAY);

	//wait at least 200 us
	HAL_Delay(1); //not ideal but shortest delay possible
	xQueueSend(LCD_InitQueue, &frame, portMAX_DELAY);

	//busy flag is now available
	frame.check_busyflag = pdTRUE;

	//@parameters is TRUE if 4-bit mode is selected
	if((uint32_t)parameters == pdTRUE)
	{
		//extra function set is required if using 4-bit mode
		frame.data = LCD_FUNC_SET | LCD_4_BIT_MODE;
		xQueueSend(LCD_InitQueue, &frame, portMAX_DELAY);
	}

	//both nibbles are hereby both written
	//only applicable for 4-bit mode
	frame.mode_selected = pdTRUE;

	//this function set is for real
	frame.data = LCD_FUNC_SET | ((uint32_t)parameters == pdTRUE ? LCD_4_BIT_MODE : LCD_8_BIT_MODE) | LCD_2_LINE_MODE | LCD_5x8_FONT;
	xQueueSend(LCD_InitQueue, &frame, portMAX_DELAY);

	//turn off display
	frame.data = LCD_DISPLAY_CTRL | LCD_DISPLAY_OFF | LCD_CURSOR_OFF | LCD_CURSOR_BLINK_OFF;
	xQueueSend(LCD_InitQueue, &frame, portMAX_DELAY);

	//clear display RAM and set cursor to (0,0)
	frame.data = LCD_CLEAR;
	xQueueSend(LCD_InitQueue, &frame, portMAX_DELAY);

	//set entry mode to increment cursor
	frame.data = LCD_ENTRY_MODE | LCD_AUTO_INCREMENT | LCD_AUTO_SHIFT_CURSOR;
	xQueueSend(LCD_InitQueue, &frame, portMAX_DELAY);

	//turn on display
	frame.data = LCD_DISPLAY_CTRL | LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_CURSOR_BLINK_OFF;
	xQueueSend(LCD_InitQueue, &frame, portMAX_DELAY);

	init_task_done = pdTRUE;

	vTaskDelete(NULL);
}

void LCD_WriteHandler(void* parameters)
{
	LCD_Frame_t frame;

	while(1)
	{
		//init task has finished loading queue but data has not been fully processed yet
		if(!init_sequence_done && init_task_done &&
				uxQueueMessagesWaiting(LCD_InitQueue) == 0)
		{
			init_sequence_done = pdTRUE;
			vQueueDelete(LCD_InitQueue);
		}

		if(init_sequence_done)
			xQueueReceive(LCD_WriteQueue, &frame, portMAX_DELAY);
		else
			xQueueReceive(LCD_InitQueue, &frame, portMAX_DELAY);


		if(frame.check_busyflag)
		{
			//Do not bother actually checking busy flag for now as it is no better than just using
			//a delay until shorter delays are implemented
			HAL_Delay(2);
		}

		if((uint32_t)parameters == pdTRUE) //use 4-bit mode
			LCD_4Bit_WritePins(frame.dest_reg, LCD_WRITE, frame.data, !frame.mode_selected);
		else //use 8-bit mode
			LCD_WritePins(frame.dest_reg, LCD_WRITE, frame.data);
	}
}

void LCD_WritePins(LCD_Register_e dest_reg, LCD_Operation_e operation, uint8_t data)
{
	//setup data
	for(uint8_t i=0; i<8; i++)
		HAL_GPIO_WritePin(LCD_DATA_GPIO_PORTS[i], LCD_DATA_GPIO_PINS[i], (data >> i) & 1);

	//setup read/write and register select
	HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, dest_reg);
	HAL_GPIO_WritePin(LCD_RW_GPIO_Port, LCD_RW_Pin, operation);
	//pulse enable high
	HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_SET);

	HAL_Delay(1);

	//LCD is written on falling edge of enable
	HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_RESET);

	HAL_Delay(1);
}

void LCD_4Bit_WritePins(LCD_Register_e dest_reg, LCD_Operation_e operation, uint8_t data, uint8_t upper_nibble_only)
{
	//setup data for upper nibble
	for(uint8_t i=4; i<8; i++)
		HAL_GPIO_WritePin(LCD_DATA_GPIO_PORTS[i], LCD_DATA_GPIO_PINS[i], (data >> i) & 1);

	//setup read/write and register select
	HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, dest_reg);
	HAL_GPIO_WritePin(LCD_RW_GPIO_Port, LCD_RW_Pin, operation);
	//pulse enable high
	HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_SET);

	HAL_Delay(1);

	//LCD is written on falling edge of enable
	HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_RESET);

	HAL_Delay(1);

	//lower nibble is not written if mode is not selected
	if(upper_nibble_only) return;

	//setup data for lower nibble
	for(uint8_t i=4, j=0; i<8; i++, j++)
		HAL_GPIO_WritePin(LCD_DATA_GPIO_PORTS[i], LCD_DATA_GPIO_PINS[i], (data >> j) & 1);

	//pulse enable high
	HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_SET);

	HAL_Delay(1);

	//LCD is written on falling edge of enable
	HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_RESET);

	HAL_Delay(1);
}

void LCD_Clear(void)
{
	//clear display RAM and set cursor to (0,0)
	LCD_Frame_t frame;

	frame.data = LCD_CLEAR;
	frame.dest_reg = LCD_REG_INSTRUCTION;
	frame.check_busyflag = pdTRUE;
	frame.mode_selected = pdTRUE;

	xQueueSend(LCD_WriteQueue, &frame, portMAX_DELAY);
}

void LCD_Home(void)
{
	//set cursor to (0,0)
	LCD_Frame_t frame;

	frame.data = LCD_HOME;
	frame.dest_reg = LCD_REG_INSTRUCTION;
	frame.check_busyflag = pdTRUE;
	frame.mode_selected = pdTRUE;

	xQueueSend(LCD_WriteQueue, &frame, portMAX_DELAY);
}

void LCD_CursorMode(BaseType_t show_cursor, BaseType_t blink_cursor)
{
	LCD_Frame_t frame;

	frame.data = LCD_DISPLAY_CTRL | LCD_DISPLAY_ON |
			(show_cursor ? LCD_CURSOR_ON : LCD_CURSOR_OFF) |
			(blink_cursor ? LCD_CURSOR_BLINK_ON: LCD_CURSOR_BLINK_OFF);
	frame.dest_reg = LCD_REG_INSTRUCTION;
	frame.check_busyflag = pdTRUE;
	frame.mode_selected = pdTRUE;

	xQueueSend(LCD_WriteQueue, &frame, portMAX_DELAY);
}

void LCD_CursorSet(uint8_t row, uint8_t column)
{
	//set cursor to (@row, @column)
	LCD_Frame_t frame;

	if(row > 1) row = 1;
	if(column > 15) column = 15;

	frame.data = LCD_CURSOR_SET | (column + row * 0x40);
	frame.dest_reg = LCD_REG_INSTRUCTION;
	frame.check_busyflag = pdTRUE;
	frame.mode_selected = pdTRUE;

	xQueueSend(LCD_WriteQueue, &frame, portMAX_DELAY);
}

void LCD_Write(const char* const text)
{
	//write text to LCD
	LCD_Frame_t frame;

	frame.dest_reg = LCD_REG_DATA;
	frame.check_busyflag = pdTRUE;
	frame.mode_selected = pdTRUE;

	for(int i=0; i < strlen(text); i++)
	{
		frame.data = text[i];
		xQueueSend(LCD_WriteQueue, &frame, portMAX_DELAY);
	}
}
