/*lcd_controller_private.h*/

#include <stdint.h>
#include <string.h>
#include "lcd_controller_public.h"
#include "main.h"

/*FreeRTOS related header files*/
#include "freeRTOS.h"
#include "task.h"
#include "queue.h"

/**********LCD Instructions***********/
#define LCD_CLEAR 		0x1
#define LCD_HOME 		0x2
#define LCD_ENTRY_MODE		0x4
#define LCD_DISPLAY_CTRL	0x8
#define LCD_SHIFT		0x10
#define LCD_FUNC_SET		0x20
#define LCD_CURSOR_SET		0x80

/**********Instruction Flags**********/
#define LCD_DONT_CARE		0
/*LCD_ENTRY_MODE*/
#define LCD_AUTO_INCREMENT	0x2
#define LCD_AUTO_DECREMENT	0
#define LCD_AUTO_SHIFT_DISPLAY 	0x1
#define LCD_AUTO_SHIFT_CURSOR	0
/*LCD_DISPLAY_CTRL*/
#define LCD_DISPLAY_ON		0x4
#define LCD_DISPLAY_OFF		0
#define LCD_CURSOR_ON		0x2
#define LCD_CURSOR_OFF		0
#define LCD_CURSOR_BLINK_ON	0x1
#define LCD_CURSOR_BLINK_OFF	0
/*LCD_SHIFT*/
#define LCD_SHIFT_DISPLAY	0x8
#define LCD_SHIFT_CURSOR	0
#define LCD_SHIFT_RIGHT		0x4
#define LCD_SHIFT_LEFT		0
/*LCD_FUNC_SET*/
#define LCD_5x11_FONT		0x4
#define LCD_5x8_FONT		0
#define LCD_8BIT_MODE	        0x10
#define LCD_4BIT_MODE		0
#define LCD_2LINE_MODE		0x8
#define LCD_1LINE_MODE		0
/*************************************/

#define LCD_UNDEF_GPIO_Port 	0
#define LCD_UNDEF_Pin		0

#ifdef LCD_D0_GPIO_Port
#define LCD_8BIT_PINS_DEFINED
#define LCD_4BIT_PINS_DEFINED
#elif defined(LCD_D4_GPIO_Port)
#define LCD_4BIT_PINS_DEFINED
#endif

#ifdef LCD_SPI_CS_GPIO_Port
#define LCD_SPI_PINS_DEFINED
#endif

#ifdef LCD_8BIT_PINS_DEFINED
GPIO_TypeDef* const LCD_DATA_GPIO_PORTS[] =
{
	LCD_D0_GPIO_Port, LCD_D1_GPIO_Port, LCD_D2_GPIO_Port, LCD_D3_GPIO_Port,
	LCD_D4_GPIO_Port, LCD_D5_GPIO_Port, LCD_D6_GPIO_Port, LCD_D7_GPIO_Port
};

const uint16_t LCD_DATA_GPIO_PINS[] =
{
	LCD_D0_Pin, LCD_D1_Pin, LCD_D2_Pin, LCD_D3_Pin,
	LCD_D4_Pin, LCD_D5_Pin, LCD_D6_Pin, LCD_D7_Pin
};
#elif LCD_4BIT_PINS_DEFINED
GPIO_TypeDef* const LCD_DATA_GPIO_PORTS[] =
{
	LCD_UNDEF_GPIO_Port, LCD_UNDEF_GPIO_Port, LCD_UNDEF_GPIO_Port, LCD_UNDEF_GPIO_Port,
	LCD_D4_GPIO_Port, LCD_D5_GPIO_Port, LCD_D6_GPIO_Port, LCD_D7_GPIO_Port
};

const uint16_t LCD_DATA_GPIO_PINS[] =
{
	LCD_UNDEF_Pin, LCD_UNDEF_Pin, LCD_UNDEF_Pin, LCD_UNDEF_Pin,
	LCD_D4_Pin, LCD_D5_Pin, LCD_D6_Pin, LCD_D7_Pin
};
#endif

typedef enum {LCD_REG_INSTRUCTION, LCD_REG_DATA} LCD_Register_e;
typedef enum {LCD_WRITE, LCD_READ} LCD_Operation_e;

typedef struct
{
	uint8_t data;
	LCD_Register_e dest_reg;
} LCD_Frame_t;

TaskHandle_t LCD_init_task;
TaskHandle_t LCD_write_task;
QueueHandle_t LCD_write_queue;

uint8_t cursor_showing;
uint8_t cursor_blinking;

#ifdef LCD_SPI_PINS_DEFINED
extern SPI_HandleTypeDef hspi2;
#endif

void LCD_WriteHandler(void* use_4bit_mode);
void LCD_WritePins(LCD_Mode_e LCD_mode, LCD_Register_e dest_reg, LCD_Operation_e operation, uint8_t data);
void LCD_8Bit_WritePins(LCD_Register_e dest_reg, LCD_Operation_e operation, uint8_t data);
void LCD_4Bit_WritePins(LCD_Register_e dest_reg, LCD_Operation_e operation, uint8_t data);
void LCD_SPI_WritePins(LCD_Register_e dest_reg, LCD_Operation_e operation, uint8_t data);
void LCD_WriteInitSeq(uint8_t use_4bit_mode);
void LCD_SetFuncMode(uint8_t data_length_flag, uint8_t line_num_flag, uint8_t font_type_flag);
void LCD_SetEntryMode(uint8_t shift_dir_flag, uint8_t shift_mode_flag);
void LCD_TurnOnDisplay(void);
void LCD_TurnOffDisplay(void);
void LCD_ClearDisplay(void);
