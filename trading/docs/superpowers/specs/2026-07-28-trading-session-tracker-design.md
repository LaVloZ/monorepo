# Trading Session Tracker — Design

**Date :** 2026-07-28
**Emplacement de l'app :** `session-tracker/` (sous-dossier du projet)
**Statut :** design validé, prêt pour le plan d'implémentation

## But

Outil perso de suivi de sessions de trading, à afficher en side-panel à côté de
TradingView. On démarre une session avec des règles de risque, on saisit ses
trades au fil de la journée, et le dashboard montre en temps réel où on en est
par rapport aux règles (daily loss limit, objectif de gain, max trades, max
trades perdants). Local-first, tourne sur le PC ou un Raspberry Pi, non exposé.

Interface **en français**, **dark mode**, layout **vertical/compact**, tout
faisable **au clavier**, lisible en un coup d'œil pendant le trading.

## Périmètre

- **MVP (ce spec)** : sessions, saisie manuelle de trades, dashboard live,
  historique + calendrier P&L, paramètres.
- **v2 (hors périmètre, mais l'archi doit rester compatible)** : intégration
  Bybit (trades récupérés en direct, jamais stockés), stats agrégées, courbe
  d'equity, import CSV IBKR, export JSON.

## Architecture

Accès REST direct à CouchDB depuis l'app Vue. Pas de backend custom, pas de
PouchDB, pas d'IndexedDB.

```
┌──────────────────────┐      REST (fetch)      ┌──────────────────────┐
│   App Vue 3          │  ───────────────────►  │   CouchDB (Docker)   │
│   (Vite + Pinia)     │  ◄───────────────────  │   volume persistant  │
└──────────────────────┘                        └──────────────────────┘
```

- L'app lit/écrit directement dans CouchDB via son API REST (`fetch`),
  encapsulée dans un module `db.js` (CRUD + requêtes).
- **CouchDB dans Docker**, volume persistant = source unique de vérité, hors du
  navigateur. Nettoyer le cache du navigateur n'affecte **pas** les données —
  c'est la réponse à la crainte de suppression accidentelle.
- **Contrepartie assumée** : pas d'offline. L'app a besoin que CouchDB tourne.
  Comme il est en Docker sur la machine/Pi (toujours up), c'est un non-sujet.

### Sécurité (légère, car non exposé)

- CouchDB avec **CORS activé** pour l'origine du front.
- Un **user CouchDB dédié** à la base de l'app ; l'app s'authentifie via le
  endpoint `_session` (cookie). Pas d'admin en clair dans le front.
- On ne durcit pas plus : l'app reste sur le réseau local (PC / Pi), non exposée
  au web.

### Stack

- **Vue 3 + Vite** (composants) + **Pinia** (état de la session en cours).
- **docker-compose** : service `couchdb` (+ volume) et le front servi en statique
  (nginx). `docker compose up` → tout est lancé.
- Image CouchDB compatible **arm64** pour Raspberry Pi (Pi 4/5 en OS 64-bit).
  À noter dans le README : les Pi plus anciens (armv7) ne sont pas garantis.

## Modèle de données (documents CouchDB)

```
session: {
  _id, type: "session",
  startedAt, endedAt | null,
  status: "open" | "closed",
  params: { lossLimit, gainTarget, maxTrades, maxLossTrades, currency }
}

trade: {                       // saisie manuelle UNIQUEMENT
  _id, type: "trade",
  sessionId,
  ticker, side: "long" | "short",
  pnl,                         // résultat en devise (+ / -)
  rMultiple | null,            // optionnel
  note,                        // note libre courte
  tags: [],                    // pour stats v2 (setup)
  createdAt
}

settings: {                    // doc unique
  _id: "settings", type: "settings",
  lossLimit, gainTarget, maxTrades, maxLossTrades, currency
}
```

- Trades = documents individuels → idéal pour les stats v2 (par instrument, par
  tag/setup, courbe d'equity) sans réécriture.
- La **durée** de session n'est pas stockée : calculée à la volée
  (`(endedAt ?? now) − startedAt`).
- Les trades Bybit ne seront **jamais** persistés (v2 : récupérés en direct et
  fusionnés à l'affichage seulement).

## Flux de session

**Démarrer** — le bouton « Démarrer une session » ouvre une **pré-session** où
l'utilisateur fixe/confirme les règles du jour, pré-remplies depuis `settings`
mais ajustables pour cette session précise :

- Daily loss limit
- Objectif de gain
- Max trades (nb total)
- Max loss trades (nb max de trades perdants)
- Devise

À la validation : création du doc `session` (`status: "open"`, `startedAt = now`).
Une seule session `open` à la fois ; plusieurs sessions par jour possibles.

**Session en cours** — dashboard live :

- **Timer de durée** (hh:mm:ss, tourne en continu depuis `startedAt`).
- **P&L du jour** en gros chiffre central (vert/rouge).
- **Jauge daily loss limit** — alerte visuelle à 80 %, état **« STOP TRADING »**
  à 100 %.
- **Jauge objectif de gain**.
- **Compteur trades / max trades**.
- **Compteur trades perdants / max loss trades** — état « STOP » à l'atteinte.
- **Win rate** du jour, **meilleur / pire trade**.
- Bouton **Clôturer la session**.

État d'alerte dominant : dès qu'un seuil « STOP » est atteint (loss limit **ou**
max trades **ou** max loss trades), le dashboard bascule en alerte visuelle
claire (bandeau « STOP TRADING »).

**Clôturer** — `status: "closed"`, `endedAt = now`. La session passe dans
l'historique.

## Écrans (MVP)

1. **Dashboard session live** — décrit ci-dessus. Écran par défaut quand une
   session est ouverte. Sinon, écran d'accueil avec le bouton « Démarrer ».
2. **Saisie rapide d'un trade** — formulaire minimal (ticker, sens long/short,
   P&L en devise, R optionnel, note courte, tags optionnels). Focus auto sur le
   premier champ, validation par `Entrée`, objectif **< 10 s**. N'est
   accessible que si une session est ouverte ; rattache le trade à la session
   courante.
3. **Historique des sessions** — liste (date, P&L, nb trades, win rate) +
   **calendrier mensuel P&L** (une case par jour, verte/rouge selon le P&L
   agrégé du jour).
4. **Paramètres** — valeurs par défaut : loss limit, objectif de gain, max
   trades, max loss trades, devise (€ / $). + réglages de connexion CouchDB
   (URL de la base, identifiants) si besoin.

## Design UI

- Sobre, lisible en un coup d'œil. Le **P&L du jour** et la **jauge de loss
  limit** sont l'information dominante.
- Rouge / vert classiques pour perte / gain, pas de fioritures.
- Dark mode par défaut. Layout vertical adapté à une fenêtre étroite (side-panel).
- Aucune friction : navigation et saisie faisables au clavier.

## Compatibilité v2 (non codé maintenant)

- **Bybit** : un service de récupération alimentera l'affichage à la volée
  (trades Bybit fusionnés visuellement avec les trades manuels, jamais stockés).
  Note : l'API privée Bybit depuis le navigateur butera sur CORS + signature
  HMAC → nécessitera probablement un petit relais (ou un passage par le Docker)
  le moment venu. Rien à décider maintenant.
- **Stats agrégées** (P&L par instrument, par jour de semaine, par tag/setup),
  **courbe d'equity**, **import CSV IBKR (Flex Query)**, **export CSV/JSON**.

## Hors périmètre (YAGNI)

- Pas de multi-utilisateur, pas de comptes, pas de déploiement web exposé.
- Pas d'analyse technique / indicateurs.
- Pas d'offline (dépendance assumée à CouchDB up).
