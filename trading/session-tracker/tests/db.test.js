import { describe, it, expect, vi, beforeEach } from "vitest";
import { createDoc, getDoc, updateDoc, find, login, deleteDoc } from "../src/db.js";

beforeEach(() => {
  globalThis.window = { APP_CONFIG: { couchUser: "u", couchPassword: "p", dbName: "testdb" } };
  vi.restoreAllMocks();
});

function mockFetchOnce({ ok = true, status = 200, json = {} }) {
  globalThis.fetch = vi.fn().mockResolvedValue({
    ok, status, json: async () => json,
  });
}

describe("login", () => {
  it("POST /db/_session avec les identifiants", async () => {
    mockFetchOnce({ json: { ok: true } });
    await login();
    const [url, opts] = fetch.mock.calls[0];
    expect(url).toBe("/db/_session");
    expect(opts.method).toBe("POST");
    expect(JSON.parse(opts.body)).toEqual({ name: "u", password: "p" });
  });
});

describe("createDoc", () => {
  it("POST vers la base et renvoie {id, rev}", async () => {
    mockFetchOnce({ json: { id: "abc", rev: "1-x" } });
    const res = await createDoc({ type: "trade", pnl: 5 });
    expect(fetch.mock.calls[0][0]).toBe("/db/testdb");
    expect(res).toEqual({ id: "abc", rev: "1-x" });
    expect(fetch.mock.calls[0][1].credentials).toBe("same-origin");
    expect(fetch.mock.calls[0][1].headers["Content-Type"]).toBe("application/json");
  });
});

describe("getDoc", () => {
  it("renvoie le doc si trouvé", async () => {
    mockFetchOnce({ json: { _id: "abc", pnl: 5 } });
    expect(await getDoc("abc")).toEqual({ _id: "abc", pnl: 5 });
  });
  it("renvoie null si 404", async () => {
    mockFetchOnce({ ok: false, status: 404 });
    expect(await getDoc("nope")).toBe(null);
  });
});

describe("updateDoc", () => {
  it("PUT sur /db/{db}/{_id}", async () => {
    mockFetchOnce({ json: { id: "abc", rev: "2-y" } });
    await updateDoc({ _id: "abc", _rev: "1-x", pnl: 9 });
    expect(fetch.mock.calls[0][0]).toBe("/db/testdb/abc");
    expect(fetch.mock.calls[0][1].method).toBe("PUT");
  });
});

describe("deleteDoc", () => {
  it("DELETE /db/{db}/{id}?rev=...", async () => {
    mockFetchOnce({ json: { ok: true } });
    await deleteDoc("abc", "1-x");
    expect(fetch.mock.calls[0][0]).toBe("/db/testdb/abc?rev=1-x");
    expect(fetch.mock.calls[0][1].method).toBe("DELETE");
  });
});

describe("find", () => {
  it("POST _find et renvoie docs", async () => {
    mockFetchOnce({ json: { docs: [{ _id: "a" }, { _id: "b" }] } });
    const docs = await find({ type: "trade" });
    expect(fetch.mock.calls[0][0]).toBe("/db/testdb/_find");
    expect(docs).toHaveLength(2);
  });
});

describe("ré-authentification sur 401", () => {
  it("relogue et réessaie une fois quand une requête renvoie 401", async () => {
    globalThis.fetch = vi.fn()
      .mockResolvedValueOnce({ ok: false, status: 401, json: async () => ({}) })
      .mockResolvedValueOnce({ ok: true, status: 200, json: async () => ({ ok: true }) })
      .mockResolvedValueOnce({ ok: true, status: 200, json: async () => ({ _id: "abc" }) });
    const doc = await getDoc("abc");
    expect(doc).toEqual({ _id: "abc" });
    expect(fetch.mock.calls.some((c) => c[0] === "/db/_session")).toBe(true);
    expect(fetch).toHaveBeenCalledTimes(3);
  });
});

describe("configuration base + auth", () => {
  it("utilise couchUrl comme base et ajoute l'en-tête Authorization", async () => {
    window.APP_CONFIG = { couchUser: "u", couchPassword: "p", dbName: "d", couchUrl: "http://cdb:5984", authHeader: "Basic ABC" };
    mockFetchOnce({ json: { id: "1", rev: "1-x" } });
    await createDoc({ type: "trade" });
    expect(fetch.mock.calls[0][0]).toBe("http://cdb:5984/d");
    expect(fetch.mock.calls[0][1].headers.Authorization).toBe("Basic ABC");
  });
});
