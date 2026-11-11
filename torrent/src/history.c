#include "history.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t next_id = 1;

void history_init(history *h)
{
    h->items = NULL;
    h->count = 0;
}

static void entry_free(hist_entry *e)
{
    tr_result_free(&e->result);
    free(e->probes);
    e->probes = NULL;
}

int history_add(history *h, const char *time, tr_result *res)
{
    if (h->count == HISTORY_MAX) {
        entry_free(&h->items[0]);
        memmove(h->items, h->items + 1, (h->count - 1) * sizeof *h->items);
        h->count--;
    } else {
        hist_entry *p = realloc(h->items, (h->count + 1) * sizeof *p);
        if (!p) {
            tr_result_free(res);
            return -1;
        }
        h->items = p;
    }
    hist_entry *e = &h->items[h->count++];
    e->id = next_id++;
    snprintf(e->time, sizeof e->time, "%s", time);
    e->result = *res;
    e->probes = NULL;
    tr_result_init(res);
    return 0;
}

hist_entry *history_get(const history *h, size_t i)
{
    if (i >= h->count)
        return NULL;
    return &h->items[h->count - 1 - i];
}

hist_entry *history_find(const history *h, uint32_t id)
{
    for (size_t i = 0; i < h->count; i++)
        if (h->items[i].id == id)
            return &h->items[i];
    return NULL;
}

void history_clear(history *h)
{
    for (size_t i = 0; i < h->count; i++)
        entry_free(&h->items[i]);
    free(h->items);
    history_init(h);
}
