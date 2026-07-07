#pragma once

#include "VoltageMonitor.h"
#include "SagHealth.h"
#include "ChargeDetector.h"

class VoltageUI {
  public:
    void begin();
    void render(const MonitorState& state, const SagHealthState& sag, const ChargeState& charge);

  private:
    void drawStatic();
    void drawFuelBar(uint8_t percent);
    void drawHistory(const SagHealthState& sag, const ChargeState& charge);
    uint8_t fuelPercent(float volts) const;
    void addHistory(float volts);

    String lastStatus_;
    String lastVoltage_;
    String lastFuel_;
    String lastBottom_;
    uint8_t lastFuelPercent_ = 255;
    bool lastCharging_ = false;
    float history_[150] = {};
    uint8_t historyCount_ = 0;
    unsigned long lastFrameMs_ = 0;
    unsigned long lastSampleMs_ = 0;
};
