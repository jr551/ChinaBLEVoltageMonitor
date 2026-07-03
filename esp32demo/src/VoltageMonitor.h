#pragma once

#include <Arduino.h>
#include <BLEDevice.h>

struct MonitorState {
    bool connected = false;
    bool scanning = false;
    bool voltageValid = false;
    float volts = 0.0F;
    uint8_t flagA = 0;
    uint8_t flagB = 0;
    int rssi = -127;
    unsigned long updatedMs = 0;
    String deviceName;
};

class VoltageMonitor {
  public:
    void begin();
    void tick();
    const MonitorState& state() const { return state_; }

  private:
    static void notifyCallback(BLERemoteCharacteristic*, uint8_t* data, size_t length, bool);
    static VoltageMonitor* callbackTarget_;

    bool scanAndConnect();
    bool connectTo(BLEAdvertisedDevice& device);
    void requestVoltage();
    void receive(const uint8_t* data, size_t length);
    void disconnect();

    MonitorState state_;
    BLEClient* client_ = nullptr;
    BLERemoteCharacteristic* notify_ = nullptr;
    BLERemoteCharacteristic* write_ = nullptr;
    uint8_t receiveBuffer_[128] = {};
    size_t receiveLength_ = 0;
    unsigned long lastPollMs_ = 0;
    unsigned long lastConnectAttemptMs_ = 0;
};
