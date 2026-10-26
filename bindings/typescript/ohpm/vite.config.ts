// Bundles the ArkTS entry of the binding for its OHPM packages:
//
//   MAPLIBRE_NATIVE_FFI_OHPM_BUNDLE=<dir> vp pack   (in ohpm)
//
// The bundle keeps the addon an import, which ArkTS resolves by the name of
// the library packed into the application.

import process from "node:process";
import { fileURLToPath } from "node:url";

import { defineConfig } from "vite-plus";

export default defineConfig({
  pack: {
    entry: {
      index: fileURLToPath(new URL("../src/arkts.ts", import.meta.url)),
    },
    outDir: process.env.MAPLIBRE_NATIVE_FFI_OHPM_BUNDLE!,
    format: "esm",
    platform: "neutral",
    fixedExtension: false,
    dts: true,
    deps: { neverBundle: [/^lib[A-Za-z0-9_]+\.so$/] },
  },
});
