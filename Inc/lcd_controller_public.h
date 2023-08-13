/*lcd_controller_public.h*/

void LCD_ControllerInit(void);
void LCD_Clear(void);
void LCD_Home(void);
void LCD_CursorMode(uint8_t show_cursor, uint8_t blink_cursor);
void LCD_CursorSet(uint8_t row, uint8_t column);
void LCD_Write(char* text);
