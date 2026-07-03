import test from "node:test";
import assert from "node:assert/strict";
import { estimateLithiumPack } from "../src/battery.js";

test("recognises the captured voltage as a likely 10S pack", () => {
  const estimate = estimateLithiumPack(40.87);
  assert.equal(estimate.seriesCells, 10);
  assert.equal(estimate.fullVoltage, 42);
  assert.equal(estimate.percent, 94);
});

test("caps a full 10S pack at 100 percent", () => {
  assert.equal(estimateLithiumPack(42).percent, 100);
});

test("does not guess outside the expected 10S voltage window", () => {
  assert.equal(estimateLithiumPack(12.8), null);
  assert.equal(estimateLithiumPack(48), null);
});
