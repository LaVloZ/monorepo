# Session Tracker A — Améliorations : Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Améliorer l'app de suivi de trading existante : retenir le dernier ticker, détail cliquable des sessions passées, calculateur de P&L simple (market value), `make stop` qui coupe toute la stack, et une suite de tests d'intégration contre une vraie CouchDB.

**Architecture:** On étend l'app Vue 3 + Pinia + CouchDB existante. Logique pure ajoutée dans `lib/metrics.js` (TDD), stores et composants étendus, `db.js` rendu configurable (base + auth) pour être testable en intégration hors navigateur.

**Tech Stack:** Vue 3, Vite, Pinia, Vitest (+ @vue/test-utils, jsdom), CouchDB 3, Docker, Make.

## Global Constraints

- Interface **en français** partout. Dark mode, layout vertical/compact, rouge=perte/vert=gain, tout au clavier.
- REST direct vers CouchDB via le proxy `/db` (pas de PouchDB/IndexedDB). Base `session_tracker`.
- App dans `session-tracker/`. Node ≥ 20. Vue 3 + Vite + Pinia.
- Calculateur **sans multiplicateur d'instrument** : `pnl = size × (exit − entry) × (side === "short" ? -1 : 1)` (market value brute).
- Nouveau doc `trade` : champs **optionnels** `size`, `entry`, `exit` (nombre ou `null`). Rétrocompatible, aucune migration.
- Tests unitaires **et** d'intégration. L'intégration tourne contre une vraie CouchDB et se **skippe automatiquement** si CouchDB n'est pas joignable (les tests unitaires restent verts hors-ligne).
- Commits directement sur `master` (convention du repo).

---

## File Structure

```
session-tracker/
  src/lib/metrics.js          # + computePnl (fonction pure)
  src/stores/session.js       # + getter lastTicker ; addTrade persiste size/entry/exit
  src/components/TradeForm.vue # + champs taille/entrée/sortie, calcul auto, ticker retenu
  src/components/SessionDetail.vue   # NOUVEAU : détail d'une session
  src/components/SessionHistory.vue  # + clic sur une session → détail
  src/db.js                   # base + auth configurables (couchUrl / authHeader)
  vite.config.js              # exclut tests/integration du run par défaut
  vitest.integration.config.js # NOUVEAU : config des tests d'intégration
  package.json                # + script test:integration
  Makefile                    # stop coupe Vite aussi ; + cible test-int
  README.md                   # doc test d'intégration
  tests/metrics.test.js       # + computePnl
  tests/session.store.test.js # + lastTicker, size/entry/exit
  tests/tradeform.test.js     # + calcul auto, ticker retenu
  tests/db.test.js            # + couchUrl/authHeader
  tests/sessiondetail.test.js # NOUVEAU
  tests/integration/couchdb.integration.test.js  # NOUVEAU
```

---

## Task 1: `computePnl` (fonction pure)

**Files:**
- Modify: `session-tracker/src/lib/metrics.js`
- Test: `session-tracker/tests/metrics.test.js`

**Interfaces:**
- Consumes: rien.
- Produces: `computePnl({ size, entry, exit, side }) -> number | null`. `size × (exit − entry) × (side === "short" ? -1 : 1)`. Renvoie `null` si `size`/`entry`/`exit` est `null`, `""`, ou non numérique.

- [ ] **Step 1: Écrire les tests** — ajouter à la fin de `tests/metrics.test.js` :

```js
import { computePnl } from "../src/lib/metrics.js";

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
```

- [ ] **Step 2: Lancer → échec attendu**

Run: `cd session-tracker && npx vitest run tests/metrics.test.js`
Expected: FAIL (`computePnl` introuvable).

- [ ] **Step 3: Implémenter** — ajouter à la fin de `src/lib/metrics.js` :

```js
export function computePnl({ size, entry, exit, side }) {
  const empty = (v) => v === null || v === undefined || v === "";
  if (empty(size) || empty(entry) || empty(exit)) return null;
  const s = Number(size), e = Number(entry), x = Number(exit);
  if (Number.isNaN(s) || Number.isNaN(e) || Number.isNaN(x)) return null;
  return s * (x - e) * (side === "short" ? -1 : 1);
}
```

- [ ] **Step 4: Lancer → succès attendu**

Run: `cd session-tracker && npx vitest run tests/metrics.test.js`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
cd session-tracker
git add src/lib/metrics.js tests/metrics.test.js
git commit -m "feat(session-tracker): computePnl (P&L market value size×(exit−entry)×sens)"
```

---

## Task 2: Store session — `lastTicker` + persistance `size/entry/exit`

**Files:**
- Modify: `session-tracker/src/stores/session.js`
- Test: `session-tracker/tests/session.store.test.js`

**Interfaces:**
- Consumes: rien de nouveau.
- Produces:
  - getter `lastTicker` : ticker du dernier trade de `trades` (`""` si aucun).
  - `addTrade(data)` persiste en plus `size`, `entry`, `exit` (`data.size ?? null`, etc.).

- [ ] **Step 1: Écrire les tests** — ajouter dans `tests/session.store.test.js`, dans le `describe("session store", ...)` :

```js
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
```

- [ ] **Step 2: Lancer → échec attendu**

Run: `cd session-tracker && npx vitest run tests/session.store.test.js`
Expected: FAIL (`lastTicker` undefined ; `size` undefined au lieu de null).

- [ ] **Step 3: Implémenter** — dans `src/stores/session.js` :

Ajouter le getter `lastTicker` dans le bloc `getters` (après `losingCount`) :

```js
    lastTicker: (s) => (s.trades.length ? s.trades[s.trades.length - 1].ticker : ""),
```

Dans l'action `addTrade`, compléter l'objet `doc` (après la ligne `createdAt: ...`) :

```js
        size: data.size ?? null,
        entry: data.entry ?? null,
        exit: data.exit ?? null,
```

- [ ] **Step 4: Lancer → succès attendu**

Run: `cd session-tracker && npx vitest run tests/session.store.test.js`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
cd session-tracker
git add src/stores/session.js tests/session.store.test.js
git commit -m "feat(session-tracker): store lastTicker + persistance size/entry/exit"
```

---

## Task 3: TradeForm — calculateur auto + ticker retenu

**Files:**
- Modify: `session-tracker/src/components/TradeForm.vue`
- Test: `session-tracker/tests/tradeform.test.js`

**Interfaces:**
- Consumes: `computePnl` (Task 1), `useSessionStore` + getter `lastTicker` (Task 2).
- Produces: formulaire avec 3 champs optionnels taille/entrée/sortie ; si les trois sont numériques, le pnl est calculé et le champ résultat passe en lecture seule ; le ticker est pré-rempli depuis `lastTicker` et conservé après ajout.

- [ ] **Step 1: Écrire les tests** — ajouter dans `tests/tradeform.test.js`, dans le `describe("TradeForm — garde-fou de saisie", ...)` :

```js
  it("calcule le pnl depuis taille/entrée/sortie (long) et persiste size/entry/exit", async () => {
    const wrapper = mount(TradeForm);
    const session = useSessionStore();
    const spy = vi.spyOn(session, "addTrade").mockResolvedValue();
    await wrapper.find('input[placeholder="Ticker (ex. NQ)"]').setValue("NQ");
    await wrapper.find('input[placeholder="Taille"]').setValue("2");
    await wrapper.find('input[placeholder="Entrée"]').setValue("100");
    await wrapper.find('input[placeholder="Sortie"]').setValue("110");
    await wrapper.find("form").trigger("submit.prevent");
    expect(spy).toHaveBeenCalledTimes(1);
    const arg = spy.mock.calls[0][0];
    expect(arg.pnl).toBe(20);
    expect(arg.size).toBe(2);
    expect(arg.entry).toBe(100);
    expect(arg.exit).toBe(110);
  });

  it("retient le ticker après un ajout", async () => {
    const wrapper = mount(TradeForm);
    const session = useSessionStore();
    vi.spyOn(session, "addTrade").mockResolvedValue();
    await wrapper.find('input[placeholder="Ticker (ex. NQ)"]').setValue("NQ");
    await wrapper.find('input[placeholder="Résultat (€/$)"]').setValue("15");
    await wrapper.find("form").trigger("submit.prevent");
    await wrapper.vm.$nextTick();
    expect(wrapper.find('input[placeholder="Ticker (ex. NQ)"]').element.value).toBe("NQ");
  });
```

- [ ] **Step 2: Lancer → échec attendu**

Run: `cd session-tracker && npx vitest run tests/tradeform.test.js`
Expected: FAIL (champs Taille/Entrée/Sortie absents ; ticker vidé après ajout).

- [ ] **Step 3: Remplacer entièrement `src/components/TradeForm.vue`** par :

```vue
<template>
  <form class="card" @submit.prevent="submit">
    <div class="grid">
      <input ref="tickerEl" v-model="f.ticker" placeholder="Ticker (ex. NQ)" />
      <div class="side">
        <button type="button" :class="{ on: f.side === 'long' }" @click="f.side = 'long'">Long</button>
        <button type="button" :class="{ on: f.side === 'short' }" @click="f.side = 'short'">Short</button>
      </div>
    </div>
    <div class="grid three">
      <input v-model.number="f.size" type="number" step="any" placeholder="Taille" />
      <input v-model.number="f.entry" type="number" step="any" placeholder="Entrée" />
      <input v-model.number="f.exit" type="number" step="any" placeholder="Sortie" />
    </div>
    <div class="grid">
      <input v-if="autoPnl === null" v-model.number="f.pnl" type="number" step="any" placeholder="Résultat (€/$)" />
      <input v-else :value="autoPnl" type="number" readonly class="ro" title="Calculé depuis taille/entrée/sortie" />
      <input v-model.number="f.rMultiple" type="number" step="any" placeholder="R (optionnel)" />
    </div>
    <input v-model="f.note" placeholder="Note (setup, erreur, émotion)" />
    <input v-model="f.tagsCsv" placeholder="Tags (séparés par des virgules)" />
    <button type="submit" class="add">＋ Ajouter le trade (Entrée)</button>
    <p v-if="disabled" class="warn">⛔ Limite atteinte — trade quand même possible.</p>
    <p v-if="error" class="err">{{ error }}</p>
  </form>
</template>

<script setup>
import { reactive, ref, computed, onMounted } from "vue";
import { useSessionStore } from "../stores/session.js";
import { computePnl } from "../lib/metrics.js";

const props = defineProps({ disabled: { type: Boolean, default: false } });
const emit = defineEmits(["added"]);
const session = useSessionStore();
const tickerEl = ref(null);
const empty = () => ({
  ticker: "", side: "long", pnl: null, rMultiple: null, note: "", tagsCsv: "",
  size: null, entry: null, exit: null,
});
const f = reactive(empty());
const error = ref("");

const autoPnl = computed(() =>
  computePnl({ size: f.size, entry: f.entry, exit: f.exit, side: f.side })
);

onMounted(() => {
  f.ticker = session.lastTicker || "";
  tickerEl.value?.focus();
});

const num = (v) => (v === "" || v === null ? null : Number(v));

async function submit() {
  const effectivePnl = autoPnl.value !== null ? autoPnl.value : Number(f.pnl);
  const pnlProvided = autoPnl.value !== null || (f.pnl !== "" && f.pnl !== null);
  if (f.ticker.trim() === "" || !pnlProvided || Number.isNaN(effectivePnl)) return;
  const r = f.rMultiple === "" || f.rMultiple === null ? null : Number(f.rMultiple);
  try {
    await session.addTrade({
      ticker: f.ticker.trim().toUpperCase(),
      side: f.side,
      pnl: effectivePnl,
      rMultiple: r,
      note: f.note.trim(),
      tags: f.tagsCsv.split(",").map((t) => t.trim()).filter(Boolean),
      size: num(f.size),
      entry: num(f.entry),
      exit: num(f.exit),
    });
  } catch (e) {
    error.value = "Échec de l'enregistrement du trade — réessaie.";
    return;
  }
  error.value = "";
  const keptTicker = f.ticker;
  Object.assign(f, empty());
  f.ticker = keptTicker;
  tickerEl.value?.focus();
  emit("added");
}
</script>

<style scoped>
.card { background: var(--panel); border: 1px solid var(--border); border-radius: 10px; padding: 12px; display: flex; flex-direction: column; gap: 8px; }
.grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; }
.grid.three { grid-template-columns: 1fr 1fr 1fr; }
.side { display: flex; gap: 6px; }
.side button { flex: 1; }
.side button.on { background: var(--accent); border-color: var(--accent); color: #fff; }
.ro { opacity: .8; background: var(--panel); }
.add { background: var(--green); border-color: var(--green); color: #06210f; font-weight: 600; }
.warn { color: var(--amber); font-size: 12px; margin: 0; text-align: center; }
</style>
```

- [ ] **Step 4: Lancer → succès attendu**

Run: `cd session-tracker && npx vitest run tests/tradeform.test.js && npx vitest run`
Expected: PASS (nouveaux tests + aucune régression sur la suite complète).

- [ ] **Step 5: Vérifier le build**

Run: `cd session-tracker && npx vite build`
Expected: build propre.

- [ ] **Step 6: Commit**

```bash
cd session-tracker
git add src/components/TradeForm.vue tests/tradeform.test.js
git commit -m "feat(session-tracker): TradeForm calculateur P&L auto + ticker retenu"
```

---

## Task 4: Détail de session (`SessionDetail.vue`) + historique cliquable

**Files:**
- Create: `session-tracker/src/components/SessionDetail.vue`
- Modify: `session-tracker/src/components/SessionHistory.vue`
- Test: `session-tracker/tests/sessiondetail.test.js`

**Interfaces:**
- Consumes: `metrics.js` (`sumPnl`, `winRate`, `bestTrade`, `worstTrade`, `formatDuration`).
- Produces: `SessionDetail.vue` props `session` (objet), `trades` (array) ; émet `back`. `SessionHistory` affiche le détail quand une session est sélectionnée.

- [ ] **Step 1: Écrire les tests** — créer `tests/sessiondetail.test.js` :

```js
import { describe, it, expect } from "vitest";
import { mount } from "@vue/test-utils";
import SessionDetail from "../src/components/SessionDetail.vue";

const session = { _id: "s1", startedAt: "2026-07-20T08:00:00.000Z", endedAt: "2026-07-20T09:00:00.000Z" };
const trades = [
  { _id: "t1", sessionId: "s1", ticker: "NQ", side: "long", pnl: 30, rMultiple: 1.5, note: "bon setup", tags: ["breakout"], createdAt: "2026-07-20T08:10:00.000Z", size: 2, entry: 100, exit: 115 },
  { _id: "t2", sessionId: "s1", ticker: "ES", side: "short", pnl: -10, rMultiple: null, note: "", tags: [], createdAt: "2026-07-20T08:30:00.000Z", size: null, entry: null, exit: null },
];

describe("SessionDetail", () => {
  it("affiche le P&L total, le nombre de trades et la durée", () => {
    const w = mount(SessionDetail, { props: { session, trades } });
    expect(w.text()).toContain("+20.00");
    expect(w.text()).toContain("2 trades");
    expect(w.text()).toContain("01:00:00");
  });
  it("liste chaque trade avec son ticker et son pnl", () => {
    const w = mount(SessionDetail, { props: { session, trades } });
    expect(w.text()).toContain("NQ");
    expect(w.text()).toContain("+30.00");
    expect(w.text()).toContain("ES");
    expect(w.text()).toContain("-10.00");
  });
  it("émet back au clic sur Retour", async () => {
    const w = mount(SessionDetail, { props: { session, trades } });
    await w.find(".back").trigger("click");
    expect(w.emitted("back")).toBeTruthy();
  });
  it("affiche 'Aucun trade' si la session est vide", () => {
    const w = mount(SessionDetail, { props: { session, trades: [] } });
    expect(w.text()).toContain("Aucun trade");
  });
});
```

- [ ] **Step 2: Lancer → échec attendu**

Run: `cd session-tracker && npx vitest run tests/sessiondetail.test.js`
Expected: FAIL (`SessionDetail.vue` introuvable).

- [ ] **Step 3: Créer `src/components/SessionDetail.vue`** :

```vue
<template>
  <section class="detail">
    <button class="back" @click="emit('back')">← Retour</button>
    <div class="stats">
      <div class="pnl" :class="pnl >= 0 ? 'pos' : 'neg'">{{ pnl >= 0 ? '+' : '' }}{{ pnl.toFixed(2) }}</div>
      <div class="meta">
        <span>{{ trades.length }} trades</span>
        <span>{{ wr === null ? '—' : Math.round(wr * 100) + '%' }} réussite</span>
        <span>⏱ {{ duration }}</span>
      </div>
      <div class="meta" v-if="trades.length">
        <span>Meilleur : <b class="pos">+{{ best.pnl.toFixed(2) }}</b></span>
        <span>Pire : <b class="neg">{{ worst.pnl.toFixed(2) }}</b></span>
      </div>
    </div>
    <ul class="list">
      <li v-for="t in ordered" :key="t._id">
        <div class="row1">
          <span>{{ t.side === 'long' ? '▲' : '▼' }} {{ t.ticker }}</span>
          <span :class="t.pnl >= 0 ? 'pos' : 'neg'">{{ t.pnl >= 0 ? '+' : '' }}{{ t.pnl.toFixed(2) }}</span>
        </div>
        <div class="row2">
          <span>{{ time(t.createdAt) }}</span>
          <span v-if="t.rMultiple != null">R {{ t.rMultiple }}</span>
          <span v-if="t.size != null">{{ t.size }} @ {{ t.entry }} → {{ t.exit }}</span>
        </div>
        <div class="row2 note" v-if="t.note">{{ t.note }}</div>
        <div class="tags" v-if="t.tags && t.tags.length">
          <span v-for="tag in t.tags" :key="tag" class="tag">{{ tag }}</span>
        </div>
      </li>
    </ul>
    <p v-if="!trades.length" class="muted">Aucun trade.</p>
  </section>
</template>

<script setup>
import { computed } from "vue";
import { sumPnl, winRate, bestTrade, worstTrade, formatDuration } from "../lib/metrics.js";

const props = defineProps({
  session: { type: Object, required: true },
  trades: { type: Array, default: () => [] },
});
const emit = defineEmits(["back"]);

const pnl = computed(() => sumPnl(props.trades));
const wr = computed(() => winRate(props.trades));
const best = computed(() => bestTrade(props.trades));
const worst = computed(() => worstTrade(props.trades));
const ordered = computed(() => [...props.trades].sort((a, b) => a.createdAt.localeCompare(b.createdAt)));
const duration = computed(() => {
  const end = new Date(props.session.endedAt || props.session.startedAt).getTime();
  return formatDuration(end - new Date(props.session.startedAt).getTime());
});
function time(iso) {
  return new Date(iso).toLocaleTimeString("fr-FR", { hour: "2-digit", minute: "2-digit" });
}
</script>

<style scoped>
.detail { display: flex; flex-direction: column; gap: 10px; }
.back { align-self: flex-start; }
.stats { background: var(--panel); border: 1px solid var(--border); border-radius: 10px; padding: 12px; text-align: center; }
.pnl { font-size: 28px; font-weight: 700; font-variant-numeric: tabular-nums; }
.meta { display: flex; justify-content: center; gap: 12px; font-size: 12px; color: var(--muted); margin-top: 4px; flex-wrap: wrap; }
.list { list-style: none; padding: 0; margin: 0; display: flex; flex-direction: column; gap: 6px; }
.list li { background: var(--panel); border: 1px solid var(--border); border-radius: 8px; padding: 8px 10px; display: flex; flex-direction: column; gap: 2px; }
.row1 { display: flex; justify-content: space-between; font-variant-numeric: tabular-nums; }
.row2 { display: flex; gap: 10px; font-size: 11px; color: var(--muted); }
.note { font-style: italic; }
.tags { display: flex; gap: 4px; flex-wrap: wrap; }
.tag { font-size: 10px; background: var(--bg); border: 1px solid var(--border); border-radius: 4px; padding: 1px 5px; color: var(--muted); }
.muted { color: var(--muted); }
</style>
```

- [ ] **Step 4: Lancer → succès attendu**

Run: `cd session-tracker && npx vitest run tests/sessiondetail.test.js`
Expected: PASS.

- [ ] **Step 5: Rendre les sessions cliquables** — remplacer entièrement `src/components/SessionHistory.vue` par :

```vue
<template>
  <section class="hist">
    <SessionDetail
      v-if="selectedSession"
      :session="selectedSession"
      :trades="selectedTrades"
      @back="selected = null"
    />
    <template v-else>
      <PnlCalendar :pnl-by-day="byDay" :year="year" :month="month" />
      <h3>Sessions</h3>
      <ul class="list">
        <li v-for="r in summaries" :key="r.id" class="clickable" @click="selected = r.id">
          <span class="date">{{ fmt(r.startedAt) }}</span>
          <span :class="r.pnl >= 0 ? 'pos' : 'neg'">{{ r.pnl >= 0 ? '+' : '' }}{{ r.pnl.toFixed(2) }}</span>
          <span class="meta">{{ r.count }} trades · {{ r.winRate === null ? '—' : Math.round(r.winRate * 100) + '%' }}</span>
        </li>
      </ul>
      <p v-if="!summaries.length" class="muted">Aucune session enregistrée.</p>
      <p v-if="error" class="err">{{ error }}</p>
    </template>
  </section>
</template>

<script setup>
import { ref, computed, onMounted } from "vue";
import { find } from "../db.js";
import { sessionSummaries, pnlByDay } from "../lib/history.js";
import PnlCalendar from "./PnlCalendar.vue";
import SessionDetail from "./SessionDetail.vue";

const sessions = ref([]);
const trades = ref([]);
const error = ref("");
const selected = ref(null);
const nowDate = new Date();
const year = nowDate.getFullYear();
const month = nowDate.getMonth();

onMounted(async () => {
  try {
    sessions.value = await find({ type: "session" });
    trades.value = await find({ type: "trade" });
  } catch (e) {
    error.value = "Impossible de charger l'historique.";
  }
});

const byDay = computed(() => pnlByDay(sessions.value, trades.value));
const summaries = computed(() => {
  const byS = {};
  for (const t of trades.value) (byS[t.sessionId] ||= []).push(t);
  return sessionSummaries(sessions.value, byS)
    .sort((a, b) => b.startedAt.localeCompare(a.startedAt));
});
const selectedSession = computed(() => sessions.value.find((s) => s._id === selected.value) || null);
const selectedTrades = computed(() => trades.value.filter((t) => t.sessionId === selected.value));

function fmt(iso) {
  return new Date(iso).toLocaleString("fr-FR", { dateStyle: "short", timeStyle: "short" });
}
</script>

<style scoped>
.hist { display: flex; flex-direction: column; gap: 12px; }
.list { list-style: none; padding: 0; margin: 0; display: flex; flex-direction: column; gap: 6px; }
.list li { display: grid; grid-template-columns: 1fr auto; gap: 2px 8px; background: var(--panel); border: 1px solid var(--border); border-radius: 8px; padding: 8px 10px; }
.clickable { cursor: pointer; }
.clickable:hover { border-color: var(--accent); }
.date { font-size: 13px; }
.meta { grid-column: 1 / -1; font-size: 11px; color: var(--muted); }
.muted { color: var(--muted); }
</style>
```

- [ ] **Step 6: Lancer la suite + build**

Run: `cd session-tracker && npx vitest run && npx vite build`
Expected: PASS + build propre (aucune régression).

- [ ] **Step 7: Commit**

```bash
cd session-tracker
git add src/components/SessionDetail.vue src/components/SessionHistory.vue tests/sessiondetail.test.js
git commit -m "feat(session-tracker): détail de session cliquable dans l'historique"
```

---

## Task 5: `make stop` coupe toute la stack

**Files:**
- Modify: `session-tracker/Makefile`

**Interfaces:**
- Consumes: rien.
- Produces: `make stop` arrête CouchDB **et** le serveur Vite de dev.

- [ ] **Step 1: Modifier la cible `stop` du `Makefile`** — remplacer les deux lignes actuelles :

```make
stop: ## Arrête les conteneurs (garde les données)
	docker compose stop
```

par :

```make
stop: ## Arrête toute la stack : conteneurs Docker + serveur Vite de dev (garde les données)
	docker compose stop
	-pkill -f "vite" || true
	@echo "🛑 Stack arrêtée (Docker + Vite)."
```

(Le `-` en tête et le `|| true` évitent toute erreur `make` si aucun serveur Vite ne tourne.)

- [ ] **Step 2: Vérifier**

Run: `cd session-tracker && make -n stop`
Expected: affiche les commandes `docker compose stop`, `pkill -f "vite" || true`, et l'echo — sans erreur de parse.

- [ ] **Step 3: Commit**

```bash
cd session-tracker
git add Makefile
git commit -m "fix(session-tracker): make stop coupe aussi le serveur Vite (toute la stack)"
```

---

## Task 6: `db.js` configurable + tests d'intégration CouchDB

**Files:**
- Modify: `session-tracker/src/db.js`
- Modify: `session-tracker/tests/db.test.js`
- Modify: `session-tracker/vite.config.js`
- Create: `session-tracker/vitest.integration.config.js`
- Create: `session-tracker/tests/integration/couchdb.integration.test.js`
- Modify: `session-tracker/package.json`
- Modify: `session-tracker/Makefile`
- Modify: `session-tracker/README.md`

**Interfaces:**
- Consumes: tout (stores, db.js).
- Produces: `db.js` lit `window.APP_CONFIG.couchUrl` (base, défaut `/db`) et `window.APP_CONFIG.authHeader` (en-tête `Authorization` optionnel). Suite d'intégration `npm run test:integration` / `make test-int`, skippée si CouchDB absente.

- [ ] **Step 1: Écrire le test unitaire de config** — ajouter à la fin de `tests/db.test.js` :

```js
describe("configuration base + auth", () => {
  it("utilise couchUrl comme base et ajoute l'en-tête Authorization", async () => {
    window.APP_CONFIG = { couchUser: "u", couchPassword: "p", dbName: "d", couchUrl: "http://cdb:5984", authHeader: "Basic ABC" };
    mockFetchOnce({ json: { id: "1", rev: "1-x" } });
    await createDoc({ type: "trade" });
    expect(fetch.mock.calls[0][0]).toBe("http://cdb:5984/d");
    expect(fetch.mock.calls[0][1].headers.Authorization).toBe("Basic ABC");
  });
});
```

- [ ] **Step 2: Lancer → échec attendu**

Run: `cd session-tracker && npx vitest run tests/db.test.js`
Expected: FAIL (base reste `/db/d`, pas d'`Authorization`).

- [ ] **Step 3: Rendre `db.js` configurable** — remplacer le haut du fichier (lignes 1 à 23, `cfg`/`base`/`req`/`login`) par :

```js
const cfg = () => window.APP_CONFIG;
const root = () => cfg().couchUrl || "/db";
const base = () => `${root()}/${cfg().dbName}`;

async function req(url, options = {}, retried = false) {
  const headers = { "Content-Type": "application/json", ...(options.headers || {}) };
  if (cfg().authHeader) headers.Authorization = cfg().authHeader;
  const res = await fetch(url, {
    credentials: "same-origin",
    ...options,
    headers,
  });
  if (res.status === 401 && !retried && url !== `${root()}/_session`) {
    await login();
    return req(url, options, true);
  }
  return res;
}

export async function login() {
  const res = await req(`${root()}/_session`, {
    method: "POST",
    body: JSON.stringify({ name: cfg().couchUser, password: cfg().couchPassword }),
  });
  if (!res.ok) throw new Error(`login CouchDB: ${res.status}`);
}
```

(Le reste du fichier — `createDoc`, `getDoc`, `updateDoc`, `deleteDoc`, `find` — est inchangé : ils utilisent déjà `base()`.)

- [ ] **Step 4: Lancer → succès attendu**

Run: `cd session-tracker && npx vitest run tests/db.test.js`
Expected: PASS (nouveau test + les anciens, dont la ré-auth 401, toujours verts).

- [ ] **Step 5: Exclure l'intégration du run par défaut** — remplacer `vite.config.js` par :

```js
import { defineConfig, configDefaults } from "vitest/config";
import vue from "@vitejs/plugin-vue";

export default defineConfig({
  plugins: [vue()],
  server: {
    port: 8080,
    proxy: {
      // Même origine côté navigateur → aucun CORS. CouchDB écoute en 5984.
      "/db": {
        target: "http://localhost:5984",
        changeOrigin: true,
        rewrite: (p) => p.replace(/^\/db/, ""),
      },
    },
  },
  test: {
    environment: "jsdom",
    globals: true,
    exclude: [...configDefaults.exclude, "tests/integration/**"],
  },
});
```

- [ ] **Step 6: Créer `vitest.integration.config.js`** :

```js
import { defineConfig } from "vitest/config";

export default defineConfig({
  test: {
    environment: "jsdom",
    globals: true,
    include: ["tests/integration/**/*.test.js"],
  },
});
```

- [ ] **Step 7: Créer `tests/integration/couchdb.integration.test.js`** :

```js
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
```

- [ ] **Step 8: Ajouter le script `test:integration`** — dans `package.json`, section `scripts`, ajouter après `"test:watch": "vitest"` (ajouter une virgule à la ligne précédente) :

```json
    "test:integration": "vitest run --config vitest.integration.config.js"
```

- [ ] **Step 9: Ajouter la cible Make `test-int`** — dans `Makefile` :

Ajouter `test-int` à la ligne `.PHONY: ...`. Puis ajouter cette cible (après la cible `test`) :

```make
test-int: ## Tests d'intégration contre une vraie CouchDB (démarre CouchDB si besoin)
	docker compose up -d couchdb
	npm run test:integration
```

- [ ] **Step 10: Documenter dans `README.md`** — ajouter dans la section « Dév » (après la ligne `npm test`) :

```markdown
make test-int   # tests d'intégration contre une vraie CouchDB (skippés si CouchDB absente)
```

- [ ] **Step 11: Vérifier — unitaires hors-ligne, puis intégration si CouchDB dispo**

Run (unitaires, doivent rester verts sans CouchDB) : `cd session-tracker && npx vitest run`
Expected: PASS, et **aucun** test d'intégration exécuté (dossier exclu).

Run (intégration, si Docker/CouchDB dispo) : `cd session-tracker && docker compose up -d couchdb && npm run test:integration`
Expected: soit PASS (CouchDB joignable → 2 tests verts), soit tout skippé (`describe.skipIf`) si CouchDB injoignable. Dans les deux cas, exit 0.

- [ ] **Step 12: Commit**

```bash
cd session-tracker
git add src/db.js tests/db.test.js vite.config.js vitest.integration.config.js tests/integration package.json Makefile README.md
git commit -m "test(session-tracker): db.js configurable + suite d'intégration CouchDB (skip auto)"
```

---

## Self-Review (auteur du plan)

**Couverture du spec :**
- Retenir le dernier ticker → Task 2 (getter) + Task 3 (pré-remplissage + conservation). ✓
- Détail de session dans l'historique → Task 4. ✓
- Calculateur P&L market value + persistance size/entry/exit → Task 1 (`computePnl`), Task 2 (store), Task 3 (UI). ✓
- `make stop` coupe toute la stack → Task 5. ✓
- Tests unitaires + intégration (skip auto) → tests unitaires dans Tasks 1-4/6 ; intégration dans Task 6. ✓
- Doc `trade` + champs optionnels rétrocompatibles → Task 2 (`?? null`), lus `!= null` dans Task 4. ✓

**Placeholders :** aucun ; code complet à chaque étape. ✓

**Cohérence des types/signatures :**
- `computePnl({ size, entry, exit, side })` identique en Task 1 (def), Task 3 (appel via `autoPnl`). ✓
- `addTrade(data)` accepte `size/entry/exit` (Task 2) et les reçoit de TradeForm (Task 3). ✓
- `lastTicker` défini (Task 2), consommé (Task 3). ✓
- `SessionDetail` props `session`/`trades`, émet `back` — cohérent entre Task 4 def et l'usage dans `SessionHistory`. ✓
- `db.js` `root()`/`authHeader` — le test 401 existant compare désormais à `${root()}/_session` ; en config par défaut `root()` vaut `/db`, donc `/db/_session` inchangé. ✓
