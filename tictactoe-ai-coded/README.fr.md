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

> **Un exemple parlant — le menu de difficulté (YAGNI en action).**
> Tant qu'il n'y avait qu'une seule IA (l'adversaire « Facile » aléatoire),
> il n'y avait *aucun menu* — il n'en fallait pas. Le menu de difficulté est
> apparu dans le commit juste *après* l'ajout de l'heuristique moyenne
> (`feat: heuristique du niveau moyen` → `feat: écran de menu`) : dès qu'un vrai
> choix a existé, on a construit l'interface pour faire ce choix. On n'a pas
> anticipé le menu à l'avance ; c'est la deuxième difficulté qui l'a justifié.

## Expérimentation de prompt

Une petite expérience : Claude est-il capable de reconstituer l'interface du jeu
**sans le lancer**, uniquement en lisant le code source ?

> **Prompt**
> *« Es-tu capable, sans lancer le jeu et juste en analysant le code, de dessiner
> l'interface graphique du jeu ? »*

**Résultat.** Claude a lu `board.c`, `board.h` et `main.c`, puis a reconstitué les
deux écrans à partir des seules constantes et des appels de dessin. Fenêtre :
**600×700 px** (un plateau de 600×600 plus une bande de statut de 100 px en bas).
Il y a deux écrans.

### Écran 1 — Menu (`STATE_MENU`)

```
┌──────────────────────────────────────────────┐  ← 600 px
│                                                │
│                                                │
│              Tic Tac Toe                       │  y=110, taille 50, gris foncé, centré
│                                                │
│                                                │
│         ┌────────────────────────┐             │
│         │        Facile          │             │  bouton 0  y=220
│         └────────────────────────┘             │  280×70, fond SKYBLUE, bordure gris foncé
│                                                │
│         ┌────────────────────────┐             │
│         │        Moyen           │             │  bouton 1  y=320
│         └────────────────────────┘             │  texte DARKBLUE, taille 30, centré
│                                                │
│         ┌────────────────────────┐             │
│         │       Difficile        │             │  bouton 2  y=420
│         └────────────────────────┘             │
│                                                │
│                                                │
│      Difficile = Claude (ANTHROPIC_API_KEY)    │  y=560, taille 18, gris
│                                                │
└──────────────────────────────────────────────┘  hauteur 700 px
```

### Écran 2 — Jeu (`STATE_PLAYING`)

```
┌──────────────┬──────────────┬──────────────┐  ← plateau 600×600
│              │              │              │
│      ╲  ╱    │              │      ╲  ╱    │   X = 2 traits rouges (RED, ép. 8)
│       ╲╱     │      ◯       │       ╲╱     │   O = anneau bleu (BLUE)
│       ╱╲     │              │       ╱╲     │   marge interne 45 px
│      ╱  ╲    │              │      ╱  ╲    │
├──────────────┼──────────────┼──────────────┤  ← lignes à x=200/400, y=200/400
│              │              │              │     DARKGRAY, épaisseur 4
│              │      ╲  ╱    │              │
│      ◯       │       ╲╱     │              │
│              │       ╱╲     │              │
│              │      ╱  ╲    │              │
├──────────────┼──────────────┼──────────────┤
│              │              │              │   cellules de 200×200
│              │              │              │
│              │              │      ◯       │
│              │              │              │
│              │              │              │
├──────────────┴──────────────┴──────────────┤  y=600
│                                              │  ← bande de statut (100 px)
│  Au tour de X - Facile                       │  texte à (20, 635), taille 24, DARKGRAY
│                                              │
└──────────────────────────────────────────────┘  y=700
```

**Bande de statut** — une seule ligne en bas à gauche, selon l'état :

- `Au tour de X - <Difficulté>` / `Au tour de O - <Difficulté>`
- `X gagne ! Clic pour rejouer - <Difficulté>`
- `O gagne ! Clic pour rejouer - <Difficulté>`
- `Match nul - Clic pour rejouer - <Difficulté>`
- `Reflexion...` en **MAROON** quand Claude réfléchit (mode Difficile)

**Détails visuels lus dans le code :**

- Fond `RAYWHITE` sur les deux écrans.
- Le **X** s'anime : 1ʳᵉ diagonale (`╲`) puis la 2ᵉ (`╱`), chaque trait se dessine
  progressivement (animation 0,20 s).
- Le **O** est un anneau tracé de 0° à 360° (effet de cercle qui « s'enroule »).
- Joueur humain = **X** (rouge), IA = **O** (bleu).
- Titre de la fenêtre : `Box Game — Tic Tac Toe`.
