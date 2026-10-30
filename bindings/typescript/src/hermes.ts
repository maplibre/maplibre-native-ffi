/**
 * Hermes embedding backend. Install `mln::hermes::Module` into the runtime on
 * its JavaScript thread before evaluating this module. Its scheduler must post
 * work to that thread asynchronously. Destroy the module on the same thread,
 * before destroying the runtime; cleanup ends all outstanding native work.
 * The host supplies graphics objects and initializes Android platform services
 * through the C API when it uses them. No native views are created.
 *
 * @module hermes
 */
import { bindAddon, type Addon } from "./internal/addon.ts";
import { installRealm } from "./internal/realm.ts";

const addon = (globalThis as typeof globalThis & { __mlnNativeFfi?: Addon })
  .__mlnNativeFfi;
if (!addon)
  throw new Error("MapLibre Native FFI's JSI bindings were not installed");
bindAddon(addon, {}, installRealm);

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
