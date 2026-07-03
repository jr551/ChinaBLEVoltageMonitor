#pragma once

#include <Arduino.h>
#include <Preferences.h>

struct SagHealthState {
    bool inSag = false;
    bool scoreValid = false;
    float referenceVoltage = 0.0F;
    float lastSagVolts = 0.0F;
    uint8_t score = 0;
    uint8_t eventCount = 0;
};

class SagHealth {
  public:
    void begin();
    void add(float volts, unsigned long timeMs);
    void reset();
    const SagHealthState& state() const { return state_; }

  private:
    static constexpr uint8_t kMaxEvents = 50;
    void finishEvent(float volts, unsigned long timeMs, bool recovered);
    void save();
    void updateScore();
    float idleMedian() const;
    static float median(float* values, uint8_t count);

    Preferences preferences_;
    SagHealthState state_;
    float idle_[6] = {};
    uint8_t idleCount_ = 0;
    float sagPercents_[kMaxEvents] = {};
    bool active_ = false;
    float activeBaseline_ = 0.0F;
    float activeMinimum_ = 0.0F;
    unsigned long activeStartedMs_ = 0;
    unsigned long lastSampleMs_ = 0;
};
