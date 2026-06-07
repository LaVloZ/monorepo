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

static float Clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

void DrawMarks(const Game *g, const float progress[9]) {
    const float margin = 45.0f;
    for (int i = 0; i < 9; i++) {
        if (g->cells[i] == CELL_EMPTY) continue;
        float p = progress[i];
        int row = i / 3;
        int col = i % 3;
        float x = col * CELL_SIZE;
        float y = row * CELL_SIZE;
        if (g->cells[i] == CELL_X) {
            float f1 = Clamp01(p / 0.5f);
            float f2 = Clamp01((p - 0.5f) / 0.5f);
            Vector2 a1 = {x + margin, y + margin};
            Vector2 b1 = {x + CELL_SIZE - margin, y + CELL_SIZE - margin};
            if (f1 > 0.0f) {
                DrawLineEx(a1,
                    (Vector2){a1.x + f1 * (b1.x - a1.x), a1.y + f1 * (b1.y - a1.y)},
                    8.0f, RED);
            }
            Vector2 a2 = {x + CELL_SIZE - margin, y + margin};
            Vector2 b2 = {x + margin, y + CELL_SIZE - margin};
            if (f2 > 0.0f) {
                DrawLineEx(a2,
                    (Vector2){a2.x + f2 * (b2.x - a2.x), a2.y + f2 * (b2.y - a2.y)},
                    8.0f, RED);
            }
        } else { // CELL_O
            Vector2 center = {x + CELL_SIZE / 2.0f, y + CELL_SIZE / 2.0f};
            float radius = CELL_SIZE / 2.0f - margin;
            if (p > 0.0f) {
                DrawRing(center, radius - 6.0f, radius, 0.0f, p * 360.0f, 64, BLUE);
            }
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
