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

int AiChooseMove(const Game *g, Difficulty difficulty) {
    switch (difficulty) {
        case DIFFICULTY_EASY:   return ChooseRandom(g);
        case DIFFICULTY_MEDIUM: return ChooseMedium(g);
        case DIFFICULTY_HARD:   return ChooseMedium(g); // repli temporaire
        default:                return ChooseRandom(g);
    }
}
