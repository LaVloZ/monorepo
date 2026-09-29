# Session Tracker — Sous-projet A : Améliorations app

**Date :** 2026-07-29
**App :** `session-tracker/` (existante, MVP livré)
**Statut :** design validé, prêt pour le plan d'implémentation

## Contexte & feuille de route

Suite au MVP (démarrer/clôturer session, saisie manuelle, dashboard live,
historique + calendrier, paramètres). L'utilisateur a donné son feedback et fixé
l'ordre des sous-projets :

- **A (ce spec)** — améliorations de l'app.
- **B (ensuite)** — connexion Bybit (récupération en direct, jamais stockée ;
  nécessitera un petit relais pour CORS + signature HMAC).
- **C (après)** — IA : serveur MCP en Python (lecture + écriture) parlant à la
  même base CouchDB.

Ce spec ne couvre que **A**.

## Périmètre de A

1. Retenir le dernier ticker.
2. Historique enrichi : vue détail d'une session.
3. Calculateur de P&L simple (market value) + persistance taille/entrée/sortie.
4. `make stop` arrête toute la stack (pas seulement CouchDB).
5. Contrainte transverse : tests automatisés **unitaires + intégration**.

## Contraintes (rappel, inchangées)

Interface **française**, dark mode, layout vertical/compact, tout au clavier,
rouge=perte/vert=gain. REST direct vers CouchDB via le proxy `/db`. Base
`session_tracker`. Vue 3 + Vite + Pinia. Node ≥ 20.

## 1. Retenir le dernier ticker

**But :** ne pas re-saisir le ticker quand on enchaîne des trades sur le même
instrument.

- Le store `session` expose un getter `lastTicker` = ticker du trade le plus
  récent de la session courante (`""` si aucun trade).
- `TradeForm.vue` :
  - À l'affichage, initialise le champ ticker avec `session.lastTicker`.
  - Après un ajout réussi, **conserve** le ticker saisi (au lieu de vider le
    champ) ; les autres champs (pnl, R, note, tags, taille/entrée/sortie) sont
    réinitialisés. Le focus repart sur le ticker (on peut taper par-dessus pour
    changer d'instrument).

**Critères d'acceptation :** après avoir ajouté « NQ », le champ ticker affiche
encore « NQ » et le focus est dessus ; une nouvelle session vierge démarre avec
le ticker vide.

## 2. Historique enrichi — détail de session

**But :** revoir une session passée en détail.

- Nouveau composant `SessionDetail.vue`. Props : la session + ses trades.
  Affiche :
  - **Stats de la session** : P&L total, nb trades, win rate, meilleur/pire
    trade, **durée** (`endedAt − startedAt`, via `formatDuration`).
  - **Liste des trades** de la session : ticker, sens (▲/▼), pnl (vert/rouge),
    R si présent, note, tags, heure (`createdAt`), et taille/entrée/sortie si
    renseignés.
  - Bouton **retour** vers la liste des sessions.
- `SessionHistory.vue` : cliquer une ligne de session sélectionne cette session
  et affiche `SessionDetail` (le calendrier + la liste laissent place au détail ;
  retour = on revient à la liste). Les trades sont déjà chargés en mémoire — on
  filtre par `sessionId`, aucune requête supplémentaire.

**Critères d'acceptation :** cliquer une session ouvre son détail avec ses
trades et ses stats ; le bouton retour revient à la liste ; une session sans
trade affiche « Aucun trade ».

## 3. Calculateur de P&L simple (market value)

**But :** calculer le € automatiquement au lieu de le taper.

- Fonction pure `computePnl({ size, entry, exit, side })` dans `lib/metrics.js` :
  - `size × (exit − entry) × (side === "long" ? 1 : -1)`.
  - Renvoie `null` si `size`, `entry` ou `exit` manquant / non numérique.
  - Pas de multiplicateur d'instrument (futures NQ/MNQ non gérés à ce stade —
    market value brute).
- `TradeForm.vue` : ajoute 3 champs optionnels **taille**, **entrée**, **sortie**.
  - Quand les trois sont renseignés (numériques) : le pnl est **calculé
    automatiquement** et le champ résultat manuel passe **en lecture seule**
    (affiche la valeur calculée). Une seule source de vérité.
  - Si l'un des trois est vide : le champ résultat redevient éditable (saisie
    manuelle du € comme dans le MVP).
  - La validation existante reste : on ne soumet pas sans pnl valide (calculé ou
    manuel) ni sans ticker.
- Le doc `trade` gagne 3 champs optionnels : `size`, `entry`, `exit`
  (nombres, ou `null`). **Rétrocompatible** : les trades existants les ont à
  `null` ; `SessionDetail` ne les affiche que s'ils sont présents.

**Critères d'acceptation :** saisir taille=2, entrée=100, sortie=110, sens=long
→ le résultat affiche 20 et est en lecture seule ; sens=short → −20 ; effacer la
sortie → le champ résultat redevient saisissable. Un trade avec ces champs les
persiste et les montre dans le détail de session.

## 4. `make stop` arrête toute la stack

**Bug signalé :** après `make stop`, le front (serveur Vite en mode dev) reste
accessible — `make stop` ne fait que `docker compose stop` (CouchDB seul).

- `make stop` doit **aussi** arrêter le serveur Vite de dev :
  `docker compose stop` **+** arrêt du process `vite` (ex. `pkill -f vite`,
  tolérant à l'absence de process — pas d'erreur si rien ne tourne).
- Le mode tout-Docker (`make up`) est déjà entièrement couvert par
  `docker compose stop` (le front y est un conteneur). La cible `stop` gère les
  deux cas.

**Critères d'acceptation :** après `make dev` puis `make stop`, `http://localhost:8080`
ne répond plus (ni front ni proxy) ; `make stop` ne renvoie pas d'erreur si le
serveur Vite n'était pas lancé.

## 5. Tests (contrainte qualité)

- **Unitaires (Vitest)** — pur, hors-ligne :
  - `computePnl` : long, short, champs manquants → `null`, valeurs décimales.
  - Getter `lastTicker` du store session (dernier trade / vide).
  - Stats de `SessionDetail` (réutilisent les fonctions `metrics.js` déjà
    testées ; tester surtout la durée et le filtrage par `sessionId`).
- **Intégration** — contre une **vraie CouchDB** :
  - Un script `npm run test:integration` (et une cible `make test-int`).
  - Crée une **base temporaire** dédiée, exécute : `db.js` CRUD, puis le flux
    complet via les stores (démarrer session → addTrade → clôturer → recharger
    dans une nouvelle instance → historique cohérent), puis **supprime** la base
    temporaire.
  - **Skip automatique** si CouchDB n'est pas joignable (les tests unitaires et
    la CI hors-ligne restent verts). Documenté dans README + Makefile.

## Modèle de données (changement)

`trade` : ajout de 3 champs **optionnels**.

```
trade: {
  _id, type: "trade", sessionId,
  ticker, side, pnl, rMultiple | null, note, tags: [], createdAt,
  size | null, entry | null, exit | null       // ← nouveau (optionnels)
}
```

Rétrocompatible : aucune migration. Les docs sans ces champs restent valides ;
le code lit `?? null`.

## Hors périmètre (YAGNI, pour A)

- Multiplicateurs d'instruments / valeur au point (futures) — plus tard si
  besoin.
- Édition / suppression d'un trade existant.
- Bybit et MCP (sous-projets B et C).
