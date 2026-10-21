// The entry point for Node.js, Bun, and Deno.

import process from "node:process";
import { isMainThread } from "node:worker_threads";

import { bindAddon } from "./internal/addon.ts";
import { loadAddon } from "./internal/loader.ts";
import { installRealm } from "./internal/realm.ts";

// A worker's exit event ends only its realm, whose own teardown releases its
// queue, so only the main realm ends native work before exit(). Deno reports
// isMainThread in a web Worker as well, which its global scope tells apart.
const scope = globalThis as {
  WorkerGlobalScope?: abstract new () => object;
  Deno?: unknown;
  addEventListener?: (type: string, listener: () => void) => void;
};
const mainRealm =
  isMainThread &&
  !(
    scope.WorkerGlobalScope !== undefined &&
    globalThis instanceof scope.WorkerGlobalScope
  );

function onExit(listener: () => void): void {
  process.on("exit", listener);
  // Deno.exit() runs neither exit listeners nor Node-API cleanup hooks, only
  // the global unload event. The listener runs twice at a normal Deno exit,
  // and the second run finds nothing left to end.
  if (scope.Deno !== undefined) {
    scope.addEventListener?.("unload", listener);
  }
}

bindAddon(
  loadAddon(),
  {
    ...(mainRealm && { onExit }),
    // A process warning, unlike console output, is what --no-warnings and
    // process.on("warning") filter.
    warnLeak: (message) =>
      process.emitWarning(message, {
        type: "ResourceWarning",
        code: "MAPLIBRE_NATIVE_FFI_LEAK",
      }),
  },
  installRealm,
);

export * from "./generated_operations.ts";
export * from "./generated_values.ts";
export type { CommandCompletion } from "./internal/completions.ts";
export { AbiVersionMismatchError, MaplibreError } from "./internal/errors.ts";
export {
  AwaitableWake,
  type AwaitableWakeOptions,
  type AwaitableWakeSignal,
} from "./internal/wakes.ts";
