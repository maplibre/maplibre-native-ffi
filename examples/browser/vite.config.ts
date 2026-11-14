// Builds the browser example: one page per backend, under the docs site's base
// path, into the site's public/demo, where the site's build picks it up.
//
// The browser modules aren't in the repository. The example's mise tasks copy
// them into public/ from the binding's builds for the web presets. Each page's
// map runs in a module Web Worker, which loads its page's module.

import { fileURLToPath } from "node:url";

import { defineConfig } from "vite-plus";

const here = (path: string) =>
  fileURLToPath(new URL(path, import.meta.url).href);

export default defineConfig({
  base: "/maplibre-native-ffi/demo/",
  // The comparison imports GL JS on demand. Bundling it up front saves the dev
  // server a reload when the comparison first opens.
  optimizeDeps: { include: ["maplibre-gl"] },
  worker: { format: "es" },
  build: {
    outDir: here("../../docs/public/demo"),
    emptyOutDir: true,
    target: "es2024",
    // GL JS alone is about 1 MB, in a chunk that loads only for the comparison.
    chunkSizeWarningLimit: 1200,
    rollupOptions: {
      input: {
        index: here("index.html"),
        webgl: here("webgl/index.html"),
        webgpu: here("webgpu/index.html"),
      },
    },
  },
});
