#include "VoltageUI.h"

#include "BitmapFont.h"
#include "ST7789.h"

#include <math.h>

namespace {
constexpr uint16_t kBg = 0x0841;
constexpr uint16_t kPanel = 0x18E4;
constexpr uint16_t kPanelDark = 0x10A2;
constexpr uint16_t kInk = 0xE73C;
constexpr uint16_t kMuted = 0x9492;
constexpr uint16_t kLine = 0x4208;
constexpr uint16_t kAcid = 0xBFCB;
constexpr uint16_t kOrange = 0xFCCA;
constexpr uint16_t kRed = 0xF986;

void border(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    LCD_FillRect(x, y, w, 1, color);
    LCD_FillRect(x, y + h - 1, w, 1, color);
    LCD_FillRect(x, y, 1, h, color);
    LCD_FillRect(x + w - 1, y, 1, h, color);
}

void line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
    int dx = abs(static_cast<int>(x1) - static_cast<int>(x0));
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(static_cast<int>(y1) - static_cast<int>(y0));
    int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;
    while (true) {
        LCD_DrawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        const int twice = 2 * error;
        if (twice >= dy) {
            error += dy;
            x0 = static_cast<uint16_t>(static_cast<int>(x0) + sx);
        }
        if (twice <= dx) {
            error += dx;
            y0 = static_cast<uint16_t>(static_cast<int>(y0) + sy);
        }
    }
}
}

void VoltageUI::begin() {
    LCD_Init();
    LCD_Backlight_Set(58);
    drawStatic();
}

void VoltageUI::drawStatic() {
    LCD_FillScreen(kBg);
    LCD_FillRect(0, 0, LCD_WIDTH, 25, kPanelDark);
    BitmapFont::drawString(10, 8, "BLE VOLTAGE LAB", kInk, kPanelDark, 1);

    LCD_FillRect(8, 32, 201, 82, kPanel);
    border(8, 32, 201, 82, kLine);
    BitmapFont::drawString(18, 41, "TERMINAL VOLTAGE", kMuted, kPanel, 1);

    LCD_FillRect(216, 32, 96, 82, kPanel);
    border(216, 32, 96, 82, kLine);
    BitmapFont::drawString(226, 42, "PACK", kMuted, kPanel, 1);
    BitmapFont::drawString(226, 57, "10S LI-ION", kAcid, kPanel, 1);
    BitmapFont::drawString(226, 75, "FULL 42.0V", kInk, kPanel, 1);

    LCD_FillRect(8, 121, 304, 43, kPanelDark);
    border(8, 121, 304, 43, kLine);
    BitmapFont::drawString(17, 128, "VOLTAGE TRACE", kMuted, kPanelDark, 1);
}

uint8_t VoltageUI::fuelPercent(float volts) const {
    const float perCell = volts / 10.0F;
    struct Point { float volts; uint8_t percent; };
    constexpr Point curve[] = {
        {3.0F, 0}, {3.3F, 5}, {3.5F, 10}, {3.6F, 20}, {3.7F, 40},
        {3.8F, 60}, {3.9F, 75}, {4.0F, 85}, {4.1F, 95}, {4.2F, 100},
    };
    if (perCell <= curve[0].volts) return 0;
    if (perCell >= curve[9].volts) return 100;
    for (uint8_t index = 1; index < 10; ++index) {
        if (perCell <= curve[index].volts) {
            const Point lower = curve[index - 1];
            const Point upper = curve[index];
            const float position = (perCell - lower.volts) /
                                   (upper.volts - lower.volts);
            return static_cast<uint8_t>(roundf(
                lower.percent + position * (upper.percent - lower.percent)));
        }
    }
    return 0;
}

void VoltageUI::drawFuelBar(uint8_t percent) {
    LCD_FillRect(18, 94, 181, 11, kPanelDark);
    const uint16_t color = percent < 15 ? kRed : (percent < 35 ? kOrange : kAcid);
    const uint16_t width = static_cast<uint16_t>((177UL * percent) / 100UL);
    if (width > 0) LCD_FillRect(20, 96, width, 7, color);
    border(18, 94, 181, 11, kLine);
}

void VoltageUI::addHistory(float volts) {
    if (historyCount_ < 58) {
        history_[historyCount_++] = volts;
    } else {
        memmove(history_, history_ + 1, sizeof(float) * 57);
        history_[57] = volts;
    }
}

void VoltageUI::drawHistory() {
    LCD_FillRect(17, 140, 286, 17, kPanelDark);
    if (historyCount_ < 2) return;
    float low = history_[0];
    float high = history_[0];
    for (uint8_t index = 1; index < historyCount_; ++index) {
        low = min(low, history_[index]);
        high = max(high, history_[index]);
    }
    if (high - low < 0.2F) {
        high += 0.1F;
        low -= 0.1F;
    }
    for (uint8_t index = 1; index < historyCount_; ++index) {
        const uint16_t x0 = 17 + (index - 1) * 5;
        const uint16_t x1 = 17 + index * 5;
        const uint16_t y0 = 156 - static_cast<uint16_t>(
            ((history_[index - 1] - low) / (high - low)) * 15.0F);
        const uint16_t y1 = 156 - static_cast<uint16_t>(
            ((history_[index] - low) / (high - low)) * 15.0F);
        line(x0, y0, x1, y1, kAcid);
    }
}

void VoltageUI::render(const MonitorState& state) {
    if (millis() - lastFrameMs_ < 250) return;
    lastFrameMs_ = millis();

    LCD_FillRect(236, 7, 75, 10, kPanelDark);
    const char* status = state.connected ? "CONNECTED" :
                         (state.scanning ? "SCANNING" : "SEARCHING");
    BitmapFont::drawString(236, 8, status,
                           state.connected ? kAcid : kOrange, kPanelDark, 1);

    LCD_FillRect(18, 56, 181, 33, kPanel);
    if (state.voltageValid) {
        BitmapFont::drawString(18, 57, String(state.volts, 2), kInk, kPanel, 4);
        BitmapFont::drawString(158, 70, "V", kAcid, kPanel, 2);
        const uint8_t fuel = fuelPercent(state.volts);
        BitmapFont::drawString(226, 94, String(fuel) + "/100", kInk, kPanel, 1);
        drawFuelBar(fuel);

        if (millis() - lastSampleMs_ >= 1000) {
            lastSampleMs_ = millis();
            addHistory(state.volts);
        }
    } else {
        BitmapFont::drawString(18, 65, "--.--", kMuted, kPanel, 3);
        BitmapFont::drawString(226, 94, "--/100", kMuted, kPanel, 1);
        drawFuelBar(0);
    }

    LCD_FillRect(226, 107, 77, 6, kPanel);
    String flags = "F " + String(state.flagA) + "/" + String(state.flagB);
    if (state.rssi > -120) flags += " " + String(state.rssi);
    BitmapFont::drawString(226, 106, flags, kMuted, kPanel, 1);
    drawHistory();
}
