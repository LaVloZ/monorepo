#include <assert.h>
#include <stdio.h>
#include "game.h"

static void test_game_init(void) {
    Game g;
    GameInit(&g);
    for (int i = 0; i < 9; i++) assert(g.cells[i] == CELL_EMPTY);
    assert(g.turn == CELL_X);
    assert(g.status == GAME_PLAYING);
}

int main(void) {
    test_game_init();
    printf("Tous les tests passent\n");
    return 0;
}
