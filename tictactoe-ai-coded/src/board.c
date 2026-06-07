#include "raylib.h"
#include "board.h"

void DrawBoardGrid(void) {
    DrawLineEx((Vector2){CELL_SIZE, 0},
               (Vector2){CELL_SIZE, BOARD_SIZE}, LINE_THICKNESS, DARKGRAY);
    DrawLineEx((Vector2){2 * CELL_SIZE, 0},
               (Vector2){2 * CELL_SIZE, BOARD_SIZE}, LINE_THICKNESS, DARKGRAY);
    DrawLineEx((Vector2){0, CELL_SIZE},
               (Vector2){BOARD_SIZE, CELL_SIZE}, LINE_THICKNESS, DARKGRAY);
    DrawLineEx((Vector2){0, 2 * CELL_SIZE},
               (Vector2){BOARD_SIZE, 2 * CELL_SIZE}, LINE_THICKNESS, DARKGRAY);
}

void DrawMarks(const Game *g) {
    const float margin = 45.0f;
    for (int i = 0; i < 9; i++) {
        int row = i / 3;
        int col = i % 3;
        float x = col * CELL_SIZE;
        float y = row * CELL_SIZE;
        if (g->cells[i] == CELL_X) {
            DrawLineEx((Vector2){x + margin, y + margin},
                       (Vector2){x + CELL_SIZE - margin, y + CELL_SIZE - margin},
                       8.0f, RED);
            DrawLineEx((Vector2){x + CELL_SIZE - margin, y + margin},
                       (Vector2){x + margin, y + CELL_SIZE - margin},
                       8.0f, RED);
        } else if (g->cells[i] == CELL_O) {
            Vector2 center = {x + CELL_SIZE / 2.0f, y + CELL_SIZE / 2.0f};
            float radius = CELL_SIZE / 2.0f - margin;
            DrawRing(center, radius - 6.0f, radius, 0, 360, 64, BLUE);
        }
    }
}

void DrawStatusText(const Game *g) {
    const char *text;
    switch (g->status) {
        case GAME_X_WINS: text = "X gagne ! Clic pour rejouer"; break;
        case GAME_O_WINS: text = "O gagne ! Clic pour rejouer"; break;
        case GAME_DRAW:   text = "Match nul - Clic pour rejouer"; break;
        default:          text = (g->turn == CELL_X) ? "Au tour de X"
                                                     : "Au tour de O"; break;
    }
    DrawText(text, 20, BOARD_SIZE + 35, 28, DARKGRAY);
}
