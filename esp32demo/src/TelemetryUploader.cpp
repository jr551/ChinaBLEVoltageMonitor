#include "TelemetryUploader.h"

#include "UserConfig.h"

#include <HTTPClient.h>
#include <WiFi.h>

namespace {
constexpr unsigned long kWifiRetryMs = 30000;
}

void TelemetryUploader::begin() {
    if (!WIFI_UPLOAD_ENABLED) {
        Serial.println("HTTP upload: disabled in UserConfig.h");
        return;
    }
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PSK);
    lastConnectAttemptMs_ = millis();
    Serial.printf("WiFi: connecting to %s\n", WIFI_SSID);
}

void TelemetryUploader::tick(
    const MonitorState& monitor, const SagHealthState& sag) {
    if (!WIFI_UPLOAD_ENABLED) return;

    if (WiFi.status() != WL_CONNECTED) {
        if (millis() - lastConnectAttemptMs_ >= kWifiRetryMs) {
            lastConnectAttemptMs_ = millis();
            WiFi.disconnect();
            WiFi.begin(WIFI_SSID, WIFI_PSK);
            Serial.println("WiFi: reconnecting");
        }
        return;
    }

    if (!monitor.voltageValid ||
        (lastPostMs_ != 0 && millis() - lastPostMs_ < HTTP_POST_INTERVAL_MS)) {
        return;
    }
    lastPostMs_ = millis();

    String body = "{";
    body += "\"voltage\":" + String(monitor.volts, 2);
    body += ",\"flag_a\":" + String(monitor.flagA);
    body += ",\"flag_b\":" + String(monitor.flagB);
    body += ",\"ble_rssi\":" + String(monitor.rssi);
    body += ",\"sag_active\":" + String(sag.inSag ? "true" : "false");
    body += ",\"fuel_reference_voltage\":" + String(sag.referenceVoltage, 2);
    body += ",\"sag_events\":" + String(sag.eventCount);
    if (sag.scoreValid) {
        body += ",\"relative_sag_health\":" + String(sag.score);
    } else {
        body += ",\"relative_sag_health\":null";
    }
    body += "}";

    HTTPClient http;
    http.begin(HTTP_POST_URL);
    http.addHeader("Content-Type", "application/json");
    const int status = http.POST(body);
    Serial.printf("HTTP: POST status=%d body=%s\n", status, body.c_str());
    http.end();
}
