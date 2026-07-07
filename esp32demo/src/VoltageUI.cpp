#include "VoltageUI.h"

#include "BitmapFont.h"
#include "ST7789.h"

#include <math.h>

namespace {
constexpr uint16_t kBg = 0x0000;
constexpr uint16_t kPanel = 0x0841;
constexpr uint16_t kPanelDark = 0x0020;
constexpr uint16_t kInk = 0xFFFF;
constexpr uint16_t kMuted = 0xAD55;
constexpr uint16_t kLine = 0x2945;
constexpr uint16_t kAcid = 0xD7E6;
constexpr uint16_t kOrange = 0xFD20;
constexpr uint16_t kRed = 0xF800;
constexpr uint16_t kAmber = 0xFFE0;

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

uint16_t graphYFor(float volts, float low, float high, uint16_t graphY, uint16_t graphH) {
    if (high <= low) return graphY + graphH / 2;
    const float clamped = min(high, max(low, volts));
    return graphY + graphH - 1 - static_cast<uint16_t>(
        ((clamped - low) / (high - low)) * (graphH - 2));
}
}

void VoltageUI::begin() {
    LCD_Init();
    LCD_Backlight_Set(58);
    drawStatic();
}

void VoltageUI::drawStatic() {
    LCD_FillScreen(kBg);
    LCD_FillRect(0, 0, LCD_WIDTH, 26, kPanelDark);
    BitmapFont::drawString(8, 9, "BLE VOLTAGE", kMuted, kPanelDark, 1);
    BitmapFont::drawString(258, 9, "WAIT", kOrange, kPanelDark, 1);
    border(8, 56, 304, 84, kLine);
    LCD_FillRect(8, 150, 304, 14, kPanelDark);
    border(8, 150, 304, 14, kLine);
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
    LCD_FillRect(8, 150, 304, 14, kPanelDark);
    const uint16_t color = percent < 15 ? kRed : (percent < 35 ? kOrange : kAcid);
    const uint16_t width = static_cast<uint16_t>((296UL * percent) / 100UL);
    if (width > 0) LCD_FillRect(12, 154, width, 6, color);
    border(8, 150, 304, 14, kLine);
}

void VoltageUI::addHistory(float volts) {
    if (historyCount_ < 150) {
        history_[historyCount_++] = volts;
    } else {
        memmove(history_, history_ + 1, sizeof(float) * 149);
        history_[149] = volts;
    }
}

void VoltageUI::drawHistory(const SagHealthState& sag, const ChargeState& charge) {
    constexpr uint16_t graphX = 10;
    constexpr uint16_t graphY = 58;
    constexpr uint16_t graphW = 300;
    constexpr uint16_t graphH = 80;
    LCD_FillRect(graphX, graphY, graphW, graphH, kBg);
    LCD_FillRect(graphX, graphY + 19, graphW, 1, kLine);
    LCD_FillRect(graphX, graphY + 39, graphW, 1, kLine);
    LCD_FillRect(graphX, graphY + 59, graphW, 1, kLine);
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

    BitmapFont::drawString(graphX + 4, graphY + 3, String(high, 1), kMuted, kBg, 1);
    BitmapFont::drawString(graphX + 4, graphY + graphH - 11, String(low, 1), kMuted, kBg, 1);

    for (uint8_t index = 1; index < historyCount_; ++index) {
        const uint16_t x0 = graphX + (index - 1) * 2;
        const uint16_t x1 = graphX + index * 2;
        const uint16_t y0 = graphYFor(history_[index - 1], low, high, graphY, graphH);
        const uint16_t y1 = graphYFor(history_[index], low, high, graphY, graphH);
        line(x0, y0, x1, y1, kAcid);
        line(x0, y0 + 1, x1, y1 + 1, kAcid);
    }

    const float latest = history_[historyCount_ - 1];
    const uint16_t latestY = graphYFor(latest, low, high, graphY, graphH);
    const uint16_t markerColor = sag.inSag ? kOrange : (charge.likelyCharging ? kAmber : kInk);
    LCD_FillRect(graphX + graphW - 5, latestY > graphY + 1 ? latestY - 1 : latestY, 4, 4, markerColor);

    const uint16_t tagY = latestY < graphY + 12
        ? graphY + 12
        : (latestY > graphY + graphH - 18 ? graphY + graphH - 18 : latestY - 6);
    LCD_FillRect(graphX + graphW - 58, tagY, 54, 10, kPanelDark);
    BitmapFont::drawString(graphX + graphW - 55, tagY + 2, String(latest, 2) + "V", markerColor, kPanelDark, 1);

    if (sag.inSag) {
        BitmapFont::drawString(graphX + graphW - 44, graphY + 4, "SAG", kOrange, kBg, 1);
    } else if (charge.likelyCharging) {
        BitmapFont::drawString(graphX + graphW - 62, graphY + 4, "CHARGE", kAmber, kBg, 1);
    }
}

void VoltageUI::render(
    const MonitorState& state, const SagHealthState& sag, const ChargeState& charge) {
    if (millis() - lastFrameMs_ < 500) return;
    lastFrameMs_ = millis();

    const String status = charge.likelyCharging
        ? "CHARGE"
        : (state.connected
            ? "LIVE"
            : (state.scanning ? "SCAN" : "WAIT"));
    if (status != lastStatus_) {
        LCD_FillRect(232, 6, 82, 12, kPanelDark);
        BitmapFont::drawString(258, 9, status,
                               (state.connected || charge.likelyCharging) ? kAcid : kOrange,
                               kPanelDark, 1);
        lastStatus_ = status;
    }

    String voltageText = "--.--V";
    String fuelText = "--";
    uint8_t fuel = 0;
    uint16_t fuelColor = kMuted;
    if (state.voltageValid) {
        voltageText = String(state.volts, 2) + "V";
        const float fuelVoltage = sag.referenceVoltage > 0.0F
            ? sag.referenceVoltage : state.volts;
        fuel = fuelPercent(fuelVoltage);
        fuelText = String(fuel);
        fuelColor = fuel < 15 ? kRed : (fuel < 35 ? kAmber : kAcid);

        if (millis() - lastSampleMs_ >= 1000) {
            lastSampleMs_ = millis();
            addHistory(state.volts);
            drawHistory(sag, charge);
        }
    }

    if (voltageText != lastVoltage_) {
        LCD_FillRect(0, 30, 190, 24, kBg);
        const uint8_t voltageScale = 3;
        const uint16_t voltageY = 32;
        BitmapFont::drawString(8, voltageY, voltageText, state.voltageValid ? kInk : kMuted, kBg, voltageScale);
        lastVoltage_ = voltageText;
    }

    if (fuelText != lastFuel_) {
        LCD_FillRect(226, 30, 90, 24, kBg);
        BitmapFont::drawString(226, 32, fuelText + "%", fuelColor, kBg, 3);
        lastFuel_ = fuelText;
    }

    if (fuel != lastFuelPercent_) {
        drawFuelBar(fuel);
        lastFuelPercent_ = fuel;
    }

    String bottom = "";
    if (sag.scoreValid) bottom = "HEALTH " + String(sag.score) + "%";
    if (charge.likelyCharging) bottom = "CHARGING";
    if (sag.inSag) bottom = "SAGGING";
    if (bottom != lastBottom_) {
        LCD_FillRect(160, 8, 70, 8, kPanelDark);
        BitmapFont::drawString(160, 9, bottom, sag.inSag ? kOrange : kMuted, kPanelDark, 1);
        lastBottom_ = bottom;
    }

    if (charge.likelyCharging != lastCharging_) {
        border(0, 0, LCD_WIDTH, LCD_HEIGHT,
               charge.likelyCharging ? kAcid : kBg);
        if (charge.likelyCharging) {
            border(2, 2, LCD_WIDTH - 4, LCD_HEIGHT - 4, kAcid);
        } else {
            drawStatic();
            lastStatus_ = "";
            lastVoltage_ = "";
            lastFuel_ = "";
            lastBottom_ = "";
            lastFuelPercent_ = 255;
        }
        lastCharging_ = charge.likelyCharging;
    }
}
