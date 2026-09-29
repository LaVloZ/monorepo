<template>
  <div class="cal">
    <div class="head">{{ monthLabel }}</div>
    <div class="grid">
      <span v-for="d in ['L','M','M','J','V','S','D']" :key="d" class="dow">{{ d }}</span>
      <span v-for="n in offset" :key="'b' + n" class="cell empty"></span>
      <span
        v-for="day in daysInMonth"
        :key="day"
        class="cell"
        :class="cls(day)"
        :title="titleFor(day)"
      >{{ day }}</span>
    </div>
  </div>
</template>

<script setup>
import { computed } from "vue";
const props = defineProps({ pnlByDay: { type: Map, required: true }, year: Number, month: Number });

const MONTHS = ["janvier","février","mars","avril","mai","juin","juillet","août","septembre","octobre","novembre","décembre"];
const monthLabel = computed(() => `${MONTHS[props.month]} ${props.year}`);
const daysInMonth = computed(() => new Date(props.year, props.month + 1, 0).getDate());
const offset = computed(() => {
  const jsDay = new Date(props.year, props.month, 1).getDay(); // 0=dim
  return (jsDay + 6) % 7; // lundi = 0
});
function key(day) {
  const mm = String(props.month + 1).padStart(2, "0");
  const dd = String(day).padStart(2, "0");
  return `${props.year}-${mm}-${dd}`;
}
function cls(day) {
  const v = props.pnlByDay.get(key(day));
  if (v === undefined) return "flat";
  return v > 0 ? "win" : v < 0 ? "lose" : "flat";
}
function titleFor(day) {
  const v = props.pnlByDay.get(key(day));
  return v === undefined ? "" : `${v >= 0 ? "+" : ""}${v.toFixed(2)}`;
}
</script>

<style scoped>
.cal { background: var(--panel); border: 1px solid var(--border); border-radius: 10px; padding: 12px; }
.head { text-align: center; margin-bottom: 8px; text-transform: capitalize; }
.grid { display: grid; grid-template-columns: repeat(7, 1fr); gap: 4px; }
.dow { text-align: center; font-size: 11px; color: var(--muted); }
.cell { aspect-ratio: 1; display: flex; align-items: center; justify-content: center; border-radius: 4px; font-size: 12px; background: var(--bg); border: 1px solid var(--border); }
.cell.empty { background: transparent; border: none; }
.cell.win { background: rgba(41,196,106,.25); border-color: var(--green); }
.cell.lose { background: rgba(242,73,92,.25); border-color: var(--red); }
</style>
