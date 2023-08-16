/*lcd_controller_public.h*/

void LCD_InitController(uint32_t use_4bit_mode);
void LCD_TurnOnDisplay(void);
void LCD_TurnOffDisplay(void);
void LCD_WriteText(const char* text);
void LCD_ClearDisplay(void);
void LCD_SetCursorMode(uint8_t show_cursor, uint8_t blink_cursor);
void LCD_SetCursorPos(uint8_t row, uint8_t column);
void LCD_SetCursorHome(void);
