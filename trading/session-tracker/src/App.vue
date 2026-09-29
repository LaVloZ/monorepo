<template>
  <main class="app">
    <nav class="tabs">
      <button :class="{ on: view === 'session' }" @click="view = 'session'">Session</button>
      <button :class="{ on: view === 'history' }" @click="view = 'history'">Historique</button>
      <button :class="{ on: view === 'settings' }" @click="view = 'settings'">Paramètres</button>
    </nav>

    <p v-if="error" class="err">{{ error }}</p>

    <template v-if="!loading">
      <template v-if="view === 'session'">
        <LiveDashboard v-if="session.current" @closed="onClosed" />
        <StartSession v-else @start="onStart" />
      </template>
      <SessionHistory v-else-if="view === 'history'" :key="historyKey" />
      <SettingsView v-else />
    </template>
    <p v-else class="muted">Connexion à la base…</p>
  </main>
</template>

<script setup>
import { ref, onMounted } from "vue";
import { login } from "./db.js";
import { useSettingsStore } from "./stores/settings.js";
import { useSessionStore } from "./stores/session.js";
import StartSession from "./components/StartSession.vue";
import LiveDashboard from "./components/LiveDashboard.vue";
import SessionHistory from "./components/SessionHistory.vue";
import SettingsView from "./components/SettingsView.vue";

const view = ref("session");
const loading = ref(true);
const error = ref("");
const historyKey = ref(0);
const settings = useSettingsStore();
const session = useSessionStore();

onMounted(async () => {
  try {
    await login();
    await settings.load();
    await session.loadOpen();
  } catch (e) {
    error.value = "Impossible de joindre CouchDB. Est-il lancé ? (" + e.message + ")";
  } finally {
    loading.value = false;
  }
});

async function onStart(params) {
  await session.start(params);
}
function onClosed() {
  historyKey.value++; // force le rechargement de l'historique la prochaine fois
}
</script>

<style scoped>
.app { max-width: 420px; margin: 0 auto; padding: 12px; display: flex; flex-direction: column; gap: 12px; }
.tabs { display: flex; gap: 6px; }
.tabs button { flex: 1; }
.tabs button.on { background: var(--accent); border-color: var(--accent); color: #fff; }
.err { color: var(--red); font-size: 13px; }
.muted { color: var(--muted); }
</style>
