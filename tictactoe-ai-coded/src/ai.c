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
