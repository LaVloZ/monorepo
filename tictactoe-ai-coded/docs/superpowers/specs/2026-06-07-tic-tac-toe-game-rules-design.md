# Box Game — Étape 2 : Règles du jeu

**Date :** 2026-06-07
**Langage :** C (C11)
**Librairie graphique :** Raylib
**Tests :** CTest + exécutable de test maison (assert)

## Contexte

Suite de l'étape 1 (fenêtre + grille 3×3 dessinée). Cette étape ajoute la
**logique de jeu** du morpion : poser des pions au clic, jouer un
adversaire, détecter la fin de partie. L'adversaire est ici une **IA
aléatoire** placée derrière une fonction dédiée, qui sera remplacée par
Claude à l'étape 3.

Découpage global du projet :

1. ✅ Étape 1 — Dessiner le plateau (fenêtre + grille 3×3)
2. **🎯 Cette spec — Règles du jeu** (clics, tours, victoire/nul, rejoue)
3. Plus tard — Remplacer l'IA aléatoire par l'API Claude

## Comportement attendu

- **Humain = X**, joue en cliquant sur une case vide.
- **IA = O**, joue automatiquement un **coup aléatoire légal**
  immédiatement après le coup de l'humain.
- Une **barre de statut** affiche l'état : « Au tour de X », « Au tour de
  O », « X gagne ! », « O gagne ! » ou « Match nul ».
- À la **fin de partie** (victoire ou nul) : le message de résultat
  s'affiche, les clics de jeu sont ignorés, et **un clic réinitialise**
  une nouvelle partie.
- X commence toujours.

**Critère de succès :** on peut jouer une partie complète à la souris
contre l'IA, voir le bon résultat, et rejouer d'un clic ; la suite de
tests de la logique passe au vert.

## Architecture — séparation logique / dessin

Principe directeur : la **logique pure** est isolée du **dessin**. La
logique ne dépend pas de Raylib, ce qui la rend testable de façon
déterministe. Le choix du coup de l'IA est isolé dans son propre module
pour préparer son remplacement par Claude (étape 3).

```
src/
├── game.h / game.c   # logique pure : état, coups, victoire/nul — AUCUN Raylib
├── ai.h  / ai.c      # AiChooseMove() : coup aléatoire légal
├── board.h / board.c # dessin : grille + X/O + barre de statut
└── main.c            # boucle : entrées → coups → fin/rejoue → dessin
tests/
└── test_game.c       # tests de la logique via CTest
```

### `game.h` / `game.c` — logique pure (testée)

```c
typedef enum { CELL_EMPTY, CELL_X, CELL_O } Cell;
typedef enum { GAME_PLAYING, GAME_X_WINS, GAME_O_WINS, GAME_DRAW } GameStatus;
typedef struct {
    Cell cells[9];   // plateau à plat, index 0..8 (ligne par ligne)
    Cell turn;       // CELL_X ou CELL_O : à qui de jouer
    GameStatus status;
} Game;

void GameInit(Game *g);                  // plateau vide, turn = X, PLAYING
bool GamePlayMove(Game *g, int index);   // pose le pion si légal ; maj statut + tour ; true si appliqué
```

Règles internes de `GamePlayMove` :
- Refuse (renvoie `false`, aucun changement) si `status != GAME_PLAYING`,
  si `index` hors [0,8], ou si la case n'est pas `CELL_EMPTY`.
- Sinon pose `g->turn` dans `cells[index]`.
- Recalcule le statut : si une ligne gagnante existe pour le joueur qui
  vient de jouer → `GAME_X_WINS` / `GAME_O_WINS` ; sinon si plus aucune
  case vide → `GAME_DRAW` ; sinon reste `GAME_PLAYING` et le tour passe à
  l'autre joueur.

Détection de victoire : 8 lignes gagnantes (3 lignes, 3 colonnes, 2
diagonales) sous forme de triplets d'index.

### `ai.h` / `ai.c` — le « siège » de l'adversaire

```c
int AiChooseMove(const Game *g);   // index d'une case vide au hasard, ou -1 si plateau plein
```

Utilise `rand()`. C'est exactement cette fonction que l'étape 3
remplacera par un appel à Claude (même signature).

### `board.h` / `board.c` — dessin

- Constantes ajustées : `BOARD_SIZE 600`, `CELL_SIZE 200`,
  `WINDOW_WIDTH 600`, `WINDOW_HEIGHT 700` (bande de statut de 100px en
  bas), `LINE_THICKNESS 4.0f`.
- `DrawBoardGrid(void)` — grille 3×3 (lignes limitées à `BOARD_SIZE` en
  hauteur, plus seulement `WINDOW_SIZE`).
- `DrawMarks(const Game *g)` — pour chaque case : X = deux lignes en
  croix, O = un cercle (`DrawCircleLines` ou `DrawRing`), avec une marge
  intérieure dans la cellule.
- `DrawStatusText(const Game *g)` — texte dans la bande basse selon
  `status`/`turn` ; ajoute « — clique pour rejouer » en fin de partie.

### `main.c` — boucle de jeu

À chaque frame :
1. Si `status == GAME_PLAYING` et `turn == CELL_X` (humain) : si clic
   gauche dans la zone plateau (`mouseY < BOARD_SIZE`), convertir en
   index `col + row*3` et appeler `GamePlayMove`.
2. Si `status == GAME_PLAYING` et `turn == CELL_O` (IA) : appeler
   `AiChooseMove` puis `GamePlayMove`.
3. Si `status != GAME_PLAYING` : un clic gauche appelle `GameInit`
   (rejoue).
4. Dessiner : `ClearBackground`, `DrawBoardGrid`, `DrawMarks`,
   `DrawStatusText`.

`srand(time(NULL))` est appelé une fois au démarrage pour varier l'IA.

## Tests

`tests/test_game.c` : exécutable séparé, branché via `enable_testing()` +
`add_test` (CTest), sans dépendance externe (macros `assert`).

Cas couverts (logique `game.c`) :
- `GameInit` : toutes cases vides, `turn == CELL_X`, `status ==
  GAME_PLAYING`.
- Un coup légal pose le bon pion et passe le tour à l'autre joueur.
- Coup sur case occupée → `false`, état inchangé.
- Coup hors [0,8] → `false`, état inchangé.
- Coup quand `status != GAME_PLAYING` → `false`.
- Victoire détectée pour chaque type de ligne (au moins une ligne, une
  colonne, une diagonale).
- Match nul détecté quand le plateau est plein sans gagnant.

Cas couverts (`ai.c`) :
- `AiChooseMove` renvoie un index dont la case est `CELL_EMPTY`.
- Plateau plein → renvoie -1.

Lancement : `ctest --test-dir build` (après build).

## Ce que cette étape prépare

**Étape 3 (Claude)** : `AiChooseMove(const Game *g)` est remplacée (même
signature) par une implémentation qui sérialise le plateau, appelle
l'API Claude et parse le coup renvoyé. Le reste (boucle, dessin, règles)
ne bouge pas.
