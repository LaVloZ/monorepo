# Tic Tac Toe

Un morpion (Tic-Tac-Toe) en C avec interface graphique [raylib](https://www.raylib.com/).
Vous jouez les **X**, l'ordinateur joue les **O**, avec trois niveaux de difficulté —
dont un niveau **Difficile** piloté par l'API Claude d'Anthropic.

## Fonctionnalités

- Interface graphique (grille 3×3, animation d'apparition des pions)
- Menu de sélection de la difficulté
- Trois niveaux d'IA :
  - **Facile** — coups aléatoires
  - **Moyen** — heuristique (gagner / bloquer / centre / coin)
  - **Difficile** — coup choisi par Claude (`claude-opus-4-8`) via l'API Anthropic,
    avec repli sur l'heuristique moyenne en cas d'erreur réseau
- `Échap` pour revenir au menu à tout moment

## Prérequis

- CMake ≥ 3.16 et un compilateur C11
- [raylib](https://www.raylib.com/)
- [libcurl](https://curl.se/libcurl/)
- [cJSON](https://github.com/DaveGamble/cJSON) (`libcjson`)
- pthreads (généralement fourni par le système)

Sur macOS (Homebrew) :

```sh
brew install cmake raylib curl cjson
```

## Compilation

```sh
cmake -B build
cmake --build build
```

L'exécutable est généré dans `build/box-game`.

## Lancer le jeu

```sh
./build/box-game
```

Pour le niveau **Difficile**, exportez votre clé API Anthropic avant de lancer :

```sh
export ANTHROPIC_API_KEY="sk-ant-..."
./build/box-game
```

Sans clé valide (ou en cas d'erreur réseau), le niveau Difficile se replie
automatiquement sur l'heuristique du niveau Moyen.

## Tests

```sh
cmake --build build
ctest --test-dir build
```

Les tests unitaires couvrent les règles du jeu, l'IA heuristique et le parsing
des réponses de Claude.

## Structure du projet

```
src/
  main.c          Boucle de jeu raylib + machine à états (menu / partie)
  game.{c,h}      Règles du morpion (plateau, coups, conditions de victoire)
  ai.{c,h}        IA heuristique (facile / moyen)
  claude.{c,h}    Client API Claude (libcurl) + wrapper asynchrone (pthread)
  claude_parse.{c,h}  Construction du prompt + parsing JSON de la réponse
  board.{c,h}     Rendu raylib (grille, pions, textes, menu)
tests/
  test_game.c     Tests unitaires
docs/             Specs et plans d'implémentation
```

## Architecture

Le jeu est piloté par une machine à états (`AppState` : menu / partie) et une
sous-machine pour la partie (`PlayPhase` : attente, animation, attente IA,
réflexion de Claude). L'appel à l'API Claude est lancé dans un thread séparé et
interrogé sans bloquer la boucle de rendu (`ClaudeRequestStart` / `…Poll` /
`…Free`), ce qui garde l'interface fluide pendant que Claude « réfléchit ».
