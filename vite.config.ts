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
      "**/dist/**",
      "third_party/**",
      "docs/public/reference/**",
    ],
  },
});
