import { describe, it, expect } from "vitest";
import { sessionSummaries, pnlByDay } from "../src/lib/history.js";

const sessions = [
  { _id: "s1", startedAt: "2026-07-01T08:00:00.000Z" },
  { _id: "s2", startedAt: "2026-07-01T14:00:00.000Z" },
];
const trades = [
  { sessionId: "s1", pnl: 10 },
  { sessionId: "s1", pnl: -3 },
  { sessionId: "s2", pnl: 5 },
];

describe("sessionSummaries", () => {
  it("agrège pnl, count, winRate par session", () => {
    const byS = { s1: trades.filter((t) => t.sessionId === "s1"), s2: trades.filter((t) => t.sessionId === "s2") };
    const res = sessionSummaries(sessions, byS);
    const s1 = res.find((r) => r.id === "s1");
    expect(s1.pnl).toBe(7);
    expect(s1.count).toBe(2);
    expect(s1.winRate).toBe(0.5);
  });
});

describe("pnlByDay", () => {
  it("agrège le pnl par jour", () => {
    const map = pnlByDay(sessions, trades);
    expect(map.get("2026-07-01")).toBe(12);
  });
});
