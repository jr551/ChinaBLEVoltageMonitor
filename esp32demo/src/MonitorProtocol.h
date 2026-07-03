#pragma once

#include <Arduino.h>

namespace MonitorProtocol {

constexpr uint8_t kLiveVoltageCommand[2] = {0x0B, 0x0B};

inline uint16_t crc16X25(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t index = 0; index < length; ++index) {
        crc ^= data[index];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crc = (crc & 1U) ? static_cast<uint16_t>((crc >> 1U) ^ 0x8408U)
                             : static_cast<uint16_t>(crc >> 1U);
        }
    }
    return static_cast<uint16_t>(crc ^ 0xFFFFU);
}

inline size_t buildRequest(const uint8_t command[2], uint8_t* output, size_t capacity) {
    constexpr size_t kLength = 10;
    if (capacity < kLength) {
        return 0;
    }

    output[0] = 0x40;
    output[1] = 0x40;
    output[2] = kLength;
    output[3] = 0x00;
    output[4] = command[0];
    output[5] = command[1];
    const uint16_t crc = crc16X25(output, 6);
    output[6] = static_cast<uint8_t>(crc & 0xFF);
    output[7] = static_cast<uint8_t>(crc >> 8);
    output[8] = 0x0D;
    output[9] = 0x0A;
    return kLength;
}

inline bool decodeLiveVoltage(const uint8_t* packet, size_t length, float& volts, uint8_t& flagA, uint8_t& flagB) {
    if (length != 14 || packet[0] != 0x24 || packet[1] != 0x24 ||
        packet[2] != 14 || packet[3] != 0 || packet[4] != 0x4B || packet[5] != 0x0B ||
        packet[12] != 0x0D || packet[13] != 0x0A) {
        return false;
    }
    const uint16_t expected = static_cast<uint16_t>(packet[10]) |
                              (static_cast<uint16_t>(packet[11]) << 8);
    if (crc16X25(packet, 10) != expected) {
        return false;
    }
    const uint16_t centivolts = static_cast<uint16_t>(packet[6]) |
                                (static_cast<uint16_t>(packet[7]) << 8);
    volts = centivolts / 100.0F;
    flagA = packet[8];
    flagB = packet[9];
    return true;
}

}  // namespace MonitorProtocol
