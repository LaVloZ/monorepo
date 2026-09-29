import { describe, it, expect, beforeAll, afterAll } from "vitest";
import { setActivePinia, createPinia } from "pinia";
import { createDoc, getDoc, updateDoc, deleteDoc } from "../../src/db.js";
import { useSessionStore } from "../../src/stores/session.js";

const COUCH = "http://localhost:5984";
const AUTH = "Basic " + btoa("admin:admin");
const DB = "st_int_" + Date.now();

async function couchUp() {
  try {
    return (await fetch(`${COUCH}/_up`)).ok;
  } catch {
    return false;
  }
}
const UP = await couchUp();

async function admin(path, options = {}) {
  return fetch(`${COUCH}${path}`, {
    ...options,
    headers: { "Content-Type": "application/json", Authorization: AUTH, ...(options.headers || {}) },
  });
}

describe.skipIf(!UP)("intégration CouchDB", () => {
  beforeAll(async () => {
    await admin(`/${DB}`, { method: "PUT" });
    await admin(`/${DB}/_index`, { method: "POST", body: JSON.stringify({ index: { fields: ["type", "status"] }, name: "type-status" }) });
    await admin(`/${DB}/_index`, { method: "POST", body: JSON.stringify({ index: { fields: ["type", "sessionId"] }, name: "type-session" }) });
    window.APP_CONFIG = { couchUrl: COUCH, dbName: DB, authHeader: AUTH };
    setActivePinia(createPinia());
  });

  afterAll(async () => {
    await admin(`/${DB}`, { method: "DELETE" });
  });

  it("CRUD via db.js (create → get → update → delete)", async () => {
    const { id, rev } = await createDoc({ type: "trade", ticker: "NQ", pnl: 12 });
    expect((await getDoc(id)).pnl).toBe(12);
    const upd = await updateDoc({ ...(await getDoc(id)), pnl: 20 });
    expect((await getDoc(id)).pnl).toBe(20);
    await deleteDoc(id, upd.rev);
    expect(await getDoc(id)).toBe(null);
  });

  it("flux complet via les stores : start → addTrade → clôture → rechargement persisté", async () => {
    const s1 = useSessionStore();
    await s1.start({ lossLimit: 100, gainTarget: 200, maxTrades: 5, maxLossTrades: 3, currency: "EUR" });
    await s1.addTrade({ ticker: "ES", side: "long", pnl: 15, note: "", tags: [], size: 1, entry: 10, exit: 25 });
    await s1.addTrade({ ticker: "NQ", side: "short", pnl: -5, note: "", tags: [] });
    expect(s1.pnl).toBe(10);

    setActivePinia(createPinia());
    const s2 = useSessionStore();
    await s2.loadOpen();
    expect(s2.current).toBeTruthy();
    expect(s2.trades).toHaveLength(2);
    expect(s2.pnl).toBe(10);
    await s2.close();

    setActivePinia(createPinia());
    const s3 = useSessionStore();
    await s3.loadOpen();
    expect(s3.current).toBe(null);
  });
});
