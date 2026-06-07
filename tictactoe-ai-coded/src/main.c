#include "raylib.h"
#include <stdlib.h>
#include <time.h>
#include "board.h"
#include "game.h"
#include "ai.h"

int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Box Game — Tic Tac Toe");
    SetTargetFPS(60);
    srand((unsigned int)time(NULL));

    Difficulty difficulty = DIFFICULTY_EASY;
    Game game;
    GameInit(&game);

    while (!WindowShouldClose()) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (game.status != GAME_PLAYING) {
                GameInit(&game);                 // rejoue
            } else if (game.turn == CELL_X) {
                Vector2 m = GetMousePosition();
                if (m.y < BOARD_SIZE) {
                    int col = (int)(m.x / CELL_SIZE);
                    int row = (int)(m.y / CELL_SIZE);
                    if (col >= 0 && col < 3 && row >= 0 && row < 3) {
                        GamePlayMove(&game, row * 3 + col);
                    }
                }
            }
        }

        // Tour de l'IA (O), juste après le coup de l'humain
        if (game.status == GAME_PLAYING && game.turn == CELL_O) {
            int move = AiChooseMove(&game, difficulty);
            if (move >= 0) GamePlayMove(&game, move);
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawBoardGrid();
        DrawMarks(&game);
        DrawStatusText(&game, difficulty);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
