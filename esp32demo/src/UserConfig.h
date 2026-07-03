#pragma once

// Optional telemetry upload. Change to true after filling every value below.
// Do not commit real credentials if this repository becomes public/shared.
constexpr bool WIFI_UPLOAD_ENABLED = false;
constexpr char WIFI_SSID[] = "YOUR_WIFI_SSID";
constexpr char WIFI_PSK[] = "YOUR_WIFI_PASSWORD";
constexpr char HTTP_POST_URL[] = "http://192.168.1.10:8080/ble-voltage";
constexpr unsigned long HTTP_POST_INTERVAL_MS = 60000;
