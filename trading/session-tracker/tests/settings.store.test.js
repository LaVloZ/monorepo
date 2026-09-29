import { describe, it, expect, vi, beforeEach } from "vitest";
import { setActivePinia, createPinia } from "pinia";

vi.mock("../src/db.js", () => ({
  getDoc: vi.fn(),
  createDoc: vi.fn(async (d) => ({ id: d._id || "gen", rev: "1-x" })),
  updateDoc: vi.fn(async (d) => ({ id: d._id, rev: "2-y" })),
  find: vi.fn(),
}));
import * as db from "../src/db.js";
import { useSettingsStore } from "../src/stores/settings.js";

beforeEach(() => {
  setActivePinia(createPinia());
  vi.clearAllMocks();
});

describe("settings store", () => {
  it("utilise les défauts si aucun doc settings", async () => {
    db.getDoc.mockResolvedValue(null);
    const s = useSettingsStore();
    await s.load();
    expect(s.settings.maxTrades).toBe(5);
    expect(s.settings.currency).toBe("EUR");
  });

  it("charge le doc settings existant", async () => {
    db.getDoc.mockResolvedValue({
      _id: "settings", type: "settings",
      lossLimit: 300, gainTarget: 600, maxTrades: 4, maxLossTrades: 2, currency: "USD",
    });
    const s = useSettingsStore();
    await s.load();
    expect(s.settings.lossLimit).toBe(300);
    expect(s.settings.currency).toBe("USD");
  });

  it("save utilise createDoc la 1re fois puis updateDoc en suivant le _rev", async () => {
    db.getDoc.mockResolvedValue(null);
    const s = useSettingsStore();
    await s.load();
    await s.save({ lossLimit: 200, gainTarget: 400, maxTrades: 3, maxLossTrades: 2, currency: "USD" });
    expect(db.createDoc).toHaveBeenCalledTimes(1);
    expect(db.updateDoc).not.toHaveBeenCalled();
    await s.save({ lossLimit: 250, gainTarget: 400, maxTrades: 3, maxLossTrades: 2, currency: "USD" });
    expect(db.updateDoc).toHaveBeenCalledTimes(1);
    expect(db.updateDoc.mock.calls[0][0]._rev).toBe("1-x");
  });
});
