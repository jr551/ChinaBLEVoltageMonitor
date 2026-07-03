export const UUIDS = Object.freeze({
  service: 0xfff0,
  notify: 0xfff1,
  write: 0xfff2,
});

export function crc16X25(bytes) {
  let crc = 0xffff;
  for (const byte of bytes) {
    crc ^= byte;
    for (let bit = 0; bit < 8; bit += 1) {
      crc = (crc & 1) ? (crc >>> 1) ^ 0x8408 : crc >>> 1;
    }
  }
  return (crc ^ 0xffff) & 0xffff;
}

export function buildRequest(command, payload = new Uint8Array()) {
  if (!(command instanceof Uint8Array) || command.length !== 2) {
    throw new TypeError("Command must contain exactly two bytes");
  }
  const body = new Uint8Array(6 + payload.length);
  body.set([0x40, 0x40, body.length + 4, 0x00], 0);
  body.set(command, 4);
  body.set(payload, 6);
  const crc = crc16X25(body);
  const packet = new Uint8Array(body.length + 4);
  packet.set(body);
  packet.set([crc & 0xff, crc >>> 8, 0x0d, 0x0a], body.length);
  return packet;
}

export function parsePacket(packet) {
  if (!(packet instanceof Uint8Array) || packet.length < 10) throw new Error("Packet is too short");
  if (packet[0] !== 0x24 || packet[1] !== 0x24) throw new Error("Invalid response magic");
  const declaredLength = packet[2] | (packet[3] << 8);
  if (declaredLength !== packet.length) throw new Error(`Length mismatch: expected ${declaredLength}`);
  if (packet.at(-2) !== 0x0d || packet.at(-1) !== 0x0a) throw new Error("Invalid terminator");
  const content = packet.subarray(0, -4);
  const expectedCrc = packet.at(-4) | (packet.at(-3) << 8);
  if (crc16X25(content) !== expectedCrc) throw new Error("CRC check failed");
  return {
    command: packet.subarray(4, 6),
    payload: packet.subarray(6, -4),
  };
}

export class PacketBuffer {
  #bytes = new Uint8Array();

  push(fragment) {
    const combined = new Uint8Array(this.#bytes.length + fragment.length);
    combined.set(this.#bytes);
    combined.set(fragment, this.#bytes.length);
    this.#bytes = combined;
    const packets = [];

    while (this.#bytes.length >= 4) {
      const magicAt = this.#bytes.findIndex((byte, index, all) =>
        byte === 0x24 && all[index + 1] === 0x24);
      if (magicAt < 0) {
        this.#bytes = new Uint8Array();
        break;
      }
      if (magicAt > 0) this.#bytes = this.#bytes.subarray(magicAt);
      if (this.#bytes.length < 4) break;
      const length = this.#bytes[2] | (this.#bytes[3] << 8);
      if (length < 10 || length > 4096) {
        this.#bytes = this.#bytes.subarray(2);
        continue;
      }
      if (this.#bytes.length < length) break;
      packets.push(this.#bytes.slice(0, length));
      this.#bytes = this.#bytes.slice(length);
    }
    return packets;
  }
}

export function parseHex(value) {
  const compact = value.replaceAll(/\s+/g, "");
  if (!compact) return new Uint8Array();
  if (!/^[0-9a-f]+$/i.test(compact) || compact.length % 2) {
    throw new Error("Hex must contain complete byte pairs");
  }
  return Uint8Array.from(compact.match(/.{2}/g), (byte) => Number.parseInt(byte, 16));
}

export function toHex(bytes) {
  return [...bytes].map((byte) => byte.toString(16).padStart(2, "0")).join(" ").toUpperCase();
}
