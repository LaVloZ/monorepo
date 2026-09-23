# Plan d'implémentation — Étape 2 : Règles du jeu

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Jouer une partie de morpion à la souris (humain = X) contre une IA aléatoire (O), avec détection victoire/nul, indicateur de tour et rejoue d'un clic.

**Architecture:** Logique pure testée (`game.c`) séparée du dessin (`board.c`) et des entrées (`main.c`). Le coup de l'IA est isolé dans `ai.c` (`AiChooseMove`), point de remplacement pour Claude à l'étape 3. La logique est développée en TDD via un exécutable de test branché sur CTest, sans dépendance Raylib.

**Tech Stack:** C (C11), Raylib, CMake + CTest.

**Convention plateau :** tableau plat `cells[9]`, index `0..8` ligne par ligne ; `index = row * 3 + col`.

---

## Structure des fichiers

- `src/game.h` / `src/game.c` — Création. Types (`Cell`, `GameStatus`, `Game`) + logique (`GameInit`, `GamePlayMove`). Aucun Raylib.
- `src/ai.h` / `src/ai.c` — Création. `AiChooseMove` (coup aléatoire légal).
- `src/board.h` / `src/board.c` — Modification. Nouvelles constantes (fenêtre 600×700) + `DrawMarks`, `DrawStatusText`.
- `src/main.c` — Modification. Boucle : clics, tour IA, rejoue.
- `tests/test_game.c` — Création. Tests de la logique (CTest).
- `CMakeLists.txt` — Modification. Cible de tests + ajout des sources.

---

## Task 1 : Harnais de test + `GameInit`

**Files:**
- Create: `src/game.h`, `src/game.c`, `tests/test_game.c`
- Modify: `CMakeLists.txt`

- [ ] **Step 1 : Écrire le test qui échoue — `tests/test_game.c`**

```c
#include <assert.h>
#include <stdio.h>
#include "game.h"

static void test_game_init(void) {
    Game g;
    GameInit(&g);
    for (int i = 0; i < 9; i++) assert(g.cells[i] == CELL_EMPTY);
    assert(g.turn == CELL_X);
    assert(g.status == GAME_PLAYING);
}

int main(void) {
    test_game_init();
    printf("Tous les tests passent\n");
    return 0;
}
```

- [ ] **Step 2 : Brancher la cible de tests — ajouter à la fin de `CMakeLists.txt`**

```cmake
enable_testing()
add_executable(box-game-tests
    tests/test_game.c
    src/game.c
)
target_include_directories(box-game-tests PRIVATE src)
add_test(NAME game_tests COMMAND box-game-tests)
```

- [ ] **Step 3 : Vérifier que ça échoue à la compilation**

Run :
```sh
cmake -B build >/dev/null && cmake --build build --target box-game-tests
```
Expected : ÉCHEC — `fatal error: 'game.h' file not found` (le module n'existe pas encore).

- [ ] **Step 4 : Créer `src/game.h`**

```c
#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

typedef enum { CELL_EMPTY, CELL_X, CELL_O } Cell;
typedef enum { GAME_PLAYING, GAME_X_WINS, GAME_O_WINS, GAME_DRAW } GameStatus;

typedef struct {
    Cell cells[9];   // plateau à plat, index 0..8
    Cell turn;       // à qui de jouer
    GameStatus status;
} Game;

void GameInit(Game *g);
bool GamePlayMove(Game *g, int index);

#endif // GAME_H
```

- [ ] **Step 5 : Créer `src/game.c` (avec `GamePlayMove` en stub)**

```c
#include "game.h"

void GameInit(Game *g) {
    for (int i = 0; i < 9; i++) g->cells[i] = CELL_EMPTY;
    g->turn = CELL_X;
    g->status = GAME_PLAYING;
}

bool GamePlayMove(Game *g, int index) {
    (void)g;
    (void)index;
    return false;
}
```

- [ ] **Step 6 : Vérifier que les tests passent**

Run :
```sh
cmake -B build >/dev/null && cmake --build build --target box-game-tests && ctest --test-dir build --output-on-failure
```
Expected : compilation OK, `100% tests passed`.

- [ ] **Step 7 : Commit**

```sh
git add src/game.h src/game.c tests/test_game.c CMakeLists.txt
git commit -m "test: harnais CTest + GameInit"
```

---

## Task 2 : `GamePlayMove` — poser un pion et passer le tour

**Files:**
- Modify: `tests/test_game.c`, `src/game.c`

- [ ] **Step 1 : Ajouter le test (avant `int main`) dans `tests/test_game.c`**

```c
static void test_play_move_places_mark_and_switches_turn(void) {
    Game g;
    GameInit(&g);

    bool ok = GamePlayMove(&g, 4);   // X joue au centre
    assert(ok == true);
    assert(g.cells[4] == CELL_X);
    assert(g.turn == CELL_O);        // au tour de O maintenant

    ok = GamePlayMove(&g, 0);        // O joue en haut-gauche
    assert(ok == true);
    assert(g.cells[0] == CELL_O);
    assert(g.turn == CELL_X);
}
```

- [ ] **Step 2 : L'appeler dans `main()` de `tests/test_game.c`**

Ajouter la ligne avant `printf(...)` :
```c
    test_play_move_places_mark_and_switches_turn();
```

- [ ] **Step 3 : Vérifier que ça échoue**

Run :
```sh
cmake --build build --target box-game-tests && ctest --test-dir build --output-on-failure
```
Expected : ÉCHEC (assertion sur `ok == true` : le stub renvoie `false`).

- [ ] **Step 4 : Implémenter le minimum — remplacer `GamePlayMove` dans `src/game.c`**

```c
bool GamePlayMove(Game *g, int index) {
    g->cells[index] = g->turn;
    g->turn = (g->turn == CELL_X) ? CELL_O : CELL_X;
    return true;
}
```

- [ ] **Step 5 : Vérifier que les tests passent**

Run :
```sh
cmake --build build --target box-game-tests && ctest --test-dir build --output-on-failure
```
Expected : `100% tests passed`.

- [ ] **Step 6 : Commit**

```sh
git add tests/test_game.c src/game.c
git commit -m "feat: GamePlayMove pose un pion et alterne les tours"
```

---

## Task 3 : `GamePlayMove` — rejeter les coups illégaux

**Files:**
- Modify: `tests/test_game.c`, `src/game.c`

- [ ] **Step 1 : Ajouter le test (avant `int main`)**

```c
static void test_play_move_rejects_illegal(void) {
    Game g;
    GameInit(&g);

    GamePlayMove(&g, 4);                 // X au centre
    bool ok = GamePlayMove(&g, 4);       // O tente une case occupée
    assert(ok == false);
    assert(g.cells[4] == CELL_X);        // inchangé
    assert(g.turn == CELL_O);            // tour inchangé

    assert(GamePlayMove(&g, -1) == false);  // hors limites
    assert(GamePlayMove(&g, 9) == false);   // hors limites

    g.status = GAME_X_WINS;              // partie terminée
    assert(GamePlayMove(&g, 1) == false);
    assert(g.cells[1] == CELL_EMPTY);
}
```

- [ ] **Step 2 : L'appeler dans `main()`**

```c
    test_play_move_rejects_illegal();
```

- [ ] **Step 3 : Vérifier que ça échoue**

Run :
```sh
cmake --build build --target box-game-tests && ctest --test-dir build --output-on-failure
```
Expected : ÉCHEC (le coup sur case occupée renvoie `true` et écrase la case).

- [ ] **Step 4 : Ajouter les gardes — remplacer `GamePlayMove` dans `src/game.c`**

```c
bool GamePlayMove(Game *g, int index) {
    if (g->status != GAME_PLAYING) return false;
    if (index < 0 || index > 8) return false;
    if (g->cells[index] != CELL_EMPTY) return false;

    g->cells[index] = g->turn;
    g->turn = (g->turn == CELL_X) ? CELL_O : CELL_X;
    return true;
}
```

- [ ] **Step 5 : Vérifier que les tests passent**

Run :
```sh
cmake --build build --target box-game-tests && ctest --test-dir build --output-on-failure
```
Expected : `100% tests passed`.

- [ ] **Step 6 : Commit**

```sh
git add tests/test_game.c src/game.c
git commit -m "feat: GamePlayMove rejette les coups illégaux"
```

---

## Task 4 : Détection de victoire

**Files:**
- Modify: `tests/test_game.c`, `src/game.c`

- [ ] **Step 1 : Ajouter les tests (avant `int main`)**

```c
static void test_win_row(void) {
    Game g;
    GameInit(&g);
    // X: 0,1,2 (ligne du haut) ; O: 3,4 entre les deux
    GamePlayMove(&g, 0); // X
    GamePlayMove(&g, 3); // O
    GamePlayMove(&g, 1); // X
    GamePlayMove(&g, 4); // O
    GamePlayMove(&g, 2); // X gagne la ligne 0,1,2
    assert(g.status == GAME_X_WINS);
}

static void test_win_column(void) {
    Game g;
    GameInit(&g);
    // X: 0,3,6 (colonne gauche)
    GamePlayMove(&g, 0); // X
    GamePlayMove(&g, 1); // O
    GamePlayMove(&g, 3); // X
    GamePlayMove(&g, 2); // O
    GamePlayMove(&g, 6); // X gagne la colonne 0,3,6
    assert(g.status == GAME_X_WINS);
}

static void test_win_diagonal(void) {
    Game g;
    GameInit(&g);
    // X: 0,4,8 (diagonale)
    GamePlayMove(&g, 0); // X
    GamePlayMove(&g, 1); // O
    GamePlayMove(&g, 4); // X
    GamePlayMove(&g, 2); // O
    GamePlayMove(&g, 8); // X gagne la diagonale 0,4,8
    assert(g.status == GAME_X_WINS);
}
```

- [ ] **Step 2 : Les appeler dans `main()`**

```c
    test_win_row();
    test_win_column();
    test_win_diagonal();
```

- [ ] **Step 3 : Vérifier que ça échoue**

Run :
```sh
cmake --build build --target box-game-tests && ctest --test-dir build --output-on-failure
```
Expected : ÉCHEC (`status` reste `GAME_PLAYING`, la victoire n'est pas détectée).

- [ ] **Step 4 : Implémenter la détection — éditer `src/game.c`**

Ajouter en haut du fichier, juste après `#include "game.h"` :

```c
static const int WIN_LINES[8][3] = {
    {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, // lignes
    {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, // colonnes
    {0, 4, 8}, {2, 4, 6}             // diagonales
};

static bool HasWon(const Game *g, Cell player) {
    for (int i = 0; i < 8; i++) {
        const int *l = WIN_LINES[i];
        if (g->cells[l[0]] == player &&
            g->cells[l[1]] == player &&
            g->cells[l[2]] == player) {
            return true;
        }
    }
    return false;
}
```

Puis remplacer `GamePlayMove` par :

```c
bool GamePlayMove(Game *g, int index) {
    if (g->status != GAME_PLAYING) return false;
    if (index < 0 || index > 8) return false;
    if (g->cells[index] != CELL_EMPTY) return false;

    Cell player = g->turn;
    g->cells[index] = player;

    if (HasWon(g, player)) {
        g->status = (player == CELL_X) ? GAME_X_WINS : GAME_O_WINS;
    } else {
        g->turn = (player == CELL_X) ? CELL_O : CELL_X;
    }
    return true;
}
```

- [ ] **Step 5 : Vérifier que les tests passent**

Run :
```sh
cmake --build build --target box-game-tests && ctest --test-dir build --output-on-failure
```
Expected : `100% tests passed`.

- [ ] **Step 6 : Commit**

```sh
git add tests/test_game.c src/game.c
git commit -m "feat: détection de victoire (lignes, colonnes, diagonales)"
```

---

## Task 5 : Détection du match nul

**Files:**
- Modify: `tests/test_game.c`, `src/game.c`

- [ ] **Step 1 : Ajouter le test (avant `int main`)**

```c
static void test_draw(void) {
    Game g;
    GameInit(&g);
    // Séquence menant à un plateau plein sans gagnant :
    // X O X
    // X O O
    // O X X
    GamePlayMove(&g, 0); // X
    GamePlayMove(&g, 1); // O
    GamePlayMove(&g, 2); // X
    GamePlayMove(&g, 4); // O
    GamePlayMove(&g, 3); // X
    GamePlayMove(&g, 5); // O
    GamePlayMove(&g, 7); // X
    GamePlayMove(&g, 6); // O
    GamePlayMove(&g, 8); // X
    assert(g.status == GAME_DRAW);
}
```

- [ ] **Step 2 : L'appeler dans `main()`**

```c
    test_draw();
```

- [ ] **Step 3 : Vérifier que ça échoue**

Run :
```sh
cmake --build build --target box-game-tests && ctest --test-dir build --output-on-failure
```
Expected : ÉCHEC (`status` reste `GAME_PLAYING`, le nul n'est pas détecté).

- [ ] **Step 4 : Ajouter `BoardFull` et la branche nul — éditer `src/game.c`**

Ajouter après `HasWon` :

```c
static bool BoardFull(const Game *g) {
    for (int i = 0; i < 9; i++) {
        if (g->cells[i] == CELL_EMPTY) return false;
    }
    return true;
}
```

Puis remplacer le bloc de mise à jour du statut dans `GamePlayMove` par :

```c
    if (HasWon(g, player)) {
        g->status = (player == CELL_X) ? GAME_X_WINS : GAME_O_WINS;
    } else if (BoardFull(g)) {
        g->status = GAME_DRAW;
    } else {
        g->turn = (player == CELL_X) ? CELL_O : CELL_X;
    }
```

- [ ] **Step 5 : Vérifier que les tests passent**

Run :
```sh
cmake --build build --target box-game-tests && ctest --test-dir build --output-on-failure
```
Expected : `100% tests passed`.

- [ ] **Step 6 : Commit**

```sh
git add tests/test_game.c src/game.c
git commit -m "feat: détection du match nul"
```

---

## Task 6 : IA aléatoire `AiChooseMove`

**Files:**
- Create: `src/ai.h`, `src/ai.c`
- Modify: `tests/test_game.c`, `CMakeLists.txt`

- [ ] **Step 1 : Ajouter les tests (avant `int main`) dans `tests/test_game.c`**

```c
static void test_ai_returns_empty_cell(void) {
    Game g;
    GameInit(&g);
    g.cells[0] = CELL_X;
    g.cells[4] = CELL_O;
    g.cells[8] = CELL_X;
    for (int k = 0; k < 50; k++) {
        int m = AiChooseMove(&g);
        assert(m >= 0 && m < 9);
        assert(g.cells[m] == CELL_EMPTY);
    }
}

static void test_ai_full_board_returns_minus_one(void) {
    Game g;
    GameInit(&g);
    for (int i = 0; i < 9; i++) g.cells[i] = CELL_X;
    assert(AiChooseMove(&g) == -1);
}
```

Ajouter aussi l'include en haut du fichier, sous `#include "game.h"` :
```c
#include "ai.h"
```

- [ ] **Step 2 : Les appeler dans `main()`**

```c
    test_ai_returns_empty_cell();
    test_ai_full_board_returns_minus_one();
```

- [ ] **Step 3 : Ajouter `src/ai.c` à la cible de tests — éditer `CMakeLists.txt`**

Remplacer le bloc `add_executable(box-game-tests ...)` par :

```cmake
add_executable(box-game-tests
    tests/test_game.c
    src/game.c
    src/ai.c
)
```

- [ ] **Step 4 : Vérifier que ça échoue à la compilation**

Run :
```sh
cmake -B build >/dev/null && cmake --build build --target box-game-tests
```
Expected : ÉCHEC — `fatal error: 'ai.h' file not found`.

- [ ] **Step 5 : Créer `src/ai.h`**

```c
#ifndef AI_H
#define AI_H

#include "game.h"

// Renvoie l'index d'une case vide choisie au hasard, ou -1 si plateau plein.
int AiChooseMove(const Game *g);

#endif // AI_H
```

- [ ] **Step 6 : Créer `src/ai.c`**

```c
#include "ai.h"
#include <stdlib.h>

int AiChooseMove(const Game *g) {
    int empty[9];
    int count = 0;
    for (int i = 0; i < 9; i++) {
        if (g->cells[i] == CELL_EMPTY) empty[count++] = i;
    }
    if (count == 0) return -1;
    return empty[rand() % count];
}
```

- [ ] **Step 7 : Vérifier que les tests passent**

Run :
```sh
cmake -B build >/dev/null && cmake --build build --target box-game-tests && ctest --test-dir build --output-on-failure
```
Expected : `100% tests passed`.

- [ ] **Step 8 : Commit**

```sh
git add src/ai.h src/ai.c tests/test_game.c CMakeLists.txt
git commit -m "feat: IA aléatoire AiChooseMove"
```

---

## Task 7 : Dessin — fenêtre 600×700, pions X/O, barre de statut

**Files:**
- Modify: `src/board.h`, `src/board.c`

> Validation visuelle (pas de test unitaire). `box-game` reste compilable car `main.c` n'utilise pas encore les nouvelles fonctions.

- [ ] **Step 1 : Remplacer le contenu de `src/board.h`**

```c
#ifndef BOARD_H
#define BOARD_H

#include "game.h"

#define BOARD_SIZE 600
#define CELL_SIZE 200
#define STATUS_AREA_HEIGHT 100
#define WINDOW_WIDTH BOARD_SIZE
#define WINDOW_HEIGHT (BOARD_SIZE + STATUS_AREA_HEIGHT)
#define LINE_THICKNESS 4.0f

void DrawBoardGrid(void);
void DrawMarks(const Game *g);
void DrawStatusText(const Game *g);

#endif // BOARD_H
```

- [ ] **Step 2 : Remplacer le contenu de `src/board.c`**

```c
#include "raylib.h"
#include "board.h"

void DrawBoardGrid(void) {
    DrawLineEx((Vector2){CELL_SIZE, 0},
               (Vector2){CELL_SIZE, BOARD_SIZE}, LINE_THICKNESS, DARKGRAY);
    DrawLineEx((Vector2){2 * CELL_SIZE, 0},
               (Vector2){2 * CELL_SIZE, BOARD_SIZE}, LINE_THICKNESS, DARKGRAY);
    DrawLineEx((Vector2){0, CELL_SIZE},
               (Vector2){BOARD_SIZE, CELL_SIZE}, LINE_THICKNESS, DARKGRAY);
    DrawLineEx((Vector2){0, 2 * CELL_SIZE},
               (Vector2){BOARD_SIZE, 2 * CELL_SIZE}, LINE_THICKNESS, DARKGRAY);
}

void DrawMarks(const Game *g) {
    const float margin = 45.0f;
    for (int i = 0; i < 9; i++) {
        int row = i / 3;
        int col = i % 3;
        float x = col * CELL_SIZE;
        float y = row * CELL_SIZE;
        if (g->cells[i] == CELL_X) {
            DrawLineEx((Vector2){x + margin, y + margin},
                       (Vector2){x + CELL_SIZE - margin, y + CELL_SIZE - margin},
                       8.0f, RED);
            DrawLineEx((Vector2){x + CELL_SIZE - margin, y + margin},
                       (Vector2){x + margin, y + CELL_SIZE - margin},
                       8.0f, RED);
        } else if (g->cells[i] == CELL_O) {
            Vector2 center = {x + CELL_SIZE / 2.0f, y + CELL_SIZE / 2.0f};
            float radius = CELL_SIZE / 2.0f - margin;
            DrawRing(center, radius - 6.0f, radius, 0, 360, 64, BLUE);
        }
    }
}

void DrawStatusText(const Game *g) {
    const char *text;
    switch (g->status) {
        case GAME_X_WINS: text = "X gagne ! Clic pour rejouer"; break;
        case GAME_O_WINS: text = "O gagne ! Clic pour rejouer"; break;
        case GAME_DRAW:   text = "Match nul - Clic pour rejouer"; break;
        default:          text = (g->turn == CELL_X) ? "Au tour de X"
                                                     : "Au tour de O"; break;
    }
    DrawText(text, 20, BOARD_SIZE + 35, 28, DARKGRAY);
}
```

- [ ] **Step 3 : Vérifier que tout compile encore**

Run :
```sh
cmake --build build
```
Expected : `box-game` et `box-game-tests` compilent sans erreur (les nouvelles fonctions de dessin ne sont pas encore appelées).

- [ ] **Step 4 : Commit**

```sh
git add src/board.h src/board.c
git commit -m "feat: dessin des pions X/O et barre de statut (fenêtre 600x700)"
```

---

## Task 8 : Boucle de jeu — clics, tour IA, rejoue

**Files:**
- Modify: `src/main.c`, `CMakeLists.txt`

> Validation finale visuelle : jouer une partie complète.

- [ ] **Step 1 : Ajouter `game.c` et `ai.c` à l'exécutable `box-game` — éditer `CMakeLists.txt`**

Remplacer le bloc `add_executable(box-game ...)` par :

```cmake
add_executable(box-game
    src/main.c
    src/board.c
    src/game.c
    src/ai.c
)
```

- [ ] **Step 2 : Remplacer le contenu de `src/main.c`**

```c
#include "raylib.h"
#include <stdlib.h>
#include <time.h>
#include "board.h"
#include "game.h"
#include "ai.h"

int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Box Game — Tic Tac Toe");
    SetTargetFPS(60);
    srand((unsigned int)time(NULL));

    Game game;
    GameInit(&game);

    while (!WindowShouldClose()) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (game.status != GAME_PLAYING) {
                GameInit(&game);                 // rejoue
            } else if (game.turn == CELL_X) {
                Vector2 m = GetMousePosition();
                if (m.y < BOARD_SIZE) {
                    int col = (int)(m.x / CELL_SIZE);
                    int row = (int)(m.y / CELL_SIZE);
                    if (col >= 0 && col < 3 && row >= 0 && row < 3) {
                        GamePlayMove(&game, row * 3 + col);
                    }
                }
            }
        }

        // Tour de l'IA (O), juste après le coup de l'humain
        if (game.status == GAME_PLAYING && game.turn == CELL_O) {
            int move = AiChooseMove(&game);
            if (move >= 0) GamePlayMove(&game, move);
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawBoardGrid();
        DrawMarks(&game);
        DrawStatusText(&game);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
```

- [ ] **Step 3 : Recompiler**

Run :
```sh
cmake -B build >/dev/null && cmake --build build
```
Expected : compilation réussie de `box-game`.

- [ ] **Step 4 : Vérification visuelle — jouer une partie**

Run :
```sh
./build/box-game
```
Expected :
- Fenêtre 600×700, grille en haut, texte « Au tour de X » en bas.
- Clic sur une case vide → un X rouge apparaît, l'IA place aussitôt un O bleu, le texte alterne.
- Quand X ou O aligne 3 pions → « X gagne ! » / « O gagne ! Clic pour rejouer », les clics de jeu sont ignorés.
- Plateau plein sans gagnant → « Match nul - Clic pour rejouer ».
- Un clic après la fin réinitialise une partie vierge.

- [ ] **Step 5 : Lancer toute la suite de tests une dernière fois**

Run :
```sh
ctest --test-dir build --output-on-failure
```
Expected : `100% tests passed`.

- [ ] **Step 6 : Commit**

```sh
git add src/main.c CMakeLists.txt
git commit -m "feat: boucle de jeu (clics, tour IA, rejoue)"
```

---

## Auto-revue du plan

- **Couverture de la spec :**
  - Humain=X au clic, IA=O aléatoire (Task 6 + Task 8) ✓
  - Barre de statut « Au tour de … » / résultats (Task 7 `DrawStatusText`) ✓
  - Fin de partie : message + clic pour rejouer (Task 8 boucle) ✓
  - Architecture game/ai/board/main séparée (Tasks 1, 6, 7, 8) ✓
  - Modèle de données `Cell`/`GameStatus`/`Game` (Task 1) ✓
  - Fenêtre 600×700 (Task 7 constantes) ✓
  - Tests : init, coup légal/illégal, victoires lignes/colonnes/diagonales, nul, IA légale + plateau plein (Tasks 1-6) ✓
- **Placeholders :** aucun — tout le code est complet à chaque étape.
- **Cohérence des types/signatures :** `GameInit(Game*)`, `GamePlayMove(Game*, int)`, `AiChooseMove(const Game*)`, `DrawMarks(const Game*)`, `DrawStatusText(const Game*)`, `DrawBoardGrid(void)` — identiques entre déclarations (`.h`), implémentations (`.c`) et appels (`main.c`, tests). Constantes `BOARD_SIZE`/`CELL_SIZE`/`WINDOW_WIDTH`/`WINDOW_HEIGHT` cohérentes entre `board.h`, `board.c` et `main.c`.
