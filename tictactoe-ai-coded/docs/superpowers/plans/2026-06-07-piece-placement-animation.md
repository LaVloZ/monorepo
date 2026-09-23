# Plan d'implémentation — Animation d'apparition des pions

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Les pions (X et O) se tracent progressivement à l'apparition, avec un court délai avant le coup de l'IA, sans changer la logique de jeu.

**Architecture:** Le rendu devient piloté par le temps (`GetFrameTime`). `board.c` dessine des pions partiels selon un `progress[9]`. `main.c` gère un sous-état de jeu PLAY_IDLE / PLAY_ANIMATING / PLAY_WAIT_AI. `game.c` et `ai.c` ne changent pas → les tests existants restent verts.

**Tech Stack:** C (C11), Raylib, CMake + CTest.

**Validation :** visuelle (animation) + tests existants verts. Pas de nouveau test unitaire (logique inchangée, animation purement temporelle).

---

## Structure des fichiers

- `src/board.h` / `src/board.c` — Modification. `DrawMarks` prend `progress[9]` et dessine des pions partiels.
- `src/main.c` — Modification. Tableau `progress`, machine à états temporelle, délai IA.

---

## Task 1 : Dessin partiel des pions (`DrawMarks` avec progress)

**Files:**
- Modify: `src/board.h`, `src/board.c`, `src/main.c`

> Après cette tâche, le comportement visuel est identique à aujourd'hui (pions pleins instantanés) car `main.c` passe un progress de 1 partout. L'animation arrive en Task 2.

- [ ] **Step 1 : Modifier la déclaration dans `src/board.h`**

Remplacer `void DrawMarks(const Game *g);` par :
```c
void DrawMarks(const Game *g, const float progress[9]);
```

- [ ] **Step 2 : Remplacer la fonction `DrawMarks` dans `src/board.c`**

Remplacer entièrement la fonction `DrawMarks` existante par (et ajouter le helper `Clamp01` juste avant) :

```c
static float Clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

void DrawMarks(const Game *g, const float progress[9]) {
    const float margin = 45.0f;
    for (int i = 0; i < 9; i++) {
        if (g->cells[i] == CELL_EMPTY) continue;
        float p = progress[i];
        int row = i / 3;
        int col = i % 3;
        float x = col * CELL_SIZE;
        float y = row * CELL_SIZE;
        if (g->cells[i] == CELL_X) {
            float f1 = Clamp01(p / 0.5f);
            float f2 = Clamp01((p - 0.5f) / 0.5f);
            Vector2 a1 = {x + margin, y + margin};
            Vector2 b1 = {x + CELL_SIZE - margin, y + CELL_SIZE - margin};
            if (f1 > 0.0f) {
                DrawLineEx(a1,
                    (Vector2){a1.x + f1 * (b1.x - a1.x), a1.y + f1 * (b1.y - a1.y)},
                    8.0f, RED);
            }
            Vector2 a2 = {x + CELL_SIZE - margin, y + margin};
            Vector2 b2 = {x + margin, y + CELL_SIZE - margin};
            if (f2 > 0.0f) {
                DrawLineEx(a2,
                    (Vector2){a2.x + f2 * (b2.x - a2.x), a2.y + f2 * (b2.y - a2.y)},
                    8.0f, RED);
            }
        } else { // CELL_O
            Vector2 center = {x + CELL_SIZE / 2.0f, y + CELL_SIZE / 2.0f};
            float radius = CELL_SIZE / 2.0f - margin;
            if (p > 0.0f) {
                DrawRing(center, radius - 6.0f, radius, 0.0f, p * 360.0f, 64, BLUE);
            }
        }
    }
}
```

- [ ] **Step 3 : Adapter `src/main.c` (progress plein temporaire)**

Après la ligne `GameInit(&game);` dans `main`, ajouter :
```c
    float progress[9];
    for (int i = 0; i < 9; i++) progress[i] = 1.0f;
```
Puis remplacer la ligne `DrawMarks(&game);` par :
```c
        DrawMarks(&game, progress);
```

- [ ] **Step 4 : Build + tests existants**

Run :
```sh
cmake --build build && ctest --test-dir build --output-on-failure
```
Expected : compilation OK des deux cibles ; `100% tests passed`.

- [ ] **Step 5 : Vérification visuelle rapide**

Run :
```sh
./build/box-game
```
Expected : comportement identique à avant — menu, puis pions pleins qui apparaissent instantanément (pas encore d'animation).

- [ ] **Step 6 : Commit**

```sh
git add src/board.h src/board.c src/main.c
git commit -m "feat: DrawMarks dessine des pions partiels (progress)"
```

---

## Task 2 : Boucle pilotée par le temps + animation + délai IA

**Files:**
- Modify: `src/main.c`

> Validation finale visuelle. `game.c`/`ai.c` inchangés → tests toujours verts.

- [ ] **Step 1 : Remplacer tout le contenu de `src/main.c`**

```c
#include "raylib.h"
#include <stdlib.h>
#include <time.h>
#include "board.h"
#include "game.h"
#include "ai.h"

typedef enum { STATE_MENU, STATE_PLAYING } AppState;
typedef enum { PLAY_IDLE, PLAY_ANIMATING, PLAY_WAIT_AI } PlayPhase;

#define ANIM_DURATION 0.20f
#define AI_DELAY 0.35f

static void ResetProgress(float progress[9]) {
    for (int i = 0; i < 9; i++) progress[i] = 0.0f;
}

int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Box Game — Tic Tac Toe");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    srand((unsigned int)time(NULL));

    AppState state = STATE_MENU;
    Difficulty difficulty = DIFFICULTY_EASY;
    Game game;
    GameInit(&game);

    float progress[9];
    ResetProgress(progress);
    PlayPhase phase = PLAY_IDLE;
    int animCell = -1;
    float animTime = 0.0f;
    float aiWait = 0.0f;

    while (!WindowShouldClose()) {
        if (state == STATE_MENU) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 m = GetMousePosition();
                for (int i = 0; i < 2; i++) { // 0=Facile, 1=Moyen
                    if (CheckCollisionPointRec(m, GetMenuButtonRect(i))) {
                        difficulty = (i == 0) ? DIFFICULTY_EASY : DIFFICULTY_MEDIUM;
                        GameInit(&game);
                        ResetProgress(progress);
                        phase = PLAY_IDLE;
                        state = STATE_PLAYING;
                    }
                }
            }
            BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawMenu();
            EndDrawing();
        } else { // STATE_PLAYING
            float dt = GetFrameTime();

            if (IsKeyPressed(KEY_ESCAPE)) {
                state = STATE_MENU;
            } else if (phase == PLAY_IDLE) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (game.status != GAME_PLAYING) {
                        GameInit(&game);
                        ResetProgress(progress);
                    } else if (game.turn == CELL_X) {
                        Vector2 m = GetMousePosition();
                        if (m.y < BOARD_SIZE) {
                            int col = (int)(m.x / CELL_SIZE);
                            int row = (int)(m.y / CELL_SIZE);
                            if (col >= 0 && col < 3 && row >= 0 && row < 3) {
                                int index = row * 3 + col;
                                if (GamePlayMove(&game, index)) {
                                    animCell = index;
                                    progress[index] = 0.0f;
                                    animTime = 0.0f;
                                    phase = PLAY_ANIMATING;
                                }
                            }
                        }
                    }
                }
            } else if (phase == PLAY_ANIMATING) {
                animTime += dt;
                float p = animTime / ANIM_DURATION;
                if (p >= 1.0f) p = 1.0f;
                progress[animCell] = p;
                if (p >= 1.0f) {
                    if (game.status == GAME_PLAYING && game.turn == CELL_O) {
                        phase = PLAY_WAIT_AI;
                        aiWait = 0.0f;
                    } else {
                        phase = PLAY_IDLE;
                    }
                }
            } else { // PLAY_WAIT_AI
                aiWait += dt;
                if (aiWait >= AI_DELAY) {
                    int move = AiChooseMove(&game, difficulty);
                    if (move >= 0 && GamePlayMove(&game, move)) {
                        animCell = move;
                        progress[move] = 0.0f;
                        animTime = 0.0f;
                        phase = PLAY_ANIMATING;
                    } else {
                        phase = PLAY_IDLE;
                    }
                }
            }

            BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawBoardGrid();
            DrawMarks(&game, progress);
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
- Clic → ton X se trace trait par trait (~0,2 s).
- Court instant (~0,35 s) puis le O de l'IA se trace en arc qui se referme.
- Impossible de cliquer pendant qu'un pion s'anime ou pendant l'attente de l'IA.
- Fin de partie : message ; clic → nouvelle partie (les pions de la nouvelle partie s'animent aussi).
- Échap → menu, fonctionne toujours.

- [ ] **Step 4 : Tests existants**

Run :
```sh
ctest --test-dir build --output-on-failure
```
Expected : `100% tests passed`.

- [ ] **Step 5 : Commit**

```sh
git add src/main.c
git commit -m "feat: animation du tracé des pions + délai IA"
```

---

## Auto-revue du plan

- **Couverture de la spec :**
  - Tracé progressif X (deux traits) et O (arc) — Task 1 `DrawMarks` ✓
  - Animation pour X et O — Task 2 (chaque coup déclenche PLAY_ANIMATING) ✓
  - Délai ~0,35 s avant le coup IA — Task 2 `PLAY_WAIT_AI` / `AI_DELAY` ✓
  - Clics ignorés pendant animation/attente — Task 2 (clic traité seulement en `PLAY_IDLE`) ✓
  - Reste inchangé (menu, difficulté, rejoue, Échap) — Task 2 ✓
  - Logique inchangée, tests existants verts — Tasks 1 & 2 (étapes de test) ✓
- **Placeholders :** aucun — code complet.
- **Cohérence des types/signatures :** `DrawMarks(const Game*, const float[9])` (board.h, board.c, main.c) ; `PlayPhase`/`AppState` ; constantes `ANIM_DURATION`/`AI_DELAY` ; `progress[9]` réinitialisé via `ResetProgress` à chaque nouvelle partie. Invariant : seules les cases non vides sont dessinées, leur `progress` est piloté 0→1.
