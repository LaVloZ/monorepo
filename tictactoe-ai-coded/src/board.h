#ifndef BOARD_H
#define BOARD_H

#include "raylib.h"
#include "game.h"
#include "ai.h"

#define BOARD_SIZE 600
#define CELL_SIZE 200
#define STATUS_AREA_HEIGHT 100
#define WINDOW_WIDTH BOARD_SIZE
#define WINDOW_HEIGHT (BOARD_SIZE + STATUS_AREA_HEIGHT)
#define LINE_THICKNESS 4.0f

void DrawBoardGrid(void);
void DrawMarks(const Game *g, const float progress[9]);
void DrawStatusText(const Game *g, Difficulty difficulty);
void DrawThinkingText(void);
void DrawMenu(void);
Rectangle GetMenuButtonRect(int index);

#endif // BOARD_H
