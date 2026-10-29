/**
 * React Native's New Architecture backend, tested with React Native 0.88 and
 * its prebuilt Hermes. Native work and outstanding owners end when the runtime
 * reloads. The app's Android Application must expose its ReactHost through
 * ReactApplication.reactHost so teardown can run before the JS queue stops.
 *
 * Android defaults to OpenGL; set maplibre.android.backend=vulkan in
 * gradle.properties to select Vulkan. iOS uses Metal. This entry exposes
 * the same low-level API as the desktop binding and creates no native views.
 *
 * @module react-native
 */
import "./specs/NativeMaplibreNativeFfi.ts";
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
