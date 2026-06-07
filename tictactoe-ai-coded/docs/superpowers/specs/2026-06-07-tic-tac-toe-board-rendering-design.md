# Box Game — Étape 1 : Fondation « fenêtre + grille 3×3 »

**Date :** 2026-06-07
**Langage :** C (C11)
**Librairie graphique :** Raylib (installée via Homebrew)
**Plateforme de dev :** macOS

## Contexte

Projet d'apprentissage du développement de jeux vidéo. L'auteur est
développeur mais débutant en jeux vidéo ; le morpion (tic-tac-toe) sert
de support pédagogique. Objectif final : un humain affronte un LLM
(Claude) au morpion, en tour par tour.

Le travail est découpé en 3 étapes successives, construites une à la fois :

1. **🎯 Cette spec — Dessiner le plateau à l'écran** (fenêtre + grille 3×3 vide)
2. Plus tard — Règles du jeu (clic pour placer X/O, détection victoire/nul)
3. Plus tard — Adversaire piloté par Claude

La partie « dessin » est celle qui intimide le plus l'auteur ; on l'isole
donc et on la traite en premier, seule.

## Périmètre de cette étape

**Inclus :**
- Ouvrir une fenêtre Raylib.
- Y dessiner une grille de morpion 3×3 vide (les 4 traits du `#`).
- Boucle de jeu standard qui maintient la fenêtre ouverte jusqu'à
  fermeture par l'utilisateur.

**Explicitement exclus (étapes ultérieures) :**
- Aucune gestion du clic / des entrées de jeu.
- Aucun X ni O dessiné.
- Aucune règle, aucun état de partie, aucune détection de victoire.
- Aucune intégration LLM.

**Critère de succès :** lancer l'exécutable affiche une fenêtre avec une
grille 3×3 nette et centrée ; la fenêtre se ferme proprement.

## Architecture

Boucle de jeu Raylib classique. On sépare dès maintenant « la boucle » du
« dessin » pour que les étapes 2 et 3 s'y greffent sans refonte.

```
box-game/
├── CMakeLists.txt          # build C, trouve Raylib (Homebrew)
└── src/
    ├── main.c              # init fenêtre + boucle de jeu
    ├── board.h             # déclaration de DrawBoardGrid() + constantes
    └── board.c             # dessin de la grille
```

### `main.c`
Point d'entrée. Responsabilités :
- `InitWindow(WINDOW_SIZE, WINDOW_SIZE, "Box Game — Tic Tac Toe")`
- `SetTargetFPS(60)`
- Boucle : `while (!WindowShouldClose()) { BeginDrawing();
  ClearBackground(...); DrawBoardGrid(); EndDrawing(); }`
- `CloseWindow()` à la sortie.

### `board.h` / `board.c`
Module de dessin du plateau.
- `board.h` expose `void DrawBoardGrid(void);` et les constantes de
  géométrie (taille fenêtre, taille cellule, épaisseur des lignes).
- `board.c` implémente `DrawBoardGrid()` : trace 2 lignes verticales et
  2 lignes horizontales avec `DrawLineEx` (lignes épaisses).
- C'est ce module qui, à l'étape 2, accueillera l'état du plateau et le
  dessin des X / O.

## Détails visuels

| Élément          | Valeur                                   |
|------------------|------------------------------------------|
| Taille fenêtre   | 600 × 600 px                             |
| Titre            | « Box Game — Tic Tac Toe »               |
| Grille           | 3×3, centrée, cellules de 200 px         |
| Lignes           | 2 verticales + 2 horizontales, ~4 px     |
| Couleur fond     | clair (ex. `RAYWHITE`)                   |
| Couleur lignes   | sombre (ex. `DARKGRAY`)                  |
| FPS cible        | 60                                       |

Les lignes verticales sont en x = 200 et x = 400 ; les horizontales en
y = 200 et y = 400 (pour une fenêtre 600×600, cellules 200px).

## Build

- **CMake**, projet en langage C, standard C11.
- Dépendance Raylib installée via Homebrew : `brew install raylib`.
- `CMakeLists.txt` utilise `find_package(raylib)` et lie l'exécutable
  `box-game` à Raylib.
- Commandes :
  ```sh
  cmake -B build
  cmake --build build
  ./build/box-game
  ```

## Tests

Pas de tests automatisés à cette étape : la sortie est purement visuelle
et la validation se fait à l'œil (« je vois la grille »). Les tests
automatisés arriveront à l'étape 2 (les règles du jeu sont, elles,
testables de façon déterministe).

## Ce que cette fondation prépare

- **Étape 2 (règles)** : `board.c` reçoit un tableau d'état `int
  cells[3][3]`, `DrawBoardGrid` se complète d'un dessin des X/O, et
  `main.c` capte les clics souris pour remplir les cellules. Un module de
  logique (`game.c`) testable gère tours et détection de victoire/nul.
- **Étape 3 (LLM)** : une interface « joueur » abstraite (humain vs LLM)
  et un client HTTP vers l'API Claude qui reçoit l'état du plateau et
  renvoie un coup.
