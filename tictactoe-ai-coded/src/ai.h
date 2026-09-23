#ifndef AI_H
#define AI_H

#include "game.h"

typedef enum { DIFFICULTY_EASY, DIFFICULTY_MEDIUM, DIFFICULTY_HARD } Difficulty;

// Choisit un coup selon la difficulté. Renvoie une case vide, ou -1 si plateau plein.
int AiChooseMove(const Game *g, Difficulty difficulty);

#endif // AI_H
