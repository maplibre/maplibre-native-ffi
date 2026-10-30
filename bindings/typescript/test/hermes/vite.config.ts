import { createRequire } from "node:module";
import process from "node:process";
import { fileURLToPath } from "node:url";

import { defineConfig } from "vite-plus";

const here = (path: string) =>
  fileURLToPath(new URL(path, import.meta.url).href);
const unavailable = here("../browser/node/unavailable.ts");
const nodeModules: Record<string, string> = {
  "node:assert/strict": here("../arkts/node/assert.ts"),
  "node:process": here("../browser/node/process.ts"),
  "node:timers": here("../browser/node/timers.ts"),
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
    entry: {
      [process.env.MLN_HERMES_SMOKE ? "smoke" : "suite"]: here(
        process.env.MLN_HERMES_SMOKE ? "./smoke.ts" : "./runner.ts",
      ),
    },
    // The caller names the app bundle directory;
    // nothing outside it is cleaned.
    outDir: process.env.MAPLIBRE_NATIVE_FFI_HERMES_BUNDLE!,
    clean: false,
    format: "iife",
    target: "es2021",
    define: { "import.meta.url": JSON.stringify("hermes-test://suite") },
    platform: "browser",
    fixedExtension: false,
    hash: false,
    plugins: [
      {
        name: "hermes-host",
        async resolveId(source, importer, options) {
          if (source === "@maplibre/native-ffi/hermes") {
            return createRequire(
              process.env.MLN_HERMES_CONSUMER + "/package.json",
            ).resolve(source);
          }
          if (source in nodeModules) {
            return nodeModules[source];
          }
          if (source.startsWith("node:")) {
            throw new Error(
              `no Hermes test shim for ${source}, from ${importer}`,
            );
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
