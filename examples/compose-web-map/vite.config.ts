import { defineConfig } from "vite-plus";

export default defineConfig({
  server: {
    headers: {
      "Cache-Control": "no-store",
      "Cross-Origin-Embedder-Policy": "require-corp",
      "Cross-Origin-Opener-Policy": "same-origin",
    },
  },
});
