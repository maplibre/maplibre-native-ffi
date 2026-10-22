/**
 * The low-level TypeScript binding for the MapLibre Native C API.
 *
 * Every callback reaches the realm's event loop after the native call that
 * raised it has returned, so native code answers each synchronous decision
 * itself. A provider or transform request arrives as an owner that the host
 * answers later.
 *
 * Native code in the host can supply a wake or a queue lock as a native
 * registration: `Wake.native({ callback, userData, releaseUserData })` or
 * `QueueLock.native({ lock, unlock, userData, releaseUserData })`, with each
 * function pointer and the context as a `bigint` address. Native code calls a
 * native wake directly, without a post to the realm. A queue lock has only this
 * form, because native code relies on the lock being held when its callback
 * returns. The first call that carries a native registration consumes it, and a
 * later call throws a `MaplibreError` with the invalid-state status. Native code
 * releases the context of a call that it accepts, and the binding calls the
 * release for any other call.
 *
 * A pending binding Promise keeps the process alive, and a callback
 * registration or a wake does not. To wait for a callback, call the `wake`
 * member of an {@link AwaitableWake} from it and await `next()`.
 *
 * A realm runs on one thread, so it can drive a caller-driven render session:
 * make its context current on the JavaScript thread and call
 * `serviceDriverWork()` when the driver-work wake arrives. A realm loads one
 * copy of the binding.
 *
 * @packageDocumentation
 */

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
export type { NativeRegistration } from "./internal/native_registrations.ts";
export {
  AwaitableWake,
  type AwaitableWakeOptions,
  type AwaitableWakeSignal,
} from "./internal/wakes.ts";
