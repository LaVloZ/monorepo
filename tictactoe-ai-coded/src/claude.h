#ifndef CLAUDE_H
#define CLAUDE_H

#include <stdbool.h>
#include "game.h"

// Appel bloquant : ANTHROPIC_API_KEY -> POST API -> coup légal, ou -1 sur toute erreur.
int ClaudeChooseMove(const Game *g);

// Wrapper asynchrone (un thread). Opaque pour l'appelant.
typedef struct ClaudeRequest ClaudeRequest;
ClaudeRequest *ClaudeRequestStart(const Game *g);          // copie g, lance le thread
bool ClaudeRequestPoll(ClaudeRequest *req, int *move_out); // true si terminé (sans bloquer)
void ClaudeRequestFree(ClaudeRequest *req);                // joint le thread et libère

#endif // CLAUDE_H
