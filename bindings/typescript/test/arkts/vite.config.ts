// Bundles the suite for ArkTS, which resolves modules by record name rather
// than through node_modules:
//
//   MAPLIBRE_NATIVE_FFI_ARKTS_BUNDLE=<dir> vp pack   (in test/arkts)
//
// The bundle has three entries: the hypium runner, the realm of a spawned
// worker, and the realm that loads the packed OHPM package, whose directory
// MAPLIBRE_NATIVE_FFI_ARKTS_PACKAGE names. ArkTS has no node: modules, so the
// bundle takes the ArkTS entry of the binding, the ArkTS host capabilities and
// realm child, and small shims for the node: imports of the cases. System
// modules, hypium, and the packed native libraries stay imports, which
// test-arkts.sh rewrites to records.

import process from "node:process";
import { fileURLToPath } from "node:url";

import { defineConfig } from "vite-plus";

const here = (path: string) =>
  fileURLToPath(new URL(path, import.meta.url).href);
const unavailable = here("./node/unavailable.ts");
const nodeModules: Record<string, string> = {
  "node:assert/strict": here("./node/assert.ts"),
  "node:process": here("./node/process.ts"),
  "node:timers": here("./node/timers.ts"),
  "node:child_process": unavailable,
  "node:fs": unavailable,
  "node:os": unavailable,
  "node:path": unavailable,
  "node:url": unavailable,
};
const replaced: Record<string, string> = {
  [here("../../src/index.ts")]: here("../../src/arkts.ts"),
  [here("../support/host-capabilities.ts")]: here("./host-capabilities.ts"),
  [here("../support/realm-child.ts")]: here("./realm-child.ts"),
};

export default defineConfig({
  pack: {
    entry: {
      TestRunner: here("./TestRunner.ts"),
      realm: here("./realm.ts"),
      consumer: here("./consumer.ts"),
    },
    // The output directory is the HAP's staging directory, which the caller
    // names; nothing outside it is cleaned.
    outDir: process.env.MAPLIBRE_NATIVE_FFI_ARKTS_BUNDLE!,
    clean: false,
    format: "esm",
    platform: "neutral",
    fixedExtension: false,
    // A record's name is its path, so chunk names carry no hash.
    hash: false,
    // es2abc compiles no import.meta. Only the child-process variants of the
    // cases name it, and they do not run on ArkTS.
    define: { "import.meta.url": JSON.stringify("file:///arkts/") },
    deps: { neverBundle: [/^@ohos[./]/, /^lib[A-Za-z0-9_]+\.so$/] },
    plugins: [
      {
        name: "arkts-host",
        async resolveId(source, importer, options) {
          if (source in nodeModules) {
            return nodeModules[source];
          }
          if (source === "@maplibre/native-ffi") {
            return `${process.env.MAPLIBRE_NATIVE_FFI_ARKTS_PACKAGE!}/index.js`;
          }
          if (source.startsWith("node:")) {
            throw new Error(`no ArkTS shim for ${source}, from ${importer}`);
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
