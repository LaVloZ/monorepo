# Plan d'implémentation — Niveau Difficile piloté par Claude

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Le niveau Difficile fait jouer l'adversaire O par Claude via un appel HTTP asynchrone, avec « Réflexion… » et repli sur l'heuristique Moyen.

**Architecture:** `claude_parse.c` (pur : prompt + parsing JSON, testé) est séparé de `claude.c` (libcurl + thread pthread, non testé). `main.c` ajoute une phase `PLAY_THINKING`. Le repli réutilise `AiChooseMove(..., DIFFICULTY_MEDIUM)`.

**Tech Stack:** C (C11), Raylib, libcurl, cJSON, pthread, CMake + CTest.

**Validation :** parsing en TDD (CTest) ; réseau/thread/menu en visuel (avec et sans `ANTHROPIC_API_KEY`).

---

## Structure des fichiers

- `src/claude_parse.h` / `src/claude_parse.c` — Création. `ClaudeBuildPrompt`, `ClaudeParseMove`. cJSON + game.
- `src/claude.h` / `src/claude.c` — Création. `ClaudeChooseMove` (libcurl) + wrapper async (pthread).
- `src/board.h` / `src/board.c` — Modification. `DrawThinkingText`, menu Difficile activé.
- `src/main.c` — Modification. `PLAY_THINKING`, sélection Difficile, init/cleanup curl.
- `tests/test_game.c` — Modification. Tests `ClaudeParseMove`.
- `CMakeLists.txt` — Modification. cJSON (pkg-config), CURL, Threads.

---

## Task 0 : Installer cJSON

- [ ] **Step 1 : Installer cJSON via Homebrew**

Run :
```sh
brew list cjson >/dev/null 2>&1 && echo "déjà installé" || brew install cjson
```
Expected : `cjson` installé. Vérifier que pkg-config le trouve :
```sh
pkg-config --exists libcjson && echo OK
```
Expected : `OK`. (libcurl et pthread sont fournis par le système macOS.)

---

## Task 1 : `claude_parse` — prompt + parsing (TDD)

**Files:**
- Create: `src/claude_parse.h`, `src/claude_parse.c`
- Modify: `tests/test_game.c`, `CMakeLists.txt`

- [ ] **Step 1 : Créer `src/claude_parse.h`**

```c
#ifndef CLAUDE_PARSE_H
#define CLAUDE_PARSE_H

#include "game.h"

// Construit le prompt décrivant le plateau (Claude joue O) dans `out`.
void ClaudeBuildPrompt(const Game *g, char *out, int out_size);

// Parse la réponse JSON de l'API : 1er index 0..8 désignant une case vide
// légale dans content[0].text. Renvoie -1 si réponse invalide / pas de coup légal.
int ClaudeParseMove(const char *response_json, const Game *g);

#endif // CLAUDE_PARSE_H
```

- [ ] **Step 2 : Ajouter les tests (avant `int main`) dans `tests/test_game.c`**

Ajouter l'include sous les autres includes :
```c
#include "claude_parse.h"
```

Ajouter les fonctions de test :
```c
static void test_claude_parse_valid_move(void) {
    Game g;
    GameInit(&g);
    const char *json = "{\"content\":[{\"type\":\"text\",\"text\":\"4\"}]}";
    assert(ClaudeParseMove(json, &g) == 4);
}

static void test_claude_parse_with_prose(void) {
    Game g;
    GameInit(&g);
    const char *json = "{\"content\":[{\"type\":\"text\",\"text\":\"Je joue 2.\"}]}";
    assert(ClaudeParseMove(json, &g) == 2);
}

static void test_claude_parse_occupied_cell(void) {
    Game g;
    GameInit(&g);
    g.cells[4] = CELL_X;
    const char *json = "{\"content\":[{\"type\":\"text\",\"text\":\"4\"}]}";
    assert(ClaudeParseMove(json, &g) == -1);
}

static void test_claude_parse_out_of_range(void) {
    Game g;
    GameInit(&g);
    const char *json = "{\"content\":[{\"type\":\"text\",\"text\":\"9\"}]}";
    assert(ClaudeParseMove(json, &g) == -1);
}

static void test_claude_parse_invalid_json(void) {
    Game g;
    GameInit(&g);
    assert(ClaudeParseMove("not json {", &g) == -1);
}

static void test_claude_parse_missing_text(void) {
    Game g;
    GameInit(&g);
    const char *json = "{\"content\":[]}";
    assert(ClaudeParseMove(json, &g) == -1);
}
```

Et les appeler dans `main()` :
```c
    test_claude_parse_valid_move();
    test_claude_parse_with_prose();
    test_claude_parse_occupied_cell();
    test_claude_parse_out_of_range();
    test_claude_parse_invalid_json();
    test_claude_parse_missing_text();
```

- [ ] **Step 3 : Brancher cJSON + claude_parse à la cible de tests — éditer `CMakeLists.txt`**

Ajouter après `find_package(raylib REQUIRED)` :
```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(CJSON REQUIRED libcjson)
```

Remplacer le bloc `add_executable(box-game-tests ...)` et ses options par :
```cmake
add_executable(box-game-tests
    tests/test_game.c
    src/game.c
    src/ai.c
    src/claude_parse.c
)
target_include_directories(box-game-tests PRIVATE src ${CJSON_INCLUDE_DIRS})
target_link_directories(box-game-tests PRIVATE ${CJSON_LIBRARY_DIRS})
target_link_libraries(box-game-tests PRIVATE ${CJSON_LIBRARIES})
add_test(NAME game_tests COMMAND box-game-tests)
```

- [ ] **Step 4 : Vérifier l'échec de compilation**

Run :
```sh
cmake -B build >/dev/null && cmake --build build --target box-game-tests
```
Expected : ÉCHEC — `'claude_parse.h' file not found` (le .c n'existe pas encore).

- [ ] **Step 5 : Créer `src/claude_parse.c`**

```c
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
```

- [ ] **Step 6 : Vérifier que les tests passent**

Run :
```sh
cmake -B build >/dev/null && cmake --build build --target box-game-tests && ctest --test-dir build --output-on-failure
```
Expected : `100% tests passed`.

- [ ] **Step 7 : Commit**

```sh
git add src/claude_parse.h src/claude_parse.c tests/test_game.c CMakeLists.txt
git commit -m "feat: claude_parse (prompt + parsing JSON du coup)"
```

---

## Task 2 : `claude` — appel HTTP + thread async

**Files:**
- Create: `src/claude.h`, `src/claude.c`
- Modify: `CMakeLists.txt`

> `claude.c` est compilé dans `box-game` mais pas encore appelé par `main.c` → le build reste vert. Validation : compilation.

- [ ] **Step 1 : Créer `src/claude.h`**

```c
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
```

- [ ] **Step 2 : Créer `src/claude.c`**

```c
#include "claude.h"
#include "claude_parse.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

#define CLAUDE_MODEL "claude-opus-4-8"
#define CLAUDE_URL "https://api.anthropic.com/v1/messages"

struct ResponseBuffer {
    char *data;
    size_t size;
};

static size_t WriteCallback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total = size * nmemb;
    struct ResponseBuffer *buf = (struct ResponseBuffer *)userdata;
    char *grown = realloc(buf->data, buf->size + total + 1);
    if (grown == NULL) return 0;
    buf->data = grown;
    memcpy(buf->data + buf->size, ptr, total);
    buf->size += total;
    buf->data[buf->size] = '\0';
    return total;
}

int ClaudeChooseMove(const Game *g) {
    const char *api_key = getenv("ANTHROPIC_API_KEY");
    if (api_key == NULL || api_key[0] == '\0') return -1;

    char prompt[512];
    ClaudeBuildPrompt(g, prompt, sizeof(prompt));

    cJSON *body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "model", CLAUDE_MODEL);
    cJSON_AddNumberToObject(body, "max_tokens", 1024);
    cJSON *messages = cJSON_AddArrayToObject(body, "messages");
    cJSON *msg = cJSON_CreateObject();
    cJSON_AddStringToObject(msg, "role", "user");
    cJSON_AddStringToObject(msg, "content", prompt);
    cJSON_AddItemToArray(messages, msg);
    char *body_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);
    if (body_str == NULL) return -1;

    CURL *curl = curl_easy_init();
    if (curl == NULL) { free(body_str); return -1; }

    struct ResponseBuffer buf = {0};
    char auth_header[256];
    snprintf(auth_header, sizeof(auth_header), "x-api-key: %s", api_key);
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, auth_header);
    headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");
    headers = curl_slist_append(headers, "content-type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, CLAUDE_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_str);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    int move = -1;
    if (res == CURLE_OK && http_code >= 200 && http_code < 300 && buf.data != NULL) {
        move = ClaudeParseMove(buf.data, g);
    }

    free(buf.data);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(body_str);
    return move;
}

struct ClaudeRequest {
    pthread_t thread;
    pthread_mutex_t lock;
    Game game;
    int move;
    bool done;
};

static void *ClaudeThreadFn(void *arg) {
    struct ClaudeRequest *req = (struct ClaudeRequest *)arg;
    int move = ClaudeChooseMove(&req->game);
    pthread_mutex_lock(&req->lock);
    req->move = move;
    req->done = true;
    pthread_mutex_unlock(&req->lock);
    return NULL;
}

ClaudeRequest *ClaudeRequestStart(const Game *g) {
    struct ClaudeRequest *req = calloc(1, sizeof(struct ClaudeRequest));
    if (req == NULL) return NULL;
    req->game = *g;
    req->move = -1;
    req->done = false;
    pthread_mutex_init(&req->lock, NULL);
    if (pthread_create(&req->thread, NULL, ClaudeThreadFn, req) != 0) {
        pthread_mutex_destroy(&req->lock);
        free(req);
        return NULL;
    }
    return req;
}

bool ClaudeRequestPoll(ClaudeRequest *req, int *move_out) {
    if (req == NULL) return true; // pas de requête : terminé, sans coup (move_out inchangé)
    pthread_mutex_lock(&req->lock);
    bool done = req->done;
    if (done && move_out != NULL) *move_out = req->move;
    pthread_mutex_unlock(&req->lock);
    return done;
}

void ClaudeRequestFree(ClaudeRequest *req) {
    if (req == NULL) return;
    pthread_join(req->thread, NULL);
    pthread_mutex_destroy(&req->lock);
    free(req);
}
```

- [ ] **Step 3 : Ajouter CURL/Threads/cJSON + sources à `box-game` — éditer `CMakeLists.txt`**

Ajouter après le bloc `pkg_check_modules(CJSON ...)` :
```cmake
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)
```

Remplacer le bloc `add_executable(box-game ...)` + `target_link_libraries(box-game ...)` par :
```cmake
add_executable(box-game
    src/main.c
    src/board.c
    src/game.c
    src/ai.c
    src/claude_parse.c
    src/claude.c
)
target_include_directories(box-game PRIVATE ${CJSON_INCLUDE_DIRS})
target_link_directories(box-game PRIVATE ${CJSON_LIBRARY_DIRS})
target_link_libraries(box-game PRIVATE raylib CURL::libcurl Threads::Threads ${CJSON_LIBRARIES})
```

- [ ] **Step 4 : Vérifier que tout compile**

Run :
```sh
cmake -B build >/dev/null && cmake --build build
```
Expected : `box-game` et `box-game-tests` compilent (claude.c lié mais pas encore appelé).

- [ ] **Step 5 : Commit**

```sh
git add src/claude.h src/claude.c CMakeLists.txt
git commit -m "feat: client Claude (libcurl) + wrapper async (pthread)"
```

---

## Task 3 : Intégration menu + phase PLAY_THINKING

**Files:**
- Modify: `src/board.h`, `src/board.c`, `src/main.c`

> Validation finale visuelle, avec et sans `ANTHROPIC_API_KEY`.

- [ ] **Step 1 : Déclarer `DrawThinkingText` dans `src/board.h`**

Après `void DrawStatusText(const Game *g, Difficulty difficulty);` ajouter :
```c
void DrawThinkingText(void);
```

- [ ] **Step 2 : Modifier `src/board.c` — `DrawThinkingText` + menu activé**

Ajouter la fonction (par ex. après `DrawStatusText`) :
```c
void DrawThinkingText(void) {
    DrawText("Reflexion...", 20, BOARD_SIZE + 35, 24, MAROON);
}
```

Remplacer entièrement la fonction `DrawMenu` par (les 3 boutons actifs, hint mis à jour) :
```c
void DrawMenu(void) {
    const char *title = "Tic Tac Toe";
    int titleWidth = MeasureText(title, 50);
    DrawText(title, (WINDOW_WIDTH - titleWidth) / 2, 110, 50, DARKGRAY);

    const char *labels[3] = {"Facile", "Moyen", "Difficile"};
    for (int i = 0; i < 3; i++) {
        Rectangle r = GetMenuButtonRect(i);
        DrawRectangleRec(r, SKYBLUE);
        DrawRectangleLinesEx(r, 3, DARKGRAY);
        int tw = MeasureText(labels[i], 30);
        DrawText(labels[i], (int)(r.x + (r.width - tw) / 2),
                 (int)(r.y + (r.height - 30) / 2), 30, DARKBLUE);
    }
    const char *hint = "Difficile = Claude (ANTHROPIC_API_KEY)";
    int hw = MeasureText(hint, 18);
    DrawText(hint, (WINDOW_WIDTH - hw) / 2, 560, 18, GRAY);
}
```

- [ ] **Step 3 : Remplacer tout le contenu de `src/main.c`**

```c
#include "raylib.h"
#include <stdlib.h>
#include <time.h>
#include <curl/curl.h>
#include "board.h"
#include "game.h"
#include "ai.h"
#include "claude.h"

typedef enum { STATE_MENU, STATE_PLAYING } AppState;
typedef enum { PLAY_IDLE, PLAY_ANIMATING, PLAY_WAIT_AI, PLAY_THINKING } PlayPhase;

#define ANIM_DURATION 0.20f
#define AI_DELAY 0.35f

static void ResetProgress(float progress[9]) {
    for (int i = 0; i < 9; i++) progress[i] = 0.0f;
}

int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Box Game — Tic Tac Toe");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    srand((unsigned int)time(NULL));
    curl_global_init(CURL_GLOBAL_DEFAULT);

    AppState state = STATE_MENU;
    Difficulty difficulty = DIFFICULTY_EASY;
    Game game;
    GameInit(&game);

    float progress[9];
    ResetProgress(progress);
    PlayPhase phase = PLAY_IDLE;
    int animCell = -1;
    float animTime = 0.0f;
    float aiWait = 0.0f;
    ClaudeRequest *req = NULL;

    while (!WindowShouldClose()) {
        if (state == STATE_MENU) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 m = GetMousePosition();
                for (int i = 0; i < 3; i++) {
                    if (CheckCollisionPointRec(m, GetMenuButtonRect(i))) {
                        difficulty = (i == 0) ? DIFFICULTY_EASY
                                   : (i == 1) ? DIFFICULTY_MEDIUM
                                              : DIFFICULTY_HARD;
                        GameInit(&game);
                        ResetProgress(progress);
                        phase = PLAY_IDLE;
                        state = STATE_PLAYING;
                    }
                }
            }
            BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawMenu();
            EndDrawing();
        } else { // STATE_PLAYING
            float dt = GetFrameTime();

            if (IsKeyPressed(KEY_ESCAPE)) {
                if (req != NULL) { ClaudeRequestFree(req); req = NULL; }
                state = STATE_MENU;
            } else if (phase == PLAY_IDLE) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (game.status != GAME_PLAYING) {
                        GameInit(&game);
                        ResetProgress(progress);
                    } else if (game.turn == CELL_X) {
                        Vector2 m = GetMousePosition();
                        if (m.y < BOARD_SIZE) {
                            int col = (int)(m.x / CELL_SIZE);
                            int row = (int)(m.y / CELL_SIZE);
                            if (col >= 0 && col < 3 && row >= 0 && row < 3) {
                                int index = row * 3 + col;
                                if (GamePlayMove(&game, index)) {
                                    animCell = index;
                                    progress[index] = 0.0f;
                                    animTime = 0.0f;
                                    phase = PLAY_ANIMATING;
                                }
                            }
                        }
                    }
                }
            } else if (phase == PLAY_ANIMATING) {
                animTime += dt;
                float p = animTime / ANIM_DURATION;
                if (p >= 1.0f) p = 1.0f;
                progress[animCell] = p;
                if (p >= 1.0f) {
                    if (game.status == GAME_PLAYING && game.turn == CELL_O) {
                        if (difficulty == DIFFICULTY_HARD) {
                            req = ClaudeRequestStart(&game);
                            phase = PLAY_THINKING;
                        } else {
                            phase = PLAY_WAIT_AI;
                            aiWait = 0.0f;
                        }
                    } else {
                        phase = PLAY_IDLE;
                    }
                }
            } else if (phase == PLAY_WAIT_AI) {
                aiWait += dt;
                if (aiWait >= AI_DELAY) {
                    int move = AiChooseMove(&game, difficulty);
                    if (move >= 0 && GamePlayMove(&game, move)) {
                        animCell = move;
                        progress[move] = 0.0f;
                        animTime = 0.0f;
                        phase = PLAY_ANIMATING;
                    } else {
                        phase = PLAY_IDLE;
                    }
                }
            } else { // PLAY_THINKING
                int move = -1;
                if (ClaudeRequestPoll(req, &move)) {
                    ClaudeRequestFree(req);
                    req = NULL;
                    if (move < 0) move = AiChooseMove(&game, DIFFICULTY_MEDIUM);
                    if (move >= 0 && GamePlayMove(&game, move)) {
                        animCell = move;
                        progress[move] = 0.0f;
                        animTime = 0.0f;
                        phase = PLAY_ANIMATING;
                    } else {
                        phase = PLAY_IDLE;
                    }
                }
            }

            BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawBoardGrid();
            DrawMarks(&game, progress);
            if (phase == PLAY_THINKING) {
                DrawThinkingText();
            } else {
                DrawStatusText(&game, difficulty);
            }
            EndDrawing();
        }
    }

    if (req != NULL) { ClaudeRequestFree(req); req = NULL; }
    curl_global_cleanup();
    CloseWindow();
    return 0;
}
```

- [ ] **Step 4 : Recompiler**

Run :
```sh
cmake --build build
```
Expected : compilation réussie.

- [ ] **Step 5 : Vérification visuelle — SANS clé API (repli)**

Run :
```sh
unset ANTHROPIC_API_KEY; ./build/box-game
```
Expected : le menu montre les 3 boutons actifs. En Difficile, après ton coup, « Reflexion... » s'affiche très brièvement puis l'IA joue un coup (repli heuristique Moyen) — aucun blocage, fenêtre fluide.

- [ ] **Step 6 : Vérification visuelle — AVEC clé API (Claude joue)**

Run :
```sh
export ANTHROPIC_API_KEY="<ta-clé>"; ./build/box-game
```
Expected : en Difficile, « Reflexion... » s'affiche ~1-2 s pendant que la fenêtre reste fluide, puis Claude pose un coup légal (animé). Échap revient au menu.

- [ ] **Step 7 : Relancer la suite de tests**

Run :
```sh
ctest --test-dir build --output-on-failure
```
Expected : `100% tests passed`.

- [ ] **Step 8 : Commit**

```sh
git add src/board.h src/board.c src/main.c
git commit -m "feat: niveau Difficile (Claude) + phase PLAY_THINKING"
```

---

## Auto-revue du plan

- **Couverture de la spec :**
  - Bouton Difficile actif (Task 3 menu `i < 3`, `DrawMenu`) ✓
  - Thread d'arrière-plan + « Réflexion… » (Task 2 wrapper async ; Task 3 `PLAY_THINKING` + `DrawThinkingText`) ✓
  - Repli sur Moyen sur toute erreur / pas de clé / coup illégal (Task 2 `ClaudeChooseMove` renvoie -1 ; Task 3 `AiChooseMove(..., DIFFICULTY_MEDIUM)`) ✓
  - Clé via `ANTHROPIC_API_KEY` (Task 2) ✓
  - `claude_parse` testé séparément du réseau (Task 1) ✓
  - Requête HTTP correcte : `claude-opus-4-8`, headers, pas de `temperature` (Task 2) ✓
  - Build cJSON/CURL/Threads ; tests ne lient que cJSON (Tasks 1-2) ✓
- **Placeholders :** aucun — code complet.
- **Cohérence des types/signatures :** `ClaudeBuildPrompt(const Game*, char*, int)`, `ClaudeParseMove(const char*, const Game*)`, `ClaudeChooseMove(const Game*)`, `ClaudeRequestStart/Poll/Free`, `DrawThinkingText(void)`, enum `PlayPhase` étendu avec `PLAY_THINKING` — identiques entre `.h`, `.c`, `main.c` et tests. Repli via l'API publique existante `AiChooseMove(..., DIFFICULTY_MEDIUM)`.
