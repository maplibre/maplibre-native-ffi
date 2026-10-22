// The entry point for Hermes.
//
// Hermes has no module loader of its own: each embedder loads a Node-API
// addon its own way, so the addon is a static import of
// `maplibre_native_ffi.node` that the embedder's bundler resolves to the
// loaded exports. The embedder provides setTimeout and console, which Hermes
// lacks, and a Node-API host that dispatches threadsafe functions on the
// JavaScript thread. Hermes has no exit event, and the embedder ends a realm by
// destroying its runtime, whose environment cleanup releases its queue. Leak
// reports go to console.warn.

import addon from "maplibre_native_ffi.node";

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
