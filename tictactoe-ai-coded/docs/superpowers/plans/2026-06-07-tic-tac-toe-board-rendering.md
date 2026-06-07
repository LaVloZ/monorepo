# Plan d'implémentation — Fondation « fenêtre + grille 3×3 »

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ouvrir une fenêtre et y dessiner une grille de morpion 3×3 vide, en C avec Raylib.

**Architecture:** Boucle de jeu Raylib standard dans `main.c`, dessin de la grille isolé dans un module `board.c`/`board.h` pour préparer les étapes suivantes (X/O, règles, LLM).

**Tech Stack:** C (C11), Raylib (via Homebrew), CMake.

**Note sur les tests :** cette étape produit une sortie purement visuelle. Il n'y a pas de tests unitaires ; la vérification de chaque tâche est « le build réussit et la fenêtre/grille s'affiche ». Les tests automatisés arriveront à l'étape 2 (règles du jeu).

---

## Structure des fichiers

- `CMakeLists.txt` — Création. Config de build C, trouve Raylib, déclare l'exécutable `box-game`.
- `src/main.c` — Création. Point d'entrée : init fenêtre + boucle de jeu.
- `src/board.h` — Création. Constantes de géométrie + déclaration de `DrawBoardGrid()`.
- `src/board.c` — Création. Implémentation de `DrawBoardGrid()`.
- `.gitignore` — Création. Ignore le dossier `build/`.

---

## Task 0 : Prérequis — installer Raylib

- [ ] **Step 1 : Installer Raylib via Homebrew**

Run :
```sh
brew install raylib
```
Expected : Raylib s'installe (ou « already installed »). Vérifier :
```sh
brew list raylib >/dev/null && echo OK
```
Expected : affiche `OK`.

---

## Task 1 : Fenêtre vide qui s'ouvre et se ferme proprement

**Files:**
- Create: `.gitignore`
- Create: `CMakeLists.txt`
- Create: `src/main.c`

- [ ] **Step 1 : Créer `.gitignore`**

```gitignore
build/
```

- [ ] **Step 2 : Créer `CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.16)
project(box_game C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(raylib REQUIRED)

add_executable(box-game
    src/main.c
)
target_link_libraries(box-game PRIVATE raylib)
```

- [ ] **Step 3 : Créer `src/main.c` (fenêtre vide)**

```c
#include "raylib.h"

#define WINDOW_SIZE 600

int main(void) {
    InitWindow(WINDOW_SIZE, WINDOW_SIZE, "Box Game — Tic Tac Toe");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
```

- [ ] **Step 4 : Configurer et compiler**

Run :
```sh
cmake -B build && cmake --build build
```
Expected : configuration et compilation réussies, binaire `build/box-game` créé, aucune erreur.

- [ ] **Step 5 : Lancer et vérifier visuellement**

Run :
```sh
./build/box-game
```
Expected : une fenêtre 600×600 blanche au titre « Box Game — Tic Tac Toe » s'ouvre. Elle se ferme via la croix ou la touche Échap, sans crash.

- [ ] **Step 6 : Commit**

```sh
git add .gitignore CMakeLists.txt src/main.c
git commit -m "feat: fenêtre Raylib vide (fondation)"
```

---

## Task 2 : Dessiner la grille 3×3

**Files:**
- Create: `src/board.h`
- Create: `src/board.c`
- Modify: `src/main.c` (inclure board.h, appeler DrawBoardGrid, retirer le #define local)
- Modify: `CMakeLists.txt` (ajouter src/board.c aux sources)

- [ ] **Step 1 : Créer `src/board.h`**

```c
#ifndef BOARD_H
#define BOARD_H

#define WINDOW_SIZE 600
#define CELL_SIZE 200
#define LINE_THICKNESS 4.0f

// Dessine les 4 lignes de la grille 3x3 (le "#").
void DrawBoardGrid(void);

#endif // BOARD_H
```

- [ ] **Step 2 : Créer `src/board.c`**

```c
#include "raylib.h"
#include "board.h"

void DrawBoardGrid(void) {
    // 2 lignes verticales (x = 200 et x = 400)
    DrawLineEx((Vector2){CELL_SIZE, 0},
               (Vector2){CELL_SIZE, WINDOW_SIZE}, LINE_THICKNESS, DARKGRAY);
    DrawLineEx((Vector2){2 * CELL_SIZE, 0},
               (Vector2){2 * CELL_SIZE, WINDOW_SIZE}, LINE_THICKNESS, DARKGRAY);

    // 2 lignes horizontales (y = 200 et y = 400)
    DrawLineEx((Vector2){0, CELL_SIZE},
               (Vector2){WINDOW_SIZE, CELL_SIZE}, LINE_THICKNESS, DARKGRAY);
    DrawLineEx((Vector2){0, 2 * CELL_SIZE},
               (Vector2){WINDOW_SIZE, 2 * CELL_SIZE}, LINE_THICKNESS, DARKGRAY);
}
```

- [ ] **Step 3 : Modifier `src/main.c`**

Remplacer tout le contenu par (on retire le `#define WINDOW_SIZE` local, désormais fourni par `board.h`, et on appelle `DrawBoardGrid`) :

```c
#include "raylib.h"
#include "board.h"

int main(void) {
    InitWindow(WINDOW_SIZE, WINDOW_SIZE, "Box Game — Tic Tac Toe");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawBoardGrid();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
```

- [ ] **Step 4 : Modifier `CMakeLists.txt` (ajouter board.c)**

Remplacer le bloc `add_executable(...)` par :

```cmake
add_executable(box-game
    src/main.c
    src/board.c
)
```

- [ ] **Step 5 : Recompiler**

Run :
```sh
cmake --build build
```
Expected : compilation réussie, aucune erreur ni warning.

- [ ] **Step 6 : Lancer et vérifier visuellement**

Run :
```sh
./build/box-game
```
Expected : la fenêtre affiche une grille de morpion 3×3 nette — 2 traits verticaux et 2 horizontaux gris foncé sur fond blanc, divisant la fenêtre en 9 cellules égales de 200×200 px.

- [ ] **Step 7 : Commit**

```sh
git add src/board.h src/board.c src/main.c CMakeLists.txt
git commit -m "feat: dessin de la grille 3x3"
```

---

## Auto-revue du plan

- **Couverture de la spec :** fenêtre 600×600 (Task 1) ✓ ; grille 3×3 centrée cellules 200px, lignes en 200/400 (Task 2) ✓ ; séparation main/board (Task 2) ✓ ; build CMake + Raylib Homebrew (Task 0/1) ✓ ; validation visuelle, pas de tests auto ✓.
- **Placeholders :** aucun — tout le code est complet.
- **Cohérence des types :** `WINDOW_SIZE`, `CELL_SIZE`, `LINE_THICKNESS` définis dans `board.h` et utilisés tels quels dans `board.c` et `main.c` ; `DrawBoardGrid(void)` déclarée et appelée à l'identique.
