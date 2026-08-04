import { defineConfig } from "vitest/config";

// The runners directory holds the same conformance suite registered for Bun and
// Deno. Their frameworks collect the same file names vitest does, so vitest is
// pointed at its own directory rather than at everything.
export default defineConfig({
  test: {
    include: ["test/**/*.test.ts"],
  },
});
