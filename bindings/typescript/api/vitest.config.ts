import { defineConfig } from "vitest/config";

// The runners directory holds the same conformance suite registered for Bun and
// Deno. Their frameworks collect the same file names vitest does, so vitest is
// pointed at its own directory rather than at everything.
export default defineConfig({
  test: {
    include: ["test/**/*.test.ts"],
    // The WebAssembly suites need a payload the Emscripten SDK links, and that
    // SDK is a multi-gigabyte tool an environment may leave out. The browser one
    // also needs a browser. Their own tasks name these files, so they stay out
    // of the default run.
    exclude: [
      "**/node_modules/**",
      "test/wasm.test.ts",
      "test/browser.test.ts",
    ],
  },
});
