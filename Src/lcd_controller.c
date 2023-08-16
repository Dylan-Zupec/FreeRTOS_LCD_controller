/*lcd_controller.c*/

#include "lcd_controller_private.h"

void LCD_InitController(uint8_t use_4bit_mode)
{
	cursor_showing = cursor_blinking = pdFALSE;

	//stores data to write to LCD
	LCD_WriteQueue = xQueueCreate(40, sizeof(LCD_Frame_t));

	//writes queued data to LCD
	xTaskCreate(LCD_WriteHandler, "LCD Write", 200,
			(void*)(uint32_t)use_4bit_mode, 3, &LCD_WriteTask);

	LCD_WriteInitSeq(use_4bit_mode);

	LCD_SetFuncMode(use_4bit_mode ? LCD_4_BIT_MODE : LCD_8_BIT_MODE,
			LCD_2_LINE_MODE, LCD_5x8_FONT);

	LCD_TurnOffDisplay();

	LCD_ClearDisplay();

	LCD_SetEntryMode(LCD_AUTO_INCREMENT, LCD_AUTO_SHIFT_CURSOR);

	LCD_TurnOnDisplay();
}

void LCD_WriteHandler(void* use_4bit_mode)
{
	//busy flag is not available for first part of init sequence
	uint8_t busyflag_available = pdFALSE;
	//only upper nibble is written in first part of init sequence in 4-bit mode
	uint8_t lower_nibble_writable = pdFALSE;

	LCD_Frame_t frame;

	while(1)
	{
		xQueueReceive(LCD_WriteQueue, &frame, portMAX_DELAY);

		//init task notifys index 0 when busy flag is available
		if(!busyflag_available && ulTaskNotifyTakeIndexed(0, pdTRUE, 0) == pdTRUE)
			busyflag_available = pdTRUE;

		//init task notifys index 1 when lower nibble is writable
		if(!lower_nibble_writable && ulTaskNotifyTakeIndexed(1, pdTRUE, 0) == pdTRUE)
			lower_nibble_writable = pdTRUE;

		//Do not bother actually checking busy flag for now as it is no better
		//than just using a delay until shorter delays are implemented
		if(busyflag_available)
			HAL_Delay(2);

		if((uint32_t)use_4bit_mode)
		{
			LCD_4Bit_WritePins(frame.dest_reg, LCD_WRITE, frame.data >> 4);

			if(lower_nibble_writable)
				LCD_4Bit_WritePins(frame.dest_reg, LCD_WRITE, frame.data);
		}
		else //use 8-bit mode
			LCD_8Bit_WritePins(frame.dest_reg, LCD_WRITE, frame.data);
	}
}

void LCD_8Bit_WritePins(LCD_Register_e dest_reg, LCD_Operation_e operation, uint8_t data)
{
	//setup data
	for(uint8_t i=0; i<8; i++)
		HAL_GPIO_WritePin(LCD_DATA_GPIO_PORTS[i], LCD_DATA_GPIO_PINS[i], (data >> i) & 1);

	//setup read/write and register select
	HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, dest_reg);
	HAL_GPIO_WritePin(LCD_RW_GPIO_Port, LCD_RW_Pin, operation);
	//pulse enable high
	HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_SET);

	//data setup time and enable pulse high width
	HAL_Delay(1);

	//LCD is written on falling edge of enable
	HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_RESET);

	//data hold time and enable pulse low width
	HAL_Delay(1);
}

void LCD_4Bit_WritePins(LCD_Register_e dest_reg, LCD_Operation_e operation, uint8_t data)
{
	//writes lower nibble of data only
	//setup data
	for(uint8_t i=4, j=0; i<8; i++, j++)
		HAL_GPIO_WritePin(LCD_DATA_GPIO_PORTS[i], LCD_DATA_GPIO_PINS[i], (data >> j) & 1);

	//setup read/write and register select
	HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, dest_reg);
	HAL_GPIO_WritePin(LCD_RW_GPIO_Port, LCD_RW_Pin, operation);
	//pulse enable high
	HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_SET);

	//data setup time and enable pulse high width
	HAL_Delay(1);

	//LCD is written on falling edge of enable
	HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_RESET);

	//data hold time and enable pulse low width
	HAL_Delay(1);
}

void LCD_WriteInitSeq(uint8_t use_4bit_mode)
{
	//special init sequence required on power on
	//wait 100 ms after power on
	HAL_Delay(100);
	LCD_SetFuncMode(LCD_8_BIT_MODE, LCD_DONT_CARE, LCD_DONT_CARE);

	//wait another 10 ms
	HAL_Delay(10);
	LCD_SetFuncMode(LCD_8_BIT_MODE, LCD_DONT_CARE, LCD_DONT_CARE);

	//wait at least 200 us
	HAL_Delay(1); //not ideal but shortest delay possible
	LCD_SetFuncMode(LCD_8_BIT_MODE, LCD_DONT_CARE, LCD_DONT_CARE);

	//busy flag is now available
	xTaskNotifyGiveIndexed(LCD_WriteTask, 0);

	if(use_4bit_mode)
	{
		//extra function set is required for entering 4-bit mode
		LCD_SetFuncMode(LCD_4_BIT_MODE, LCD_DONT_CARE, LCD_DONT_CARE);

		//both nibbles are hereby written
		//only applicable in 4-bit mode
		xTaskNotifyGiveIndexed(LCD_WriteTask, 1);
	}
}

void LCD_SetFuncMode(uint8_t data_length_flag, uint8_t line_num_flag, uint8_t font_type_flag)
{
	//set data length, number of lines, and font size
	LCD_Frame_t frame;

	frame.data = LCD_FUNC_SET | data_length_flag | line_num_flag | font_type_flag;
	frame.dest_reg = LCD_REG_INSTRUCTION;

	xQueueSend(LCD_WriteQueue, &frame, portMAX_DELAY);
}

void LCD_SetEntryMode(uint8_t shift_dir_flag, uint8_t shift_mode_flag)
{
	//set cursor or display to auto increment or decrement
	LCD_Frame_t frame;

	frame.data = LCD_ENTRY_MODE | shift_dir_flag | shift_mode_flag;
	frame.dest_reg = LCD_REG_INSTRUCTION;

	xQueueSend(LCD_WriteQueue, &frame, portMAX_DELAY);
}

void LCD_TurnOnDisplay(void)
{
	LCD_Frame_t frame;

	frame.data = LCD_DISPLAY_CTRL | LCD_DISPLAY_ON | cursor_showing | cursor_blinking;
	frame.dest_reg = LCD_REG_INSTRUCTION;

	xQueueSend(LCD_WriteQueue, &frame, portMAX_DELAY);
}

void LCD_TurnOffDisplay(void)
{
	LCD_Frame_t frame;

	frame.data = LCD_DISPLAY_CTRL | LCD_DISPLAY_OFF | cursor_showing | cursor_blinking;
	frame.dest_reg = LCD_REG_INSTRUCTION;

	xQueueSend(LCD_WriteQueue, &frame, portMAX_DELAY);
}

void LCD_WriteText(const char* text)
{
	LCD_Frame_t frame;

	frame.dest_reg = LCD_REG_DATA;

	for(int i=0; i < strlen(text); i++)
	{
		frame.data = text[i];
		xQueueSend(LCD_WriteQueue, &frame, portMAX_DELAY);
	}
}

void LCD_ClearDisplay(void)
{
	LCD_Frame_t frame;

	frame.data = LCD_CLEAR;
	frame.dest_reg = LCD_REG_INSTRUCTION;

	xQueueSend(LCD_WriteQueue, &frame, portMAX_DELAY);
}

void LCD_SetCursorMode(uint8_t show_cursor, uint8_t blink_cursor)
{
	cursor_showing = show_cursor;
	cursor_blinking = blink_cursor;

	LCD_Frame_t frame;

	frame.data = LCD_DISPLAY_CTRL | LCD_DISPLAY_ON |
			(show_cursor ? LCD_CURSOR_ON : LCD_CURSOR_OFF) |
			(blink_cursor ? LCD_CURSOR_BLINK_ON: LCD_CURSOR_BLINK_OFF);
	frame.dest_reg = LCD_REG_INSTRUCTION;

	xQueueSend(LCD_WriteQueue, &frame, portMAX_DELAY);
}

void LCD_SetCursorPos(uint8_t row, uint8_t column)
{
	if(row > 1) row = 1;
	if(column > 15) column = 15;

	LCD_Frame_t frame;

	frame.data = LCD_CURSOR_SET | (column + row * 0x40);
	frame.dest_reg = LCD_REG_INSTRUCTION;

	xQueueSend(LCD_WriteQueue, &frame, portMAX_DELAY);
}

void LCD_SetCursorHome(void)
{
	LCD_Frame_t frame;

	frame.data = LCD_HOME;
	frame.dest_reg = LCD_REG_INSTRUCTION;

	xQueueSend(LCD_WriteQueue, &frame, portMAX_DELAY);
}
