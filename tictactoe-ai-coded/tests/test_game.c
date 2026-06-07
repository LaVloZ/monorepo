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

static void test_play_move_places_mark_and_switches_turn(void) {
    Game g;
    GameInit(&g);

    bool ok = GamePlayMove(&g, 4);   // X joue au centre
    assert(ok == true);
    assert(g.cells[4] == CELL_X);
    assert(g.turn == CELL_O);

    ok = GamePlayMove(&g, 0);        // O joue en haut-gauche
    assert(ok == true);
    assert(g.cells[0] == CELL_O);
    assert(g.turn == CELL_X);
}

static void test_play_move_rejects_illegal(void) {
    Game g;
    GameInit(&g);

    GamePlayMove(&g, 4);                 // X au centre
    bool ok = GamePlayMove(&g, 4);       // O tente une case occupée
    assert(ok == false);
    assert(g.cells[4] == CELL_X);
    assert(g.turn == CELL_O);

    assert(GamePlayMove(&g, -1) == false);
    assert(GamePlayMove(&g, 9) == false);

    g.status = GAME_X_WINS;
    assert(GamePlayMove(&g, 1) == false);
    assert(g.cells[1] == CELL_EMPTY);
}

int main(void) {
    test_game_init();
    test_play_move_places_mark_and_switches_turn();
    test_play_move_rejects_illegal();
    printf("Tous les tests passent\n");
    return 0;
}
