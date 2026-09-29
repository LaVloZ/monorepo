<template>
  <form class="card" @submit.prevent="save">
    <h2>Paramètres par défaut</h2>
    <label>Perte max du jour
      <input type="number" step="any" v-model.number="f.lossLimit" />
    </label>
    <label>Objectif de gain
      <input type="number" step="any" v-model.number="f.gainTarget" />
    </label>
    <label>Max trades
      <input type="number" min="0" v-model.number="f.maxTrades" />
    </label>
    <label>Max trades perdants
      <input type="number" min="0" v-model.number="f.maxLossTrades" />
    </label>
    <label>Devise
      <select v-model="f.currency">
        <option value="EUR">EUR (€)</option>
        <option value="USD">USD ($)</option>
      </select>
    </label>
    <button type="submit" class="save">Enregistrer</button>
    <p v-if="error" class="err">{{ error }}</p>
    <p v-if="saved" class="ok">Enregistré ✓</p>
  </form>
</template>

<script setup>
import { reactive, ref } from "vue";
import { useSettingsStore } from "../stores/settings.js";

const settings = useSettingsStore();
const f = reactive({ ...settings.settings });
const saved = ref(false);
const error = ref("");

async function save() {
  const clean = { ...f };
  for (const k of ["lossLimit", "gainTarget", "maxTrades", "maxLossTrades"]) {
    if (clean[k] === "" || clean[k] === null || Number.isNaN(Number(clean[k]))) {
      error.value = "Tous les champs numériques doivent être renseignés.";
      return;
    }
    clean[k] = Number(clean[k]);
  }
  error.value = "";
  await settings.save(clean);
  saved.value = true;
  setTimeout(() => (saved.value = false), 1500);
}
</script>

<style scoped>
.card { background: var(--panel); border: 1px solid var(--border); border-radius: 10px; padding: 16px; display: flex; flex-direction: column; gap: 10px; }
label { display: flex; flex-direction: column; gap: 4px; font-size: 12px; color: var(--muted); }
.save { background: var(--accent); border-color: var(--accent); color: #fff; }
.ok { color: var(--green); font-size: 12px; margin: 0; }
</style>
