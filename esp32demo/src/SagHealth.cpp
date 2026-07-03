#include "SagHealth.h"

#include <algorithm>
#include <math.h>

namespace {
constexpr float kMinimumSagVolts = 0.4F;
constexpr float kRecoveryMarginVolts = 0.15F;
constexpr unsigned long kMaximumGapMs = 2500;
constexpr unsigned long kMaximumEventMs = 30000;
}

void SagHealth::begin() {
    preferences_.begin("sag-health", false);
    const size_t storedBytes = preferences_.getBytesLength("events");
    if (storedBytes > 0) {
        const size_t bytes = min(storedBytes, sizeof(sagPercents_));
        preferences_.getBytes("events", sagPercents_, bytes);
        state_.eventCount = static_cast<uint8_t>(bytes / sizeof(float));
    }
    updateScore();
}

float SagHealth::median(float* values, uint8_t count) {
    if (count == 0) return 0.0F;
    std::sort(values, values + count);
    const uint8_t middle = count / 2;
    return count % 2 ? values[middle]
                     : (values[middle - 1] + values[middle]) * 0.5F;
}

float SagHealth::idleMedian() const {
    float copy[6];
    memcpy(copy, idle_, sizeof(float) * idleCount_);
    return median(copy, idleCount_);
}

void SagHealth::add(float volts, unsigned long timeMs) {
    if (!isfinite(volts)) return;
    if (lastSampleMs_ != 0 && timeMs - lastSampleMs_ > kMaximumGapMs) {
        idleCount_ = 0;
        active_ = false;
    }
    lastSampleMs_ = timeMs;

    if (active_) {
        activeMinimum_ = min(activeMinimum_, volts);
        const bool recovered = volts >= activeBaseline_ - kRecoveryMarginVolts;
        const bool timedOut = timeMs - activeStartedMs_ >= kMaximumEventMs;
        if (recovered || timedOut) {
            finishEvent(volts, timeMs, recovered);
        } else {
            state_.inSag = true;
            state_.referenceVoltage = activeBaseline_;
        }
        return;
    }

    const float baseline = idleMedian();
    if (idleCount_ >= 3 && baseline - volts >= kMinimumSagVolts) {
        active_ = true;
        activeBaseline_ = baseline;
        activeMinimum_ = volts;
        activeStartedMs_ = timeMs;
        state_.inSag = true;
        state_.referenceVoltage = baseline;
        return;
    }

    if (idleCount_ < 6) {
        idle_[idleCount_++] = volts;
    } else {
        memmove(idle_, idle_ + 1, sizeof(float) * 5);
        idle_[5] = volts;
    }
    state_.inSag = false;
    state_.referenceVoltage = volts;
}

void SagHealth::finishEvent(float volts, unsigned long /*timeMs*/, bool /*recovered*/) {
    const float sagVolts = activeBaseline_ - activeMinimum_;
    state_.lastSagVolts = sagVolts;
    if (sagVolts >= kMinimumSagVolts && activeBaseline_ > 0.0F) {
        const float percent = (sagVolts / activeBaseline_) * 100.0F;
        if (state_.eventCount < kMaxEvents) {
            sagPercents_[state_.eventCount++] = percent;
        } else {
            memmove(sagPercents_, sagPercents_ + 1, sizeof(float) * (kMaxEvents - 1));
            sagPercents_[kMaxEvents - 1] = percent;
        }
        save();
        updateScore();
    }
    active_ = false;
    idle_[0] = volts;
    idleCount_ = 1;
    state_.inSag = false;
    state_.referenceVoltage = volts;
}

void SagHealth::updateScore() {
    state_.scoreValid = state_.eventCount >= 3;
    if (!state_.scoreValid) {
        state_.score = 0;
        return;
    }

    float sorted[kMaxEvents];
    memcpy(sorted, sagPercents_, sizeof(float) * state_.eventCount);
    std::sort(sorted, sorted + state_.eventCount);
    const uint8_t bestCount = min<uint8_t>(5, state_.eventCount);
    const float personalBest = median(sorted, bestCount);

    const uint8_t recentCount = min<uint8_t>(5, state_.eventCount);
    float recent[5];
    memcpy(recent, sagPercents_ + state_.eventCount - recentCount,
           sizeof(float) * recentCount);
    const float recentMedian = median(recent, recentCount);
    state_.score = static_cast<uint8_t>(constrain(
        roundf((personalBest / recentMedian) * 100.0F), 0.0F, 100.0F));
}

void SagHealth::save() {
    preferences_.putBytes("events", sagPercents_,
                          sizeof(float) * state_.eventCount);
}

void SagHealth::reset() {
    memset(sagPercents_, 0, sizeof(sagPercents_));
    state_ = {};
    idleCount_ = 0;
    active_ = false;
    preferences_.remove("events");
}
