#pragma once

#include "VoltageMonitor.h"

class VoltageUI {
  public:
    void begin();
    void render(const MonitorState& state);

  private:
    void drawStatic();
    void drawFuelBar(uint8_t percent);
    void drawHistory();
    uint8_t fuelPercent(float volts) const;
    void addHistory(float volts);

    float history_[58] = {};
    uint8_t historyCount_ = 0;
    unsigned long lastFrameMs_ = 0;
    unsigned long lastSampleMs_ = 0;
};
