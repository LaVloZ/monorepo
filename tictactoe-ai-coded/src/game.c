#include "game.h"

void GameInit(Game *g) {
    for (int i = 0; i < 9; i++) g->cells[i] = CELL_EMPTY;
    g->turn = CELL_X;
    g->status = GAME_PLAYING;
}

bool GamePlayMove(Game *g, int index) {
    g->cells[index] = g->turn;
    g->turn = (g->turn == CELL_X) ? CELL_O : CELL_X;
    return true;
}
