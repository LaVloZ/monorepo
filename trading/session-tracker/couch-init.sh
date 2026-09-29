#!/usr/bin/env bash
set -e
COUCH="${COUCH:-http://admin:admin@localhost:5984}"

# Attendre que CouchDB réponde (évite un faux "OK" si le conteneur démarre encore)
for i in $(seq 1 30); do
  curl -sf "$COUCH/_up" >/dev/null 2>&1 && break
  sleep 1
done
if ! curl -sf "$COUCH/_up" >/dev/null 2>&1; then
  echo "CouchDB injoignable sur $COUCH — abandon." >&2
  exit 1
fi

# Bases système CouchDB 3 (pas toujours auto-créées en single-node) — requises avant de créer un user
curl -s -X PUT "$COUCH/_users" >/dev/null || true
curl -s -X PUT "$COUCH/_replicator" >/dev/null || true

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

# Cookie d'auth longue durée (sessions de trading de plusieurs heures)
curl -s -X PUT "$COUCH/_node/_local/_config/chttpd_auth/timeout" \
  -H "Content-Type: application/json" -d '"86400"' >/dev/null || true

echo "CouchDB init OK"
