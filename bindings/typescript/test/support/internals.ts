// What a case observes below the public API: the realm's callback roots and
// pending completions, and whether native code still knows a handle.

import process from "node:process";

import { MaplibreError, Status } from "../../src/index.ts";
import { native } from "../../src/internal/addon.ts";
import { rootReleaseListeners } from "../../src/internal/realm.ts";
import { callbackArrival, teardown } from "./harness.ts";
import { capabilities } from "./host-capabilities.ts";

export { liveRoots, pendingCompletions } from "../../src/internal/realm.ts";

/**
 * Whether native code still knows a runtime, asked through the raw export,
 * which is the only way to name a runtime that no owner holds.
 */
export function runtimeIsLive(runtime: bigint): boolean {
  try {
    native.mln_runtime_get_event_mask(runtime);
    return true;
  } catch (error) {
    // Native rejects a handle it never issued, and reports a released one as
    // stale.
    if (
      error instanceof MaplibreError &&
      (error.status === Status.invalidArgument ||
        error.status === Status.invalidState)
    ) {
      return false;
    }
    throw error;
  }
}

/**
 * Collects what the binding writes through `console[method]` during the
 * running case instead of printing it.
 */
export function captureConsole(method: "warn" | "error"): string[] {
  const lines: string[] = [];
  const original = console[method];
  console[method] = (...values: unknown[]) => {
    lines.push(values.map((value) => String(value)).join(" "));
  };
  teardown(() => {
    console[method] = original;
  });
  return lines;
}

/**
 * Collects the leak reports that the binding makes during the running case
 * instead of printing them. Node, Bun, and Deno report a leak as a process
 * warning, and ArkTS through `console.warn`.
 */
export function captureLeaks(): string[] {
  if (capabilities.host === "arkts") {
    return captureConsole("warn");
  }
  const leaks: string[] = [];
  const original = process.emitWarning.bind(process);
  process.emitWarning = ((
    warning: string,
    options?: { type?: string; code?: string },
  ) => {
    if (
      options?.type !== "ResourceWarning" ||
      options.code !== "MAPLIBRE_NATIVE_FFI_LEAK"
    ) {
      throw new Error(`an unexpected process warning: ${warning}`);
    }
    leaks.push(warning);
  }) as typeof process.emitWarning;
  teardown(() => {
    process.emitWarning = original;
  });
  return leaks;
}

/**
 * Collects the errors that the binding reports for a throwing host callback
 * during the running case. Bun and Deno define `reportError`, which their test
 * runners treat as an uncaught error. Node does not, so the binding rethrows
 * the error as an uncaught exception there, and ArkTS writes it to
 * `console.error`.
 */
export function captureReportedErrors(): string[] {
  const scope = globalThis as { reportError?: (error: unknown) => void };
  const errors: string[] = [];
  if (typeof scope.reportError !== "function") {
    if (capabilities.host === "arkts") {
      return captureConsole("error");
    }
    process.setUncaughtExceptionCaptureCallback((error) =>
      errors.push(String(error)),
    );
    teardown(() => process.setUncaughtExceptionCaptureCallback(null));
    return errors;
  }
  const original = scope.reportError;
  scope.reportError = (error) => errors.push(String(error));
  teardown(() => {
    scope.reportError = original;
  });
  return errors;
}

/**
 * Resolves once `done` holds, checking now and after each callback root
 * release. A release arrives through the realm's queue with no Promise of its
 * own, so the wait holds the event loop as a callback arrival does.
 */
export function released(done: () => boolean, what: string): Promise<void> {
  let check!: () => void;
  const settled = new Promise<void>((resolve) => {
    check = () => done() && resolve();
  });
  rootReleaseListeners.add(check);
  check();
  return callbackArrival(settled, what).finally(() =>
    rootReleaseListeners.delete(check),
  );
}
