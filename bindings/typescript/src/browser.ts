/**
 * The entry that a bundler building for the web takes in place of the default
 * one. A page loads the browser module with {@link loadModule} before any other
 * call, and imports its own graphics objects into the module for the render
 * target descriptors that name them. Every other export is the default entry's.
 *
 * @module browser
 */

// A page has no exit event that ends native work, so its runtimes end with it.

export * from "./generated_operations.ts";
export * from "./generated_values.ts";
export {
  type LoadModuleOptions,
  loadModule,
} from "./internal/browser_module.ts";
export type { CommandCompletion } from "./internal/completions.ts";
export { AbiVersionMismatchError, MaplibreError } from "./internal/errors.ts";
export type { NativeRegistration } from "./internal/native_registrations.ts";
export {
  AwaitableWake,
  type AwaitableWakeOptions,
  type AwaitableWakeSignal,
} from "./internal/wakes.ts";
export {
  ImportedWebGpuObject,
  importWebGpuDevice,
  importWebGpuSurface,
  importWebGpuTexture,
  importWebGpuTextureView,
  RegisteredWebGlContext,
  registerWebGlContext,
  webGpuObject,
  webGpuTextureFormat,
} from "./internal/web_graphics.ts";
