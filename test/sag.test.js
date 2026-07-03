import test from "node:test";
import assert from "node:assert/strict";
import { SagTracker, relativeSagHealth } from "../src/sag.js";

test("detects a voltage sag and uses the pre-sag voltage as fuel reference", () => {
  const events = [];
  const tracker = new SagTracker((event) => events.push(event));
  [40.8, 40.81, 40.79].forEach((volts, index) => tracker.add(volts, index * 2000));
  const loaded = tracker.add(39.8, 6000);
  assert.equal(loaded.inSag, true);
  assert.ok(Math.abs(loaded.referenceVoltage - 40.8) < 0.01);
  tracker.add(40.7, 8000);
  assert.equal(events.length, 1);
  assert.ok(Math.abs(events[0].sagVolts - 1) < 0.01);
});

test("requires three stored events before showing relative health", () => {
  assert.equal(relativeSagHealth([{ sagPercent: 2 }, { sagPercent: 2.1 }]), null);
  assert.equal(relativeSagHealth([
    { sagPercent: 2 },
    { sagPercent: 2.1 },
    { sagPercent: 2.2 },
  ]), 100);
});

test("relative health falls when recent sag grows against the personal baseline", () => {
  const score = relativeSagHealth([
    { sagPercent: 2 },
    { sagPercent: 2 },
    { sagPercent: 2 },
    { sagPercent: 4 },
    { sagPercent: 4 },
    { sagPercent: 4 },
  ]);
  assert.equal(score, 50);
});
