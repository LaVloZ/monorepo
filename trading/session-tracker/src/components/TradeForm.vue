<template>
  <form class="card" @submit.prevent="submit">
    <div class="grid">
      <input ref="tickerEl" v-model="f.ticker" placeholder="Ticker (ex. NQ)" />
      <div class="side">
        <button type="button" :class="{ on: f.side === 'long' }" @click="f.side = 'long'">Long</button>
        <button type="button" :class="{ on: f.side === 'short' }" @click="f.side = 'short'">Short</button>
      </div>
    </div>
    <div class="grid three">
      <input v-model.number="f.size" type="number" step="any" placeholder="Taille" />
      <input v-model.number="f.entry" type="number" step="any" placeholder="Entrée" />
      <input v-model.number="f.exit" type="number" step="any" placeholder="Sortie" />
    </div>
    <div class="grid">
      <input v-if="autoPnl === null" v-model.number="f.pnl" type="number" step="any" placeholder="Résultat (€/$)" />
      <input v-else :value="autoPnl" type="number" readonly class="ro" title="Calculé depuis taille/entrée/sortie" />
      <input v-model.number="f.rMultiple" type="number" step="any" placeholder="R (optionnel)" />
    </div>
    <input v-model="f.note" placeholder="Note (setup, erreur, émotion)" />
    <input v-model="f.tagsCsv" placeholder="Tags (séparés par des virgules)" />
    <button type="submit" class="add">＋ Ajouter le trade (Entrée)</button>
    <p v-if="disabled" class="warn">⛔ Limite atteinte — trade quand même possible.</p>
    <p v-if="error" class="err">{{ error }}</p>
  </form>
</template>

<script setup>
import { reactive, ref, computed, onMounted } from "vue";
import { useSessionStore } from "../stores/session.js";
import { computePnl } from "../lib/metrics.js";

const props = defineProps({ disabled: { type: Boolean, default: false } });
const emit = defineEmits(["added"]);
const session = useSessionStore();
const tickerEl = ref(null);
const empty = () => ({
  ticker: "", side: "long", pnl: null, rMultiple: null, note: "", tagsCsv: "",
  size: null, entry: null, exit: null,
});
const f = reactive(empty());
const error = ref("");

const autoPnl = computed(() =>
  computePnl({ size: f.size, entry: f.entry, exit: f.exit, side: f.side })
);

onMounted(() => {
  f.ticker = session.lastTicker || "";
  tickerEl.value?.focus();
});

const num = (v) => (v === "" || v === null ? null : Number(v));

async function submit() {
  const effectivePnl = autoPnl.value !== null ? autoPnl.value : Number(f.pnl);
  const pnlProvided = autoPnl.value !== null || (f.pnl !== "" && f.pnl !== null);
  if (f.ticker.trim() === "" || !pnlProvided || Number.isNaN(effectivePnl)) return;
  const r = f.rMultiple === "" || f.rMultiple === null ? null : Number(f.rMultiple);
  try {
    await session.addTrade({
      ticker: f.ticker.trim().toUpperCase(),
      side: f.side,
      pnl: effectivePnl,
      rMultiple: r,
      note: f.note.trim(),
      tags: f.tagsCsv.split(",").map((t) => t.trim()).filter(Boolean),
      size: num(f.size),
      entry: num(f.entry),
      exit: num(f.exit),
    });
  } catch (e) {
    error.value = "Échec de l'enregistrement du trade — réessaie.";
    return;
  }
  error.value = "";
  const keptTicker = f.ticker;
  Object.assign(f, empty());
  f.ticker = keptTicker;
  tickerEl.value?.focus();
  emit("added");
}
</script>

<style scoped>
.card { background: var(--panel); border: 1px solid var(--border); border-radius: 10px; padding: 12px; display: flex; flex-direction: column; gap: 8px; }
.grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; }
.grid.three { grid-template-columns: 1fr 1fr 1fr; }
.side { display: flex; gap: 6px; }
.side button { flex: 1; }
.side button.on { background: var(--accent); border-color: var(--accent); color: #fff; }
.ro { opacity: .8; background: var(--panel); }
.add { background: var(--green); border-color: var(--green); color: #06210f; font-weight: 600; }
.warn { color: var(--amber); font-size: 12px; margin: 0; text-align: center; }
</style>
