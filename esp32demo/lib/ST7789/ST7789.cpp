// ST7789 driver for ESP32-C6-LCD-1.47
// Based on official Waveshare demo code, adapted for PlatformIO

#include "ST7789.h"

static void SPI_Write(uint8_t data) {
    SPI.transfer(data);
}

static void SPI_Write16(uint16_t data) {
    SPI.transfer16(data);
}

static void LCD_WriteCommand(uint8_t cmd) {
    SPI.beginTransaction(SPISettings(LCD_SPI_FREQ, MSBFIRST, SPI_MODE0));
    digitalWrite(LCD_PIN_CS, LOW);
    digitalWrite(LCD_PIN_DC, LOW);
    SPI_Write(cmd);
    digitalWrite(LCD_PIN_CS, HIGH);
    SPI.endTransaction();
}

static void LCD_WriteData(uint8_t data) {
    SPI.beginTransaction(SPISettings(LCD_SPI_FREQ, MSBFIRST, SPI_MODE0));
    digitalWrite(LCD_PIN_CS, LOW);
    digitalWrite(LCD_PIN_DC, HIGH);
    SPI_Write(data);
    digitalWrite(LCD_PIN_CS, HIGH);
    SPI.endTransaction();
}

static void LCD_WriteData16(uint16_t data) {
    SPI.beginTransaction(SPISettings(LCD_SPI_FREQ, MSBFIRST, SPI_MODE0));
    digitalWrite(LCD_PIN_CS, LOW);
    digitalWrite(LCD_PIN_DC, HIGH);
    SPI_Write16(data);
    digitalWrite(LCD_PIN_CS, HIGH);
    SPI.endTransaction();
}

static void LCD_Reset(void) {
    digitalWrite(LCD_PIN_CS, LOW);
    delay(50);
    digitalWrite(LCD_PIN_RST, LOW);
    delay(50);
    digitalWrite(LCD_PIN_RST, HIGH);
    delay(50);
}

void LCD_Backlight_Init(void) {
    ledcAttach(LCD_PIN_BL, LCD_BL_PWM_FREQ, LCD_BL_PWM_RES);
    ledcWrite(LCD_PIN_BL, 0);
}

void LCD_Backlight_Set(uint8_t percent) {
    if (percent > 100) {
        percent = 100;
    }
    if (percent == 0) {
        ledcWrite(LCD_PIN_BL, 0);
        return;
    }
    // Quadratic curve: dim at low UI %, and cap max duty below 1023 to limit LED heat.
    constexpr uint32_t kDutyCap = 820;
    uint32_t duty = (static_cast<uint32_t>(percent) * percent * kDutyCap) / 10000;
    if (duty < 4) {
        duty = 4;
    }
    ledcWrite(LCD_PIN_BL, duty);
}

void LCD_Init(void) {
    pinMode(LCD_PIN_CS, OUTPUT);
    pinMode(LCD_PIN_DC, OUTPUT);
    pinMode(LCD_PIN_RST, OUTPUT);
    LCD_Backlight_Init();
    SPI.begin(LCD_PIN_SCLK, LCD_PIN_MISO, LCD_PIN_MOSI);

    LCD_Reset();

    // ST7789 init sequence (from Waveshare official demo)
    LCD_WriteCommand(0x11); // Sleep out
    delay(120);

    LCD_WriteCommand(0x36); // MADCTL — Memory Access Control
    LCD_WriteData(0x60);    // Landscape mode

    LCD_WriteCommand(0x3A); // COLMOD — pixel format
    LCD_WriteData(0x05);    // RGB565

    LCD_WriteCommand(0xB0);
    LCD_WriteData(0x00);
    LCD_WriteData(0xE8);

    LCD_WriteCommand(0xB2); // Porch control
    LCD_WriteData(0x0C);
    LCD_WriteData(0x0C);
    LCD_WriteData(0x00);
    LCD_WriteData(0x33);
    LCD_WriteData(0x33);

    LCD_WriteCommand(0xB7); // Gate control
    LCD_WriteData(0x35);

    LCD_WriteCommand(0xBB); // VCOM
    LCD_WriteData(0x35);

    LCD_WriteCommand(0xC0); // LCM control
    LCD_WriteData(0x2C);

    LCD_WriteCommand(0xC2); // VDV and VRH enable
    LCD_WriteData(0x01);

    LCD_WriteCommand(0xC3); // VRH set
    LCD_WriteData(0x13);

    LCD_WriteCommand(0xC4); // VDV set
    LCD_WriteData(0x20);

    LCD_WriteCommand(0xC6); // Frame rate control
    LCD_WriteData(0x0F);

    LCD_WriteCommand(0xD0); // Power control
    LCD_WriteData(0xA4);
    LCD_WriteData(0xA1);

    LCD_WriteCommand(0xD6);
    LCD_WriteData(0xA1);

    LCD_WriteCommand(0xE0); // Positive gamma
    LCD_WriteData(0xF0);
    LCD_WriteData(0x00);
    LCD_WriteData(0x04);
    LCD_WriteData(0x04);
    LCD_WriteData(0x04);
    LCD_WriteData(0x05);
    LCD_WriteData(0x29);
    LCD_WriteData(0x33);
    LCD_WriteData(0x3E);
    LCD_WriteData(0x38);
    LCD_WriteData(0x12);
    LCD_WriteData(0x12);
    LCD_WriteData(0x28);
    LCD_WriteData(0x30);

    LCD_WriteCommand(0xE1); // Negative gamma
    LCD_WriteData(0xF0);
    LCD_WriteData(0x07);
    LCD_WriteData(0x0A);
    LCD_WriteData(0x0D);
    LCD_WriteData(0x0B);
    LCD_WriteData(0x07);
    LCD_WriteData(0x28);
    LCD_WriteData(0x33);
    LCD_WriteData(0x3E);
    LCD_WriteData(0x36);
    LCD_WriteData(0x14);
    LCD_WriteData(0x14);
    LCD_WriteData(0x29);
    LCD_WriteData(0x32);

    LCD_WriteCommand(0x21); // Display inversion on
    LCD_WriteCommand(0x11); // Sleep out
    delay(120);
    LCD_WriteCommand(0x29); // Display on
}

void LCD_SetCursor(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    // Landscape mode: apply Y offset for the 172x320 panel window.
    LCD_WriteCommand(0x2A); // Column address set
    LCD_WriteData((x1 + LCD_OFFSET_X) >> 8);
    LCD_WriteData((x1 + LCD_OFFSET_X) & 0xFF);
    LCD_WriteData((x2 + LCD_OFFSET_X) >> 8);
    LCD_WriteData((x2 + LCD_OFFSET_X) & 0xFF);

    LCD_WriteCommand(0x2B); // Row address set
    LCD_WriteData((y1 + LCD_OFFSET_Y) >> 8);
    LCD_WriteData((y1 + LCD_OFFSET_Y) & 0xFF);
    LCD_WriteData((y2 + LCD_OFFSET_Y) >> 8);
    LCD_WriteData((y2 + LCD_OFFSET_Y) & 0xFF);

    LCD_WriteCommand(0x2C); // Memory write
}

void LCD_FillScreen(uint16_t color) {
    LCD_FillRect(0, 0, LCD_WIDTH, LCD_HEIGHT, color);
}

void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    LCD_SetCursor(x, y, x + w - 1, y + h - 1);
    SPI.beginTransaction(SPISettings(LCD_SPI_FREQ, MSBFIRST, SPI_MODE0));
    digitalWrite(LCD_PIN_CS, LOW);
    digitalWrite(LCD_PIN_DC, HIGH);
    for (uint32_t i = 0; i < (uint32_t)w * h; i++) {
        SPI_Write16(color);
    }
    digitalWrite(LCD_PIN_CS, HIGH);
    SPI.endTransaction();
}

void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
    LCD_SetCursor(x, y, x, y);
    LCD_WriteData16(color);
}
