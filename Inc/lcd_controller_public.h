/*lcd_controller_public.h*/

typedef enum {LCD_4BIT, LCD_8BIT, LCD_SPI} LCD_Mode_e;

void LCD_InitController(LCD_Mode_e LCD_mode);
void LCD_TurnOnDisplay(void);
void LCD_TurnOffDisplay(void);
void LCD_WriteText(const char* text);
void LCD_ClearDisplay(void);
void LCD_SetCursorMode(uint8_t show_cursor, uint8_t blink_cursor);
void LCD_SetCursorPos(uint8_t row, uint8_t column);
void LCD_SetCursorHome(void);
