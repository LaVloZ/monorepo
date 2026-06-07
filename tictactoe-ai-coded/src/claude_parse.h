#ifndef CLAUDE_PARSE_H
#define CLAUDE_PARSE_H

#include "game.h"

// Construit le prompt décrivant le plateau (Claude joue O) dans `out`.
void ClaudeBuildPrompt(const Game *g, char *out, int out_size);

// Parse la réponse JSON de l'API : 1er index 0..8 désignant une case vide
// légale dans content[0].text. Renvoie -1 si réponse invalide / pas de coup légal.
int ClaudeParseMove(const char *response_json, const Game *g);

#endif // CLAUDE_PARSE_H
