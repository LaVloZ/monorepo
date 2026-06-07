#ifndef AI_H
#define AI_H

#include "game.h"

// Renvoie l'index d'une case vide choisie au hasard, ou -1 si plateau plein.
int AiChooseMove(const Game *g);

#endif // AI_H
