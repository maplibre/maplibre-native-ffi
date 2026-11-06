// The entry point for ArkTS on OpenHarmony.
//
// ArkTS resolves a native module by the name of a library packed into the
// application, so the addon is a static import. ArkTS has no exit event: an
// application process ends when the system stops its ability, and a worker
// ends through its environment's cleanup, which releases its queue. It has no
// process warnings either, so leak reports go to console.warn.
//
// The addon comes from a backend HAR, and OHPM has no peer dependencies, so
// an application that installs only the facade resolves the import to
// undefined.

import addon from "libmaplibre_native_ffi.so";

import { type Addon, bindAddon } from "./internal/addon.ts";
import { installRealm } from "./internal/realm.ts";

if (addon === undefined) {
  throw new Error(
    "no MapLibre Native backend package is installed; install one of @maplibre/native-ffi-opengl, @maplibre/native-ffi-vulkan at the version of @maplibre/native-ffi",
  );
}
bindAddon(addon as Addon, {}, installRealm);

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
