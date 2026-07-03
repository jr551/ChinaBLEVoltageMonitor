#pragma once

#include <Arduino.h>

#include "SagHealth.h"
#include "VoltageMonitor.h"
#include "ChargeDetector.h"

class TelemetryUploader {
  public:
    void begin();
    void tick(const MonitorState& monitor, const SagHealthState& sag, const ChargeState& charge);

  private:
    unsigned long lastConnectAttemptMs_ = 0;
    unsigned long lastPostMs_ = 0;
};
