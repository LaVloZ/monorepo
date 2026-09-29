<template>
  <form class="card" @submit.prevent="submit">
    <h2>Nouvelle session</h2>
    <label>Perte max du jour ({{ form.currency }})
      <input ref="first" type="number" step="any" v-model.number="form.lossLimit" />
    </label>
    <label>Objectif de gain ({{ form.currency }})
      <input type="number" step="any" v-model.number="form.gainTarget" />
    </label>
    <label>Max trades
      <input type="number" min="0" v-model.number="form.maxTrades" />
    </label>
    <label>Max trades perdants
      <input type="number" min="0" v-model.number="form.maxLossTrades" />
    </label>
    <label>Devise
      <select v-model="form.currency">
        <option value="EUR">EUR (€)</option>
        <option value="USD">USD ($)</option>
      </select>
    </label>
    <p v-if="error" class="err">{{ error }}</p>
    <button type="submit" class="go">▶ Démarrer la session</button>
  </form>
</template>

<script setup>
import { reactive, ref, onMounted } from "vue";
import { useSettingsStore } from "../stores/settings.js";

const emit = defineEmits(["start"]);
const settings = useSettingsStore();
const first = ref(null);
const form = reactive({ ...settings.settings });
const error = ref("");

onMounted(() => first.value?.focus());

function submit() {
  const params = { ...form };
  for (const k of ["lossLimit", "gainTarget", "maxTrades", "maxLossTrades"]) {
    if (params[k] === "" || params[k] === null || Number.isNaN(Number(params[k]))) {
      error.value = "Renseigne tous les champs numériques pour démarrer.";
      return;
    }
    params[k] = Number(params[k]);
  }
  error.value = "";
  emit("start", params);
}
</script>

<style scoped>
.card { background: var(--panel); border: 1px solid var(--border); border-radius: 10px; padding: 16px; display: flex; flex-direction: column; gap: 10px; }
label { display: flex; flex-direction: column; gap: 4px; font-size: 12px; color: var(--muted); }
.go { background: var(--accent); border-color: var(--accent); color: #fff; margin-top: 6px; }
</style>
