const LI_ION_CURVE = [
  [3.0, 0],
  [3.3, 5],
  [3.5, 10],
  [3.6, 20],
  [3.7, 40],
  [3.8, 60],
  [3.9, 75],
  [4.0, 85],
  [4.1, 95],
  [4.2, 100],
];

function interpolateCurve(voltsPerCell) {
  if (voltsPerCell <= LI_ION_CURVE[0][0]) return 0;
  if (voltsPerCell >= LI_ION_CURVE.at(-1)[0]) return 100;

  for (let index = 1; index < LI_ION_CURVE.length; index += 1) {
    const [upperVoltage, upperPercent] = LI_ION_CURVE[index];
    if (voltsPerCell <= upperVoltage) {
      const [lowerVoltage, lowerPercent] = LI_ION_CURVE[index - 1];
      const position = (voltsPerCell - lowerVoltage) / (upperVoltage - lowerVoltage);
      return lowerPercent + position * (upperPercent - lowerPercent);
    }
  }
  return 0;
}

export function estimateLithiumPack(volts) {
  if (!Number.isFinite(volts) || volts < 30 || volts > 42.5) return null;

  const seriesCells = 10;
  const fullVoltage = seriesCells * 4.2;
  const voltsPerCell = volts / seriesCells;

  return {
    seriesCells,
    fullVoltage,
    voltsPerCell,
    percent: Math.round(interpolateCurve(voltsPerCell)),
    label: "Likely 10S Li-ion",
  };
}
