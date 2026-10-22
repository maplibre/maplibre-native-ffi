// The Hermes entry: one runtime on the Hermes test host, every case in order.
// Each case is limited to 300 seconds, as under the other runners, so a case
// that never settles fails and names itself instead of holding the run.

import "./globals.ts";
import "../callbacks.test.ts";
import "../completions.test.ts";
import "../errors.test.ts";
import "../event-loop.test.ts";
import "../lifecycle.test.ts";
import "../log.test.ts";
import "../platform.test.ts";
import "../realms.test.ts";
import "../render.test.ts";
import "../resource-provider.test.ts";
import "../values.test.ts";
import { bounded, execute, scheduled } from "../support/harness.ts";

function describe(error: unknown): string {
  if (error instanceof Error) {
    return error.stack ?? `${error.name}: ${error.message}`;
  }
  return String(error);
}

async function run(): Promise<void> {
  const order = scheduled();
  console.log(`harness registered ${order.length} cases`);
  let failed = 0;
  for (const entry of order) {
    const started = Date.now();
    try {
      await bounded(execute(entry), entry.name, 300_000);
      console.log(`ok - ${entry.name} (${Date.now() - started} ms)`);
    } catch (error) {
      failed += 1;
      console.log(`not ok - ${entry.name} (${Date.now() - started} ms)`);
      console.log(describe(error).replace(/^/gm, "  # "));
    }
  }
  console.log(`${order.length - failed} passed, ${failed} failed`);
  hermesHost.exit(failed === 0 ? 0 : 1);
}

run().catch((error: unknown) => {
  console.error(describe(error));
  hermesHost.exit(1);
});
