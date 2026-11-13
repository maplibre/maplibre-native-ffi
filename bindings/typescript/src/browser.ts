/**
 * The browser API: {@link loadModule}, which a page calls first, and
 * {@link WebObject}, which lends the page's objects, such as a canvas or a
 * `GPUDevice`, to render target descriptors. Everything else matches the
 * default entry.
 *
 * @module browser
 */

// A page has no exit event that ends native work, so its runtimes end with it.

export * from "./generated_operations.ts";
export * from "./generated_values.ts";
export { loadModule } from "./internal/browser_module.ts";
export type { CommandCompletion } from "./internal/completions.ts";
export { AbiVersionMismatchError, MaplibreError } from "./internal/errors.ts";
export type { NativeRegistration } from "./internal/native_registrations.ts";
export {
  AwaitableWake,
  type AwaitableWakeOptions,
  type AwaitableWakeSignal,
} from "./internal/wakes.ts";
export { WebObject } from "./internal/web_objects.ts";
