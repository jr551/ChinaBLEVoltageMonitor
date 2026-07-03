# BK Voltage Lab

A local-first Web Bluetooth dashboard for BKmonitor-compatible battery voltage
monitors. It connects directly from a supported browser, displays live terminal
voltage, graphs readings, reads the device identifier, and exposes a decoded
packet log.

This project turns the original
[BKmonitor protocol reverse-engineering gist](https://gist.github.com/jr551/21f1e88d8efa7113deb9c139310b23b9)
into a tested, maintainable repository.

## Try it

Web Bluetooth requires a secure context. Use the published GitHub Pages site,
or run the local development server:

```bash
npm install
npm run dev
```

Open the displayed `localhost` URL in Chrome or Edge on desktop or Android,
select the monitor (tested name: `bikebattery`), then use **Read now** or
**Start monitor**.

Safari and Firefox do not currently expose Web Bluetooth. iOS browsers cannot
use this app because they all use WebKit.

## Safety

Only the verified, read-only commands `02 01` and `0B 0B` have one-click
controls. Commands with incomplete payload semantics and state-changing
commands are shown as locked documentation. The raw console is intended for
protocol research on hardware you own and can recover.

## Development

```bash
npm test
npm run build
```

The protocol implementation is dependency-free and lives in
[`src/protocol.js`](src/protocol.js). It includes CRC-16/X-25 framing, response
validation, and notification-fragment reassembly.

## Protocol

The tested device exposes service `FFF0`, notification characteristic `FFF1`,
and write-without-response characteristic `FFF2`. See
[`protocol/BKMONITOR_BLE_PROTOCOL.md`](protocol/BKMONITOR_BLE_PROTOCOL.md).

## Contributing

Captures and decoded payloads are welcome. Redact device identifiers,
manufacturer data, and other unique values before opening an issue.

This is independent interoperability research, not vendor documentation.
