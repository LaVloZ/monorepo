# Box Game — Animation d'apparition des pions

**Date :** 2026-06-07
**Langage :** C (C11)
**Librairie graphique :** Raylib

## Contexte

Le morpion est jouable (menu, niveaux Facile/Moyen, règles testées). Avant
d'intégrer Claude (niveau Difficile), on améliore le ressenti : les pions
s'**animent** à l'apparition au lieu de surgir instantanément. Le jeu
passe d'un rendu statique à un rendu **piloté par le temps**.

## Comportement attendu

- À la pose d'un pion, il se **dessine progressivement** sur ~200 ms :
  - **X** : la 1ʳᵉ diagonale se trace (progress 0 → 0,5), puis la 2ᵈᵉ
    (0,5 → 1).
  - **O** : un arc qui se referme de 0° à 360°.
- L'animation s'applique aux pions **de X (humain) et de O (IA)**.
- Après le coup de l'humain, l'IA **attend ~350 ms** avant de jouer puis
  d'animer son pion (réponse « naturelle », non instantanée).
- Pendant qu'un pion s'anime **et** pendant l'attente de l'IA, les clics
  de jeu sont **ignorés** (pas de double-coup).
- Le reste est inchangé : règles, menu, sélection de difficulté, statut,
  rejoue au clic, Échap → menu.

**Critère de succès :** on voit chaque pion (X comme O) se tracer
progressivement ; le coup de l'IA arrive après une courte pause ; on ne
peut pas cliquer pendant une animation ; la suite de tests existante reste
verte.

## Architecture

Le jeu devient piloté par le temps via `GetFrameTime()`. La **logique
pure** (`game.c`, `ai.c`) ne change pas. On modifie seulement le **dessin**
(`board.c`) et la **boucle** (`main.c`).

```
src/
├── board.h / board.c   DrawMarks(const Game*, const float progress[9])
└── main.c              machine à états de jeu pilotée par le temps
```

### `board.h` / `board.c` — dessin partiel

`DrawMarks` reçoit un tableau d'avancement :

```c
void DrawMarks(const Game *g, const float progress[9]);
```

- `progress[i]` ∈ [0, 1] : fraction tracée du pion de la case `i`
  (0 = rien, 1 = pion complet). Les cases vides ne sont pas dessinées.
- **X** : fraction du trait 1 = `clamp(p / 0.5, 0, 1)`, fraction du trait
  2 = `clamp((p - 0.5) / 0.5, 0, 1)`. Chaque trait est dessiné de son
  origine jusqu'à `origine + frac × (fin − origine)`.
- **O** : `DrawRing(center, radius - 6, radius, 0, p × 360, 64, BLUE)`.
- Les couleurs, marges et épaisseurs restent celles actuelles.

### `main.c` — boucle pilotée par le temps

Nouveau sous-état du mode partie :

```c
typedef enum { PLAY_IDLE, PLAY_ANIMATING, PLAY_WAIT_AI } PlayPhase;
```

État local : `float progress[9]`, `PlayPhase phase`, `int animCell`,
`float animTime`, `float aiWait`.

Constantes : `ANIM_DURATION = 0.20f`, `AI_DELAY = 0.35f`.

Chaque frame en `STATE_PLAYING`, avec `dt = GetFrameTime()` :

- **Échap** → retour menu (réinitialise une partie : `GameInit`, `phase =
  PLAY_IDLE`, `progress` remis à 0).
- **PLAY_IDLE** :
  - Sur clic gauche : si `status != GAME_PLAYING` → `GameInit`, remettre
    `progress` à 0 (rejoue). Sinon si `turn == CELL_X` et clic sur une
    case vide valide → `GamePlayMove` ; si appliqué : `animCell = index`,
    `progress[index] = 0`, `animTime = 0`, `phase = PLAY_ANIMATING`.
- **PLAY_ANIMATING** :
  - `animTime += dt` ; `progress[animCell] = min(1, animTime /
    ANIM_DURATION)`.
  - Quand `progress[animCell] >= 1` : le fixer à 1 ; si `status ==
    GAME_PLAYING` et `turn == CELL_O` → `phase = PLAY_WAIT_AI`, `aiWait =
    0` ; sinon → `phase = PLAY_IDLE`.
- **PLAY_WAIT_AI** :
  - `aiWait += dt` ; quand `aiWait >= AI_DELAY` : `move = AiChooseMove(&g,
    difficulty)` ; si `move >= 0` → `GamePlayMove(move)`, `animCell =
    move`, `progress[move] = 0`, `animTime = 0`, `phase = PLAY_ANIMATING`.

Dessin (partie) : `DrawBoardGrid()`, `DrawMarks(&game, progress)`,
`DrawStatusText(&game, difficulty)`.

Au passage menu → partie (clic Facile/Moyen) : `GameInit`, `progress`
remis à 0, `phase = PLAY_IDLE`.

**Invariant** : toute case portant un pion a un `progress` qui a été piloté
(0→1) ; les cases vides ne sont pas dessinées, donc leur valeur est sans
effet.

## Tests

La logique de jeu (`game.c`, `ai.c`) est inchangée : **les tests
existants restent verts** et doivent être relancés. L'animation est
temporelle et visuelle ; pas de nouveau test unitaire (validation à
l'œil), cohérent avec les étapes de dessin précédentes.

## Ce que cette étape prépare

Le rendu piloté par le temps et le sous-état `PLAY_WAIT_AI` faciliteront
l'intégration de Claude (niveau Difficile) : l'appel réseau (latent) se
logera naturellement dans une phase d'attente, sans figer la fenêtre.
