#include "history.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void history_init(history *h)
{
    h->items = NULL;
    h->count = 0;
}

int history_add(history *h, const char *time, tr_result *res)
{
    if (h->count == HISTORY_MAX) {
        tr_result_free(&h->items[0].result);
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
    snprintf(e->time, sizeof e->time, "%s", time);
    e->result = *res;
    tr_result_init(res);
    return 0;
}

const hist_entry *history_get(const history *h, size_t i)
{
    if (i >= h->count)
        return NULL;
    return &h->items[h->count - 1 - i];
}

void history_clear(history *h)
{
    for (size_t i = 0; i < h->count; i++)
        tr_result_free(&h->items[i].result);
    free(h->items);
    history_init(h);
}
