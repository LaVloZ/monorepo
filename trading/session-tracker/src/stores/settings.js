import { defineStore } from "pinia";
import { getDoc, createDoc, updateDoc } from "../db.js";

const DEFAULTS = {
  lossLimit: 500, gainTarget: 500, maxTrades: 5, maxLossTrades: 3, currency: "EUR",
};

export const useSettingsStore = defineStore("settings", {
  state: () => ({ settings: { ...DEFAULTS }, _rev: null }),
  actions: {
    async load() {
      const doc = await getDoc("settings");
      if (doc) {
        this.settings = {
          lossLimit: doc.lossLimit, gainTarget: doc.gainTarget,
          maxTrades: doc.maxTrades, maxLossTrades: doc.maxLossTrades,
          currency: doc.currency,
        };
        this._rev = doc._rev;
      } else {
        this.settings = { ...DEFAULTS };
        this._rev = null;
      }
    },
    async save(newSettings) {
      this.settings = { ...newSettings };
      const doc = { _id: "settings", type: "settings", ...this.settings };
      if (this._rev) doc._rev = this._rev;
      const res = this._rev ? await updateDoc(doc) : await createDoc(doc);
      this._rev = res.rev;
    },
  },
});
