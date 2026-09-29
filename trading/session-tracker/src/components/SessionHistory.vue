<template>
  <section class="hist">
    <SessionDetail
      v-if="selectedSession"
      :session="selectedSession"
      :trades="selectedTrades"
      @back="selected = null"
    />
    <template v-else>
      <PnlCalendar :pnl-by-day="byDay" :year="year" :month="month" />
      <h3>Sessions</h3>
      <ul class="list">
        <li v-for="r in summaries" :key="r.id" class="clickable" @click="selected = r.id" tabindex="0" role="button" @keydown.enter="selected = r.id" @keydown.space.prevent="selected = r.id">
          <span class="date">{{ fmt(r.startedAt) }}</span>
          <span :class="r.pnl >= 0 ? 'pos' : 'neg'">{{ r.pnl >= 0 ? '+' : '' }}{{ r.pnl.toFixed(2) }}</span>
          <span class="meta">{{ r.count }} trades · {{ r.winRate === null ? '—' : Math.round(r.winRate * 100) + '%' }}</span>
        </li>
      </ul>
      <p v-if="!summaries.length" class="muted">Aucune session enregistrée.</p>
      <p v-if="error" class="err">{{ error }}</p>
    </template>
  </section>
</template>

<script setup>
import { ref, computed, onMounted } from "vue";
import { find } from "../db.js";
import { sessionSummaries, pnlByDay } from "../lib/history.js";
import PnlCalendar from "./PnlCalendar.vue";
import SessionDetail from "./SessionDetail.vue";

const sessions = ref([]);
const trades = ref([]);
const error = ref("");
const selected = ref(null);
const nowDate = new Date();
const year = nowDate.getFullYear();
const month = nowDate.getMonth();

onMounted(async () => {
  try {
    sessions.value = await find({ type: "session" });
    trades.value = await find({ type: "trade" });
  } catch (e) {
    error.value = "Impossible de charger l'historique.";
  }
});

const byDay = computed(() => pnlByDay(sessions.value, trades.value));
const summaries = computed(() => {
  const byS = {};
  for (const t of trades.value) (byS[t.sessionId] ||= []).push(t);
  return sessionSummaries(sessions.value, byS)
    .sort((a, b) => b.startedAt.localeCompare(a.startedAt));
});
const selectedSession = computed(() => sessions.value.find((s) => s._id === selected.value) || null);
const selectedTrades = computed(() => trades.value.filter((t) => t.sessionId === selected.value));

function fmt(iso) {
  return new Date(iso).toLocaleString("fr-FR", { dateStyle: "short", timeStyle: "short" });
}
</script>

<style scoped>
.hist { display: flex; flex-direction: column; gap: 12px; }
.list { list-style: none; padding: 0; margin: 0; display: flex; flex-direction: column; gap: 6px; }
.list li { display: grid; grid-template-columns: 1fr auto; gap: 2px 8px; background: var(--panel); border: 1px solid var(--border); border-radius: 8px; padding: 8px 10px; }
.clickable { cursor: pointer; }
.clickable:hover { border-color: var(--accent); }
.clickable:focus { outline: 2px solid var(--accent); outline-offset: 1px; }
.date { font-size: 13px; }
.meta { grid-column: 1 / -1; font-size: 11px; color: var(--muted); }
.muted { color: var(--muted); }
</style>
