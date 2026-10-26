/**
 * The browser API: {@link loadModule}, which a page calls first, and imports of
 * the page's graphics objects for render target descriptors. Everything else
 * matches the default entry.
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
