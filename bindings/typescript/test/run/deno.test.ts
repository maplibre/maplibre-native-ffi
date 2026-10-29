// The Deno entry: one process, every case.
//
//   deno test --no-check --allow-ffi --allow-read --allow-env --allow-net \
//     --allow-run --allow-write --v8-flags=--expose-gc test/run/deno.test.ts

import { execute, scheduled } from "../support/harness.ts";
import "../suite.ts";

declare const Deno: {
  test(definition: {
    name: string;
    fn: () => Promise<void>;
    sanitizeOps: boolean;
    sanitizeResources: boolean;
    sanitizeExit: boolean;
  }): void;
};

for (const entry of scheduled()) {
  Deno.test({
    name: entry.name,
    fn: () => execute(entry),
    // Deno's sanitizers see the addon's threadsafe function and worker
    // messages as leaked operations.
    sanitizeOps: false,
    sanitizeResources: false,
    sanitizeExit: false,
  });
}
