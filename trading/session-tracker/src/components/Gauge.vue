<template>
  <div class="gauge">
    <div class="row">
      <span class="label">{{ label }}</span>
      <span class="pct">{{ Math.round(pct * 100) }}%</span>
    </div>
    <div class="track">
      <div class="fill" :class="color" :style="{ width: Math.round(pct * 100) + '%' }"></div>
    </div>
  </div>
</template>

<script setup>
import { computed } from "vue";
const props = defineProps({
  label: String,
  value: { type: Number, default: 0 },
  max: Number,
  variant: { type: String, default: "neutral" },
  warn: { type: Boolean, default: false },
});
const pct = computed(() => {
  if (!props.max || props.max <= 0) return 0;
  return Math.min(1, Math.max(0, props.value / props.max));
});
const color = computed(() => (props.warn ? "amber" : props.variant));
</script>

<style scoped>
.gauge { margin: 8px 0; }
.row { display: flex; justify-content: space-between; font-size: 12px; color: var(--muted); }
.track { height: 10px; background: var(--bg); border: 1px solid var(--border); border-radius: 6px; overflow: hidden; }
.fill { height: 100%; transition: width .3s; }
.fill.loss { background: var(--red); }
.fill.gain { background: var(--green); }
.fill.neutral { background: var(--accent); }
.fill.amber { background: var(--amber); }
</style>
