#include "game.h"

void GameInit(Game *g) {
    for (int i = 0; i < 9; i++) g->cells[i] = CELL_EMPTY;
    g->turn = CELL_X;
    g->status = GAME_PLAYING;
}

bool GamePlayMove(Game *g, int index) {
    if (g->status != GAME_PLAYING) return false;
    if (index < 0 || index > 8) return false;
    if (g->cells[index] != CELL_EMPTY) return false;

    g->cells[index] = g->turn;
    g->turn = (g->turn == CELL_X) ? CELL_O : CELL_X;
    return true;
}
