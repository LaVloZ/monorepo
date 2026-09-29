import { describe, it, expect, vi, beforeEach } from "vitest";
import { setActivePinia, createPinia } from "pinia";

let store = {};
vi.mock("../src/db.js", () => ({
  getDoc: vi.fn(),
  createDoc: vi.fn(async (d) => {
    const id = "id" + Math.random().toString(36).slice(2);
    store[id] = { ...d, _id: id, _rev: "1-x" };
    return { id, rev: "1-x" };
  }),
  updateDoc: vi.fn(async (d) => {
    store[d._id] = { ...d, _rev: "2-y" };
    return { id: d._id, rev: "2-y" };
  }),
  find: vi.fn(async (selector) =>
    Object.values(store).filter((d) =>
      Object.entries(selector).every(([k, v]) => d[k] === v)
    )
  ),
}));
import { useSessionStore } from "../src/stores/session.js";

const PARAMS = { lossLimit: 100, gainTarget: 200, maxTrades: 5, maxLossTrades: 3, currency: "EUR" };

beforeEach(() => {
  setActivePinia(createPinia());
  store = {};
  vi.clearAllMocks();
});

describe("session store", () => {
  it("démarre une session ouverte", async () => {
    const s = useSessionStore();
    await s.start(PARAMS);
    expect(s.current).toBeTruthy();
    expect(s.current.status).toBe("open");
    expect(s.current.params.lossLimit).toBe(100);
    expect(s.trades).toEqual([]);
  });

  it("ajoute un trade rattaché à la session et met à jour le P&L", async () => {
    const s = useSessionStore();
    await s.start(PARAMS);
    await s.addTrade({ ticker: "NQ", side: "long", pnl: 25, note: "", tags: [] });
    expect(s.trades).toHaveLength(1);
    expect(s.trades[0].sessionId).toBe(s.current._id);
    expect(s.pnl).toBe(25);
  });

  it("calcule l'état STOP via les métriques", async () => {
    const s = useSessionStore();
    await s.start(PARAMS);
    await s.addTrade({ ticker: "NQ", side: "short", pnl: -100, note: "", tags: [] });
    expect(s.stop.stopped).toBe(true);
    expect(s.stop.reasons).toContain("lossLimit");
  });

  it("clôture la session", async () => {
    const s = useSessionStore();
    await s.start(PARAMS);
    await s.close();
    expect(s.current).toBe(null);
  });

  it("recharge la session ouverte et ses trades", async () => {
    const s1 = useSessionStore();
    await s1.start(PARAMS);
    await s1.addTrade({ ticker: "ES", side: "long", pnl: 10, note: "", tags: [] });

    setActivePinia(createPinia());
    const s2 = useSessionStore();
    await s2.loadOpen();
    expect(s2.current).toBeTruthy();
    expect(s2.trades).toHaveLength(1);
    expect(s2.pnl).toBe(10);
  });

  it("addTrade lève une erreur s'il n'y a pas de session ouverte", async () => {
    const s = useSessionStore();
    await expect(
      s.addTrade({ ticker: "NQ", side: "long", pnl: 5, note: "", tags: [] })
    ).rejects.toThrow("Aucune session ouverte");
  });

  it("lastTicker renvoie le ticker du dernier trade (vide si aucun)", async () => {
    const s = useSessionStore();
    expect(s.lastTicker).toBe("");
    await s.start(PARAMS);
    await s.addTrade({ ticker: "ES", side: "long", pnl: 5, note: "", tags: [] });
    await s.addTrade({ ticker: "NQ", side: "short", pnl: -2, note: "", tags: [] });
    expect(s.lastTicker).toBe("NQ");
  });

  it("addTrade enregistre size/entry/exit (null si absents)", async () => {
    const s = useSessionStore();
    await s.start(PARAMS);
    await s.addTrade({ ticker: "NQ", side: "long", pnl: 20, size: 2, entry: 100, exit: 110, note: "", tags: [] });
    expect(s.trades[0].size).toBe(2);
    expect(s.trades[0].entry).toBe(100);
    expect(s.trades[0].exit).toBe(110);
    await s.addTrade({ ticker: "ES", side: "long", pnl: 5, note: "", tags: [] });
    expect(s.trades[1].size).toBe(null);
  });
});
