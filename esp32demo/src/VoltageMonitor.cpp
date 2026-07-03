#include "VoltageMonitor.h"

#include "MonitorProtocol.h"

namespace {
constexpr char kServiceUuid[] = "0000fff0-0000-1000-8000-00805f9b34fb";
constexpr char kNotifyUuid[] = "0000fff1-0000-1000-8000-00805f9b34fb";
constexpr char kWriteUuid[] = "0000fff2-0000-1000-8000-00805f9b34fb";
constexpr unsigned long kPollMs = 1000;
constexpr unsigned long kReconnectMs = 8000;
constexpr uint32_t kScanSeconds = 5;
}

VoltageMonitor* VoltageMonitor::callbackTarget_ = nullptr;

void VoltageMonitor::begin() {
    callbackTarget_ = this;
    BLEDevice::init("BLE Voltage Lab");
    BLEDevice::setPower(ESP_PWR_LVL_P9);
}

void VoltageMonitor::tick() {
    if (client_ != nullptr && state_.connected && !client_->isConnected()) {
        disconnect();
    }

    if (!state_.connected) {
        if (lastConnectAttemptMs_ == 0 || millis() - lastConnectAttemptMs_ >= kReconnectMs) {
            lastConnectAttemptMs_ = millis();
            scanAndConnect();
        }
        return;
    }

    if (millis() - lastPollMs_ >= kPollMs) {
        lastPollMs_ = millis();
        requestVoltage();
        const int rssi = client_->getRssi();
        if (rssi >= -120 && rssi <= -10) {
            state_.rssi = rssi;
        }
    }
}

bool VoltageMonitor::scanAndConnect() {
    state_.scanning = true;
    Serial.println("BLE: scanning for FFF0 / bikebattery");

    BLEScan* scan = BLEDevice::getScan();
    scan->setActiveScan(true);
    BLEScanResults* results = scan->start(kScanSeconds, false);
    if (results == nullptr) {
        state_.scanning = false;
        return false;
    }

    int bestIndex = -1;
    int bestRssi = -127;
    BLEUUID service(kServiceUuid);
    for (int index = 0; index < results->getCount(); ++index) {
        BLEAdvertisedDevice device = results->getDevice(index);
        String name = device.haveName() ? device.getName() : "";
        String lowered = name;
        lowered.toLowerCase();
        const bool matches = device.isAdvertisingService(service) ||
                             lowered.indexOf("bikebattery") >= 0;
        if (matches && device.getRSSI() > bestRssi) {
            bestIndex = index;
            bestRssi = device.getRSSI();
        }
    }

    bool connected = false;
    if (bestIndex >= 0) {
        BLEAdvertisedDevice selected = results->getDevice(bestIndex);
        connected = connectTo(selected);
    }
    scan->clearResults();
    state_.scanning = false;
    if (!connected) {
        Serial.println("BLE: compatible monitor not found");
    }
    return connected;
}

bool VoltageMonitor::connectTo(BLEAdvertisedDevice& device) {
    Serial.printf("BLE: connecting to %s (%s)\n",
                  device.haveName() ? device.getName().c_str() : "<unnamed>",
                  device.getAddress().toString().c_str());
    client_ = BLEDevice::createClient();
    if (client_ == nullptr ||
        !client_->connect(device.getAddress(), device.getAddressType())) {
        disconnect();
        return false;
    }

    BLERemoteService* service = client_->getService(BLEUUID(kServiceUuid));
    if (service == nullptr) {
        disconnect();
        return false;
    }
    notify_ = service->getCharacteristic(BLEUUID(kNotifyUuid));
    write_ = service->getCharacteristic(BLEUUID(kWriteUuid));
    if (notify_ == nullptr || write_ == nullptr || !notify_->canNotify()) {
        disconnect();
        return false;
    }

    notify_->registerForNotify(notifyCallback);
    state_.connected = true;
    state_.deviceName = device.haveName() ? device.getName() : "bikebattery";
    state_.rssi = device.getRSSI();
    receiveLength_ = 0;
    Serial.println("BLE: connected");
    requestVoltage();
    return true;
}

void VoltageMonitor::requestVoltage() {
    if (write_ == nullptr || !state_.connected) {
        return;
    }
    uint8_t request[10];
    const size_t length = MonitorProtocol::buildRequest(
        MonitorProtocol::kLiveVoltageCommand, request, sizeof(request));
    write_->writeValue(request, length, false);
}

void VoltageMonitor::notifyCallback(
    BLERemoteCharacteristic*, uint8_t* data, size_t length, bool) {
    if (callbackTarget_ != nullptr) {
        callbackTarget_->receive(data, length);
    }
}

void VoltageMonitor::receive(const uint8_t* data, size_t length) {
    if (length > sizeof(receiveBuffer_) - receiveLength_) {
        receiveLength_ = 0;
    }
    memcpy(receiveBuffer_ + receiveLength_, data, length);
    receiveLength_ += length;

    while (receiveLength_ >= 4) {
        size_t magic = 0;
        while (magic + 1 < receiveLength_ &&
               !(receiveBuffer_[magic] == 0x24 && receiveBuffer_[magic + 1] == 0x24)) {
            ++magic;
        }
        if (magic > 0) {
            memmove(receiveBuffer_, receiveBuffer_ + magic, receiveLength_ - magic);
            receiveLength_ -= magic;
        }
        if (receiveLength_ < 4) {
            return;
        }
        const size_t packetLength = static_cast<size_t>(receiveBuffer_[2]) |
                                    (static_cast<size_t>(receiveBuffer_[3]) << 8);
        if (packetLength < 10 || packetLength > sizeof(receiveBuffer_)) {
            receiveLength_ = 0;
            return;
        }
        if (receiveLength_ < packetLength) {
            return;
        }

        float volts = 0.0F;
        uint8_t flagA = 0;
        uint8_t flagB = 0;
        if (MonitorProtocol::decodeLiveVoltage(
                receiveBuffer_, packetLength, volts, flagA, flagB)) {
            state_.volts = volts;
            state_.flagA = flagA;
            state_.flagB = flagB;
            state_.voltageValid = true;
            state_.updatedMs = millis();
            Serial.printf("BLE: %.2f V flags=%u/%u\n", volts, flagA, flagB);
        }
        memmove(receiveBuffer_, receiveBuffer_ + packetLength,
                receiveLength_ - packetLength);
        receiveLength_ -= packetLength;
    }
}

void VoltageMonitor::disconnect() {
    if (client_ != nullptr && client_->isConnected()) {
        client_->disconnect();
    }
    state_.connected = false;
    state_.scanning = false;
    notify_ = nullptr;
    write_ = nullptr;
    client_ = nullptr;
    receiveLength_ = 0;
}
