/*lcd_controller.c*/

#include "lcd_controller_private.h"

void LCD_InitController(LCD_Mode_e LCD_mode)
{
	cursor_showing = cursor_blinking = pdFALSE;

	LCD_init_task = xTaskGetCurrentTaskHandle();

	if(LCD_mode == LCD_SPI)
	{
#ifdef LCD_SPI_PINS_DEFINED
		//chip select is active low
		HAL_GPIO_WritePin(LCD_SPI_CS_GPIO_Port, LCD_SPI_CS_Pin, GPIO_PIN_SET);
#else
		configTHROW_EXCEPTION("error: SPI LCD pins not defined");
#endif
	}

	//data to write to LCD
	LCD_write_queue = xQueueCreate(40, sizeof(LCD_Frame_t));

	//writes queued data to LCD
	xTaskCreate(LCD_WriteHandler, "LCD Write", 200,
			(void*)(uint32_t)LCD_mode, 3, &LCD_write_task);

	LCD_WriteInitSeq(LCD_mode == LCD_4BIT);

	LCD_SetFuncMode(LCD_mode == LCD_4BIT ? LCD_4BIT_MODE : LCD_8BIT_MODE,
			LCD_2LINE_MODE, LCD_5x8_FONT);

	LCD_TurnOffDisplay();

	LCD_ClearDisplay();

	LCD_SetEntryMode(LCD_AUTO_INCREMENT, LCD_AUTO_SHIFT_CURSOR);

	LCD_TurnOnDisplay();
}

void LCD_WriteHandler(void* LCD_mode)
{
	//busy flag is not available for first part of init sequence
	uint8_t busyflag_available = pdFALSE;
	//only upper nibble is written in first part of init sequence in 4-bit mode
	uint8_t lower_nibble_writable = pdFALSE;
	//queue must be fully processed before either flag is set
	uint8_t awaiting_empty_queue = pdFALSE;

	LCD_Frame_t frame;

	while(1)
	{
		xQueueReceive(LCD_write_queue, &frame, portMAX_DELAY);

		if(!awaiting_empty_queue)
		{
			//init task notifies index 0 when busy flag is available
			if(!busyflag_available && ulTaskNotifyTakeIndexed(0, pdTRUE, 0) == 1)
				awaiting_empty_queue = pdTRUE;

			//init task notifies index 1 when lower nibble is writable
			else if((uint32_t)LCD_mode == LCD_4BIT && !lower_nibble_writable &&
					ulTaskNotifyTakeIndexed(1, pdTRUE, 0) == 1)
				awaiting_empty_queue = pdTRUE;
		}

		//Do not bother actually checking busy flag for now as it is no better
		//than just using a delay until shorter delays are implemented
		if(busyflag_available)
			HAL_Delay(2);

		LCD_WritePins((uint32_t)LCD_mode, frame.dest_reg, LCD_WRITE, frame.data);

		if(awaiting_empty_queue)
		{
			if(!busyflag_available && uxQueueMessagesWaiting(LCD_write_queue) == 0)
			{
				//queue has been processed to point where busy flag is available
				busyflag_available = pdTRUE;
				awaiting_empty_queue = pdFALSE;
				xTaskNotify(LCD_init_task, 0, eNoAction);
			}

			else if((uint32_t)LCD_mode == LCD_4BIT && !lower_nibble_writable &&
					uxQueueMessagesWaiting(LCD_write_queue) == 0)
			{
				//queue has been processed to point where lower nibble is writable
				lower_nibble_writable = pdTRUE;
				awaiting_empty_queue = pdFALSE;
				xTaskNotify(LCD_init_task, 0, eNoAction);
			}
		}
	}
}

void LCD_WritePins(LCD_Mode_e LCD_mode, LCD_Register_e dest_reg, LCD_Operation_e operation, uint8_t data)
{
	switch(LCD_mode)
	{
	case LCD_4BIT:
#ifdef LCD_4BIT_PINS_DEFINED
		LCD_4Bit_WritePins(dest_reg, LCD_WRITE, data >> 4);
		if(lower_nibble_writable)
			LCD_4Bit_WritePins(dest_reg, LCD_WRITE, data);
#else
		configTHROW_EXCEPTION("error: 4-bit LCD pins not defined");
#endif
		break;
	case LCD_8BIT:
#ifdef LCD_8BIT_PINS_DEFINED
		LCD_8Bit_WritePins(dest_reg, LCD_WRITE, data);
#else
		configTHROW_EXCEPTION("error: 8-bit LCD pins not defined");
#endif
		break;
	case LCD_SPI:
#ifdef LCD_SPI_PINS_DEFINED
		LCD_SPI_WritePins(dest_reg, LCD_WRITE, data);
#else
		configTHROW_EXCEPTION("error: SPI LCD pins not defined");
#endif
		break;
	}
}

#ifdef LCD_8BIT_PINS_DEFINED
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
#endif

#ifdef LCD_4BIT_PINS_DEFINED
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
#endif

#ifdef LCD_SPI_PINS_DEFINED
void LCD_SPI_WritePins(LCD_Register_e dest_reg, LCD_Operation_e operation, uint8_t data)
{
	/********Frame Format********/
	//MSB first
	//byte 2: [D7|...|D0]
	//byte 1: [E|RW|RS|-|-|-|X|X]
	/****************************/
	//D7-D0	 = 	data pins 7-0
	//E 	 =	enable pin
	//RW 	 = 	read/write pin
	//RS 	 = 	register select
	//-	 =      unused
	//X	 = 	reserved
	/****************************/
	uint8_t frame_bytes[2];

	//pull chip select low to select LCD
	HAL_GPIO_WritePin(LCD_SPI_CS_GPIO_Port, LCD_SPI_CS_Pin, GPIO_PIN_RESET);

	//setup data, read/write, and register select; pulse enable high
	frame_bytes[0] = (1 << 7) | (operation << 6) | (dest_reg << 5);
	frame_bytes[1] = data;
	HAL_SPI_Transmit(&hspi2, frame_bytes, 1, HAL_MAX_DELAY);

	//data setup time and enable pulse high width
	HAL_Delay(1);

	//LCD is written on falling edge of enable
	frame_bytes[0] = (0 << 7) | (operation << 6) | (dest_reg << 5);
	HAL_SPI_Transmit(&hspi2, frame_bytes, 1, HAL_MAX_DELAY);

	//data hold time and enable pulse low width
	HAL_Delay(1);

	//unselect LCD and reset ICs by pullig chip select high
	HAL_GPIO_WritePin(LCD_SPI_CS_GPIO_Port, LCD_SPI_CS_Pin, GPIO_PIN_SET);
}
#endif

void LCD_WriteInitSeq(uint8_t use_4bit_mode)
{
	//special init sequence required on power on
	//wait 100 ms after power on
	HAL_Delay(100);
	LCD_SetFuncMode(LCD_8BIT_MODE, LCD_DONT_CARE, LCD_DONT_CARE);

	//wait another 10 ms
	HAL_Delay(10);
	LCD_SetFuncMode(LCD_8BIT_MODE, LCD_DONT_CARE, LCD_DONT_CARE);

	//wait at least 200 us
	HAL_Delay(1); //not ideal but shortest delay possible
	LCD_SetFuncMode(LCD_8BIT_MODE, LCD_DONT_CARE, LCD_DONT_CARE);

	//busy flag is available once preceding data is written
	xTaskNotifyGiveIndexed(LCD_write_task, 0);
	//wait for queued data to be written
	xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);

	if(use_4bit_mode)
	{
		//extra function set is required for entering 4-bit mode
		LCD_SetFuncMode(LCD_4BIT_MODE, LCD_DONT_CARE, LCD_DONT_CARE);

		//lower nibble is writable once preceding data is written
		//only applicable in 4-bit mode
		xTaskNotifyGiveIndexed(LCD_write_task, 1);
		//wait for queued data to be written
		xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
	}
}

void LCD_SetFuncMode(uint8_t data_length_flag, uint8_t line_num_flag, uint8_t font_type_flag)
{
	//set data length, number of lines, and font size
	LCD_Frame_t frame;

	frame.data = LCD_FUNC_SET | data_length_flag | line_num_flag | font_type_flag;
	frame.dest_reg = LCD_REG_INSTRUCTION;

	xQueueSend(LCD_write_queue, &frame, portMAX_DELAY);
}

void LCD_SetEntryMode(uint8_t shift_dir_flag, uint8_t shift_mode_flag)
{
	//set cursor or display to auto increment or decrement
	LCD_Frame_t frame;

	frame.data = LCD_ENTRY_MODE | shift_dir_flag | shift_mode_flag;
	frame.dest_reg = LCD_REG_INSTRUCTION;

	xQueueSend(LCD_write_queue, &frame, portMAX_DELAY);
}

void LCD_TurnOnDisplay(void)
{
	LCD_Frame_t frame;

	frame.data = LCD_DISPLAY_CTRL | LCD_DISPLAY_ON | cursor_showing | cursor_blinking;
	frame.dest_reg = LCD_REG_INSTRUCTION;

	xQueueSend(LCD_write_queue, &frame, portMAX_DELAY);
}

void LCD_TurnOffDisplay(void)
{
	LCD_Frame_t frame;

	frame.data = LCD_DISPLAY_CTRL | LCD_DISPLAY_OFF | cursor_showing | cursor_blinking;
	frame.dest_reg = LCD_REG_INSTRUCTION;

	xQueueSend(LCD_write_queue, &frame, portMAX_DELAY);
}

void LCD_WriteText(const char* text)
{
	LCD_Frame_t frame;

	frame.dest_reg = LCD_REG_DATA;

	for(int i=0; i < strlen(text); i++)
	{
		frame.data = text[i];
		xQueueSend(LCD_write_queue, &frame, portMAX_DELAY);
	}
}

void LCD_ClearDisplay(void)
{
	LCD_Frame_t frame;

	frame.data = LCD_CLEAR;
	frame.dest_reg = LCD_REG_INSTRUCTION;

	xQueueSend(LCD_write_queue, &frame, portMAX_DELAY);
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

	xQueueSend(LCD_write_queue, &frame, portMAX_DELAY);
}

void LCD_SetCursorPos(uint8_t row, uint8_t column)
{
	if(row > 1) row = 1;
	if(column > 15) column = 15;

	LCD_Frame_t frame;

	frame.data = LCD_CURSOR_SET | (column + row * 0x40);
	frame.dest_reg = LCD_REG_INSTRUCTION;

	xQueueSend(LCD_write_queue, &frame, portMAX_DELAY);
}

void LCD_SetCursorHome(void)
{
	LCD_Frame_t frame;

	frame.data = LCD_HOME;
	frame.dest_reg = LCD_REG_INSTRUCTION;

	xQueueSend(LCD_write_queue, &frame, portMAX_DELAY);
}
