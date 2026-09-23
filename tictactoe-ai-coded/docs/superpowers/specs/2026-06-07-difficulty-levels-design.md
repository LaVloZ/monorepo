# Box Game — Niveaux de difficulté

**Date :** 2026-06-07
**Langage :** C (C11)
**Librairie graphique :** Raylib
**Tests :** CTest

## Contexte

Le morpion fonctionne : humain (X) contre IA aléatoire (O), avec écran de
jeu, détection victoire/nul et rejoue d'un clic. Cette fonctionnalité
ajoute un **choix de difficulté** via un écran de menu au démarrage.

Trois niveaux sont prévus :
- **Facile** — coup aléatoire (déjà implémenté).
- **Moyen** — heuristique à base de règles (cette spec).
- **Difficile** — délégué à Claude (LLM). **Hors périmètre ici** : ce
  niveau apparaît au menu mais grisé/désactivé ; il sera implémenté comme
  une fonctionnalité ultérieure isolée (libcurl + JSON + clé API).

## Comportement attendu

- Au lancement : **écran de menu** avec un titre et 3 boutons —
  « Facile », « Moyen », « Difficile ».
- « Difficile » est dessiné grisé et **ignoré au clic** (« bientôt »).
- Clic sur « Facile » ou « Moyen » → démarre une nouvelle partie avec ce
  niveau (`STATE_PLAYING`).
- Pendant la partie : la touche **Échap** revient au menu.
- La barre de statut rappelle le niveau actif, ex. « Au tour de X —
  Moyen ».
- Le reste du jeu est inchangé (X au clic, O = IA selon la difficulté,
  victoire/nul, clic pour rejouer).

**Critère de succès :** depuis le menu, on choisit Facile ou Moyen, on
joue une partie complète contre l'IA correspondante, on peut revenir au
menu via Échap ; la suite de tests passe au vert, dont les nouveaux tests
de l'heuristique.

## Architecture

On introduit un **état applicatif** (menu vs partie) dans `main.c`, et on
transforme `ai.c` en sélecteur de coup par difficulté. La logique de
décision testable reste hors de Raylib.

```
src/
├── game.h / game.c   + GameIsWinningMove(const Game*, int index, Cell player)
├── ai.h   / ai.c     Difficulty + AiChooseMove(const Game*, Difficulty)
├── board.h / board.c + DrawMenu(void), GetMenuButtonRect(int), label difficulté au statut
└── main.c            machine à états STATE_MENU / STATE_PLAYING
tests/
└── test_game.c       + tests heuristique + GameIsWinningMove
```

### `game.h` / `game.c` — nouveau helper testable

```c
// true si jouer `player` à `index` (case supposée vide) complète une ligne gagnante.
bool GameIsWinningMove(const Game *g, int index, Cell player);
```

Implémentation : pose temporairement `player` à `index` sur une copie (ou
teste sans muter l'original), réutilise la table `WIN_LINES`/`HasWon`
existante, et indique si cela gagne. Ne modifie pas le `Game` reçu.

### `ai.h` / `ai.c` — sélecteur par difficulté

```c
typedef enum { DIFFICULTY_EASY, DIFFICULTY_MEDIUM, DIFFICULTY_HARD } Difficulty;

// Choisit un coup selon la difficulté. Renvoie un index de case vide, ou -1 si plateau plein.
int AiChooseMove(const Game *g, Difficulty difficulty);
```

- `DIFFICULTY_EASY` : coup aléatoire (logique actuelle, conservée).
- `DIFFICULTY_MEDIUM` : heuristique (voir ci-dessous).
- `DIFFICULTY_HARD` : repli temporaire sur l'heuristique moyenne (le
  niveau n'est pas sélectionnable au menu tant que le LLM n'existe pas).

**Heuristique (niveau moyen)**, avec `me = g->turn` et `opp` l'autre
joueur, dans l'ordre :
1. S'il existe une case vide `i` telle que `GameIsWinningMove(g, i, me)`
   → la jouer (gagner).
2. Sinon, s'il existe une case vide `i` telle que
   `GameIsWinningMove(g, i, opp)` → la jouer (bloquer).
3. Sinon, si le centre (index 4) est vide → le jouer.
4. Sinon, s'il existe un coin vide (0, 2, 6, 8) → en jouer un au hasard.
5. Sinon → coup aléatoire parmi les cases vides.

### `board.h` / `board.c` — menu et statut

- `DrawMenu(void)` : dessine le titre et 3 boutons (« Difficile » en
  gris). Utilise `GetMenuButtonRect`.
- `Rectangle GetMenuButtonRect(int index)` : géométrie d'un bouton
  (0 = Facile, 1 = Moyen, 2 = Difficile), partagée entre dessin et
  test de clic. 3 rectangles empilés et centrés.
- `DrawStatusText` est étendue pour afficher le niveau actif : elle prend
  désormais la difficulté en paramètre →
  `DrawStatusText(const Game *g, Difficulty difficulty)`, et concatène le
  libellé (« Facile »/« Moyen »/« Difficile ») au texte de statut.

### `main.c` — machine à états

```c
typedef enum { STATE_MENU, STATE_PLAYING } AppState;
```

Boucle :
- **STATE_MENU** : sur clic gauche, tester `GetMenuButtonRect(0)` et
  `(1)` avec `CheckCollisionPointRec` ; si Facile/Moyen cliqué →
  enregistrer la difficulté, `GameInit`, passer en `STATE_PLAYING`. Le
  bouton 2 (Difficile) est ignoré. Dessiner `DrawMenu`.
- **STATE_PLAYING** : logique actuelle (clic = coup humain / rejoue, tour
  IA via `AiChooseMove(&game, difficulty)`), plus : touche **Échap** →
  revenir en `STATE_MENU`. Dessiner grille + pions + statut.

## Tests

Nouveaux cas dans `tests/test_game.c` (déterministes) :
- `GameIsWinningMove` : renvoie `true` pour une case qui complète une
  ligne, `false` sinon ; ne modifie pas le `Game`.
- Heuristique — gagne : l'IA a deux pions alignés avec la 3ᵉ case vide →
  `AiChooseMove(g, DIFFICULTY_MEDIUM)` renvoie cette case.
- Heuristique — bloque : l'adversaire a deux pions alignés (et l'IA ne
  peut pas gagner immédiatement) → l'IA renvoie la case bloquante.
- Heuristique — centre : plateau sans menace immédiate avec centre vide →
  l'IA renvoie 4.

Les tests existants appelant `AiChooseMove(&g)` sont mis à jour pour la
nouvelle signature (`AiChooseMove(&g, DIFFICULTY_EASY)`).

Menu, boutons et rendu : validation visuelle.

## Ce que cette étape prépare

**Niveau Difficile (Claude)** : remplacer la branche `DIFFICULTY_HARD` de
`AiChooseMove` par un appel à l'API Claude (sérialisation du plateau →
requête HTTP → parsing du coup), et activer le bouton « Difficile » du
menu. Le reste (menu, états, heuristique) ne bouge pas.
