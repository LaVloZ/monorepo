# Trading Session Tracker — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Une app web perso de suivi de sessions de trading (démarrer/clôturer une session avec règles de risque, saisie manuelle rapide de trades, dashboard live), local-first, données dans CouchDB en Docker.

**Architecture:** Front Vue 3 (Vite + Pinia) qui parle directement à CouchDB via REST (`fetch`). Un reverse-proxy `/db` (Vite en dev, nginx en prod) route vers CouchDB → même origine, **aucun CORS**. CouchDB tourne en Docker avec un volume persistant = source unique de vérité, hors navigateur. Seuls les trades saisis manuellement sont stockés (Bybit = v2, jamais persisté).

**Tech Stack:** Vue 3, Vite, Pinia, Vitest (+ jsdom, @vue/test-utils), CouchDB 3, Docker / docker-compose, nginx.

## Global Constraints

- Interface **en français** partout (labels, messages, boutons).
- **Dark mode par défaut**, layout **vertical/compact** (utilisable en fenêtre étroite / side-panel).
- Rouge = perte, vert = gain. Sobre, lisible en un coup d'œil.
- Tout doit être **faisable au clavier** (focus auto, validation `Entrée`).
- **Pas de PouchDB, pas d'IndexedDB** : accès REST direct à CouchDB.
- Nom de la base CouchDB : `session_tracker`.
- L'app vit dans le sous-dossier `session-tracker/` du repo.
- Node ≥ 20. CouchDB image `couchdb:3` (compatible arm64 pour Raspberry Pi 4/5 en OS 64-bit).
- Une seule session `status: "open"` à la fois ; plusieurs sessions par jour autorisées.
- Les trades Bybit ne sont **jamais** stockés dans CouchDB.
- Types de documents : `session`, `trade`, `settings` (voir spec). `settings` a `_id: "settings"`.

**Params d'une session / settings** (mêmes clés partout) :
`{ lossLimit: number, gainTarget: number, maxTrades: number, maxLossTrades: number, currency: "EUR" | "USD" }`

---

## File Structure

```
session-tracker/
  docker-compose.yml         # services couchdb (+volume) et front (nginx)
  Dockerfile                 # build du front, servi par nginx
  nginx.conf                 # sert le front + reverse-proxy /db -> couchdb
  .dockerignore
  .gitignore
  package.json
  vite.config.js             # inclut proxy /db -> couchdb (dev) + config Vitest
  index.html
  public/
    config.js                # window.APP_CONFIG : identifiants CouchDB (local, non exposé)
  README.md
  src/
    main.js                  # bootstrap Vue + Pinia
    App.vue                  # shell + navigation entre vues
    db.js                    # client REST CouchDB (login, CRUD, find)
    lib/
      metrics.js             # fonctions pures : pnl, winRate, jauges, états STOP, durée
    composables/
      useTimer.js            # horloge réactive (tick 1s) pour la durée de session
    stores/
      settings.js            # Pinia : settings par défaut
      session.js             # Pinia : session courante + trades + métriques dérivées
    components/
      Gauge.vue              # jauge générique (valeur/max, seuils couleur)
      StopBanner.vue         # bandeau « STOP TRADING »
      StartSession.vue       # écran pré-session (règles du jour)
      TradeForm.vue          # saisie rapide d'un trade
      LiveDashboard.vue      # dashboard de la session en cours
      SessionHistory.vue     # liste des sessions + calendrier P&L
      PnlCalendar.vue        # calendrier mensuel vert/rouge
      SettingsView.vue       # réglages par défaut + connexion CouchDB
  tests/
    metrics.test.js
    db.test.js
    session.store.test.js
    settings.store.test.js
```

---

## Task 1: Scaffold projet + Docker + CouchDB (proxy `/db`, sans CORS)

**Files:**
- Create: `session-tracker/package.json`
- Create: `session-tracker/index.html`
- Create: `session-tracker/vite.config.js`
- Create: `session-tracker/src/main.js`
- Create: `session-tracker/src/App.vue`
- Create: `session-tracker/public/config.js`
- Create: `session-tracker/docker-compose.yml`
- Create: `session-tracker/Dockerfile`
- Create: `session-tracker/nginx.conf`
- Create: `session-tracker/.dockerignore`
- Create: `session-tracker/.gitignore`
- Create: `session-tracker/README.md`
- Test: `session-tracker/tests/smoke.test.js`

**Interfaces:**
- Consumes: rien (première tâche).
- Produces : projet Vite qui démarre (`npm run dev`), Vitest opérationnel (`npm test`), `docker compose up` qui lance CouchDB (base `session_tracker` créée) accessible via le proxy `/db`. `window.APP_CONFIG = { couchUser, couchPassword, dbName }`.

- [ ] **Step 1: Créer `package.json`**

```json
{
  "name": "session-tracker",
  "private": true,
  "version": "0.1.0",
  "type": "module",
  "scripts": {
    "dev": "vite",
    "build": "vite build",
    "preview": "vite preview",
    "test": "vitest run",
    "test:watch": "vitest"
  },
  "dependencies": {
    "pinia": "^2.2.0",
    "vue": "^3.5.0"
  },
  "devDependencies": {
    "@vitejs/plugin-vue": "^5.1.0",
    "@vue/test-utils": "^2.4.6",
    "jsdom": "^25.0.0",
    "vite": "^5.4.0",
    "vitest": "^2.1.0"
  }
}
```

- [ ] **Step 2: Créer `index.html`**

```html
<!DOCTYPE html>
<html lang="fr">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>Session Tracker</title>
    <script src="/config.js"></script>
  </head>
  <body>
    <div id="app"></div>
    <script type="module" src="/src/main.js"></script>
  </body>
</html>
```

- [ ] **Step 3: Créer `public/config.js`** (identifiants locaux, non exposés)

```js
// Config locale (machine/Pi, non exposée). Doit correspondre à docker-compose.yml.
window.APP_CONFIG = {
  couchUser: "tracker",
  couchPassword: "tracker",
  dbName: "session_tracker",
};
```

- [ ] **Step 4: Créer `vite.config.js`** (proxy `/db` → CouchDB + config Vitest)

```js
import { defineConfig } from "vite";
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
  },
});
```

- [ ] **Step 5: Créer `src/main.js`**

```js
import { createApp } from "vue";
import { createPinia } from "pinia";
import App from "./App.vue";
import "./style.css";

createApp(App).use(createPinia()).mount("#app");
```

- [ ] **Step 6: Créer `src/style.css`** (dark mode, base compacte)

```css
:root {
  --bg: #0e1116;
  --panel: #171b22;
  --border: #262c36;
  --text: #e6e9ef;
  --muted: #8b93a1;
  --green: #29c46a;
  --red: #f2495c;
  --amber: #f5a623;
  --accent: #3b82f6;
  color-scheme: dark;
}
* { box-sizing: border-box; }
body {
  margin: 0;
  background: var(--bg);
  color: var(--text);
  font: 14px/1.4 system-ui, -apple-system, Segoe UI, Roboto, sans-serif;
}
button {
  font: inherit;
  cursor: pointer;
  border: 1px solid var(--border);
  background: var(--panel);
  color: var(--text);
  border-radius: 6px;
  padding: 8px 12px;
}
button:hover { border-color: var(--accent); }
input, select {
  font: inherit;
  background: var(--bg);
  color: var(--text);
  border: 1px solid var(--border);
  border-radius: 6px;
  padding: 6px 8px;
  width: 100%;
}
.pos { color: var(--green); }
.neg { color: var(--red); }
```

- [ ] **Step 7: Créer `src/App.vue`** (placeholder pour cette tâche)

```vue
<template>
  <main class="app">
    <h1>Session Tracker</h1>
    <p class="muted">Scaffold OK.</p>
  </main>
</template>

<script setup>
</script>

<style scoped>
.app { max-width: 420px; margin: 0 auto; padding: 16px; }
.muted { color: var(--muted); }
</style>
```

- [ ] **Step 8: Créer `docker-compose.yml`**

```yaml
services:
  couchdb:
    image: couchdb:3
    restart: unless-stopped
    environment:
      COUCHDB_USER: admin
      COUCHDB_PASSWORD: admin
    ports:
      - "5984:5984"
    volumes:
      - couch_data:/opt/couchdb/data

  front:
    build: .
    restart: unless-stopped
    depends_on:
      - couchdb
    ports:
      - "8080:80"

volumes:
  couch_data:
```

- [ ] **Step 9: Créer `nginx.conf`** (sert le front + proxy `/db` → couchdb)

```nginx
server {
  listen 80;
  root /usr/share/nginx/html;
  index index.html;

  location /db/ {
    proxy_pass http://couchdb:5984/;
    proxy_set_header Host $host;
  }

  location / {
    try_files $uri $uri/ /index.html;
  }
}
```

- [ ] **Step 10: Créer `Dockerfile`** (build Vite → nginx)

```dockerfile
FROM node:20-alpine AS build
WORKDIR /app
COPY package.json ./
RUN npm install
COPY . .
RUN npm run build

FROM nginx:alpine
COPY nginx.conf /etc/nginx/conf.d/default.conf
COPY --from=build /app/dist /usr/share/nginx/html
```

- [ ] **Step 11: Créer `.dockerignore` et `.gitignore`**

`.dockerignore` :
```
node_modules
dist
.git
```

`.gitignore` :
```
node_modules
dist
```

- [ ] **Step 12: Créer le test de fumée `tests/smoke.test.js`**

```js
import { describe, it, expect } from "vitest";

describe("smoke", () => {
  it("le runner fonctionne", () => {
    expect(1 + 1).toBe(2);
  });
});
```

- [ ] **Step 13: Installer et lancer le test de fumée**

Run:
```bash
cd session-tracker && npm install && npm test
```
Expected: PASS (1 test), pas d'erreur de config Vitest.

- [ ] **Step 14: Vérifier CouchDB via le proxy dev**

Run (2 terminaux) :
```bash
cd session-tracker && docker compose up -d couchdb
cd session-tracker && npm run dev
```
Puis créer la base + le user applicatif :
```bash
curl -X PUT http://admin:admin@localhost:5984/session_tracker
curl -X PUT http://admin:admin@localhost:5984/_users/org.couchdb.user:tracker \
  -H "Content-Type: application/json" \
  -d '{"name":"tracker","password":"tracker","roles":[],"type":"user"}'
curl -X PUT http://admin:admin@localhost:5984/session_tracker/_security \
  -H "Content-Type: application/json" \
  -d '{"members":{"names":["tracker"],"roles":[]},"admins":{"names":[],"roles":[]}}'
```
Vérifier le proxy : ouvrir `http://localhost:8080/db/` → JSON CouchDB (`{"couchdb":"Welcome",...}`).
Expected: la base répond via `/db`. Documenter ces commandes dans le README (Step 15).

- [ ] **Step 15: Écrire `README.md`**

````markdown
# Session Tracker

Suivi perso de sessions de trading. Front Vue 3, données dans CouchDB (Docker).

## Lancer (prod, un seul fichier de commande)

```bash
docker compose up -d
```
Front sur http://localhost:8080. CouchDB derrière le proxy `/db`.

## Première init de la base (une fois)

```bash
curl -X PUT http://admin:admin@localhost:5984/session_tracker
curl -X PUT http://admin:admin@localhost:5984/_users/org.couchdb.user:tracker \
  -H "Content-Type: application/json" \
  -d '{"name":"tracker","password":"tracker","roles":[],"type":"user"}'
curl -X PUT http://admin:admin@localhost:5984/session_tracker/_security \
  -H "Content-Type: application/json" \
  -d '{"members":{"names":["tracker"],"roles":[]}}'
```

## Dév

```bash
docker compose up -d couchdb
npm install
npm run dev   # http://localhost:8080, proxy /db -> localhost:5984
npm test
```

## Identifiants

`public/config.js` contient l'utilisateur applicatif CouchDB (local, non exposé).
Change-les si besoin (doivent exister dans `_users`).

## Raspberry Pi

Image `couchdb:3` compatible arm64 (Pi 4/5, OS 64-bit). Les Pi armv7 ne sont pas garantis.
````

- [ ] **Step 16: Commit**

```bash
cd session-tracker
git add -A
git commit -m "feat(session-tracker): scaffold Vite+Vue+Pinia, Docker CouchDB, proxy /db"
```

---

## Task 2: Bibliothèque de métriques pures (`lib/metrics.js`)

Le cœur logique, 100 % testable. Fonctions pures, aucune dépendance.

**Files:**
- Create: `session-tracker/src/lib/metrics.js`
- Test: `session-tracker/tests/metrics.test.js`

**Interfaces:**
- Consumes: rien.
- Produces (signatures exactes, réutilisées par le store et l'UI) :
  - `sumPnl(trades) -> number`
  - `losingCount(trades) -> number` (trades dont `pnl < 0`)
  - `winRate(trades) -> number | null` (proportion 0..1 de trades `pnl > 0`, `null` si aucun trade)
  - `bestTrade(trades) -> trade | null`
  - `worstTrade(trades) -> trade | null`
  - `lossLimitPct(trades, lossLimit) -> number` (0..∞, fraction de la loss limit consommée par la perte nette)
  - `gainTargetPct(trades, gainTarget) -> number` (0..∞, fraction de l'objectif atteinte par le gain net)
  - `stopState(trades, params) -> { stopped: boolean, reasons: string[], lossWarning: boolean }`
    - `reasons` ⊂ `["lossLimit", "maxTrades", "maxLossTrades"]`
    - `lossWarning` vrai si `lossLimitPct >= 0.8` (et non encore stoppé pour loss)
  - `formatDuration(ms) -> string` (format `"HH:MM:SS"`, zéro-paddé)

  Un `trade` = `{ pnl: number, ... }` (les autres champs sont ignorés ici).
  `params` = `{ lossLimit, gainTarget, maxTrades, maxLossTrades }`.

- [ ] **Step 1: Écrire les tests `tests/metrics.test.js`**

```js
import { describe, it, expect } from "vitest";
import {
  sumPnl, losingCount, winRate, bestTrade, worstTrade,
  lossLimitPct, gainTargetPct, stopState, formatDuration,
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
});

describe("formatDuration", () => {
  it("formate en HH:MM:SS", () => expect(formatDuration(3661000)).toBe("01:01:01"));
  it("gère 0", () => expect(formatDuration(0)).toBe("00:00:00"));
  it("clampe le négatif à 0", () => expect(formatDuration(-5000)).toBe("00:00:00"));
});
```

- [ ] **Step 2: Lancer les tests → échec attendu**

Run: `cd session-tracker && npx vitest run tests/metrics.test.js`
Expected: FAIL (module `metrics.js` introuvable).

- [ ] **Step 3: Implémenter `src/lib/metrics.js`**

```js
export function sumPnl(trades) {
  return trades.reduce((acc, t) => acc + t.pnl, 0);
}

export function losingCount(trades) {
  return trades.filter((t) => t.pnl < 0).length;
}

export function winRate(trades) {
  if (trades.length === 0) return null;
  const wins = trades.filter((t) => t.pnl > 0).length;
  return wins / trades.length;
}

export function bestTrade(trades) {
  if (trades.length === 0) return null;
  return trades.reduce((b, t) => (t.pnl > b.pnl ? t : b));
}

export function worstTrade(trades) {
  if (trades.length === 0) return null;
  return trades.reduce((w, t) => (t.pnl < w.pnl ? t : w));
}

export function lossLimitPct(trades, lossLimit) {
  if (!lossLimit || lossLimit <= 0) return 0;
  const consumed = Math.max(0, -sumPnl(trades));
  return consumed / lossLimit;
}

export function gainTargetPct(trades, gainTarget) {
  if (!gainTarget || gainTarget <= 0) return 0;
  const gain = Math.max(0, sumPnl(trades));
  return gain / gainTarget;
}

export function stopState(trades, params) {
  const reasons = [];
  const lossPct = lossLimitPct(trades, params.lossLimit);
  if (lossPct >= 1) reasons.push("lossLimit");
  if (params.maxTrades > 0 && trades.length >= params.maxTrades) reasons.push("maxTrades");
  if (params.maxLossTrades > 0 && losingCount(trades) >= params.maxLossTrades)
    reasons.push("maxLossTrades");
  const stopped = reasons.length > 0;
  return {
    stopped,
    reasons,
    lossWarning: !reasons.includes("lossLimit") && lossPct >= 0.8,
  };
}

export function formatDuration(ms) {
  const total = Math.max(0, Math.floor(ms / 1000));
  const h = Math.floor(total / 3600);
  const m = Math.floor((total % 3600) / 60);
  const s = total % 60;
  const pad = (n) => String(n).padStart(2, "0");
  return `${pad(h)}:${pad(m)}:${pad(s)}`;
}
```

- [ ] **Step 4: Lancer les tests → succès attendu**

Run: `cd session-tracker && npx vitest run tests/metrics.test.js`
Expected: PASS (tous les tests métriques).

- [ ] **Step 5: Commit**

```bash
cd session-tracker
git add src/lib/metrics.js tests/metrics.test.js
git commit -m "feat(session-tracker): métriques pures (pnl, winrate, jauges, états STOP, durée)"
```

---

## Task 3: Client CouchDB REST (`db.js`)

**Files:**
- Create: `session-tracker/src/db.js`
- Test: `session-tracker/tests/db.test.js`

**Interfaces:**
- Consumes: `window.APP_CONFIG` (`couchUser`, `couchPassword`, `dbName`).
- Produces (async, base d'URL `/db`) :
  - `login() -> Promise<void>` (POST `/db/_session` avec les identifiants de `APP_CONFIG`)
  - `createDoc(doc) -> Promise<{ id, rev }>` (POST `/db/{dbName}`)
  - `getDoc(id) -> Promise<doc | null>` (GET ; `null` si 404)
  - `updateDoc(doc) -> Promise<{ id, rev }>` (PUT `/db/{dbName}/{_id}`, requiert `_rev`)
  - `deleteDoc(id, rev) -> Promise<void>` (DELETE)
  - `find(selector, opts?) -> Promise<doc[]>` (POST `/db/{dbName}/_find`, renvoie `docs`)

  Toutes les requêtes utilisent `credentials: "same-origin"`. En cas de statut non-OK (hors 404 sur `getDoc`), lever une `Error` avec le statut.

- [ ] **Step 1: Écrire les tests `tests/db.test.js`** (fetch mocké)

```js
import { describe, it, expect, vi, beforeEach } from "vitest";
import { createDoc, getDoc, updateDoc, find, login } from "../src/db.js";

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

describe("find", () => {
  it("POST _find et renvoie docs", async () => {
    mockFetchOnce({ json: { docs: [{ _id: "a" }, { _id: "b" }] } });
    const docs = await find({ type: "trade" });
    expect(fetch.mock.calls[0][0]).toBe("/db/testdb/_find");
    expect(docs).toHaveLength(2);
  });
});
```

- [ ] **Step 2: Lancer les tests → échec attendu**

Run: `cd session-tracker && npx vitest run tests/db.test.js`
Expected: FAIL (module `db.js` introuvable).

- [ ] **Step 3: Implémenter `src/db.js`**

```js
const cfg = () => window.APP_CONFIG;
const base = () => `/db/${cfg().dbName}`;

async function req(url, options = {}) {
  const res = await fetch(url, {
    credentials: "same-origin",
    headers: { "Content-Type": "application/json" },
    ...options,
  });
  return res;
}

export async function login() {
  const res = await req("/db/_session", {
    method: "POST",
    body: JSON.stringify({ name: cfg().couchUser, password: cfg().couchPassword }),
  });
  if (!res.ok) throw new Error(`login CouchDB: ${res.status}`);
}

export async function createDoc(doc) {
  const res = await req(base(), { method: "POST", body: JSON.stringify(doc) });
  if (!res.ok) throw new Error(`createDoc: ${res.status}`);
  return res.json();
}

export async function getDoc(id) {
  const res = await req(`${base()}/${id}`);
  if (res.status === 404) return null;
  if (!res.ok) throw new Error(`getDoc: ${res.status}`);
  return res.json();
}

export async function updateDoc(doc) {
  const res = await req(`${base()}/${doc._id}`, {
    method: "PUT",
    body: JSON.stringify(doc),
  });
  if (!res.ok) throw new Error(`updateDoc: ${res.status}`);
  return res.json();
}

export async function deleteDoc(id, rev) {
  const res = await req(`${base()}/${id}?rev=${rev}`, { method: "DELETE" });
  if (!res.ok) throw new Error(`deleteDoc: ${res.status}`);
}

export async function find(selector, opts = {}) {
  const res = await req(`${base()}/_find`, {
    method: "POST",
    body: JSON.stringify({ selector, limit: 10000, ...opts }),
  });
  if (!res.ok) throw new Error(`find: ${res.status}`);
  const data = await res.json();
  return data.docs;
}
```

- [ ] **Step 4: Lancer les tests → succès attendu**

Run: `cd session-tracker && npx vitest run tests/db.test.js`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
cd session-tracker
git add src/db.js tests/db.test.js
git commit -m "feat(session-tracker): client REST CouchDB (login, CRUD, _find)"
```

---

## Task 4: Stores Pinia (settings + session)

**Files:**
- Create: `session-tracker/src/stores/settings.js`
- Create: `session-tracker/src/stores/session.js`
- Test: `session-tracker/tests/settings.store.test.js`
- Test: `session-tracker/tests/session.store.test.js`

**Interfaces:**
- Consumes: `db.js` (`getDoc`, `createDoc`, `updateDoc`, `find`), `lib/metrics.js`.
- Produces :
  - `useSettingsStore` : state `settings` (`{ lossLimit, gainTarget, maxTrades, maxLossTrades, currency }`) ; actions `load()`, `save(newSettings)`. Doc `_id: "settings"`. Défauts si absent : `{ lossLimit: 500, gainTarget: 500, maxTrades: 5, maxLossTrades: 3, currency: "EUR" }`.
  - `useSessionStore` : state `current` (doc session `open` ou `null`), `trades` (array) ; getters `pnl`, `winRate`, `best`, `worst`, `lossPct`, `gainPct`, `stop`, `losingCount` (via metrics) ; actions `loadOpen()`, `start(params)`, `close()`, `addTrade(data)`.

- [ ] **Step 1: Écrire `tests/settings.store.test.js`** (db mocké)

```js
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
});
```

- [ ] **Step 2: Écrire `tests/session.store.test.js`**

```js
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
});
```

- [ ] **Step 3: Lancer les tests → échec attendu**

Run: `cd session-tracker && npx vitest run tests/settings.store.test.js tests/session.store.test.js`
Expected: FAIL (stores introuvables).

- [ ] **Step 4: Implémenter `src/stores/settings.js`**

```js
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
```

- [ ] **Step 5: Implémenter `src/stores/session.js`**

```js
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
      };
      const res = await createDoc(doc);
      this.trades.push({ ...doc, _id: res.id, _rev: res.rev });
    },
  },
});
```

- [ ] **Step 6: Lancer les tests → succès attendu**

Run: `cd session-tracker && npx vitest run tests/settings.store.test.js tests/session.store.test.js`
Expected: PASS.

- [ ] **Step 7: Commit**

```bash
cd session-tracker
git add src/stores tests/settings.store.test.js tests/session.store.test.js
git commit -m "feat(session-tracker): stores Pinia settings + session (métriques dérivées)"
```

---

## Task 5: Composants d'affichage réutilisables (Gauge, StopBanner) + timer

**Files:**
- Create: `session-tracker/src/composables/useTimer.js`
- Create: `session-tracker/src/components/Gauge.vue`
- Create: `session-tracker/src/components/StopBanner.vue`
- Test: `session-tracker/tests/gauge.test.js`

**Interfaces:**
- Consumes: `lib/metrics.js` (`formatDuration`).
- Produces :
  - `useTimer()` -> `{ now }` (ref réactive mise à jour chaque seconde, nettoyée à l'unmount).
  - `Gauge.vue` props : `label: string`, `value: number`, `max: number`, `variant: "loss" | "gain" | "neutral"`, `warn?: boolean`. Affiche une barre remplie à `clamp(value/max, 0, 1)`, colorée selon `variant` (loss=rouge, gain=vert), passe en ambre si `warn`.
  - `StopBanner.vue` props : `reasons: string[]`. Affiche « STOP TRADING » + la liste lisible des raisons. Rien si `reasons` vide.

- [ ] **Step 1: Écrire `tests/gauge.test.js`**

```js
import { describe, it, expect } from "vitest";
import { mount } from "@vue/test-utils";
import Gauge from "../src/components/Gauge.vue";
import StopBanner from "../src/components/StopBanner.vue";

describe("Gauge", () => {
  it("remplit à la bonne fraction (clampée)", () => {
    const w = mount(Gauge, { props: { label: "Loss", value: 40, max: 100, variant: "loss" } });
    expect(w.find(".fill").attributes("style")).toContain("40%");
  });
  it("clampe au-dessus de 100%", () => {
    const w = mount(Gauge, { props: { label: "Loss", value: 150, max: 100, variant: "loss" } });
    expect(w.find(".fill").attributes("style")).toContain("100%");
  });
});

describe("StopBanner", () => {
  it("n'affiche rien sans raison", () => {
    const w = mount(StopBanner, { props: { reasons: [] } });
    expect(w.text()).toBe("");
  });
  it("affiche STOP TRADING avec des raisons", () => {
    const w = mount(StopBanner, { props: { reasons: ["lossLimit"] } });
    expect(w.text()).toContain("STOP TRADING");
  });
});
```

- [ ] **Step 2: Lancer → échec attendu**

Run: `cd session-tracker && npx vitest run tests/gauge.test.js`
Expected: FAIL (composants introuvables).

- [ ] **Step 3: Implémenter `src/composables/useTimer.js`**

```js
import { ref, onMounted, onUnmounted } from "vue";

export function useTimer() {
  const now = ref(Date.now());
  let id = null;
  onMounted(() => {
    id = setInterval(() => { now.value = Date.now(); }, 1000);
  });
  onUnmounted(() => { if (id) clearInterval(id); });
  return { now };
}
```

- [ ] **Step 4: Implémenter `src/components/Gauge.vue`**

```vue
<template>
  <div class="gauge">
    <div class="row">
      <span class="label">{{ label }}</span>
      <span class="pct">{{ Math.round(pct * 100) }}%</span>
    </div>
    <div class="track">
      <div class="fill" :class="color" :style="{ width: pct * 100 + '%' }"></div>
    </div>
  </div>
</template>

<script setup>
import { computed } from "vue";
const props = defineProps({
  label: String,
  value: Number,
  max: Number,
  variant: { type: String, default: "neutral" },
  warn: { type: Boolean, default: false },
});
const pct = computed(() => {
  if (!props.max || props.max <= 0) return 0;
  return Math.min(1, Math.max(0, props.value / props.max));
});
const color = computed(() => (props.warn ? "amber" : props.variant));
</script>

<style scoped>
.gauge { margin: 8px 0; }
.row { display: flex; justify-content: space-between; font-size: 12px; color: var(--muted); }
.track { height: 10px; background: var(--bg); border: 1px solid var(--border); border-radius: 6px; overflow: hidden; }
.fill { height: 100%; transition: width .3s; }
.fill.loss { background: var(--red); }
.fill.gain { background: var(--green); }
.fill.neutral { background: var(--accent); }
.fill.amber { background: var(--amber); }
</style>
```

- [ ] **Step 5: Implémenter `src/components/StopBanner.vue`**

```vue
<template>
  <div v-if="reasons.length" class="banner">
    <strong>⛔ STOP TRADING</strong>
    <span class="why">{{ label }}</span>
  </div>
</template>

<script setup>
import { computed } from "vue";
const props = defineProps({ reasons: { type: Array, default: () => [] } });
const LABELS = {
  lossLimit: "perte max atteinte",
  maxTrades: "nombre max de trades atteint",
  maxLossTrades: "nombre max de trades perdants atteint",
};
const label = computed(() => props.reasons.map((r) => LABELS[r] || r).join(" · "));
</script>

<style scoped>
.banner {
  background: var(--red); color: #fff; padding: 10px 12px; border-radius: 8px;
  display: flex; flex-direction: column; gap: 2px; margin-bottom: 12px; text-align: center;
}
.why { font-size: 12px; opacity: .9; }
</style>
```

- [ ] **Step 6: Lancer → succès attendu**

Run: `cd session-tracker && npx vitest run tests/gauge.test.js`
Expected: PASS.

- [ ] **Step 7: Commit**

```bash
cd session-tracker
git add src/composables src/components/Gauge.vue src/components/StopBanner.vue tests/gauge.test.js
git commit -m "feat(session-tracker): Gauge, StopBanner, useTimer"
```

---

## Task 6: Écran pré-session (`StartSession.vue`) + saisie trade (`TradeForm.vue`)

**Files:**
- Create: `session-tracker/src/components/StartSession.vue`
- Create: `session-tracker/src/components/TradeForm.vue`

**Interfaces:**
- Consumes: `useSettingsStore` (défauts pré-remplis), `useSessionStore` (`start`, `addTrade`).
- Produces :
  - `StartSession.vue` émet `start(params)` avec `{ lossLimit, gainTarget, maxTrades, maxLossTrades, currency }`. Champs pré-remplis depuis les settings. Focus auto sur le premier champ. Validation `Entrée` (submit du formulaire).
  - `TradeForm.vue` émet `added` après enregistrement. Champs : ticker (focus auto), side (long/short), pnl (nombre), rMultiple (optionnel), note (courte), tags (CSV → array). `Entrée` = soumettre. Reset + refocus ticker après ajout (< 10 s pour le trade suivant). Désactivé si `disabled` (état STOP) mais la saisie reste possible (le STOP est indicatif) — afficher un avertissement visuel si `disabled`.

- [ ] **Step 1: Implémenter `src/components/StartSession.vue`**

```vue
<template>
  <form class="card" @submit.prevent="submit">
    <h2>Nouvelle session</h2>
    <label>Perte max du jour ({{ form.currency }})
      <input ref="first" type="number" step="any" v-model.number="form.lossLimit" />
    </label>
    <label>Objectif de gain ({{ form.currency }})
      <input type="number" step="any" v-model.number="form.gainTarget" />
    </label>
    <label>Max trades
      <input type="number" min="0" v-model.number="form.maxTrades" />
    </label>
    <label>Max trades perdants
      <input type="number" min="0" v-model.number="form.maxLossTrades" />
    </label>
    <label>Devise
      <select v-model="form.currency">
        <option value="EUR">EUR (€)</option>
        <option value="USD">USD ($)</option>
      </select>
    </label>
    <button type="submit" class="go">▶ Démarrer la session</button>
  </form>
</template>

<script setup>
import { reactive, ref, onMounted } from "vue";
import { useSettingsStore } from "../stores/settings.js";

const emit = defineEmits(["start"]);
const settings = useSettingsStore();
const first = ref(null);
const form = reactive({ ...settings.settings });

onMounted(() => first.value?.focus());

function submit() {
  emit("start", { ...form });
}
</script>

<style scoped>
.card { background: var(--panel); border: 1px solid var(--border); border-radius: 10px; padding: 16px; display: flex; flex-direction: column; gap: 10px; }
label { display: flex; flex-direction: column; gap: 4px; font-size: 12px; color: var(--muted); }
.go { background: var(--accent); border-color: var(--accent); color: #fff; margin-top: 6px; }
</style>
```

- [ ] **Step 2: Implémenter `src/components/TradeForm.vue`**

```vue
<template>
  <form class="card" @submit.prevent="submit">
    <div class="grid">
      <input ref="tickerEl" v-model="f.ticker" placeholder="Ticker (ex. NQ)" />
      <div class="side">
        <button type="button" :class="{ on: f.side === 'long' }" @click="f.side = 'long'">Long</button>
        <button type="button" :class="{ on: f.side === 'short' }" @click="f.side = 'short'">Short</button>
      </div>
      <input v-model.number="f.pnl" type="number" step="any" placeholder="Résultat (€/$)" />
      <input v-model.number="f.rMultiple" type="number" step="any" placeholder="R (optionnel)" />
    </div>
    <input v-model="f.note" placeholder="Note (setup, erreur, émotion)" />
    <input v-model="f.tagsCsv" placeholder="Tags (séparés par des virgules)" />
    <button type="submit" class="add">＋ Ajouter le trade (Entrée)</button>
    <p v-if="disabled" class="warn">⛔ Limite atteinte — trade quand même possible.</p>
  </form>
</template>

<script setup>
import { reactive, ref, onMounted } from "vue";
import { useSessionStore } from "../stores/session.js";

const props = defineProps({ disabled: { type: Boolean, default: false } });
const emit = defineEmits(["added"]);
const session = useSessionStore();
const tickerEl = ref(null);
const empty = () => ({ ticker: "", side: "long", pnl: null, rMultiple: null, note: "", tagsCsv: "" });
const f = reactive(empty());

onMounted(() => tickerEl.value?.focus());

async function submit() {
  if (f.ticker === "" || f.pnl === null || Number.isNaN(f.pnl)) return;
  await session.addTrade({
    ticker: f.ticker.trim().toUpperCase(),
    side: f.side,
    pnl: f.pnl,
    rMultiple: f.rMultiple,
    note: f.note.trim(),
    tags: f.tagsCsv.split(",").map((t) => t.trim()).filter(Boolean),
  });
  Object.assign(f, empty());
  tickerEl.value?.focus();
  emit("added");
}
</script>

<style scoped>
.card { background: var(--panel); border: 1px solid var(--border); border-radius: 10px; padding: 12px; display: flex; flex-direction: column; gap: 8px; }
.grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; }
.side { display: flex; gap: 6px; }
.side button { flex: 1; }
.side button.on { background: var(--accent); border-color: var(--accent); color: #fff; }
.add { background: var(--green); border-color: var(--green); color: #06210f; font-weight: 600; }
.warn { color: var(--amber); font-size: 12px; margin: 0; text-align: center; }
</style>
```

- [ ] **Step 3: Vérifier le build (pas de test unitaire ici, composants pilotés par l'app)**

Run: `cd session-tracker && npx vitest run` (les tests existants passent toujours)
Expected: PASS (aucune régression).

- [ ] **Step 4: Commit**

```bash
cd session-tracker
git add src/components/StartSession.vue src/components/TradeForm.vue
git commit -m "feat(session-tracker): écrans pré-session et saisie rapide de trade"
```

---

## Task 7: Dashboard live (`LiveDashboard.vue`)

**Files:**
- Create: `session-tracker/src/components/LiveDashboard.vue`

**Interfaces:**
- Consumes: `useSessionStore` (getters + `close`), `useTimer`, `lib/metrics.js` (`formatDuration`), `Gauge.vue`, `StopBanner.vue`, `TradeForm.vue`.
- Produces : dashboard de la session ouverte. Émet `closed` après clôture.

- [ ] **Step 1: Implémenter `src/components/LiveDashboard.vue`**

```vue
<template>
  <section class="dash">
    <header class="top">
      <span class="timer">⏱ {{ duration }}</span>
      <button class="close" @click="closeSession">Clôturer</button>
    </header>

    <StopBanner :reasons="s.stop.reasons" />

    <div class="pnl" :class="s.pnl >= 0 ? 'pos' : 'neg'">
      {{ s.pnl >= 0 ? "+" : "" }}{{ s.pnl.toFixed(2) }} {{ sym }}
    </div>

    <Gauge label="Perte max" :value="lossConsumed" :max="p.lossLimit" variant="loss" :warn="s.stop.lossWarning" />
    <Gauge label="Objectif de gain" :value="Math.max(0, s.pnl)" :max="p.gainTarget" variant="gain" />

    <div class="counters">
      <div class="c">
        <span class="k">Trades</span>
        <span class="v" :class="{ neg: s.trades.length >= p.maxTrades }">{{ s.trades.length }} / {{ p.maxTrades }}</span>
      </div>
      <div class="c">
        <span class="k">Perdants</span>
        <span class="v" :class="{ neg: s.losingCount >= p.maxLossTrades }">{{ s.losingCount }} / {{ p.maxLossTrades }}</span>
      </div>
      <div class="c">
        <span class="k">Win rate</span>
        <span class="v">{{ s.winRate === null ? "—" : Math.round(s.winRate * 100) + "%" }}</span>
      </div>
    </div>

    <div class="extremes" v-if="s.trades.length">
      <span>Meilleur : <b class="pos">+{{ s.best.pnl.toFixed(2) }}</b></span>
      <span>Pire : <b class="neg">{{ s.worst.pnl.toFixed(2) }}</b></span>
    </div>

    <TradeForm :disabled="s.stop.stopped" @added="() => {}" />

    <ul class="list">
      <li v-for="t in reversed" :key="t._id">
        <span>{{ t.side === 'long' ? '▲' : '▼' }} {{ t.ticker }}</span>
        <span :class="t.pnl >= 0 ? 'pos' : 'neg'">{{ t.pnl >= 0 ? '+' : '' }}{{ t.pnl.toFixed(2) }}</span>
      </li>
    </ul>
  </section>
</template>

<script setup>
import { computed } from "vue";
import { useSessionStore } from "../stores/session.js";
import { useTimer } from "../composables/useTimer.js";
import { formatDuration } from "../lib/metrics.js";
import Gauge from "./Gauge.vue";
import StopBanner from "./StopBanner.vue";
import TradeForm from "./TradeForm.vue";

const emit = defineEmits(["closed"]);
const s = useSessionStore();
const { now } = useTimer();

const p = computed(() => s.current.params);
const sym = computed(() => (p.value.currency === "USD" ? "$" : "€"));
const lossConsumed = computed(() => Math.max(0, -s.pnl));
const duration = computed(() => formatDuration(now.value - new Date(s.current.startedAt).getTime()));
const reversed = computed(() => [...s.trades].reverse());

async function closeSession() {
  await s.close();
  emit("closed");
}
</script>

<style scoped>
.dash { display: flex; flex-direction: column; gap: 10px; }
.top { display: flex; justify-content: space-between; align-items: center; }
.timer { font-variant-numeric: tabular-nums; color: var(--muted); }
.close { border-color: var(--red); color: var(--red); }
.pnl { font-size: 42px; font-weight: 700; text-align: center; font-variant-numeric: tabular-nums; }
.counters { display: flex; gap: 8px; }
.c { flex: 1; background: var(--panel); border: 1px solid var(--border); border-radius: 8px; padding: 8px; text-align: center; }
.c .k { display: block; font-size: 11px; color: var(--muted); }
.c .v { font-size: 16px; font-weight: 600; font-variant-numeric: tabular-nums; }
.extremes { display: flex; justify-content: space-between; font-size: 12px; color: var(--muted); }
.list { list-style: none; padding: 0; margin: 4px 0 0; display: flex; flex-direction: column; gap: 4px; }
.list li { display: flex; justify-content: space-between; background: var(--panel); border: 1px solid var(--border); border-radius: 6px; padding: 6px 10px; font-variant-numeric: tabular-nums; }
</style>
```

- [ ] **Step 2: Vérifier l'absence de régression**

Run: `cd session-tracker && npx vitest run`
Expected: PASS.

- [ ] **Step 3: Commit**

```bash
cd session-tracker
git add src/components/LiveDashboard.vue
git commit -m "feat(session-tracker): dashboard live (timer, P&L, jauges, compteurs)"
```

---

## Task 8: Historique + calendrier P&L (`SessionHistory.vue`, `PnlCalendar.vue`)

**Files:**
- Create: `session-tracker/src/lib/history.js`
- Create: `session-tracker/src/components/PnlCalendar.vue`
- Create: `session-tracker/src/components/SessionHistory.vue`
- Test: `session-tracker/tests/history.test.js`

**Interfaces:**
- Consumes: `db.js` (`find`), `lib/metrics.js`.
- Produces :
  - `history.js` : `sessionSummaries(sessions, tradesBySession) -> [{ id, startedAt, pnl, count, winRate }]` et `pnlByDay(sessions, trades) -> Map<"YYYY-MM-DD", number>` (P&L agrégé par jour, basé sur `startedAt` de la session du trade).
  - `PnlCalendar.vue` props : `pnlByDay: Map`, `year: number`, `month: number` (0-11). Grille du mois, case verte si P&L>0, rouge si <0, neutre sinon.
  - `SessionHistory.vue` : charge sessions closes + trades, affiche le calendrier du mois courant + la liste des sessions.

- [ ] **Step 1: Écrire `tests/history.test.js`**

```js
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
```

- [ ] **Step 2: Lancer → échec attendu**

Run: `cd session-tracker && npx vitest run tests/history.test.js`
Expected: FAIL.

- [ ] **Step 3: Implémenter `src/lib/history.js`**

```js
import { sumPnl, winRate } from "./metrics.js";

export function sessionSummaries(sessions, tradesBySession) {
  return sessions.map((s) => {
    const ts = tradesBySession[s._id] || [];
    return {
      id: s._id,
      startedAt: s.startedAt,
      pnl: sumPnl(ts),
      count: ts.length,
      winRate: winRate(ts),
    };
  });
}

export function pnlByDay(sessions, trades) {
  const dayOf = {};
  for (const s of sessions) dayOf[s._id] = s.startedAt.slice(0, 10);
  const map = new Map();
  for (const t of trades) {
    const day = dayOf[t.sessionId];
    if (!day) continue;
    map.set(day, (map.get(day) || 0) + t.pnl);
  }
  return map;
}
```

- [ ] **Step 4: Lancer → succès attendu**

Run: `cd session-tracker && npx vitest run tests/history.test.js`
Expected: PASS.

- [ ] **Step 5: Implémenter `src/components/PnlCalendar.vue`**

```vue
<template>
  <div class="cal">
    <div class="head">{{ monthLabel }}</div>
    <div class="grid">
      <span v-for="d in ['L','M','M','J','V','S','D']" :key="d" class="dow">{{ d }}</span>
      <span v-for="n in offset" :key="'b' + n" class="cell empty"></span>
      <span
        v-for="day in daysInMonth"
        :key="day"
        class="cell"
        :class="cls(day)"
        :title="titleFor(day)"
      >{{ day }}</span>
    </div>
  </div>
</template>

<script setup>
import { computed } from "vue";
const props = defineProps({ pnlByDay: { type: Map, required: true }, year: Number, month: Number });

const MONTHS = ["janvier","février","mars","avril","mai","juin","juillet","août","septembre","octobre","novembre","décembre"];
const monthLabel = computed(() => `${MONTHS[props.month]} ${props.year}`);
const daysInMonth = computed(() => new Date(props.year, props.month + 1, 0).getDate());
const offset = computed(() => {
  const jsDay = new Date(props.year, props.month, 1).getDay(); // 0=dim
  return (jsDay + 6) % 7; // lundi = 0
});
function key(day) {
  const mm = String(props.month + 1).padStart(2, "0");
  const dd = String(day).padStart(2, "0");
  return `${props.year}-${mm}-${dd}`;
}
function cls(day) {
  const v = props.pnlByDay.get(key(day));
  if (v === undefined) return "flat";
  return v > 0 ? "win" : v < 0 ? "lose" : "flat";
}
function titleFor(day) {
  const v = props.pnlByDay.get(key(day));
  return v === undefined ? "" : `${v >= 0 ? "+" : ""}${v.toFixed(2)}`;
}
</script>

<style scoped>
.cal { background: var(--panel); border: 1px solid var(--border); border-radius: 10px; padding: 12px; }
.head { text-align: center; margin-bottom: 8px; text-transform: capitalize; }
.grid { display: grid; grid-template-columns: repeat(7, 1fr); gap: 4px; }
.dow { text-align: center; font-size: 11px; color: var(--muted); }
.cell { aspect-ratio: 1; display: flex; align-items: center; justify-content: center; border-radius: 4px; font-size: 12px; background: var(--bg); border: 1px solid var(--border); }
.cell.empty { background: transparent; border: none; }
.cell.win { background: rgba(41,196,106,.25); border-color: var(--green); }
.cell.lose { background: rgba(242,73,92,.25); border-color: var(--red); }
</style>
```

- [ ] **Step 6: Implémenter `src/components/SessionHistory.vue`**

```vue
<template>
  <section class="hist">
    <PnlCalendar :pnl-by-day="byDay" :year="year" :month="month" />
    <h3>Sessions</h3>
    <ul class="list">
      <li v-for="r in summaries" :key="r.id">
        <span class="date">{{ fmt(r.startedAt) }}</span>
        <span :class="r.pnl >= 0 ? 'pos' : 'neg'">{{ r.pnl >= 0 ? '+' : '' }}{{ r.pnl.toFixed(2) }}</span>
        <span class="meta">{{ r.count }} trades · {{ r.winRate === null ? '—' : Math.round(r.winRate * 100) + '%' }}</span>
      </li>
    </ul>
    <p v-if="!summaries.length" class="muted">Aucune session enregistrée.</p>
  </section>
</template>

<script setup>
import { ref, computed, onMounted } from "vue";
import { find } from "../db.js";
import { sessionSummaries, pnlByDay } from "../lib/history.js";
import PnlCalendar from "./PnlCalendar.vue";

const sessions = ref([]);
const trades = ref([]);
const nowDate = new Date();
const year = nowDate.getFullYear();
const month = nowDate.getMonth();

onMounted(async () => {
  sessions.value = await find({ type: "session" });
  trades.value = await find({ type: "trade" });
});

const byDay = computed(() => pnlByDay(sessions.value, trades.value));
const summaries = computed(() => {
  const byS = {};
  for (const t of trades.value) (byS[t.sessionId] ||= []).push(t);
  return sessionSummaries(sessions.value, byS)
    .sort((a, b) => b.startedAt.localeCompare(a.startedAt));
});

function fmt(iso) {
  return new Date(iso).toLocaleString("fr-FR", { dateStyle: "short", timeStyle: "short" });
}
</script>

<style scoped>
.hist { display: flex; flex-direction: column; gap: 12px; }
.list { list-style: none; padding: 0; margin: 0; display: flex; flex-direction: column; gap: 6px; }
.list li { display: grid; grid-template-columns: 1fr auto; gap: 2px 8px; background: var(--panel); border: 1px solid var(--border); border-radius: 8px; padding: 8px 10px; }
.date { font-size: 13px; }
.meta { grid-column: 1 / -1; font-size: 11px; color: var(--muted); }
.muted { color: var(--muted); }
</style>
```

- [ ] **Step 7: Lancer tous les tests**

Run: `cd session-tracker && npx vitest run`
Expected: PASS (aucune régression).

- [ ] **Step 8: Commit**

```bash
cd session-tracker
git add src/lib/history.js src/components/PnlCalendar.vue src/components/SessionHistory.vue tests/history.test.js
git commit -m "feat(session-tracker): historique des sessions + calendrier P&L"
```

---

## Task 9: Paramètres (`SettingsView.vue`) + App shell + navigation

**Files:**
- Create: `session-tracker/src/components/SettingsView.vue`
- Modify: `session-tracker/src/App.vue` (remplace le placeholder du Task 1)

**Interfaces:**
- Consumes: tous les stores + composants ; `db.js` (`login`).
- Produces : `App.vue` = shell final. Au montage : `login()`, charge settings + session ouverte. Navigation entre 3 vues : **Session** (LiveDashboard si session ouverte, sinon StartSession), **Historique** (SessionHistory), **Paramètres** (SettingsView).

- [ ] **Step 1: Implémenter `src/components/SettingsView.vue`**

```vue
<template>
  <form class="card" @submit.prevent="save">
    <h2>Paramètres par défaut</h2>
    <label>Perte max du jour
      <input type="number" step="any" v-model.number="f.lossLimit" />
    </label>
    <label>Objectif de gain
      <input type="number" step="any" v-model.number="f.gainTarget" />
    </label>
    <label>Max trades
      <input type="number" min="0" v-model.number="f.maxTrades" />
    </label>
    <label>Max trades perdants
      <input type="number" min="0" v-model.number="f.maxLossTrades" />
    </label>
    <label>Devise
      <select v-model="f.currency">
        <option value="EUR">EUR (€)</option>
        <option value="USD">USD ($)</option>
      </select>
    </label>
    <button type="submit" class="save">Enregistrer</button>
    <p v-if="saved" class="ok">Enregistré ✓</p>
  </form>
</template>

<script setup>
import { reactive, ref } from "vue";
import { useSettingsStore } from "../stores/settings.js";

const settings = useSettingsStore();
const f = reactive({ ...settings.settings });
const saved = ref(false);

async function save() {
  await settings.save({ ...f });
  saved.value = true;
  setTimeout(() => (saved.value = false), 1500);
}
</script>

<style scoped>
.card { background: var(--panel); border: 1px solid var(--border); border-radius: 10px; padding: 16px; display: flex; flex-direction: column; gap: 10px; }
label { display: flex; flex-direction: column; gap: 4px; font-size: 12px; color: var(--muted); }
.save { background: var(--accent); border-color: var(--accent); color: #fff; }
.ok { color: var(--green); font-size: 12px; margin: 0; }
</style>
```

- [ ] **Step 2: Réécrire `src/App.vue`**

```vue
<template>
  <main class="app">
    <nav class="tabs">
      <button :class="{ on: view === 'session' }" @click="view = 'session'">Session</button>
      <button :class="{ on: view === 'history' }" @click="view = 'history'">Historique</button>
      <button :class="{ on: view === 'settings' }" @click="view = 'settings'">Paramètres</button>
    </nav>

    <p v-if="error" class="err">{{ error }}</p>

    <template v-if="!loading">
      <template v-if="view === 'session'">
        <LiveDashboard v-if="session.current" @closed="onClosed" />
        <StartSession v-else @start="onStart" />
      </template>
      <SessionHistory v-else-if="view === 'history'" :key="historyKey" />
      <SettingsView v-else />
    </template>
    <p v-else class="muted">Connexion à la base…</p>
  </main>
</template>

<script setup>
import { ref, onMounted } from "vue";
import { login } from "./db.js";
import { useSettingsStore } from "./stores/settings.js";
import { useSessionStore } from "./stores/session.js";
import StartSession from "./components/StartSession.vue";
import LiveDashboard from "./components/LiveDashboard.vue";
import SessionHistory from "./components/SessionHistory.vue";
import SettingsView from "./components/SettingsView.vue";

const view = ref("session");
const loading = ref(true);
const error = ref("");
const historyKey = ref(0);
const settings = useSettingsStore();
const session = useSessionStore();

onMounted(async () => {
  try {
    await login();
    await settings.load();
    await session.loadOpen();
  } catch (e) {
    error.value = "Impossible de joindre CouchDB. Est-il lancé ? (" + e.message + ")";
  } finally {
    loading.value = false;
  }
});

async function onStart(params) {
  await session.start(params);
}
function onClosed() {
  historyKey.value++; // force le rechargement de l'historique la prochaine fois
}
</script>

<style scoped>
.app { max-width: 420px; margin: 0 auto; padding: 12px; display: flex; flex-direction: column; gap: 12px; }
.tabs { display: flex; gap: 6px; }
.tabs button { flex: 1; }
.tabs button.on { background: var(--accent); border-color: var(--accent); color: #fff; }
.err { color: var(--red); font-size: 13px; }
.muted { color: var(--muted); }
</style>
```

- [ ] **Step 3: Lancer tous les tests**

Run: `cd session-tracker && npx vitest run`
Expected: PASS.

- [ ] **Step 4: Vérification manuelle bout-en-bout (dev)**

Run:
```bash
cd session-tracker && docker compose up -d couchdb && npm run dev
```
Dans le navigateur (`http://localhost:8080`) : démarrer une session, ajouter 2-3 trades (dont un perdant), vérifier P&L / jauges / timer / compteurs, clôturer, aller dans Historique (session visible + case calendrier colorée), modifier les Paramètres et re-démarrer une session (défauts pré-remplis à jour).
Expected: parcours complet fonctionnel, données persistées après rechargement de la page.

- [ ] **Step 5: Commit**

```bash
cd session-tracker
git add src/App.vue src/components/SettingsView.vue
git commit -m "feat(session-tracker): vue Paramètres + shell App avec navigation"
```

---

## Task 10: Vérification finale full-stack (Docker) + index Mango

**Files:**
- Create: `session-tracker/couch-init.sh` (script d'init idempotent : base, user, index `_find`)
- Modify: `session-tracker/README.md` (référencer le script)

**Interfaces:**
- Consumes: docker-compose complet.
- Produces : lancement `docker compose up` validé de bout en bout ; index Mango sur `type` / `status` / `sessionId` pour des requêtes `_find` propres.

- [ ] **Step 1: Créer `couch-init.sh`** (idempotent)

```bash
#!/usr/bin/env bash
set -e
COUCH="${COUCH:-http://admin:admin@localhost:5984}"

curl -s -X PUT "$COUCH/session_tracker" >/dev/null || true
curl -s -X PUT "$COUCH/_users/org.couchdb.user:tracker" \
  -H "Content-Type: application/json" \
  -d '{"name":"tracker","password":"tracker","roles":[],"type":"user"}' >/dev/null || true
curl -s -X PUT "$COUCH/session_tracker/_security" \
  -H "Content-Type: application/json" \
  -d '{"members":{"names":["tracker"],"roles":[]}}' >/dev/null || true

# Index Mango pour _find
curl -s -X POST "$COUCH/session_tracker/_index" \
  -H "Content-Type: application/json" \
  -d '{"index":{"fields":["type","status"]},"name":"type-status"}' >/dev/null || true
curl -s -X POST "$COUCH/session_tracker/_index" \
  -H "Content-Type: application/json" \
  -d '{"index":{"fields":["type","sessionId"]},"name":"type-session"}' >/dev/null || true

echo "CouchDB init OK"
```

- [ ] **Step 2: Rendre exécutable et documenter**

Run: `cd session-tracker && chmod +x couch-init.sh`
Ajouter au README, section init : « Après le premier `docker compose up -d couchdb`, lancer `./couch-init.sh`. »

- [ ] **Step 3: Build + run full-stack**

Run:
```bash
cd session-tracker
docker compose up -d --build
./couch-init.sh
```
Ouvrir `http://localhost:8080`. Refaire un parcours (démarrer session, saisir trades, clôturer, historique).
Expected: l'app servie par nginx fonctionne, `/db` répond via le proxy nginx, données persistées dans le volume `couch_data` (survivent à `docker compose restart`).

- [ ] **Step 4: Vérifier la persistance hors navigateur**

Run:
```bash
cd session-tracker && docker compose restart couchdb
```
Recharger la page, vider le cache navigateur (ou onglet privé) : les sessions/trades sont toujours là (preuve que la donnée vit dans CouchDB, pas dans le navigateur).
Expected: données intactes.

- [ ] **Step 5: Commit**

```bash
cd session-tracker
git add couch-init.sh README.md
git commit -m "chore(session-tracker): script d'init CouchDB + index Mango + doc full-stack"
```

---

## Self-Review (fait par l'auteur du plan)

**Couverture du spec :**
- Démarrer/clôturer session (horodatage, multi-sessions/jour) → Task 4 (store), Task 6 (StartSession), Task 9 (App). ✓
- Pré-session avec loss limit / objectif / max trades / max loss trades / devise → Task 6. ✓
- Saisie rapide < 10 s, clavier, focus auto, `Entrée` → Task 6 (TradeForm). ✓
- Dashboard : timer durée, P&L gros, jauge loss limit (alerte 80 %, STOP 100 %), jauge objectif, compteur trades/max, compteur perdants/max, win rate, best/worst → Task 2 (métriques), Task 5 (Gauge/Banner/timer), Task 7 (dashboard). ✓
- Historique + calendrier P&L → Task 8. ✓
- Paramètres par défaut → Task 9. ✓
- Dark, vertical, FR, rouge/vert → style.css (Task 1) + composants. ✓
- CouchDB REST direct, proxy `/db` sans CORS, volume persistant, Docker, arm64 → Task 1, Task 10. ✓
- Trades Bybit non stockés → aucun code de persistance Bybit (v2). ✓

**Placeholders :** aucun « TBD/TODO » ; code complet à chaque étape. ✓
**Cohérence des types :** clés `params` identiques partout (`lossLimit, gainTarget, maxTrades, maxLossTrades, currency`) ; signatures `metrics.js` réutilisées à l'identique dans stores et UI ; `find(selector)` cohérent entre `db.js`, mocks de test et appels stores/historique. ✓
