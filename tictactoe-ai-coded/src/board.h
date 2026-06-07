#ifndef BOARD_H
#define BOARD_H

#include "game.h"

#define BOARD_SIZE 600
#define CELL_SIZE 200
#define STATUS_AREA_HEIGHT 100
#define WINDOW_WIDTH BOARD_SIZE
#define WINDOW_HEIGHT (BOARD_SIZE + STATUS_AREA_HEIGHT)
#define LINE_THICKNESS 4.0f

void DrawBoardGrid(void);
void DrawMarks(const Game *g);
void DrawStatusText(const Game *g);

#endif // BOARD_H
