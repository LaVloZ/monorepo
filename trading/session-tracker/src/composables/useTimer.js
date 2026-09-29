import { ref, onMounted, onUnmounted } from "vue";

export function useTimer() {
  const now = ref(Date.now());
  let id = null;
  onMounted(() => {
    id = setInterval(() => { now.value = Date.now(); }, 1000);
  });
  onUnmounted(() => { if (id) clearInterval(id); });
  return { now };
}
