#include "raylib.h"
#include <stdlib.h>
#include <time.h>
#include "board.h"
#include "game.h"
#include "ai.h"

typedef enum { STATE_MENU, STATE_PLAYING } AppState;
typedef enum { PLAY_IDLE, PLAY_ANIMATING, PLAY_WAIT_AI } PlayPhase;

#define ANIM_DURATION 0.20f
#define AI_DELAY 0.35f

static void ResetProgress(float progress[9]) {
    for (int i = 0; i < 9; i++) progress[i] = 0.0f;
}

int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Box Game — Tic Tac Toe");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    srand((unsigned int)time(NULL));

    AppState state = STATE_MENU;
    Difficulty difficulty = DIFFICULTY_EASY;
    Game game;
    GameInit(&game);

    float progress[9];
    ResetProgress(progress);
    PlayPhase phase = PLAY_IDLE;
    int animCell = -1;
    float animTime = 0.0f;
    float aiWait = 0.0f;

    while (!WindowShouldClose()) {
        if (state == STATE_MENU) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 m = GetMousePosition();
                for (int i = 0; i < 2; i++) { // 0=Facile, 1=Moyen
                    if (CheckCollisionPointRec(m, GetMenuButtonRect(i))) {
                        difficulty = (i == 0) ? DIFFICULTY_EASY : DIFFICULTY_MEDIUM;
                        GameInit(&game);
                        ResetProgress(progress);
                        phase = PLAY_IDLE;
                        state = STATE_PLAYING;
                    }
                }
            }
            BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawMenu();
            EndDrawing();
        } else { // STATE_PLAYING
            float dt = GetFrameTime();

            if (IsKeyPressed(KEY_ESCAPE)) {
                state = STATE_MENU;
            } else if (phase == PLAY_IDLE) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (game.status != GAME_PLAYING) {
                        GameInit(&game);
                        ResetProgress(progress);
                    } else if (game.turn == CELL_X) {
                        Vector2 m = GetMousePosition();
                        if (m.y < BOARD_SIZE) {
                            int col = (int)(m.x / CELL_SIZE);
                            int row = (int)(m.y / CELL_SIZE);
                            if (col >= 0 && col < 3 && row >= 0 && row < 3) {
                                int index = row * 3 + col;
                                if (GamePlayMove(&game, index)) {
                                    animCell = index;
                                    progress[index] = 0.0f;
                                    animTime = 0.0f;
                                    phase = PLAY_ANIMATING;
                                }
                            }
                        }
                    }
                }
            } else if (phase == PLAY_ANIMATING) {
                animTime += dt;
                float p = animTime / ANIM_DURATION;
                if (p >= 1.0f) p = 1.0f;
                progress[animCell] = p;
                if (p >= 1.0f) {
                    if (game.status == GAME_PLAYING && game.turn == CELL_O) {
                        phase = PLAY_WAIT_AI;
                        aiWait = 0.0f;
                    } else {
                        phase = PLAY_IDLE;
                    }
                }
            } else { // PLAY_WAIT_AI
                aiWait += dt;
                if (aiWait >= AI_DELAY) {
                    int move = AiChooseMove(&game, difficulty);
                    if (move >= 0 && GamePlayMove(&game, move)) {
                        animCell = move;
                        progress[move] = 0.0f;
                        animTime = 0.0f;
                        phase = PLAY_ANIMATING;
                    } else {
                        phase = PLAY_IDLE;
                    }
                }
            }

            BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawBoardGrid();
            DrawMarks(&game, progress);
            DrawStatusText(&game, difficulty);
            EndDrawing();
        }
    }

    CloseWindow();
    return 0;
}
