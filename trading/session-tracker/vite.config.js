import { defineConfig, configDefaults } from "vitest/config";
import vue from "@vitejs/plugin-vue";

export default defineConfig({
  plugins: [vue()],
  server: {
    port: 8080,
    proxy: {
      // Même origine côté navigateur → aucun CORS. CouchDB écoute en 5984.
      "/db": {
        target: "http://localhost:5984",
        changeOrigin: true,
        rewrite: (p) => p.replace(/^\/db/, ""),
      },
    },
  },
  test: {
    environment: "jsdom",
    globals: true,
    exclude: [...configDefaults.exclude, "tests/integration/**"],
  },
});
