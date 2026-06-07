#include "raylib.h"
#include "board.h"

void DrawBoardGrid(void) {
    // 2 lignes verticales (x = 200 et x = 400)
    DrawLineEx((Vector2){CELL_SIZE, 0},
               (Vector2){CELL_SIZE, WINDOW_SIZE}, LINE_THICKNESS, DARKGRAY);
    DrawLineEx((Vector2){2 * CELL_SIZE, 0},
               (Vector2){2 * CELL_SIZE, WINDOW_SIZE}, LINE_THICKNESS, DARKGRAY);

    // 2 lignes horizontales (y = 200 et y = 400)
    DrawLineEx((Vector2){0, CELL_SIZE},
               (Vector2){WINDOW_SIZE, CELL_SIZE}, LINE_THICKNESS, DARKGRAY);
    DrawLineEx((Vector2){0, 2 * CELL_SIZE},
               (Vector2){WINDOW_SIZE, 2 * CELL_SIZE}, LINE_THICKNESS, DARKGRAY);
}
