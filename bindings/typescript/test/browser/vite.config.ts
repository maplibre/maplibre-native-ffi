// Bundles the suite for a browser page:
//
//   MAPLIBRE_NATIVE_FFI_BROWSER_BUNDLE=<dir> vp pack   (in test/browser)
//
// The bundle's entry is the page runner. A page has no node: modules, so the
// bundle takes the browser entry of the binding, the browser host
// capabilities, and small shims for the node: imports of the cases. The test
// module stays outside the bundle: the runner loads it by URL, as a page loads
// the browser module.

import process from "node:process";
import { fileURLToPath } from "node:url";

import { defineConfig } from "vite-plus";

const here = (path: string) => fileURLToPath(new URL(path, import.meta.url));
const unavailable = here("./node/unavailable.ts");
const nodeModules: Record<string, string> = {
  "node:assert/strict": here("../arkts/node/assert.ts"),
  "node:process": here("./node/process.ts"),
  "node:timers": here("./node/timers.ts"),
  "node:child_process": unavailable,
  "node:fs": unavailable,
  "node:os": unavailable,
  "node:path": unavailable,
  "node:url": unavailable,
};
const replaced: Record<string, string> = {
  [here("../../src/index.ts")]: here("../../src/browser.ts"),
  [here("../support/host-capabilities.ts")]: here("./host-capabilities.ts"),
};

export default defineConfig({
  pack: {
    entry: { runner: here("./runner.ts") },
    // The output directory is the page's directory, which the caller names;
    // nothing outside it is cleaned.
    outDir: process.env.MAPLIBRE_NATIVE_FFI_BROWSER_BUNDLE!,
    clean: false,
    format: "esm",
    platform: "browser",
    fixedExtension: false,
    hash: false,
    plugins: [
      {
        name: "browser-host",
        async resolveId(source, importer, options) {
          if (source in nodeModules) {
            return nodeModules[source];
          }
          if (source.startsWith("node:")) {
            throw new Error(`no browser shim for ${source}, from ${importer}`);
          }
          const resolved = await this.resolve(source, importer, {
            ...options,
            skipSelf: true,
          });
          return resolved && (replaced[resolved.id] ?? resolved);
        },
      },
    ],
  },
});
