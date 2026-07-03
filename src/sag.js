const MIN_SAG_VOLTS = 0.4;
const RECOVERY_MARGIN_VOLTS = 0.15;
const MAX_SAMPLE_GAP_MS = 5000;
const MAX_EVENT_MS = 30000;

function median(values) {
  if (!values.length) return 0;
  const sorted = [...values].sort((left, right) => left - right);
  const middle = Math.floor(sorted.length / 2);
  return sorted.length % 2
    ? sorted[middle]
    : (sorted[middle - 1] + sorted[middle]) / 2;
}

export class SagTracker {
  #idle = [];
  #active = null;
  #lastTime = 0;
  #onEvent;

  constructor(onEvent = () => {}) {
    this.#onEvent = onEvent;
  }

  add(volts, time = Date.now()) {
    if (!Number.isFinite(volts)) return { inSag: false, referenceVoltage: volts };
    if (this.#lastTime && time - this.#lastTime > MAX_SAMPLE_GAP_MS) {
      this.#idle = [];
      this.#active = null;
    }
    this.#lastTime = time;

    if (this.#active) {
      this.#active.minimum = Math.min(this.#active.minimum, volts);
      const recovered = volts >= this.#active.baseline - RECOVERY_MARGIN_VOLTS;
      const timedOut = time - this.#active.startedAt >= MAX_EVENT_MS;
      const referenceVoltage = this.#active.baseline;

      if (recovered || timedOut) {
        const sagVolts = this.#active.baseline - this.#active.minimum;
        const event = {
          timestamp: time,
          baselineVoltage: this.#active.baseline,
          minimumVoltage: this.#active.minimum,
          sagVolts,
          sagPercent: (sagVolts / this.#active.baseline) * 100,
          durationMs: time - this.#active.startedAt,
          recovered,
        };
        this.#active = null;
        this.#idle = [volts];
        if (sagVolts >= MIN_SAG_VOLTS) this.#onEvent(event);
        return { inSag: false, referenceVoltage: volts, event };
      }
      return { inSag: true, referenceVoltage };
    }

    const baseline = median(this.#idle);
    if (this.#idle.length >= 3 && baseline - volts >= MIN_SAG_VOLTS) {
      this.#active = {
        baseline,
        minimum: volts,
        startedAt: time,
      };
      return { inSag: true, referenceVoltage: baseline };
    }

    this.#idle.push(volts);
    if (this.#idle.length > 6) this.#idle.shift();
    return { inSag: false, referenceVoltage: volts };
  }
}

export function relativeSagHealth(history) {
  if (!Array.isArray(history) || history.length < 3) return null;
  const valid = history.filter(({ sagPercent }) =>
    Number.isFinite(sagPercent) && sagPercent > 0);
  if (valid.length < 3) return null;

  const all = valid.map(({ sagPercent }) => sagPercent);
  const personalBest = median([...all].sort((a, b) => a - b).slice(0, Math.min(5, all.length)));
  const recent = median(all.slice(-5));
  return Math.round(Math.max(0, Math.min(100, (personalBest / recent) * 100)));
}

export function loadSagHistory(storage, key) {
  try {
    const value = JSON.parse(storage.getItem(key) || "[]");
    return Array.isArray(value) ? value.slice(-50) : [];
  } catch {
    return [];
  }
}

export function storeSagHistory(storage, key, history) {
  try {
    storage.setItem(key, JSON.stringify(history.slice(-50)));
  } catch {
    // Private browsing or storage policy may disable persistence.
  }
}
