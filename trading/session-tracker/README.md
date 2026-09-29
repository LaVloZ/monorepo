# Session Tracker

Suivi perso de sessions de trading. Front Vue 3, données dans CouchDB (Docker).

## Lancer (prod, un seul fichier de commande)

```bash
docker compose up -d
```
Front sur http://localhost:8080. CouchDB derrière le proxy `/db`.

## Première init de la base (une fois)

Après le premier `docker compose up -d couchdb`, lancer `./couch-init.sh`.

Le script est idempotent (rejouable sans erreur) et crée : la base `session_tracker`,
l'utilisateur applicatif `tracker`, sa `_security`, et deux index Mango
(`type`/`status` et `type`/`sessionId`) pour des requêtes `_find` propres.

## Dév

```bash
docker compose up -d couchdb
npm install
npm run dev   # http://localhost:8080, proxy /db -> localhost:5984
npm test
make test-int   # tests d'intégration contre une vraie CouchDB (skippés si CouchDB absente)
```

## Identifiants

`public/config.js` contient l'utilisateur applicatif CouchDB (local, non exposé).
Change-les si besoin (doivent exister dans `_users`).

## Raspberry Pi

Image `couchdb:3` compatible arm64 (Pi 4/5, OS 64-bit). Les Pi armv7 ne sont pas garantis.
