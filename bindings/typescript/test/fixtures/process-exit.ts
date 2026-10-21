// Ends the process while native work is live: a core-worker session in the
// middle of frames, a provider request that was never answered, and callback
// registrations of every delivery kind. Deno ends it through Deno.exit(0), and
// the other runtimes through process.exit(0).

import process from "node:process";

import { requestFrame } from "../support/graphics.ts";
import { execute } from "../support/harness.ts";
import { liveNativeWork } from "../support/scenarios.ts";

const host = globalThis as {
  Deno?: { exit(code: number): never };
  addEventListener?: (type: string, listener: () => void) => void;
};
const deno = host.Deno;

await execute({
  name: "process exit",
  needs: [],
  async run() {
    const session = await liveNativeWork();
    // The binding's own exit listener, which the first runtime registered,
    // runs before this one and abandons the session.
    const exiting = () => {
      try {
        requestFrame(session, 200n);
        console.log("the session was still live at exit");
      } catch (error) {
        console.log(`the session was abandoned at exit: ${String(error)}`);
      }
    };
    if (deno === undefined) {
      process.on("exit", exiting);
    } else {
      host.addEventListener?.("unload", exiting);
    }
    console.log("exiting with live native work");
    if (deno === undefined) {
      process.exit(0);
    } else {
      deno.exit(0);
    }
  },
});
