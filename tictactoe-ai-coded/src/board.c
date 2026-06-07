#include "raylib.h"
#include <stdio.h>
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

static const char *DifficultyLabel(Difficulty d) {
    switch (d) {
        case DIFFICULTY_EASY:   return "Facile";
        case DIFFICULTY_MEDIUM: return "Moyen";
        case DIFFICULTY_HARD:   return "Difficile";
        default:                return "?";
    }
}

void DrawStatusText(const Game *g, Difficulty difficulty) {
    const char *base;
    switch (g->status) {
        case GAME_X_WINS: base = "X gagne ! Clic pour rejouer"; break;
        case GAME_O_WINS: base = "O gagne ! Clic pour rejouer"; break;
        case GAME_DRAW:   base = "Match nul - Clic pour rejouer"; break;
        default:          base = (g->turn == CELL_X) ? "Au tour de X"
                                                     : "Au tour de O"; break;
    }
    char buffer[80];
    snprintf(buffer, sizeof(buffer), "%s - %s", base, DifficultyLabel(difficulty));
    DrawText(buffer, 20, BOARD_SIZE + 35, 24, DARKGRAY);
}

Rectangle GetMenuButtonRect(int index) {
    float w = 280.0f;
    float h = 70.0f;
    float x = (WINDOW_WIDTH - w) / 2.0f;
    float y = 220.0f + index * (h + 30.0f);
    return (Rectangle){x, y, w, h};
}

void DrawMenu(void) {
    const char *title = "Tic Tac Toe";
    int titleWidth = MeasureText(title, 50);
    DrawText(title, (WINDOW_WIDTH - titleWidth) / 2, 110, 50, DARKGRAY);

    const char *labels[3] = {"Facile", "Moyen", "Difficile"};
    for (int i = 0; i < 3; i++) {
        Rectangle r = GetMenuButtonRect(i);
        bool disabled = (i == 2);
        Color fill = disabled ? LIGHTGRAY : SKYBLUE;
        Color textColor = disabled ? GRAY : DARKBLUE;
        DrawRectangleRec(r, fill);
        DrawRectangleLinesEx(r, 3, DARKGRAY);
        int tw = MeasureText(labels[i], 30);
        DrawText(labels[i], (int)(r.x + (r.width - tw) / 2),
                 (int)(r.y + (r.height - 30) / 2), 30, textColor);
    }
    const char *hint = "Difficile : bientot disponible";
    int hw = MeasureText(hint, 18);
    DrawText(hint, (WINDOW_WIDTH - hw) / 2, 560, 18, GRAY);
}
