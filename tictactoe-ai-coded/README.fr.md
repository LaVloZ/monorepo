# Tic Tac Toe

> 🇬🇧 *Read this in [English](README.md).*

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

## Démarche d'implémentation

Tout le projet a été construit avec [Claude Code](https://claude.com/claude-code)
et les skills **Superpowers**, par petits incréments vérifiables. Chaque
fonctionnalité est passée par la même boucle en trois temps avant d'écrire la
moindre ligne de code de production :

1. **Brainstorm & spec** — explorer l'intention, les contraintes et la
   conception, consignées dans un document de spec sous `docs/`
   (skill `brainstorming`).
2. **Plan** — découper la spec en un plan ordonné, orienté tests d'abord
   (skill `writing-plans`).
3. **Implémentation, tests d'abord** — coder en tout petits commits, les tests
   avant le code, refactoriser au besoin (skill `test-driven-development`).

L'historique git reflète directement ce rythme : il se lit comme une suite
propre de triplets `spec → plan → feat/test`. Cinq jalons, chacun livrable
indépendamment :

| # | Jalon | Flux incrémental (à lire de haut en bas dans git) |
|---|-------|---------------------------------------------------|
| 1 | **Rendu du plateau** | spec → plan → fenêtre raylib vide → grille 3×3 |
| 2 | **Règles du jeu (TDD)** | spec → plan → harnais CTest → poser un coup → rejeter les coups illégaux → détection de victoire → détection du nul → IA aléatoire → dessin X/O + barre de statut → boucle de jeu complète |
| 3 | **Niveaux de difficulté** | spec → plan → helper `GameIsWinningMove` → refactor `AiChooseMove(Difficulty)` → heuristique moyenne (gagner / bloquer / centre / coin) → écran de menu → machine à états menu/partie |
| 4 | **Animation des pions** | spec → plan → dessin de pions partiels → tracé animé + délai IA |
| 5 | **Niveau Difficile (Claude)** | spec → plan → construction du prompt + parsing JSON → client libcurl asynchrone (pthread) → niveau Difficile + phase `PLAY_THINKING` |

Deux points à retenir pour un relecteur :

- **Les tests d'abord.** Le moteur de règles (jalon 2) commence par le harnais
  CTest, puis grandit d'un comportement par commit — on voit la suite de tests
  piloter la conception.
- **Chaque étape est petite et réversible.** Aucun commit ne mélange spec, plan
  et fonctionnalité ; chaque `feat:` est un comportement cohérent unique, si
  bien que l'historique fait aussi office de visite guidée, étape par étape, de
  la construction du jeu.
