import test from "node:test";
import assert from "node:assert/strict";
import { PacketBuffer, buildRequest, crc16X25, parseHex, parsePacket, toHex } from "../src/protocol.js";

test("builds the verified live-voltage request", () => {
  assert.equal(toHex(buildRequest(parseHex("0B 0B"))), "40 40 0A 00 0B 0B A9 B2 0D 0A");
});

test("builds the verified device identifier request", () => {
  assert.equal(toHex(buildRequest(parseHex("02 01"))), "40 40 0A 00 02 01 EB CA 0D 0A");
});

test("parses a verified voltage response", () => {
  const result = parsePacket(parseHex("24 24 0E 00 4B 0B F7 0F 02 01 6C A7 0D 0A"));
  assert.equal(toHex(result.command), "4B 0B");
  assert.equal(new DataView(result.payload.buffer, result.payload.byteOffset).getUint16(0, true), 4087);
});

test("assembles fragmented and adjacent packets", () => {
  const packet = parseHex("24 24 0E 00 4B 0B F7 0F 02 01 6C A7 0D 0A");
  const buffer = new PacketBuffer();
  assert.deepEqual(buffer.push(packet.subarray(0, 7)), []);
  assert.equal(buffer.push(packet.subarray(7)).length, 1);
  assert.equal(buffer.push(new Uint8Array([...packet, ...packet])).length, 2);
});

test("rejects a corrupt CRC", () => {
  const packet = parseHex("24 24 0E 00 4B 0B F7 0F 02 01 6C A7 0D 0A");
  packet[6] ^= 1;
  assert.throws(() => parsePacket(packet), /CRC/);
});

test("CRC matches the reference vector", () => {
  assert.equal(crc16X25(new TextEncoder().encode("123456789")), 0x906e);
});
