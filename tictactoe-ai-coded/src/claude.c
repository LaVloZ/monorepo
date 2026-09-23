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
