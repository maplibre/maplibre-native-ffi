import { defineConfig } from "vitest/config";

// The WebAssembly suite has its own config because the default one excludes it:
// it needs a payload the Emscripten SDK links, and that SDK is a
// multi-gigabyte tool an environment may leave out.
export default defineConfig({
  test: {
    include: ["test/wasm.test.ts"],
  },
});
