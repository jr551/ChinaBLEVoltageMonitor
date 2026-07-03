#pragma once

#include <Arduino.h>

struct ChargeState {
    bool likelyCharging = false;
    float riseVolts = 0.0F;
    float slopeVoltsPerMinute = 0.0F;
};

class ChargeDetector {
  public:
    void add(float volts, unsigned long timeMs, bool ignore);
    const ChargeState& state() const { return state_; }

  private:
    static constexpr uint8_t kMaxSamples = 90;
    void clear(unsigned long cooldownUntil = 0);

    ChargeState state_;
    float volts_[kMaxSamples] = {};
    unsigned long times_[kMaxSamples] = {};
    uint8_t count_ = 0;
    unsigned long lastTimeMs_ = 0;
    unsigned long cooldownUntilMs_ = 0;
};
