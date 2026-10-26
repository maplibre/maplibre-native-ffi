// The page entry of the browser suite.
//
// The page loads the test module first, and then the cases, whose support
// modules read the loaded module as they evaluate. It runs every case that the
// harness schedules for the browser in order, prints each outcome to the
// console, and reports the run to scripts/run-browser-test.mjs.

import { loadModule, RuntimeHandle } from "../../src/browser.ts";
import { pendingCompletions } from "../../src/internal/realm.ts";

// The suite checks against the Node types, which declare no page globals.
declare const location: { readonly href: string };

function report(status: number, output: string): void {
  void fetch("/__result", {
    method: "POST",
    headers: { "content-type": "application/json" },
    body: JSON.stringify({ status, output }),
  });
}

try {
  // A call before the load throws, which a case checks once the module loads.
  let error: unknown;
  await RuntimeHandle.create().catch((thrown: unknown) => (error = thrown));
  (globalThis as { mlnTsBeforeLoad?: unknown }).mlnTsBeforeLoad = {
    error,
    pending: pendingCompletions(),
  };
  await loadModule(new URL("maplibre_native_ffi_test.mjs", location.href));
  await import("../callbacks.test.ts");
  await import("../completions.test.ts");
  await import("../errors.test.ts");
  await import("../event-loop.test.ts");
  await import("../lifecycle.test.ts");
  await import("../log.test.ts");
  await import("../platform.test.ts");
  await import("../realms.test.ts");
  await import("../render.test.ts");
  await import("../resource-provider.test.ts");
  await import("../values.test.ts");
  const { execute, scheduled } = await import("../support/harness.ts");
  const failures: string[] = [];
  const order = scheduled();
  for (const entry of order) {
    try {
      await execute(entry);
      console.log(`ok - ${entry.name}`);
    } catch (error) {
      const detail =
        error instanceof Error ? (error.stack ?? String(error)) : String(error);
      console.log(`not ok - ${entry.name}\n${detail}`);
      failures.push(`${entry.name}\n${detail}`);
    }
  }
  const summary = `${order.length - failures.length} of ${order.length} cases passed`;
  console.log(summary);
  report(
    failures.length === 0 ? 0 : 1,
    [...failures.map((failure) => `not ok - ${failure}`), summary].join("\n\n"),
  );
} catch (error) {
  report(
    70,
    error instanceof Error ? (error.stack ?? String(error)) : String(error),
  );
}
