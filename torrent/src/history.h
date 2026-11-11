#ifndef HISTORY_H
#define HISTORY_H

#include <stddef.h>
#include <stdint.h>

#include "peer_probe.h"
#include "tracker.h"

#define HISTORY_MAX 100

typedef struct {
    uint32_t id;   /* unique, stable while the entry lives */
    char time[32]; /* local time of the announce */
    tr_result result;
    probe_result *probes; /* one per peer once probed, NULL before */
} hist_entry;

/* Announces of one tracker, oldest first. Kept in memory only. */
typedef struct {
    hist_entry *items;
    size_t count;
} history;

void history_init(history *h);
/* Moves *res into the history (res is reset). Drops the oldest beyond HISTORY_MAX. */
int history_add(history *h, const char *time, tr_result *res);
/* i = 0 is the newest entry. NULL if out of range. */
hist_entry *history_get(const history *h, size_t i);
/* Entry with this id, NULL if it was dropped. */
hist_entry *history_find(const history *h, uint32_t id);
void history_clear(history *h);

#endif
