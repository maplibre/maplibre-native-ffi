/**
 * MapLibre Native FFI's Hermes binding for React Native apps. Loading this
 * module installs the binding in the app's runtime, then exports it, so import
 * the binding from here rather than from `@maplibre/native-ffi/hermes`.
 *
 * @module
 */
import "./NativeMaplibreNativeFfi";

export * from "@maplibre/native-ffi/hermes";

export {
  default as MaplibreSurfaceView,
  type NativeProps as MaplibreSurfaceViewProps,
  type SurfaceEvent,
} from "./MaplibreSurfaceViewNativeComponent";

/**
 * A surface's handles, for the `androidWindow` and `metalLayer` targets of
 * `MapHandle.attach()`.
 */
export type SurfaceHandles =
  | { readonly kind: "android"; readonly window: bigint }
  | { readonly kind: "metal"; readonly layer: bigint };

/**
 * The surfaces that `MaplibreSurfaceView`s lend. A view reports a surface's ID
 * when the platform provides it, and again when the platform takes it away.
 * Release a surface once its session no longer uses it, which frees it if the
 * view has already let go of it.
 */
export interface Surfaces {
  /** The surface's handles, or undefined once the view has withdrawn it. */
  acquire(surface: number): SurfaceHandles | undefined;
  release(surface: number): void;
}

export const surfaces = (
  globalThis as typeof globalThis & { __mlnReactNativeSurfaces: Surfaces }
).__mlnReactNativeSurfaces;
