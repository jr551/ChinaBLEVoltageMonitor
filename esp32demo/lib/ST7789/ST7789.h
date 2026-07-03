#pragma once
#include <Arduino.h>
#include <SPI.h>

#define LCD_WIDTH   320
#define LCD_HEIGHT  172

#define LCD_SPI_FREQ               80000000
#define LCD_PIN_MISO               5
#define LCD_PIN_MOSI               6
#define LCD_PIN_SCLK               7
#define LCD_PIN_CS                 14
#define LCD_PIN_DC                 15
#define LCD_PIN_RST                21
#define LCD_PIN_BL                 22
#define LCD_BL_PWM_FREQ            1000
#define LCD_BL_PWM_RES             10

#define LCD_OFFSET_X 0
#define LCD_OFFSET_Y 34

void LCD_Init(void);
void LCD_SetCursor(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void LCD_FillScreen(uint16_t color);
void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void LCD_Backlight_Init(void);
void LCD_Backlight_Set(uint8_t percent);

// Color definitions (RGB565)
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_MAGENTA 0xF81F
#define COLOR_NAVY    0x000F
#define COLOR_ORANGE  0xFD20
