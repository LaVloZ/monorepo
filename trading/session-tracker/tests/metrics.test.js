import { describe, it, expect } from "vitest";
import {
  sumPnl, losingCount, winRate, bestTrade, worstTrade,
  lossLimitPct, gainTargetPct, stopState, formatDuration, computePnl,
} from "../src/lib/metrics.js";

const T = (pnl) => ({ pnl });

describe("sumPnl", () => {
  it("somme les pnl", () => expect(sumPnl([T(10), T(-4), T(2)])).toBe(8));
  it("0 si vide", () => expect(sumPnl([])).toBe(0));
});

describe("losingCount", () => {
  it("compte les pnl < 0", () => expect(losingCount([T(10), T(-4), T(-1), T(0)])).toBe(2));
});

describe("winRate", () => {
  it("null si aucun trade", () => expect(winRate([])).toBe(null));
  it("proportion de gagnants", () => expect(winRate([T(10), T(-4), T(2), T(-1)])).toBe(0.5));
  it("le breakeven (0) n'est pas gagnant", () => expect(winRate([T(0), T(10)])).toBe(0.5));
});

describe("best/worst", () => {
  it("meilleur trade", () => expect(bestTrade([T(3), T(9), T(-2)]).pnl).toBe(9));
  it("pire trade", () => expect(worstTrade([T(3), T(9), T(-2)]).pnl).toBe(-2));
  it("null si vide", () => {
    expect(bestTrade([])).toBe(null);
    expect(worstTrade([])).toBe(null);
  });
});

describe("lossLimitPct", () => {
  it("fraction consommée par la perte nette", () =>
    expect(lossLimitPct([T(-40)], 100)).toBeCloseTo(0.4));
  it("0 si net positif", () =>
    expect(lossLimitPct([T(50), T(-10)], 100)).toBe(0));
  it("0 si lossLimit <= 0", () =>
    expect(lossLimitPct([T(-40)], 0)).toBe(0));
});

describe("gainTargetPct", () => {
  it("fraction de l'objectif atteinte", () =>
    expect(gainTargetPct([T(30)], 100)).toBeCloseTo(0.3));
  it("0 si net négatif", () =>
    expect(gainTargetPct([T(-30)], 100)).toBe(0));
});

describe("stopState", () => {
  const params = { lossLimit: 100, gainTarget: 200, maxTrades: 5, maxLossTrades: 3 };
  it("pas de stop au repos", () => {
    const s = stopState([], params);
    expect(s.stopped).toBe(false);
    expect(s.reasons).toEqual([]);
    expect(s.lossWarning).toBe(false);
  });
  it("alerte à 80% de la loss limit", () => {
    const s = stopState([T(-80)], params);
    expect(s.lossWarning).toBe(true);
    expect(s.stopped).toBe(false);
  });
  it("stop quand loss limit atteinte", () => {
    const s = stopState([T(-100)], params);
    expect(s.stopped).toBe(true);
    expect(s.reasons).toContain("lossLimit");
  });
  it("stop au max de trades", () => {
    const s = stopState([T(1), T(1), T(1), T(1), T(1)], params);
    expect(s.stopped).toBe(true);
    expect(s.reasons).toContain("maxTrades");
  });
  it("stop au max de trades perdants", () => {
    const s = stopState([T(-1), T(-1), T(-1)], params);
    expect(s.stopped).toBe(true);
    expect(s.reasons).toContain("maxLossTrades");
  });
  it("lossWarning est false quand déjà stoppé pour la loss limit", () => {
    const s = stopState([T(-100)], params);
    expect(s.stopped).toBe(true);
    expect(s.reasons).toContain("lossLimit");
    expect(s.lossWarning).toBe(false);
  });
});

describe("formatDuration", () => {
  it("formate en HH:MM:SS", () => expect(formatDuration(3661000)).toBe("01:01:01"));
  it("gère 0", () => expect(formatDuration(0)).toBe("00:00:00"));
  it("clampe le négatif à 0", () => expect(formatDuration(-5000)).toBe("00:00:00"));
});

describe("computePnl", () => {
  it("long : size×(exit−entry)", () =>
    expect(computePnl({ size: 2, entry: 100, exit: 110, side: "long" })).toBe(20));
  it("short : inversé", () =>
    expect(computePnl({ size: 2, entry: 100, exit: 110, side: "short" })).toBe(-20));
  it("null si un champ manque", () => {
    expect(computePnl({ size: 2, entry: 100, exit: null, side: "long" })).toBe(null);
    expect(computePnl({ size: null, entry: 100, exit: 110, side: "long" })).toBe(null);
  });
  it("null si non numérique (chaîne vide)", () =>
    expect(computePnl({ size: "", entry: 100, exit: 110, side: "long" })).toBe(null));
  it("gère les décimales", () =>
    expect(computePnl({ size: 1.5, entry: 10, exit: 12, side: "long" })).toBeCloseTo(3));
});
