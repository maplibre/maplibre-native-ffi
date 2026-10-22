// Bundles the suite into one script for the Hermes test host, which runs a
// script rather than modules:
//
//   MAPLIBRE_NATIVE_FFI_HERMES_BUNDLE=<dir> vp pack   (in test/hermes)
//
// Hermes has no node: modules, so the bundle takes the Hermes entry of the
// binding, the Hermes host capabilities, and small shims for the node: imports
// of the cases. The addon import resolves to the addon that
// MAPLIBRE_NATIVE_FFI_ADDON names, which the host loads.

import process from "node:process";
import { fileURLToPath } from "node:url";

import { defineConfig } from "vite-plus";

const here = (path: string) => fileURLToPath(new URL(path, import.meta.url));
const unavailable = here("./node/unavailable.ts");
const addon = "\0maplibre-native-ffi-addon";
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
  [here("../../src/index.ts")]: here("../../src/hermes.ts"),
  [here("../support/host-capabilities.ts")]: here("./host-capabilities.ts"),
};

export default defineConfig({
  pack: {
    entry: { suite: here("./runner.ts") },
    outDir: process.env.MAPLIBRE_NATIVE_FFI_HERMES_BUNDLE!,
    clean: false,
    format: "iife",
    platform: "neutral",
    fixedExtension: false,
    hash: false,
    dts: false,
    // Only the child-process variants of the cases name import.meta, and they
    // do not run on Hermes.
    define: { "import.meta.url": JSON.stringify("file:///hermes/") },
    plugins: [
      {
        name: "hermes-host",
        async resolveId(source, importer, options) {
          if (source === "maplibre_native_ffi.node") {
            return addon;
          }
          if (source in nodeModules) {
            return nodeModules[source];
          }
          if (source.startsWith("node:")) {
            throw new Error(`no Hermes shim for ${source}, from ${importer}`);
          }
          const resolved = await this.resolve(source, importer, {
            ...options,
            skipSelf: true,
          });
          return resolved && (replaced[resolved.id] ?? resolved);
        },
        load(id) {
          if (id === addon) {
            return [
              "const path = hermesHost.env.MAPLIBRE_NATIVE_FFI_ADDON;",
              'if (!path) throw new Error("MAPLIBRE_NATIVE_FFI_ADDON names the addon");',
              "export default hermesHost.loadAddon(path);",
            ].join("\n");
          }
          return null;
        },
      },
    ],
  },
});
