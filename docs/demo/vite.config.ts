// Builds the live demo: one page per backend, under the docs site's base path.
//
// The browser modules aren't in the repository. `mise run //docs:demo-modules`
// copies them into public/ from the binding's Emscripten builds.

import { fileURLToPath } from "node:url";

import { defineConfig } from "vite-plus";

const here = (path: string) =>
  fileURLToPath(new URL(path, import.meta.url).href);

// The library shares memory between threads, which a browser allows only on a
// cross-origin isolated page. The dev server isolates its pages with these
// headers. GitHub Pages sets no headers, so the built pages load
// coi-serviceworker instead, which adds them.
const isolation = {
  "Cross-Origin-Opener-Policy": "same-origin",
  "Cross-Origin-Embedder-Policy": "require-corp",
};

export default defineConfig({
  base: "/maplibre-native-ffi/demo/",
  server: { headers: isolation },
  preview: { headers: isolation },
  build: {
    outDir: here("../public/demo"),
    emptyOutDir: true,
    target: "es2024",
    rollupOptions: {
      input: {
        index: here("index.html"),
        webgl: here("webgl/index.html"),
        webgpu: here("webgpu/index.html"),
      },
    },
  },
});
