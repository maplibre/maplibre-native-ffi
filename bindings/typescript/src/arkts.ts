// The entry point for ArkTS on OpenHarmony.
//
// ArkTS resolves a native module by the name of a library packed into the
// application, so the addon is a static import. ArkTS has no exit event: an
// application process ends when the system stops its ability, and a worker
// ends through its environment's cleanup, which releases its queue. It has no
// process warnings either, so leak reports go to console.warn.

import addon from "libmaplibre_native_ffi.so";

import { type Addon, bindAddon } from "./internal/addon.ts";
import { installRealm } from "./internal/realm.ts";

bindAddon(addon as Addon, {}, installRealm);

export * from "./generated_operations.ts";
export * from "./generated_values.ts";
export type { CommandCompletion } from "./internal/completions.ts";
export { AbiVersionMismatchError, MaplibreError } from "./internal/errors.ts";
export {
  AwaitableWake,
  type AwaitableWakeOptions,
  type AwaitableWakeSignal,
} from "./internal/wakes.ts";
