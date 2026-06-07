#include <assert.h>
#include <stdio.h>
#include "game.h"
#include "ai.h"

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

static void test_win_row(void) {
    Game g;
    GameInit(&g);
    GamePlayMove(&g, 0); // X
    GamePlayMove(&g, 3); // O
    GamePlayMove(&g, 1); // X
    GamePlayMove(&g, 4); // O
    GamePlayMove(&g, 2); // X gagne 0,1,2
    assert(g.status == GAME_X_WINS);
}

static void test_win_column(void) {
    Game g;
    GameInit(&g);
    GamePlayMove(&g, 0); // X
    GamePlayMove(&g, 1); // O
    GamePlayMove(&g, 3); // X
    GamePlayMove(&g, 2); // O
    GamePlayMove(&g, 6); // X gagne 0,3,6
    assert(g.status == GAME_X_WINS);
}

static void test_win_diagonal(void) {
    Game g;
    GameInit(&g);
    GamePlayMove(&g, 0); // X
    GamePlayMove(&g, 1); // O
    GamePlayMove(&g, 4); // X
    GamePlayMove(&g, 2); // O
    GamePlayMove(&g, 8); // X gagne 0,4,8
    assert(g.status == GAME_X_WINS);
}

static void test_draw(void) {
    Game g;
    GameInit(&g);
    // X O X / X O O / O X X  -> plateau plein sans gagnant
    GamePlayMove(&g, 0); // X
    GamePlayMove(&g, 1); // O
    GamePlayMove(&g, 2); // X
    GamePlayMove(&g, 4); // O
    GamePlayMove(&g, 3); // X
    GamePlayMove(&g, 5); // O
    GamePlayMove(&g, 7); // X
    GamePlayMove(&g, 6); // O
    GamePlayMove(&g, 8); // X
    assert(g.status == GAME_DRAW);
}

static void test_ai_returns_empty_cell(void) {
    Game g;
    GameInit(&g);
    g.cells[0] = CELL_X;
    g.cells[4] = CELL_O;
    g.cells[8] = CELL_X;
    for (int k = 0; k < 50; k++) {
        int m = AiChooseMove(&g);
        assert(m >= 0 && m < 9);
        assert(g.cells[m] == CELL_EMPTY);
    }
}

static void test_ai_full_board_returns_minus_one(void) {
    Game g;
    GameInit(&g);
    for (int i = 0; i < 9; i++) g.cells[i] = CELL_X;
    assert(AiChooseMove(&g) == -1);
}

int main(void) {
    test_game_init();
    test_ai_returns_empty_cell();
    test_ai_full_board_returns_minus_one();
    test_play_move_places_mark_and_switches_turn();
    test_play_move_rejects_illegal();
    test_win_row();
    test_win_column();
    test_win_diagonal();
    test_draw();
    printf("Tous les tests passent\n");
    return 0;
}
