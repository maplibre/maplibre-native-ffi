import { defineConfig } from "vite-plus";

// Root Vite+ config for shared lint/type-check policy across pnpm workspaces.
// Per-package vite/astro configs stay in each workspace for dev/build/test.
export default defineConfig({
  lint: {
    options: {
      typeAware: true,
      typeCheck: true,
    },
    ignorePatterns: [
      "**/node_modules/**",
      // Each runner is written against one runtime's globals, and that runtime
      // checks it: Deno type-checks a test it runs, and Bun's runner mirrors
      // the vitest one, which is checked here.
      "bindings/typescript/api/runners/**",
      "**/dist/**",
      "third_party/**",
      "docs/public/reference/**",
    ],
  },
});
