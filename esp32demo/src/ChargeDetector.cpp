#include "ChargeDetector.h"

#include <math.h>

namespace {
constexpr unsigned long kMaximumGapMs = 10000;
constexpr unsigned long kWindowMs = 90000;
constexpr unsigned long kMinimumObservationMs = 20000;
constexpr unsigned long kStopObservationMs = 30000;
}

void ChargeDetector::clear(unsigned long cooldownUntil) {
    count_ = 0;
    state_ = {};
    cooldownUntilMs_ = cooldownUntil;
}

void ChargeDetector::add(float volts, unsigned long timeMs, bool ignore) {
    if (!isfinite(volts)) return;
    if (ignore) {
        clear(timeMs + 30000);
        lastTimeMs_ = timeMs;
        return;
    }
    if (lastTimeMs_ != 0 && timeMs - lastTimeMs_ > kMaximumGapMs) {
        clear();
    }
    lastTimeMs_ = timeMs;
    if (timeMs < cooldownUntilMs_) return;

    while (count_ > 0 && timeMs - times_[0] > kWindowMs) {
        memmove(volts_, volts_ + 1, sizeof(float) * (count_ - 1));
        memmove(times_, times_ + 1, sizeof(unsigned long) * (count_ - 1));
        --count_;
    }
    if (count_ == kMaxSamples) {
        memmove(volts_, volts_ + 1, sizeof(float) * (kMaxSamples - 1));
        memmove(times_, times_ + 1, sizeof(unsigned long) * (kMaxSamples - 1));
        --count_;
    }
    volts_[count_] = volts;
    times_[count_] = timeMs;
    ++count_;

    if (count_ < 6) return;
    const unsigned long duration = timeMs - times_[0];
    if (duration < kMinimumObservationMs) return;

    const uint8_t edgeCount = min<uint8_t>(3, count_ / 2);
    float start = 0.0F;
    float end = 0.0F;
    for (uint8_t index = 0; index < edgeCount; ++index) {
        start += volts_[index];
        end += volts_[count_ - edgeCount + index];
    }
    start /= edgeCount;
    end /= edgeCount;
    state_.riseVolts = end - start;
    state_.slopeVoltsPerMinute =
        state_.riseVolts / (static_cast<float>(duration) / 60000.0F);

    if (!state_.likelyCharging &&
        state_.riseVolts >= 0.05F && state_.slopeVoltsPerMinute >= 0.03F) {
        state_.likelyCharging = true;
    } else if (state_.likelyCharging && duration >= kStopObservationMs &&
               (state_.slopeVoltsPerMinute < 0.005F || end < start - 0.05F)) {
        state_.likelyCharging = false;
    }
}
