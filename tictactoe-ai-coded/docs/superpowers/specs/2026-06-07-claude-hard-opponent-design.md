# Box Game — Niveau Difficile piloté par Claude

**Date :** 2026-06-07
**Langage :** C (C11)
**Réseau :** libcurl (HTTP brut — pas de SDK Anthropic officiel en C)
**JSON :** cJSON
**Concurrence :** pthread

## Contexte

Le morpion propose Facile (aléatoire) et Moyen (heuristique), avec menu,
animation des pions et délai IA. Cette fonctionnalité ajoute le **niveau
Difficile** : l'adversaire O est choisi par **Claude** via l'API Anthropic.

C'est la dernière étape du projet d'apprentissage. Elle introduit trois
notions nouvelles : un appel HTTP (libcurl), du parsing JSON (cJSON), et
un appel asynchrone via un **thread** pour ne pas figer la fenêtre.

## Comportement attendu

- Au menu, le bouton **Difficile** devient **actif** et sélectionnable.
- Quand l'IA (O) doit jouer en Difficile : un **thread d'arrière-plan**
  appelle l'API Claude pendant que la fenêtre affiche **« Réflexion… »**
  et reste fluide (60 FPS).
- À réception de la réponse, le coup est posé et **animé** comme les
  autres pions.
- **Repli sur l'heuristique Moyen** en cas de : absence de clé API,
  erreur réseau, réponse HTTP non-2xx, JSON invalide, ou coup illégal
  renvoyé. La partie ne se bloque jamais.
- La clé API est lue depuis la variable d'environnement
  **`ANTHROPIC_API_KEY`**.

**Critère de succès :** avec `ANTHROPIC_API_KEY` définie, choisir
Difficile fait jouer Claude (coup légal, fenêtre fluide, « Réflexion… »
visible). Sans clé ou en cas d'erreur, l'IA joue le coup Moyen sans
blocage. Les tests de parsing passent au vert.

## Architecture

Séparation stricte entre la **logique de parsing** (pure, testable, sans
réseau) et l'**appel réseau + thread** (non testé unitairement).

```
src/
├── claude_parse.h / claude_parse.c   Prompt build + parse réponse → coup légal (cJSON + game)
├── claude.h        / claude.c        Appel libcurl + wrapper async (pthread)
├── ai.c                              inchangé ; ChooseMedium sert de repli
└── main.c                            phase PLAY_THINKING : lance le thread, sonde, applique
tests/
└── test_game.c                       + tests ClaudeParseMove
```

### `claude_parse.h` / `claude_parse.c` — pur, testé

```c
// Construit le prompt utilisateur décrivant le plateau pour Claude (jouant O).
// Le tampon `out` reçoit le texte ; `out_size` est sa capacité.
void ClaudeBuildPrompt(const Game *g, char *out, int out_size);

// Parse la réponse JSON de l'API : extrait content[0].text, y trouve le
// premier index 0..8 désignant une case VIDE et légale. Renvoie -1 si la
// réponse est invalide ou ne contient aucun coup légal.
int ClaudeParseMove(const char *response_json, const Game *g);
```

`ClaudeParseMove` dépend de cJSON (parse l'enveloppe) et de `game.h`
(valider la légalité). Aucune dépendance réseau → testable avec un faux
JSON de réponse.

### `claude.h` / `claude.c` — réseau + async

```c
// Appel bloquant : vérifie ANTHROPIC_API_KEY, sérialise le plateau, POST
// via libcurl vers l'API, parse la réponse. Renvoie un coup légal, ou -1
// sur toute erreur (pas de clé, réseau, HTTP non-2xx, parsing).
int ClaudeChooseMove(const Game *g);

// Wrapper asynchrone (un thread pthread). L'état est opaque pour main.c.
typedef struct ClaudeRequest ClaudeRequest;
ClaudeRequest *ClaudeRequestStart(const Game *g);   // copie g, lance le thread
bool ClaudeRequestPoll(ClaudeRequest *req, int *move_out);  // true si terminé
void ClaudeRequestFree(ClaudeRequest *req);
```

- `ClaudeChooseMove` utilise libcurl (`CURLOPT_URL`,
  `CURLOPT_POSTFIELDS`, en-têtes `x-api-key`, `anthropic-version:
  2023-06-01`, `content-type: application/json`) et un callback
  d'écriture accumulant la réponse dans un tampon, puis appelle
  `ClaudeParseMove`.
- Le wrapper async copie le `Game`, lance un thread qui exécute
  `ClaudeChooseMove` et stocke le résultat sous mutex. `ClaudeRequestPoll`
  renvoie `true` quand le thread a fini (sans bloquer).

### Requête HTTP

- URL : `https://api.anthropic.com/v1/messages`
- Modèle : constante `CLAUDE_MODEL = "claude-opus-4-8"` (modifiable ; ex.
  `claude-haiku-4-5` pour des réponses plus rapides).
- Corps : `{"model": ..., "max_tokens": 1024, "messages": [{"role":
  "user", "content": "<prompt>"}]}`. **Pas** de `temperature` (rejeté par
  Opus 4.8).
- En-têtes : `x-api-key: $ANTHROPIC_API_KEY`, `anthropic-version:
  2023-06-01`, `content-type: application/json`.

### Prompt

Décrit le plateau et demande un coup unique :

```
Tu joues au morpion en tant que O. Les cases sont numérotées 0 à 8 :
0 1 2
3 4 5
6 7 8
Plateau actuel (X, O, ou . pour vide) :
. X .
O . .
. . X
C'est à toi (O). Réponds UNIQUEMENT par le numéro (0-8) d'une case vide.
```

`ClaudeParseMove` tolère du texte autour : il prend le premier chiffre
0–8 qui désigne une case vide légale ; sinon `-1` (→ repli).

### `main.c` — phase PLAY_THINKING

Nouvelle phase de jeu pour le tour de l'IA en Difficile :

- À l'entrée du tour O **et** `difficulty == DIFFICULTY_HARD` : `phase =
  PLAY_THINKING`, `req = ClaudeRequestStart(&game)`. Pour EASY/MEDIUM, on
  garde la phase `PLAY_WAIT_AI` existante (coup synchrone instantané).
- **PLAY_THINKING** (chaque frame) : afficher « Réflexion… » ;
  `ClaudeRequestPoll(req, &move)` ; quand terminé : si `move < 0`, repli
  `move = AiChooseMove(&game, DIFFICULTY_MEDIUM)` ; `GamePlayMove` ;
  libérer la requête ; démarrer l'animation (`PLAY_ANIMATING`).
- Le repli réutilise l'API publique `AiChooseMove` avec `DIFFICULTY_MEDIUM`.

L'affichage « Réflexion… » : une fonction `DrawThinkingText(void)` dans
`board.c` (texte ASCII « Reflexion... » pour rester compatible avec la
police par défaut de Raylib).

### Menu

Le bouton Difficile (index 2) devient cliquable dans `main.c` (boucle de
sélection passant de `i < 2` à `i < 3`) et n'est plus grisé dans
`DrawMenu` (les 3 boutons en bleu).

## Build

`CMakeLists.txt` ajoute :
- `find_package(CURL REQUIRED)` → lier `CURL::libcurl`.
- `find_package(Threads REQUIRED)` → lier `Threads::Threads`.
- cJSON via pkg-config : `find_package(PkgConfig REQUIRED)` +
  `pkg_check_modules(CJSON REQUIRED libcjson)` (`brew install cjson`).

`box-game` lie raylib + CURL + Threads + cJSON, avec `src/claude_parse.c`
et `src/claude.c`. La cible `box-game-tests` lie **seulement** cJSON (pour
`claude_parse.c`) — pas curl ni pthread.

## Tests

`ClaudeParseMove` (déterministe, faux JSON) :
- Réponse valide avec texte « 4 » sur une case vide → renvoie 4.
- Réponse pointant une case occupée → `-1`.
- Réponse avec un nombre hors [0,8] uniquement → `-1`.
- JSON syntaxiquement invalide → `-1`.
- Enveloppe sans `content[0].text` → `-1`.

La partie réseau/thread est validée manuellement : avec `ANTHROPIC_API_KEY`
(Claude joue) et sans (repli Moyen, aucun blocage).

## Fin du projet

Cette étape clôt la feuille de route initiale : dessin → règles →
difficultés → animation → adversaire Claude.
