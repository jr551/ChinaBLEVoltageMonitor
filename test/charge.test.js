import test from "node:test";
import assert from "node:assert/strict";
import { ChargeDetector } from "../src/charge.js";

test("detects a sustained rising voltage trend", () => {
  const detector = new ChargeDetector();
  let charging = false;
  for (let index = 0; index < 8; index += 1) {
    charging = detector.add(40 + index * 0.012, index * 5000);
  }
  assert.equal(charging, true);
});

test("does not call a stable noisy voltage charging", () => {
  const detector = new ChargeDetector();
  const values = [40, 40.01, 39.99, 40.01, 40, 40.01, 40, 40.01];
  let charging = false;
  values.forEach((volts, index) => {
    charging = detector.add(volts, index * 5000);
  });
  assert.equal(charging, false);
});

test("ignores sag and recovery periods", () => {
  const detector = new ChargeDetector();
  [40.8, 40.81, 40.79, 39.8].forEach((volts, index) =>
    detector.add(volts, index * 5000, index === 3));
  assert.equal(detector.add(40.8, 20000, true), false);
  assert.equal(detector.charging, false);
});
