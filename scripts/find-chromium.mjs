// Finds the Chromium that the browser suites run.
//
// Usage: node scripts/find-chromium.mjs
//
// Prints the browser's path, which run-browser-test.mjs launches and Karma
// reads from CHROME_BIN. Chromium comes from the environment where CI provides
// one, and otherwise from whatever is on PATH or in a local Playwright cache.

import { existsSync, readdirSync } from "node:fs";
import path from "node:path";
import process from "node:process";
import { fileURLToPath } from "node:url";

/** Returns the browser's path, or throws with the reason none was found. */
export function findChromium() {
  const explicit = process.env.MLN_FFI_TEST_BROWSER ?? process.env.CHROME_PATH;
  if (explicit) {
    if (!existsSync(explicit))
      throw new Error(`MLN_FFI_TEST_BROWSER is not a file: ${explicit}`);
    return explicit;
  }
  const names = [
    "/usr/bin/google-chrome",
    "/usr/bin/google-chrome-stable",
    "/usr/bin/chromium",
    "/usr/bin/chromium-browser",
    "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
  ];
  for (const name of names) if (existsSync(name)) return name;

  const cache = path.join(
    process.env.HOME ?? "",
    process.platform === "darwin"
      ? "Library/Caches/ms-playwright"
      : ".cache/ms-playwright",
  );
  if (existsSync(cache)) {
    for (const entry of readdirSync(cache).sort().reverse()) {
      if (!entry.startsWith("chromium-")) continue;
      // Playwright has shipped both layouts; newer builds carry the arch.
      for (const relative of [
        "chrome-linux64/chrome",
        "chrome-linux/chrome",
        "chrome-mac/Chromium.app/Contents/MacOS/Chromium",
      ]) {
        const candidate = path.join(cache, entry, relative);
        if (existsSync(candidate)) return candidate;
      }
    }
  }
  throw new Error(
    "no Chromium found; set MLN_FFI_TEST_BROWSER to a Chrome or Chromium binary",
  );
}

if (process.argv[1] === fileURLToPath(import.meta.url)) {
  try {
    console.log(findChromium());
  } catch (error) {
    console.error(`error: ${error.message}`);
    process.exit(2);
  }
}
