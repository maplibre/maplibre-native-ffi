// Debian's Node builds omit TypeScript support. Strip the suite's sources in
// this test-only loader, inherited by its workers and child processes.
import { readFileSync } from "node:fs";
import { registerHooks } from "node:module";

import { transformSync } from "amaro";

registerHooks({
  load(url, context, nextLoad) {
    if (!url.startsWith("file:") || !/\.(?:ts|mts)$/.test(url)) {
      return nextLoad(url, context);
    }
    const { code } = transformSync(readFileSync(new URL(url), "utf8"), {
      mode: "strip-only",
    });
    return { format: "module", source: code, shortCircuit: true };
  },
});
