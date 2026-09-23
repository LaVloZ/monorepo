#include "claude_parse.h"
#include <stdio.h>
#include <cjson/cJSON.h>

static char CellChar(Cell c) {
    if (c == CELL_X) return 'X';
    if (c == CELL_O) return 'O';
    return '.';
}

void ClaudeBuildPrompt(const Game *g, char *out, int out_size) {
    snprintf(out, out_size,
        "Tu joues au morpion en tant que O. Les cases sont numerotees 0 a 8 :\n"
        "0 1 2\n3 4 5\n6 7 8\n"
        "Plateau actuel (X, O, ou . pour vide) :\n"
        "%c %c %c\n%c %c %c\n%c %c %c\n"
        "C'est a toi (O). Reponds UNIQUEMENT par le numero (0-8) d'une case vide.",
        CellChar(g->cells[0]), CellChar(g->cells[1]), CellChar(g->cells[2]),
        CellChar(g->cells[3]), CellChar(g->cells[4]), CellChar(g->cells[5]),
        CellChar(g->cells[6]), CellChar(g->cells[7]), CellChar(g->cells[8]));
}

int ClaudeParseMove(const char *response_json, const Game *g) {
    if (response_json == NULL) return -1;
    cJSON *root = cJSON_Parse(response_json);
    if (root == NULL) return -1;

    int result = -1;
    cJSON *content = cJSON_GetObjectItemCaseSensitive(root, "content");
    if (cJSON_IsArray(content)) {
        cJSON *first = cJSON_GetArrayItem(content, 0);
        cJSON *text = first ? cJSON_GetObjectItemCaseSensitive(first, "text") : NULL;
        if (cJSON_IsString(text) && text->valuestring != NULL) {
            for (const char *p = text->valuestring; *p != '\0'; p++) {
                if (*p >= '0' && *p <= '8') {
                    int idx = *p - '0';
                    if (g->cells[idx] == CELL_EMPTY) {
                        result = idx;
                        break;
                    }
                }
            }
        }
    }
    cJSON_Delete(root);
    return result;
}
