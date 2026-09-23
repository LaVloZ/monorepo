#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

typedef enum { CELL_EMPTY, CELL_X, CELL_O } Cell;
typedef enum { GAME_PLAYING, GAME_X_WINS, GAME_O_WINS, GAME_DRAW } GameStatus;

typedef struct {
    Cell cells[9];   // plateau à plat, index 0..8
    Cell turn;       // à qui de jouer
    GameStatus status;
} Game;

void GameInit(Game *g);
bool GamePlayMove(Game *g, int index);

// true si jouer `player` à `index` (case supposée vide) complète une ligne gagnante.
// Ne modifie pas le Game.
bool GameIsWinningMove(const Game *g, int index, Cell player);

#endif // GAME_H
