<template>
  <div v-if="reasons.length" class="banner">
    <strong>⛔ STOP TRADING</strong>
    <span class="why">{{ label }}</span>
  </div>
</template>

<script setup>
import { computed } from "vue";
const props = defineProps({ reasons: { type: Array, default: () => [] } });
const LABELS = {
  lossLimit: "perte max atteinte",
  maxTrades: "nombre max de trades atteint",
  maxLossTrades: "nombre max de trades perdants atteint",
};
const label = computed(() => props.reasons.map((r) => LABELS[r] || r).join(" · "));
</script>

<style scoped>
.banner {
  background: var(--red); color: #fff; padding: 10px 12px; border-radius: 8px;
  display: flex; flex-direction: column; gap: 2px; margin-bottom: 12px; text-align: center;
}
.why { font-size: 12px; opacity: .9; }
</style>
