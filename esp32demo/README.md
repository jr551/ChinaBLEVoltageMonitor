# BLE Voltage Lab — ESP32-C6 screen demo

Standalone PlatformIO firmware for the exact display board used by the
reference project:

- **Waveshare ESP32-C6-LCD-1.47**
- ESP32-C6
- ST7789, 172×320 pixels
- landscape UI at 320×172

The project contains no vape-specific code. It reuses only the proven board
bring-up details and small ST7789/bitmap-font implementation from


## What it does

- initializes the exact onboard display and backlight;
- scans for BLE service `FFF0` or local name `bikebattery`;
- connects to notify characteristic `FFF1`;
- sends the verified live-voltage request to `FFF2` once per second;
- validates CRC-16/X-25 responses;
- displays live voltage, raw state flags, RSSI, a 10S fuel estimate out of 100,
  and a short voltage trace;
- reconnects automatically if the monitor disappears.

Only the verified read-only command `0B 0B` is sent.

## Build

Install [PlatformIO](https://platformio.org/), connect the board over USB-C,
then run:

```bash
cd esp32demo
pio run
pio run --target upload
pio device monitor
```

The firmware uses USB CDC at 115200 baud.

## Confirmed board wiring

| LCD signal | GPIO |
|---|---:|
| MOSI | 6 |
| MISO | 5 |
| SCLK | 7 |
| CS | 14 |
| DC | 15 |
| Reset | 21 |
| Backlight PWM | 22 |

The SPI display runs at 80 MHz in RGB565. Landscape orientation requires an
offset of X=0, Y=34. These details came from the working reference firmware,
which in turn records them as confirmed against the official Waveshare demo.

## First hardware test

1. Power the KONNWEI monitor and keep it nearby.
2. Flash this firmware.
3. The screen should move from `SCANNING` to `CONNECTED`.
4. A voltage such as `40.87 V` should appear.
5. Confirm the same value with a trusted meter before relying on it.

This first version is intentionally a display/transport demo. Sag history
persistence and controls can be added after the board and live BLE path have
been tested.
