import { defineStore } from "pinia";
import { createDoc, updateDoc, find } from "../db.js";
import {
  sumPnl, winRate, bestTrade, worstTrade, losingCount, stopState,
  lossLimitPct, gainTargetPct,
} from "../lib/metrics.js";

export const useSessionStore = defineStore("session", {
  state: () => ({ current: null, trades: [] }),
  getters: {
    pnl: (s) => sumPnl(s.trades),
    winRate: (s) => winRate(s.trades),
    best: (s) => bestTrade(s.trades),
    worst: (s) => worstTrade(s.trades),
    losingCount: (s) => losingCount(s.trades),
    lastTicker: (s) => (s.trades.length ? s.trades[s.trades.length - 1].ticker : ""),
    lossPct: (s) => (s.current ? lossLimitPct(s.trades, s.current.params.lossLimit) : 0),
    gainPct: (s) => (s.current ? gainTargetPct(s.trades, s.current.params.gainTarget) : 0),
    stop: (s) =>
      s.current
        ? stopState(s.trades, s.current.params)
        : { stopped: false, reasons: [], lossWarning: false },
  },
  actions: {
    async loadOpen() {
      const sessions = await find({ type: "session", status: "open" });
      this.current = sessions[0] || null;
      if (this.current) {
        this.trades = await find({ type: "trade", sessionId: this.current._id });
        this.trades.sort((a, b) => a.createdAt.localeCompare(b.createdAt));
      } else {
        this.trades = [];
      }
    },
    async start(params) {
      const doc = {
        type: "session",
        status: "open",
        startedAt: new Date().toISOString(),
        endedAt: null,
        params: { ...params },
      };
      const res = await createDoc(doc);
      this.current = { ...doc, _id: res.id, _rev: res.rev };
      this.trades = [];
    },
    async close() {
      if (!this.current) return;
      const doc = { ...this.current, status: "closed", endedAt: new Date().toISOString() };
      await updateDoc(doc);
      this.current = null;
      this.trades = [];
    },
    async addTrade(data) {
      if (!this.current) throw new Error("Aucune session ouverte");
      const doc = {
        type: "trade",
        sessionId: this.current._id,
        ticker: data.ticker,
        side: data.side,
        pnl: data.pnl,
        rMultiple: data.rMultiple ?? null,
        note: data.note || "",
        tags: data.tags || [],
        createdAt: new Date().toISOString(),
        size: data.size ?? null,
        entry: data.entry ?? null,
        exit: data.exit ?? null,
      };
      const res = await createDoc(doc);
      this.trades.push({ ...doc, _id: res.id, _rev: res.rev });
    },
  },
});
