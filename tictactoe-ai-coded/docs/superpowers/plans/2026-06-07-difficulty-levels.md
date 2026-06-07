# Plan d'implémentation — Niveaux de difficulté (facile + moyen)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ajouter un écran de menu pour choisir la difficulté, avec un niveau Facile (aléatoire) et un niveau Moyen (heuristique) ; Difficile reste grisé pour plus tard.

**Architecture:** `ai.c` devient un sélecteur de coup par `Difficulty`. La décision testable s'appuie sur un nouveau helper `GameIsWinningMove` (game.c). `main.c` gère une machine à états menu/partie. Chaque tâche garde `box-game` ET `box-game-tests` compilables et verts.

**Tech Stack:** C (C11), Raylib, CMake + CTest.

---

## Structure des fichiers

- `src/game.h` / `src/game.c` — Modification. Ajout `GameIsWinningMove`.
- `src/ai.h` / `src/ai.c` — Modification. Enum `Difficulty`, nouvelle signature `AiChooseMove`, heuristique moyenne.
- `src/board.h` / `src/board.c` — Modification. `DrawMenu`, `GetMenuButtonRect`, `DrawStatusText` avec difficulté.
- `src/main.c` — Modification. Machine à états `STATE_MENU`/`STATE_PLAYING`, Échap, sélection au clic.
- `tests/test_game.c` — Modification. Tests `GameIsWinningMove` + heuristique ; mise à jour des appels existants.

---

## Task 1 : `GameIsWinningMove`

**Files:**
- Modify: `tests/test_game.c`, `src/game.h`, `src/game.c`

- [ ] **Step 1 : Ajouter le test (avant `int main`) dans `tests/test_game.c`**

```c
static void test_is_winning_move(void) {
    Game g;
    GameInit(&g);
    g.cells[0] = CELL_X;
    g.cells[1] = CELL_X;
    assert(GameIsWinningMove(&g, 2, CELL_X) == true);   // complète 0,1,2
    assert(GameIsWinningMove(&g, 2, CELL_O) == false);  // O ne gagne pas là
    assert(GameIsWinningMove(&g, 0, CELL_X) == false);  // case occupée
    assert(g.cells[2] == CELL_EMPTY);                   // n'a pas muté g
}
```

- [ ] **Step 2 : L'appeler dans `main()`**

```c
    test_is_winning_move();
```

- [ ] **Step 3 : Déclarer dans `src/game.h` (après `GamePlayMove`)**

```c
// true si jouer `player` à `index` (case supposée vide) complète une ligne gagnante.
// Ne modifie pas le Game.
bool GameIsWinningMove(const Game *g, int index, Cell player);
```

- [ ] **Step 4 : Vérifier l'échec de compilation**

Run :
```sh
cmake --build build --target box-game-tests
```
Expected : ÉCHEC — `undefined reference to 'GameIsWinningMove'` (ou symbole manquant).

- [ ] **Step 5 : Implémenter dans `src/game.c` (à la fin du fichier)**

```c
bool GameIsWinningMove(const Game *g, int index, Cell player) {
    if (index < 0 || index > 8) return false;
    if (g->cells[index] != CELL_EMPTY) return false;
    Game copy = *g;
    copy.cells[index] = player;
    return HasWon(&copy, player);
}
```

- [ ] **Step 6 : Vérifier que les tests passent**

Run :
```sh
cmake --build build --target box-game-tests && ctest --test-dir build --output-on-failure
```
Expected : `100% tests passed`.

- [ ] **Step 7 : Commit**

```sh
git add tests/test_game.c src/game.h src/game.c
git commit -m "feat: GameIsWinningMove (helper de décision)"
```

---

## Task 2 : `Difficulty` + nouvelle signature `AiChooseMove`

**Files:**
- Modify: `src/ai.h`, `src/ai.c`, `tests/test_game.c`, `src/main.c`

> Refactor : on introduit le paramètre `Difficulty` ; toutes les difficultés font encore un coup aléatoire (l'heuristique arrive en Task 3). Build `box-game` maintenu vert via une variable `difficulty` temporaire dans `main.c`.

- [ ] **Step 1 : Remplacer le contenu de `src/ai.h`**

```c
#ifndef AI_H
#define AI_H

#include "game.h"

typedef enum { DIFFICULTY_EASY, DIFFICULTY_MEDIUM, DIFFICULTY_HARD } Difficulty;

// Choisit un coup selon la difficulté. Renvoie une case vide, ou -1 si plateau plein.
int AiChooseMove(const Game *g, Difficulty difficulty);

#endif // AI_H
```

- [ ] **Step 2 : Remplacer le contenu de `src/ai.c`**

```c
#include "ai.h"
#include <stdlib.h>

static int ChooseRandom(const Game *g) {
    int empty[9];
    int count = 0;
    for (int i = 0; i < 9; i++) {
        if (g->cells[i] == CELL_EMPTY) empty[count++] = i;
    }
    if (count == 0) return -1;
    return empty[rand() % count];
}

int AiChooseMove(const Game *g, Difficulty difficulty) {
    (void)difficulty;
    return ChooseRandom(g);
}
```

- [ ] **Step 3 : Mettre à jour les appels de test dans `tests/test_game.c`**

Dans `test_ai_returns_empty_cell`, remplacer `AiChooseMove(&g)` par :
```c
        int m = AiChooseMove(&g, DIFFICULTY_EASY);
```
Dans `test_ai_full_board_returns_minus_one`, remplacer la ligne d'assertion par :
```c
    assert(AiChooseMove(&g, DIFFICULTY_EASY) == -1);
```

- [ ] **Step 4 : Mettre à jour `src/main.c` (variable difficulté + appel)**

Après la ligne `Game game;` ... en fait ajouter la déclaration juste avant `Game game;` :
```c
    Difficulty difficulty = DIFFICULTY_EASY;
```
Et remplacer la ligne `int move = AiChooseMove(&game);` par :
```c
            int move = AiChooseMove(&game, difficulty);
```

- [ ] **Step 5 : Vérifier build complet + tests**

Run :
```sh
cmake --build build && ctest --test-dir build --output-on-failure
```
Expected : `box-game` et `box-game-tests` compilent ; `100% tests passed`.

- [ ] **Step 6 : Commit**

```sh
git add src/ai.h src/ai.c tests/test_game.c src/main.c
git commit -m "refactor: AiChooseMove prend une Difficulty"
```

---

## Task 3 : Heuristique du niveau Moyen

**Files:**
- Modify: `tests/test_game.c`, `src/ai.c`

- [ ] **Step 1 : Ajouter les tests (avant `int main`) dans `tests/test_game.c`**

```c
static void test_medium_wins(void) {
    Game g;
    GameInit(&g);
    g.turn = CELL_O;
    g.cells[0] = CELL_O;
    g.cells[1] = CELL_O;   // O gagne en jouant 2
    assert(AiChooseMove(&g, DIFFICULTY_MEDIUM) == 2);
}

static void test_medium_blocks(void) {
    Game g;
    GameInit(&g);
    g.turn = CELL_O;
    g.cells[0] = CELL_X;
    g.cells[1] = CELL_X;   // X menace en 2, O ne peut pas gagner -> bloque 2
    assert(AiChooseMove(&g, DIFFICULTY_MEDIUM) == 2);
}

static void test_medium_takes_center(void) {
    Game g;
    GameInit(&g);
    g.turn = CELL_O;
    g.cells[0] = CELL_X;   // pas de menace de ligne, centre libre
    assert(AiChooseMove(&g, DIFFICULTY_MEDIUM) == 4);
}
```

- [ ] **Step 2 : Les appeler dans `main()`**

```c
    test_medium_wins();
    test_medium_blocks();
    test_medium_takes_center();
```

- [ ] **Step 3 : Vérifier l'échec**

Run :
```sh
cmake --build build --target box-game-tests && ctest --test-dir build --output-on-failure
```
Expected : ÉCHEC (le coup moyen est encore aléatoire, ne renvoie pas la case attendue de façon fiable).

- [ ] **Step 4 : Implémenter — éditer `src/ai.c`**

Ajouter la fonction `ChooseMedium` avant `AiChooseMove` :

```c
static int ChooseMedium(const Game *g) {
    Cell me = g->turn;
    Cell opp = (me == CELL_X) ? CELL_O : CELL_X;

    // 1. gagner si possible
    for (int i = 0; i < 9; i++)
        if (g->cells[i] == CELL_EMPTY && GameIsWinningMove(g, i, me)) return i;
    // 2. sinon bloquer l'adversaire
    for (int i = 0; i < 9; i++)
        if (g->cells[i] == CELL_EMPTY && GameIsWinningMove(g, i, opp)) return i;
    // 3. centre
    if (g->cells[4] == CELL_EMPTY) return 4;
    // 4. un coin libre au hasard
    int corners[4] = {0, 2, 6, 8};
    int freeCorners[4];
    int cc = 0;
    for (int k = 0; k < 4; k++)
        if (g->cells[corners[k]] == CELL_EMPTY) freeCorners[cc++] = corners[k];
    if (cc > 0) return freeCorners[rand() % cc];
    // 5. aléatoire
    return ChooseRandom(g);
}
```

Puis remplacer `AiChooseMove` par :

```c
int AiChooseMove(const Game *g, Difficulty difficulty) {
    switch (difficulty) {
        case DIFFICULTY_EASY:   return ChooseRandom(g);
        case DIFFICULTY_MEDIUM: return ChooseMedium(g);
        case DIFFICULTY_HARD:   return ChooseMedium(g); // repli temporaire
        default:                return ChooseRandom(g);
    }
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
git add tests/test_game.c src/ai.c
git commit -m "feat: heuristique du niveau moyen (gagne/bloque/centre/coin)"
```

---

## Task 4 : Menu + statut avec difficulté (dessin)

**Files:**
- Modify: `src/board.h`, `src/board.c`, `src/main.c`

> Validation visuelle. Build maintenu vert : on met aussi à jour l'appel à `DrawStatusText` dans `main.c`.

- [ ] **Step 1 : Remplacer le contenu de `src/board.h`**

```c
#ifndef BOARD_H
#define BOARD_H

#include "raylib.h"
#include "game.h"
#include "ai.h"

#define BOARD_SIZE 600
#define CELL_SIZE 200
#define STATUS_AREA_HEIGHT 100
#define WINDOW_WIDTH BOARD_SIZE
#define WINDOW_HEIGHT (BOARD_SIZE + STATUS_AREA_HEIGHT)
#define LINE_THICKNESS 4.0f

void DrawBoardGrid(void);
void DrawMarks(const Game *g);
void DrawStatusText(const Game *g, Difficulty difficulty);
void DrawMenu(void);
Rectangle GetMenuButtonRect(int index);

#endif // BOARD_H
```

- [ ] **Step 2 : Remplacer le contenu de `src/board.c`**

```c
#include "raylib.h"
#include <stdio.h>
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

static const char *DifficultyLabel(Difficulty d) {
    switch (d) {
        case DIFFICULTY_EASY:   return "Facile";
        case DIFFICULTY_MEDIUM: return "Moyen";
        case DIFFICULTY_HARD:   return "Difficile";
        default:                return "?";
    }
}

void DrawStatusText(const Game *g, Difficulty difficulty) {
    const char *base;
    switch (g->status) {
        case GAME_X_WINS: base = "X gagne ! Clic pour rejouer"; break;
        case GAME_O_WINS: base = "O gagne ! Clic pour rejouer"; break;
        case GAME_DRAW:   base = "Match nul - Clic pour rejouer"; break;
        default:          base = (g->turn == CELL_X) ? "Au tour de X"
                                                     : "Au tour de O"; break;
    }
    char buffer[80];
    snprintf(buffer, sizeof(buffer), "%s - %s", base, DifficultyLabel(difficulty));
    DrawText(buffer, 20, BOARD_SIZE + 35, 24, DARKGRAY);
}

Rectangle GetMenuButtonRect(int index) {
    float w = 280.0f;
    float h = 70.0f;
    float x = (WINDOW_WIDTH - w) / 2.0f;
    float y = 220.0f + index * (h + 30.0f);
    return (Rectangle){x, y, w, h};
}

void DrawMenu(void) {
    const char *title = "Tic Tac Toe";
    int titleWidth = MeasureText(title, 50);
    DrawText(title, (WINDOW_WIDTH - titleWidth) / 2, 110, 50, DARKGRAY);

    const char *labels[3] = {"Facile", "Moyen", "Difficile"};
    for (int i = 0; i < 3; i++) {
        Rectangle r = GetMenuButtonRect(i);
        bool disabled = (i == 2);
        Color fill = disabled ? LIGHTGRAY : SKYBLUE;
        Color textColor = disabled ? GRAY : DARKBLUE;
        DrawRectangleRec(r, fill);
        DrawRectangleLinesEx(r, 3, DARKGRAY);
        int tw = MeasureText(labels[i], 30);
        DrawText(labels[i], (int)(r.x + (r.width - tw) / 2),
                 (int)(r.y + (r.height - 30) / 2), 30, textColor);
    }
    const char *hint = "Difficile : bientot disponible";
    int hw = MeasureText(hint, 18);
    DrawText(hint, (WINDOW_WIDTH - hw) / 2, 560, 18, GRAY);
}
```

- [ ] **Step 3 : Mettre à jour l'appel dans `src/main.c`**

Remplacer la ligne `DrawStatusText(&game);` par :
```c
        DrawStatusText(&game, difficulty);
```

- [ ] **Step 4 : Vérifier que tout compile**

Run :
```sh
cmake --build build
```
Expected : compilation OK des deux cibles.

- [ ] **Step 5 : Commit**

```sh
git add src/board.h src/board.c src/main.c
git commit -m "feat: écran de menu + difficulté dans la barre de statut"
```

---

## Task 5 : Machine à états menu/partie dans `main.c`

**Files:**
- Modify: `src/main.c`

> Validation finale visuelle. `SetExitKey(KEY_NULL)` désactive la fermeture par Échap (par défaut dans Raylib) pour réutiliser Échap → retour menu ; on quitte par la croix de la fenêtre.

- [ ] **Step 1 : Remplacer tout le contenu de `src/main.c`**

```c
#include "raylib.h"
#include <stdlib.h>
#include <time.h>
#include "board.h"
#include "game.h"
#include "ai.h"

typedef enum { STATE_MENU, STATE_PLAYING } AppState;

int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Box Game — Tic Tac Toe");
    SetExitKey(KEY_NULL);   // Échap ne ferme plus la fenêtre
    SetTargetFPS(60);
    srand((unsigned int)time(NULL));

    AppState state = STATE_MENU;
    Difficulty difficulty = DIFFICULTY_EASY;
    Game game;
    GameInit(&game);

    while (!WindowShouldClose()) {
        if (state == STATE_MENU) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 m = GetMousePosition();
                for (int i = 0; i < 2; i++) { // 0=Facile, 1=Moyen (2=Difficile désactivé)
                    if (CheckCollisionPointRec(m, GetMenuButtonRect(i))) {
                        difficulty = (i == 0) ? DIFFICULTY_EASY : DIFFICULTY_MEDIUM;
                        GameInit(&game);
                        state = STATE_PLAYING;
                    }
                }
            }
            BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawMenu();
            EndDrawing();
        } else { // STATE_PLAYING
            if (IsKeyPressed(KEY_ESCAPE)) {
                state = STATE_MENU;
            } else {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (game.status != GAME_PLAYING) {
                        GameInit(&game);
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
                if (game.status == GAME_PLAYING && game.turn == CELL_O) {
                    int move = AiChooseMove(&game, difficulty);
                    if (move >= 0) GamePlayMove(&game, move);
                }
            }
            BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawBoardGrid();
            DrawMarks(&game);
            DrawStatusText(&game, difficulty);
            EndDrawing();
        }
    }

    CloseWindow();
    return 0;
}
```

- [ ] **Step 2 : Recompiler**

Run :
```sh
cmake --build build
```
Expected : compilation réussie.

- [ ] **Step 3 : Vérification visuelle**

Run :
```sh
./build/box-game
```
Expected :
- Au lancement : menu « Tic Tac Toe » + boutons Facile / Moyen (bleus) et Difficile (gris).
- Clic Facile ou Moyen → la partie démarre, statut « Au tour de X - Facile/Moyen ».
- En Moyen, l'IA bloque les alignements et joue le centre/les coins (nettement plus dure qu'en Facile).
- Échap → retour au menu ; on peut rechoisir un niveau.
- Clic sur le bouton Difficile (gris) → aucun effet.

- [ ] **Step 4 : Lancer toute la suite de tests**

Run :
```sh
ctest --test-dir build --output-on-failure
```
Expected : `100% tests passed`.

- [ ] **Step 5 : Commit**

```sh
git add src/main.c
git commit -m "feat: machine à états menu/partie + Échap retour menu"
```

---

## Auto-revue du plan

- **Couverture de la spec :**
  - Menu avec 3 boutons, Difficile grisé/ignoré (Task 4 `DrawMenu` + Task 5 boucle 0..1) ✓
  - Facile = aléatoire, Moyen = heuristique (Tasks 2, 3) ✓
  - Échap → menu, `SetExitKey(KEY_NULL)` (Task 5) ✓
  - Niveau affiché au statut (Task 4 `DrawStatusText`) ✓
  - `GameIsWinningMove` (Task 1) ✓
  - `AiChooseMove(g, Difficulty)` (Task 2) ✓
  - Tests heuristique gagne/bloque/centre + `GameIsWinningMove` + maj des appels (Tasks 1-3) ✓
- **Placeholders :** aucun — code complet à chaque étape.
- **Cohérence des types/signatures :** `GameIsWinningMove(const Game*, int, Cell)`, `AiChooseMove(const Game*, Difficulty)`, `DrawStatusText(const Game*, Difficulty)`, `DrawMenu(void)`, `GetMenuButtonRect(int)→Rectangle`, `AppState`/`Difficulty` cohérents entre `.h`, `.c`, `main.c` et tests. `board.h` inclut `raylib.h` (type `Rectangle`) et `ai.h` (type `Difficulty`).
