# Session Tracker — raccourcis de commandes
# Prérequis : Docker Desktop lancé (CouchDB tourne dans Docker).
# Usage : `make <cible>`. `make` ou `make help` liste les cibles.

.DEFAULT_GOAL := help
.PHONY: help init install db db-init dev up build test test-int down stop logs restart reset

help: ## Affiche cette aide
	@echo "Session Tracker — cibles disponibles :"
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | \
		awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-12s\033[0m %s\n", $$1, $$2}'

init: db db-init install ## Première install : lance CouchDB, initialise la base, installe les deps
	@echo "✅ Prêt. Lance 'make dev'."

install: ## Installe les dépendances npm
	npm install

db: ## Démarre CouchDB (Docker, en tâche de fond)
	docker compose up -d couchdb

db-init: ## Crée base + user + index Mango dans CouchDB (idempotent)
	./couch-init.sh

dev: db ## Lance l'app en mode dev (Vite) sur http://localhost:8080
	npm run dev

up: ## Mode tout-Docker : build le front (nginx) + CouchDB, puis init
	docker compose up -d --build
	./couch-init.sh
	@echo "✅ App sur http://localhost:8080"

build: ## Build de production du front (Vite)
	npm run build

test: ## Lance la suite de tests (Vitest)
	npm test

test-int: ## Tests d'intégration contre une vraie CouchDB (démarre CouchDB si besoin)
	docker compose up -d couchdb
	npm run test:integration

logs: ## Suit les logs de CouchDB
	docker compose logs -f couchdb

stop: ## Arrête toute la stack : conteneurs Docker + serveur Vite de dev (garde les données)
	docker compose stop
	-pkill -f "vite" || true
	@echo "🛑 Stack arrêtée (Docker + Vite)."

down: ## Arrête et supprime les conteneurs (garde le volume de données)
	docker compose down

restart: ## Redémarre CouchDB (test de persistance)
	docker compose restart couchdb

reset: ## ⚠️ Supprime conteneurs ET volume de données (perte de tout l'historique)
	docker compose down -v
	@echo "⚠️  Volume de données supprimé. Relance 'make init'."
