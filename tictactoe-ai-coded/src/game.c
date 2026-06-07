#include "game.h"

static const int WIN_LINES[8][3] = {
    {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, // lignes
    {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, // colonnes
    {0, 4, 8}, {2, 4, 6}             // diagonales
};

static bool HasWon(const Game *g, Cell player) {
    for (int i = 0; i < 8; i++) {
        const int *l = WIN_LINES[i];
        if (g->cells[l[0]] == player &&
            g->cells[l[1]] == player &&
            g->cells[l[2]] == player) {
            return true;
        }
    }
    return false;
}

void GameInit(Game *g) {
    for (int i = 0; i < 9; i++) g->cells[i] = CELL_EMPTY;
    g->turn = CELL_X;
    g->status = GAME_PLAYING;
}

bool GamePlayMove(Game *g, int index) {
    if (g->status != GAME_PLAYING) return false;
    if (index < 0 || index > 8) return false;
    if (g->cells[index] != CELL_EMPTY) return false;

    Cell player = g->turn;
    g->cells[index] = player;

    if (HasWon(g, player)) {
        g->status = (player == CELL_X) ? GAME_X_WINS : GAME_O_WINS;
    } else {
        g->turn = (player == CELL_X) ? CELL_O : CELL_X;
    }
    return true;
}
