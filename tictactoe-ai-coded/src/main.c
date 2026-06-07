#include "raylib.h"

#define WINDOW_SIZE 600

int main(void) {
    InitWindow(WINDOW_SIZE, WINDOW_SIZE, "Box Game — Tic Tac Toe");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
