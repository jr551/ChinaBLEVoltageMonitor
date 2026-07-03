#include <Arduino.h>

#include "VoltageMonitor.h"
#include "VoltageUI.h"

namespace {
VoltageMonitor monitor;
VoltageUI ui;
}

void setup() {
    Serial.begin(115200);
    delay(250);
    ui.begin();
    monitor.begin();
    Serial.println("BLE Voltage Lab ESP32-C6 display demo");
}

void loop() {
    monitor.tick();
    ui.render(monitor.state());
    delay(5);
}
