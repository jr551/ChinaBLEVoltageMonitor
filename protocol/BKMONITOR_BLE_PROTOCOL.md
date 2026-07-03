# BKmonitor BLE protocol reverse-engineering notes

Reverse-engineered from BKmonitor Android app 1.1.5
(`com.jiawei.batteryonline`) and verified against a live device in July 2026.

This document describes the proprietary Bluetooth Low Energy protocol used by
at least one BKmonitor-compatible battery monitor. The tested retail unit was
KONNWEI-branded and advertised over BLE as `bikebattery`. Visually similar
BK300 units are sold under other names and may use the same protocol, although
matching hardware or branding does not prove firmware compatibility.

## Summary

The monitor presents a BLE serial-style service with one transmit and one
receive characteristic. Application messages have a simple binary envelope,
CRC-16/X-25 protection, and little-endian numeric fields.

The protocol is not a BMS protocol. The tested hardware primarily measures
terminal voltage and records voltage behaviour. The app uses that data for
battery, cranking, charging, ripple, and historical tests. No cell voltages,
pack current, or temperature telemetry were observed.

No pairing or application-layer authentication was required for the tested
read operations. Treat write/configuration commands cautiously.

## BLE transport

| Purpose | UUID | Properties |
|---|---:|---|
| Service | `FFF0` | Primary service |
| Device to client | `FFF1` | Notify |
| Client to device | `FFF2` | Write Without Response |

Observed characteristic metadata:

- `FFF1` has the standard Client Characteristic Configuration descriptor
  (`2902`).
- `FFF2` has a Characteristic User Description descriptor (`2901`) whose value
  is `VOL Channel`.

The tested advertisement contained:

```text
Local name:        bikebattery
Service UUID:      FFF0
Manufacturer data: <6 device-specific bytes, redacted>
```

macOS presents a privacy-preserving CoreBluetooth UUID rather than the
peripheral's public BLE address, so that identifier is intentionally omitted.

## Packet format

### Client request

```text
Offset  Size  Meaning
0       2     Magic: 40 40 ("@@")
2       2     Total packet length, uint16 little-endian
4       2     Command
6       N     Payload
6+N     2     CRC-16/X-25, uint16 little-endian
8+N     2     Terminator: 0D 0A
```

### Device response

```text
Offset  Size  Meaning
0       2     Magic: 24 24 ("$$")
2       2     Total packet length, uint16 little-endian
4       2     Response command
6       N     Payload
6+N     2     CRC-16/X-25, uint16 little-endian
8+N     2     Terminator: 0D 0A
```

For the verified commands, the response command is the request command with
bit `0x40` set in its first byte:

```text
02 01 -> 42 01
0B 0B -> 4B 0B
```

The length is the length of the complete packet, not merely the payload.

## CRC

The CRC covers every byte from the two-byte magic through the end of the
payload. It does not include the CRC field or `0D 0A` terminator.

Parameters:

```text
Name:    CRC-16/X-25
Poly:    0x1021 (reflected implementation uses 0x8408)
Init:    0xFFFF
RefIn:   true
RefOut:  true
XorOut:  0xFFFF
Storage: little-endian
```

Minimal reference implementation:

```python
def crc16_x25(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ 0x8408 if crc & 1 else crc >> 1
    return crc ^ 0xFFFF


def request(command: bytes, payload: bytes = b"") -> bytes:
    length = 10 + len(payload)
    body = b"@@" + length.to_bytes(2, "little") + command + payload
    return body + crc16_x25(body).to_bytes(2, "little") + b"\r\n"
```

## Verified read commands

### `0B 0B`: live voltage and state

Request:

```text
40 40 0A 00 0B 0B A9 B2 0D 0A
```

Observed response:

```text
24 24 0E 00 4B 0B F7 0F 02 01 6C A7 0D 0A
```

Payload:

```text
F7 0F 02 01
^^^^
uint16 little-endian = 0x0FF7 = 4087
voltage = 4087 / 100 = 40.87 V
```

Decoded structure:

```text
Offset  Size  Meaning
0       2     Terminal voltage in centivolts, uint16 little-endian
2       1     State flag A
3       1     State flag B
```

The app treats flag A as a connection/availability condition and flag B as a
state used by its battery/collection indicator. Their complete enum values are
not yet established, so clients should preserve and display the raw values.

### `02 01`: device identifier

Request:

```text
40 40 0A 00 02 01 EB CA 0D 0A
```

The observed response had a total length of 28 bytes, response command
`42 01`, and an 18-byte ASCII payload. The tested unit's unique payload and
corresponding CRC are redacted.

## Command inventory recovered from app 1.1.5

Only the two commands above were exercised against hardware. The remaining
entries are named from app call sites and response parsers. Confidence labels
distinguish verified wire behaviour from inferred semantics.

| Command | App usage | Confidence / caution |
|---:|---|---|
| `01 00` | Initialisation/handshake used throughout connection flows | High confidence usage; response format not documented |
| `02 01` | Read device identifier/about information | Verified read |
| `02 03` | App/device update-state operation | Inferred; do not send casually |
| `03 01` | Enter/start firmware update path | State-changing; special fixed frame in app |
| `03 02` | Firmware update metadata/control | State-changing |
| `03 05` | Firmware update data/control | State-changing |
| `06 01` | Run a quick battery test with supplied test parameters | Test operation |
| `06 02` | Quick-test follow-up/result operation | Test operation |
| `09 01` | Waveform/test operation | Inferred from test activity |
| `0A 01` | Historical record request using a timestamp | Read-oriented, inferred |
| `0A 03` | Record deletion/progress operation | Destructive |
| `0B 01` | Request latest collected data using a timestamp | Read-oriented, high confidence |
| `0B 02` | Write scheduled-collection configuration | State-changing |
| `0B 03` | Request scheduled-collection entries/state | Read-oriented, inferred |
| `0B 04` | Collection/history navigation operation | Semantics incomplete |
| `0B 05` | Collection/history operation; used both with and without payload | Semantics incomplete |
| `0B 06` | Device setting write | State-changing |
| `0B 07` | Device/test setting write | State-changing |
| `0B 08` | Read collection enabled/disabled state | Read-oriented, high confidence |
| `0B 09` | Scheduled-collection state/action | May change state |
| `0B 0A` | Collection/history state operation | Semantics incomplete |
| `0B 0B` | Read live terminal voltage and state flags | Verified read |
| `0C 02` | Bluetooth/device-name change path | State-changing |

The app also parses response command families including `42xx`, `43xx`,
`46xx`, `48xx`, `49xx`, `4Bxx`, `4Cxx`, and `59xx`. In general, `0x40` is
added to the first request-command byte for the corresponding response.

## Capabilities visible in the app

The Android application exposes these user-facing functions:

- live terminal voltage;
- standard/quick battery test;
- CCA, internal resistance, state of charge (SOC), and state of health (SOH)
  estimates;
- cranking test with minimum/maximum voltage and cranking time;
- charging-system test at idle and elevated RPM;
- loaded/unloaded and ripple-voltage results;
- live voltage waveform;
- immediate and scheduled voltage collection;
- stored voltage/test history;
- device identifier, Bluetooth-name setting, and firmware update.

These values should not all be assumed to be directly measured telemetry.
Several are test results or estimates derived from terminal-voltage behaviour
and user-supplied battery type/rating.

## CoreBluetooth client sequence

1. Scan for service `FFF0` or the expected local name.
2. Connect without pairing.
3. Discover service `FFF0`.
4. Enable notifications on `FFF1`.
5. Wait until notification subscription succeeds.
6. Write a complete request frame to `FFF2` using Write Without Response.
7. Accumulate notifications until the declared packet length is available.
8. Validate magic, total length, terminator, and CRC before decoding.

Do not assume one BLE notification equals one application packet. The app
accumulates notification fragments before parsing, and larger history or
firmware messages can span multiple notifications.

## Security and safety notes

- The tested device accepted connections and status queries without pairing or
  authentication.
- Nearby clients may therefore be able to read its voltage and identifier.
- The same unauthenticated channel appears to expose configuration, deletion,
  test-control, Bluetooth-name, and firmware-update commands.
- Prefer the verified `02 01` and `0B 0B` reads until the remaining payload
  formats are understood.
- Never experiment with update or destructive commands on hardware you cannot
  recover.

## Provenance and limits

This is an independent interoperability analysis, not vendor documentation.
Names and semantics were recovered from decompiled app call sites, UI strings,
and live BLE captures. No firmware was modified.

Tested:

```text
Retail brand:    KONNWEI
Product family:  BK300
Android package: com.jiawei.batteryonline
App version:     BKmonitor 1.1.5
BLE local name:  bikebattery
Service:         FFF0
Notify:          FFF1
Write:           FFF2
Observed volts:  40.87 V
```

An example BK300 listing (AliExpress item `1005007797314538`) was approximately
£6.49 in July 2026. Marketplace pricing and branding are volatile; this detail
is included only to help identify the inexpensive hardware family, not as a
current-price claim or compatibility guarantee.

Different firmware versions may reuse the transport while changing command
payloads. Please contribute captures with secrets and unique identifiers
redacted.
