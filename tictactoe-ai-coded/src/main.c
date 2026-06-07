#include "raylib.h"
#include <stdlib.h>
#include <time.h>
#include "board.h"
#include "game.h"
#include "ai.h"

typedef enum { STATE_MENU, STATE_PLAYING } AppState;

int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Box Game — Tic Tac Toe");
    SetExitKey(KEY_NULL);   // Échap ne ferme plus la fenêtre
    SetTargetFPS(60);
    srand((unsigned int)time(NULL));

    AppState state = STATE_MENU;
    Difficulty difficulty = DIFFICULTY_EASY;
    Game game;
    GameInit(&game);

    while (!WindowShouldClose()) {
        if (state == STATE_MENU) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 m = GetMousePosition();
                for (int i = 0; i < 2; i++) { // 0=Facile, 1=Moyen (2=Difficile désactivé)
                    if (CheckCollisionPointRec(m, GetMenuButtonRect(i))) {
                        difficulty = (i == 0) ? DIFFICULTY_EASY : DIFFICULTY_MEDIUM;
                        GameInit(&game);
                        state = STATE_PLAYING;
                    }
                }
            }
            BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawMenu();
            EndDrawing();
        } else { // STATE_PLAYING
            if (IsKeyPressed(KEY_ESCAPE)) {
                state = STATE_MENU;
            } else {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (game.status != GAME_PLAYING) {
                        GameInit(&game);
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
                if (game.status == GAME_PLAYING && game.turn == CELL_O) {
                    int move = AiChooseMove(&game, difficulty);
                    if (move >= 0) GamePlayMove(&game, move);
                }
            }
            BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawBoardGrid();
            DrawMarks(&game);
            DrawStatusText(&game, difficulty);
            EndDrawing();
        }
    }

    CloseWindow();
    return 0;
}
