/**
 * The low-level TypeScript binding for the MapLibre Native C API.
 *
 * The desktop binding supports Linux with glibc on Node.js, Bun, and Deno,
 * Linux with musl on Node.js and Bun, and macOS on all three runtimes.
 * The GNU RISC-V and LoongArch64 addons support Node.js.
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
 * Every 64-bit C integer, such as a generation, a timestamp, or a 64-bit mask,
 * is a `bigint`, so it keeps its full range. `JSON.stringify` throws a
 * `TypeError` on a `bigint` unless a replacer converts it.
 *
 * A JSON value is any value that `JSON.stringify` takes, typed `unknown`, so
 * `setStyleLayerProperty("roads", "line-color", "red")` needs no inner quotes.
 * In a parameter or a record member, `undefined` is absent, which native code
 * reads as empty, and `null` is JSON null, such as the null that restores a
 * layer property's default. A value that `JSON.stringify` rejects throws its
 * error before the call reaches native code, and one that it skips, such as a
 * function, throws a `TypeError`. Returned JSON is the value that `JSON.parse`
 * reads, and absent JSON is `undefined`. `JSON.parse` reads every number as a
 * `number`, so an integer above 2^53, such as some feature IDs, loses
 * precision.
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
    // Node has no global reportError. An error rethrown from a microtask
    // reaches uncaughtException, as a throwing EventEmitter listener does.
    reportError: (error) =>
      queueMicrotask(() => {
        throw error;
      }),
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
