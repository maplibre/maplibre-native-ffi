// What the binding reports outside the public API: its console output and the
// errors that host callbacks throw.

import process from "node:process";

import { teardown } from "./harness.ts";
import { capabilities } from "./host-capabilities.ts";

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
    if (capabilities.host === "arkts" || capabilities.host === "hermes") {
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
