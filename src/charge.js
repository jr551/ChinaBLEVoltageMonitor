export class ChargeDetector {
  #samples = [];
  #charging = false;
  #cooldownUntil = 0;
  #lastTime = 0;

  add(volts, time = Date.now(), ignore = false) {
    if (!Number.isFinite(volts)) return this.#charging;
    if (ignore) {
      this.#samples = [];
      this.#charging = false;
      this.#cooldownUntil = time + 30000;
      this.#lastTime = time;
      return false;
    }
    if (this.#lastTime && time - this.#lastTime > 10000) {
      this.#samples = [];
      this.#charging = false;
    }
    this.#lastTime = time;
    if (time < this.#cooldownUntil) return false;

    this.#samples.push({ volts, time });
    this.#samples = this.#samples.filter((sample) => time - sample.time <= 90000);
    if (this.#samples.length < 6) return this.#charging;

    const durationMs = time - this.#samples[0].time;
    if (durationMs < 20000) return this.#charging;
    const edgeCount = Math.min(3, Math.floor(this.#samples.length / 2));
    const average = (samples) =>
      samples.reduce((sum, sample) => sum + sample.volts, 0) / samples.length;
    const start = average(this.#samples.slice(0, edgeCount));
    const end = average(this.#samples.slice(-edgeCount));
    const rise = end - start;
    const slopePerMinute = rise / (durationMs / 60000);

    if (!this.#charging && rise >= 0.05 && slopePerMinute >= 0.03) {
      this.#charging = true;
    } else if (this.#charging && durationMs >= 30000 &&
               (slopePerMinute < 0.005 || end < start - 0.05)) {
      this.#charging = false;
    }
    return this.#charging;
  }

  get charging() {
    return this.#charging;
  }
}
