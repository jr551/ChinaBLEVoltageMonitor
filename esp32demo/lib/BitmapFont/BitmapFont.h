#pragma once

#include <Arduino.h>

namespace BitmapFont {

void drawChar(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg, uint8_t scale);
void drawString(uint16_t x, uint16_t y, const String& text, uint16_t fg, uint16_t bg, uint8_t scale);
void drawString(uint16_t x, uint16_t y, const char* text, uint16_t fg, uint16_t bg, uint8_t scale);
void drawStringCentered(uint16_t y, const String& text, uint16_t fg, uint16_t bg, uint8_t scale);
uint16_t textWidth(const String& text, uint8_t scale);

}  // namespace BitmapFont
