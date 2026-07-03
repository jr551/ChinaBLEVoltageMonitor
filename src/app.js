import { UUIDS, PacketBuffer, buildRequest, parseHex, parsePacket, toHex } from "./protocol.js";
import { estimateLithiumPack } from "./battery.js";

const $ = (selector) => document.querySelector(selector);
const elements = {
  connect: $("#connect-button"),
  statusDot: $("#status-dot"),
  status: $("#status-label"),
  livePill: $("#live-pill"),
  voltage: $("#voltage-value"),
  batteryEstimate: $("#battery-estimate"),
  packLabel: $("#pack-label"),
  chargePercent: $("#charge-percent"),
  chargeFill: $("#charge-fill"),
  chargeTrack: $(".charge-track"),
  packVoltage: $("#pack-voltage"),
  flagA: $("#flag-a"),
  flagB: $("#flag-b"),
  updated: $("#updated-at"),
  readVoltage: $("#read-voltage"),
  toggleMonitor: $("#toggle-monitor"),
  clearChart: $("#clear-chart"),
  chart: $("#voltage-chart"),
  chartEmpty: $("#chart-empty"),
  deviceName: $("#device-name"),
  deviceId: $("#device-id"),
  readId: $("#read-id"),
  commandList: $("#command-list"),
  rawCommand: $("#raw-command"),
  rawPayload: $("#raw-payload"),
  sendRaw: $("#send-raw"),
  log: $("#packet-log"),
  copyLog: $("#copy-log"),
  toast: $("#toast"),
};

const commands = [
  { hex: "02 01", label: "Device identifier", safe: true },
  { hex: "0B 0B", label: "Live voltage & state", safe: true },
  { hex: "01 00", label: "Initialisation / handshake", safe: false },
  { hex: "06 01", label: "Quick battery test", safe: false },
  { hex: "09 01", label: "Waveform / test", safe: false },
  { hex: "0A 01", label: "Historical records", safe: false },
  { hex: "0B 01", label: "Latest collected data", safe: false },
  { hex: "0B 03", label: "Scheduled entries", safe: false },
  { hex: "0B 08", label: "Collection state", safe: false },
  { hex: "0C 02", label: "Change Bluetooth name", danger: true },
  { hex: "0A 03", label: "Delete records", danger: true },
  { hex: "03 01", label: "Firmware update mode", danger: true },
];

let device;
let server;
let writer;
let monitorTimer;
let isConnecting = false;
const packetBuffer = new PacketBuffer();
const readings = [];

function renderCommands() {
  elements.commandList.innerHTML = commands.map((command) => `
    <button class="command ${command.safe ? "is-safe" : ""}" data-command="${command.hex}"
      ${command.safe ? "" : "disabled"} title="${command.safe ? "Verified read command" : "Payload or safety semantics are not yet verified"}">
      <code>${command.hex}</code>
      <span>${command.label}</span>
      <small>${command.safe ? "READ" : command.danger ? "LOCKED · RISK" : "LOCKED"}</small>
    </button>
  `).join("");
}

function setConnected(connected) {
  elements.statusDot.classList.toggle("online", connected);
  elements.status.textContent = connected ? device?.name || "Connected" : "Not connected";
  elements.livePill.textContent = connected ? "CONNECTED" : "OFFLINE";
  elements.livePill.classList.toggle("online", connected);
  elements.connect.textContent = connected ? "Disconnect" : "Connect device";
  [elements.readVoltage, elements.toggleMonitor, elements.readId, elements.sendRaw]
    .forEach((button) => { button.disabled = !connected; });
  document.querySelectorAll(".command.is-safe").forEach((button) => { button.disabled = !connected; });
}

function addLog(direction, bytes, detail = "") {
  elements.log.querySelector(".log-placeholder")?.remove();
  const row = document.createElement("div");
  row.className = `log-row ${direction === "TX" ? "tx" : "rx"}`;
  row.innerHTML = `<time>${new Date().toLocaleTimeString()}</time><b>${direction}</b><code>${toHex(bytes)}</code><span>${detail}</span>`;
  elements.log.prepend(row);
}

function showToast(message) {
  elements.toast.textContent = message;
  elements.toast.classList.add("visible");
  setTimeout(() => elements.toast.classList.remove("visible"), 2400);
}

async function send(command, payload = new Uint8Array()) {
  if (!writer) throw new Error("Device is not connected");
  const packet = buildRequest(command, payload);
  addLog("TX", packet, `Command ${toHex(command)}`);
  await writer.writeValueWithoutResponse(packet);
}

async function connect() {
  if (device?.gatt?.connected) {
    device.gatt.disconnect();
    return;
  }
  if (!navigator.bluetooth) {
    throw new Error("Web Bluetooth is unavailable. Use Chrome or Edge on desktop or Android.");
  }
  isConnecting = true;
  elements.connect.disabled = true;
  elements.status.textContent = "Choose your monitor…";
  try {
    device = await navigator.bluetooth.requestDevice({
      filters: [{ services: [UUIDS.service] }],
      optionalServices: [UUIDS.service],
    });
    device.addEventListener("gattserverdisconnected", onDisconnected);
    elements.status.textContent = "Connecting…";
    server = await device.gatt.connect();
    const service = await server.getPrimaryService(UUIDS.service);
    const notifier = await service.getCharacteristic(UUIDS.notify);
    writer = await service.getCharacteristic(UUIDS.write);
    await notifier.startNotifications();
    notifier.addEventListener("characteristicvaluechanged", onNotification);
    elements.deviceName.textContent = device.name || "Unnamed BLE device";
    setConnected(true);
    showToast("Monitor connected");
    await send(parseHex("0B 0B"));
  } finally {
    isConnecting = false;
    elements.connect.disabled = false;
  }
}

function onDisconnected() {
  clearInterval(monitorTimer);
  monitorTimer = undefined;
  writer = undefined;
  server = undefined;
  elements.toggleMonitor.textContent = "Start monitor";
  setConnected(false);
  addLog("RX", new Uint8Array(), "Device disconnected");
}

function onNotification(event) {
  const fragment = new Uint8Array(
    event.target.value.buffer,
    event.target.value.byteOffset,
    event.target.value.byteLength,
  );
  for (const bytes of packetBuffer.push(fragment)) {
    try {
      const packet = parsePacket(bytes);
      handlePacket(packet);
      addLog("RX", bytes, describePacket(packet));
    } catch (error) {
      addLog("RX", bytes, error.message);
    }
  }
}

function describePacket({ command, payload }) {
  const code = toHex(command);
  if (code === "4B 0B" && payload.length >= 4) {
    return `${(new DataView(payload.buffer, payload.byteOffset).getUint16(0, true) / 100).toFixed(2)} V · flags ${payload[2]}/${payload[3]}`;
  }
  if (code === "42 01") return `Identifier: ${new TextDecoder().decode(payload).replaceAll("\0", "")}`;
  return `Response ${code} · ${payload.length} payload bytes`;
}

function handlePacket({ command, payload }) {
  const code = toHex(command);
  if (code === "4B 0B" && payload.length >= 4) {
    const volts = new DataView(payload.buffer, payload.byteOffset).getUint16(0, true) / 100;
    elements.voltage.textContent = volts.toFixed(2);
    const estimate = estimateLithiumPack(volts);
    elements.batteryEstimate.hidden = !estimate;
    if (estimate) {
      elements.packLabel.textContent = estimate.label;
      elements.chargePercent.textContent = estimate.percent;
      elements.chargeFill.style.width = `${estimate.percent}%`;
      elements.chargeTrack.setAttribute("aria-valuenow", estimate.percent);
      elements.packVoltage.textContent = `${volts.toFixed(2)} / ${estimate.fullVoltage.toFixed(1)} V`;
    }
    elements.flagA.textContent = payload[2];
    elements.flagB.textContent = payload[3];
    elements.updated.textContent = new Date().toLocaleTimeString([], { hour: "2-digit", minute: "2-digit", second: "2-digit" });
    readings.push({ time: Date.now(), volts });
    if (readings.length > 120) readings.shift();
    drawChart();
  } else if (code === "42 01") {
    elements.deviceId.textContent = new TextDecoder().decode(payload).replaceAll("\0", "").trim() || toHex(payload);
  }
}

function drawChart() {
  const canvas = elements.chart;
  const box = canvas.getBoundingClientRect();
  const ratio = window.devicePixelRatio || 1;
  canvas.width = box.width * ratio;
  canvas.height = box.height * ratio;
  const context = canvas.getContext("2d");
  context.scale(ratio, ratio);
  elements.chartEmpty.hidden = readings.length > 0;
  if (!readings.length) return;

  const width = box.width;
  const height = box.height;
  const pad = 30;
  const values = readings.map(({ volts }) => volts);
  const low = Math.floor((Math.min(...values) - 0.25) * 2) / 2;
  const high = Math.ceil((Math.max(...values) + 0.25) * 2) / 2 || low + 1;
  context.strokeStyle = "rgba(220, 231, 213, .12)";
  context.fillStyle = "#798476";
  context.font = "11px DM Mono";
  for (let line = 0; line < 4; line += 1) {
    const y = pad + ((height - pad * 2) * line) / 3;
    context.beginPath();
    context.moveTo(pad, y);
    context.lineTo(width - pad, y);
    context.stroke();
    context.fillText((high - ((high - low) * line) / 3).toFixed(1), 0, y + 4);
  }
  const x = (index) => pad + (index / Math.max(readings.length - 1, 1)) * (width - pad * 2);
  const y = (volts) => pad + ((high - volts) / Math.max(high - low, 0.1)) * (height - pad * 2);
  const gradient = context.createLinearGradient(0, pad, 0, height - pad);
  gradient.addColorStop(0, "rgba(192, 242, 88, .28)");
  gradient.addColorStop(1, "rgba(192, 242, 88, 0)");
  context.beginPath();
  readings.forEach((reading, index) => index ? context.lineTo(x(index), y(reading.volts)) : context.moveTo(x(index), y(reading.volts)));
  context.lineTo(x(readings.length - 1), height - pad);
  context.lineTo(x(0), height - pad);
  context.closePath();
  context.fillStyle = gradient;
  context.fill();
  context.beginPath();
  readings.forEach((reading, index) => index ? context.lineTo(x(index), y(reading.volts)) : context.moveTo(x(index), y(reading.volts)));
  context.strokeStyle = "#c0f258";
  context.lineWidth = 2;
  context.stroke();
}

elements.connect.addEventListener("click", () => connect().catch((error) => {
  if (error.name !== "NotFoundError") showToast(error.message);
  setConnected(false);
}));
elements.readVoltage.addEventListener("click", () => send(parseHex("0B 0B")).catch((error) => showToast(error.message)));
elements.readId.addEventListener("click", () => send(parseHex("02 01")).catch((error) => showToast(error.message)));
elements.toggleMonitor.addEventListener("click", () => {
  if (monitorTimer) {
    clearInterval(monitorTimer);
    monitorTimer = undefined;
    elements.toggleMonitor.textContent = "Start monitor";
  } else {
    send(parseHex("0B 0B"));
    monitorTimer = setInterval(() => send(parseHex("0B 0B")).catch(() => {}), 2000);
    elements.toggleMonitor.textContent = "Stop monitor";
  }
});
elements.clearChart.addEventListener("click", () => { readings.length = 0; drawChart(); });
elements.commandList.addEventListener("click", (event) => {
  const button = event.target.closest("[data-command]");
  if (button) send(parseHex(button.dataset.command)).catch((error) => showToast(error.message));
});
elements.sendRaw.addEventListener("click", () => {
  try {
    const command = parseHex(elements.rawCommand.value);
    if (command.length !== 2) throw new Error("Enter a two-byte command");
    send(command, parseHex(elements.rawPayload.value)).catch((error) => showToast(error.message));
  } catch (error) {
    showToast(error.message);
  }
});
elements.copyLog.addEventListener("click", async () => {
  await navigator.clipboard.writeText(elements.log.innerText);
  showToast("Log copied");
});
window.addEventListener("resize", drawChart);
window.addEventListener("beforeunload", () => device?.gatt?.disconnect());

renderCommands();
setConnected(false);
drawChart();

if ("serviceWorker" in navigator) {
  window.addEventListener("load", () => {
    navigator.serviceWorker.register("./service-worker.js", { scope: "./" })
      .catch((error) => console.warn("Offline support could not start:", error));
  });
}
