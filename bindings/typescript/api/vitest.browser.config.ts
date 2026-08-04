import { playwright } from "@vitest/browser-playwright";
import { fileURLToPath } from "node:url";
import { defineConfig } from "vitest/config";

// The browser suite is the WebAssembly one in the place that payload is for.
// Node instantiates the same artifact, which proves the transport but not that
// a browser can host it: the module is threaded, reaches its resources through
// the page, and renders through a real WebGL context. Those only exist here.
export default defineConfig({
  // The payload is served rather than imported from disk, because that is how a
  // page loads it — and Emscripten finds the `.wasm` beside the `.mjs` through
  // the URL it was loaded from.
  publicDir: fileURLToPath(new URL("../runtime-wasm", import.meta.url)),
  server: {
    headers: {
      // A threaded module needs `SharedArrayBuffer`, which a browser withholds
      // from a page that is not cross-origin isolated.
      "Cross-Origin-Opener-Policy": "same-origin",
      "Cross-Origin-Embedder-Policy": "require-corp",
    },
  },
  test: {
    include: ["test/browser.test.ts"],
    browser: {
      enabled: true,
      provider: playwright(),
      headless: true,
      instances: [{ browser: "chromium" }],
    },
  },
});
