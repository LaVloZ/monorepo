<template>
  <section class="dash">
    <header class="top">
      <span class="timer">⏱ {{ duration }}</span>
      <button class="close" @click="closeSession">Clôturer</button>
    </header>

    <StopBanner :reasons="s.stop.reasons" />
    <p v-if="error" class="err">{{ error }}</p>

    <div class="pnl" :class="s.pnl >= 0 ? 'pos' : 'neg'">
      {{ s.pnl >= 0 ? "+" : "" }}{{ s.pnl.toFixed(2) }} {{ sym }}
    </div>

    <Gauge label="Perte max" :value="lossConsumed" :max="p.lossLimit" variant="loss" :warn="s.stop.lossWarning" />
    <Gauge label="Objectif de gain" :value="Math.max(0, s.pnl)" :max="p.gainTarget" variant="gain" />

    <div class="counters">
      <div class="c">
        <span class="k">Trades</span>
        <span class="v" :class="{ neg: p.maxTrades > 0 && s.trades.length >= p.maxTrades }">{{ s.trades.length }} / {{ p.maxTrades > 0 ? p.maxTrades : '∞' }}</span>
      </div>
      <div class="c">
        <span class="k">Perdants</span>
        <span class="v" :class="{ neg: p.maxLossTrades > 0 && s.losingCount >= p.maxLossTrades }">{{ s.losingCount }} / {{ p.maxLossTrades > 0 ? p.maxLossTrades : '∞' }}</span>
      </div>
      <div class="c">
        <span class="k">Taux de réussite</span>
        <span class="v">{{ s.winRate === null ? "—" : Math.round(s.winRate * 100) + "%" }}</span>
      </div>
    </div>

    <div class="extremes" v-if="s.trades.length">
      <span>Meilleur : <b :class="s.best.pnl >= 0 ? 'pos' : 'neg'">{{ s.best.pnl >= 0 ? '+' : '' }}{{ s.best.pnl.toFixed(2) }}</b></span>
      <span>Pire : <b :class="s.worst.pnl >= 0 ? 'pos' : 'neg'">{{ s.worst.pnl >= 0 ? '+' : '' }}{{ s.worst.pnl.toFixed(2) }}</b></span>
    </div>

    <TradeForm :disabled="s.stop.stopped" @added="() => {}" />

    <ul class="list">
      <li v-for="t in reversed" :key="t._id">
        <span>{{ t.side === 'long' ? '▲' : '▼' }} {{ t.ticker }}</span>
        <span :class="t.pnl >= 0 ? 'pos' : 'neg'">{{ t.pnl >= 0 ? '+' : '' }}{{ t.pnl.toFixed(2) }}</span>
      </li>
    </ul>
  </section>
</template>

<script setup>
import { computed, ref } from "vue";
import { useSessionStore } from "../stores/session.js";
import { useTimer } from "../composables/useTimer.js";
import { formatDuration } from "../lib/metrics.js";
import Gauge from "./Gauge.vue";
import StopBanner from "./StopBanner.vue";
import TradeForm from "./TradeForm.vue";

const emit = defineEmits(["closed"]);
const s = useSessionStore();
const { now } = useTimer();

const p = computed(() => s.current.params);
const sym = computed(() => (p.value.currency === "USD" ? "$" : "€"));
const lossConsumed = computed(() => Math.max(0, -s.pnl));
const duration = computed(() => formatDuration(now.value - new Date(s.current.startedAt).getTime()));
const reversed = computed(() => [...s.trades].reverse());
const error = ref("");

async function closeSession() {
  try {
    await s.close();
  } catch (e) {
    error.value = "Échec de la clôture — réessaie.";
    return;
  }
  error.value = "";
  emit("closed");
}
</script>

<style scoped>
.dash { display: flex; flex-direction: column; gap: 10px; }
.top { display: flex; justify-content: space-between; align-items: center; }
.timer { font-variant-numeric: tabular-nums; color: var(--muted); }
.close { border-color: var(--red); color: var(--red); }
.pnl { font-size: 42px; font-weight: 700; text-align: center; font-variant-numeric: tabular-nums; }
.counters { display: flex; gap: 8px; }
.c { flex: 1; background: var(--panel); border: 1px solid var(--border); border-radius: 8px; padding: 8px; text-align: center; }
.c .k { display: block; font-size: 11px; color: var(--muted); }
.c .v { font-size: 16px; font-weight: 600; font-variant-numeric: tabular-nums; }
.extremes { display: flex; justify-content: space-between; font-size: 12px; color: var(--muted); }
.list { list-style: none; padding: 0; margin: 4px 0 0; display: flex; flex-direction: column; gap: 4px; }
.list li { display: flex; justify-content: space-between; background: var(--panel); border: 1px solid var(--border); border-radius: 6px; padding: 6px 10px; font-variant-numeric: tabular-nums; }
</style>
