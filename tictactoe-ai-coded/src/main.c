#include "raylib.h"
#include "board.h"

int main(void) {
    InitWindow(WINDOW_SIZE, WINDOW_SIZE, "Box Game — Tic Tac Toe");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawBoardGrid();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
