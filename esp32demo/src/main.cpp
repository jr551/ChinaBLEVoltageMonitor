#include <Arduino.h>

#include "VoltageMonitor.h"
#include "VoltageUI.h"
#include "SagHealth.h"
#include "TelemetryUploader.h"
#include "ChargeDetector.h"

namespace {
VoltageMonitor monitor;
VoltageUI ui;
SagHealth sagHealth;
TelemetryUploader uploader;
ChargeDetector chargeDetector;
unsigned long lastProcessedUpdateMs = 0;
}

void setup() {
    Serial.begin(115200);
    delay(250);
    ui.begin();
    sagHealth.begin();
    monitor.begin();
    uploader.begin();
    Serial.println("BLE Voltage Lab ESP32-C6 display demo");
}

void loop() {
    monitor.tick();
    const MonitorState& state = monitor.state();
    if (state.voltageValid && state.updatedMs != lastProcessedUpdateMs) {
        lastProcessedUpdateMs = state.updatedMs;
        sagHealth.add(state.volts, state.updatedMs);
        chargeDetector.add(state.volts, state.updatedMs, sagHealth.state().inSag);
    }
    ui.render(state, sagHealth.state(), chargeDetector.state());
    uploader.tick(state, sagHealth.state(), chargeDetector.state());
    delay(5);
}
