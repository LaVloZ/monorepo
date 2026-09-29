<template>
  <section class="detail">
    <button class="back" @click="emit('back')">← Retour</button>
    <div class="stats">
      <div class="pnl" :class="pnl >= 0 ? 'pos' : 'neg'">{{ pnl >= 0 ? '+' : '' }}{{ pnl.toFixed(2) }}</div>
      <div class="meta">
        <span>{{ trades.length }} trades</span>
        <span>{{ wr === null ? '—' : Math.round(wr * 100) + '%' }} réussite</span>
        <span>⏱ {{ duration }}</span>
      </div>
      <div class="meta" v-if="trades.length">
        <span>Meilleur : <b :class="best.pnl >= 0 ? 'pos' : 'neg'">{{ best.pnl >= 0 ? '+' : '' }}{{ best.pnl.toFixed(2) }}</b></span>
        <span>Pire : <b :class="worst.pnl >= 0 ? 'pos' : 'neg'">{{ worst.pnl >= 0 ? '+' : '' }}{{ worst.pnl.toFixed(2) }}</b></span>
      </div>
    </div>
    <ul class="list">
      <li v-for="t in ordered" :key="t._id">
        <div class="row1">
          <span>{{ t.side === 'long' ? '▲' : '▼' }} {{ t.ticker }}</span>
          <span :class="t.pnl >= 0 ? 'pos' : 'neg'">{{ t.pnl >= 0 ? '+' : '' }}{{ t.pnl.toFixed(2) }}</span>
        </div>
        <div class="row2">
          <span>{{ time(t.createdAt) }}</span>
          <span v-if="t.rMultiple != null">R {{ t.rMultiple }}</span>
          <span v-if="t.size != null && t.entry != null && t.exit != null">{{ t.size }} @ {{ t.entry }} → {{ t.exit }}</span>
        </div>
        <div class="row2 note" v-if="t.note">{{ t.note }}</div>
        <div class="tags" v-if="t.tags && t.tags.length">
          <span v-for="tag in t.tags" :key="tag" class="tag">{{ tag }}</span>
        </div>
      </li>
    </ul>
    <p v-if="!trades.length" class="muted">Aucun trade.</p>
  </section>
</template>

<script setup>
import { computed } from "vue";
import { sumPnl, winRate, bestTrade, worstTrade, formatDuration } from "../lib/metrics.js";

const props = defineProps({
  session: { type: Object, required: true },
  trades: { type: Array, default: () => [] },
});
const emit = defineEmits(["back"]);

const pnl = computed(() => sumPnl(props.trades));
const wr = computed(() => winRate(props.trades));
const best = computed(() => bestTrade(props.trades));
const worst = computed(() => worstTrade(props.trades));
const ordered = computed(() => [...props.trades].sort((a, b) => a.createdAt.localeCompare(b.createdAt)));
const duration = computed(() => {
  const end = new Date(props.session.endedAt || props.session.startedAt).getTime();
  return formatDuration(end - new Date(props.session.startedAt).getTime());
});
function time(iso) {
  return new Date(iso).toLocaleTimeString("fr-FR", { hour: "2-digit", minute: "2-digit" });
}
</script>

<style scoped>
.detail { display: flex; flex-direction: column; gap: 10px; }
.back { align-self: flex-start; }
.stats { background: var(--panel); border: 1px solid var(--border); border-radius: 10px; padding: 12px; text-align: center; }
.pnl { font-size: 28px; font-weight: 700; font-variant-numeric: tabular-nums; }
.meta { display: flex; justify-content: center; gap: 12px; font-size: 12px; color: var(--muted); margin-top: 4px; flex-wrap: wrap; }
.list { list-style: none; padding: 0; margin: 0; display: flex; flex-direction: column; gap: 6px; }
.list li { background: var(--panel); border: 1px solid var(--border); border-radius: 8px; padding: 8px 10px; display: flex; flex-direction: column; gap: 2px; }
.row1 { display: flex; justify-content: space-between; font-variant-numeric: tabular-nums; }
.row2 { display: flex; gap: 10px; font-size: 11px; color: var(--muted); }
.note { font-style: italic; }
.tags { display: flex; gap: 4px; flex-wrap: wrap; }
.tag { font-size: 10px; background: var(--bg); border: 1px solid var(--border); border-radius: 4px; padding: 1px 5px; color: var(--muted); }
.muted { color: var(--muted); }
</style>
