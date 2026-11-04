// Import this before anything imports "@maplibre/native-ffi/hermes". Loading
// the native module installs the binding in this runtime.
import "./NativeMaplibreHost";

export {
  default as MapSurfaceView,
  type NativeProps as MapSurfaceViewProps,
  type SurfaceEvent,
} from "./MapSurfaceViewNativeComponent";

/** What a view's surface is, as handles for the binding. */
export type SurfaceHandles =
  | {
      readonly kind: "egl";
      readonly display: bigint;
      readonly config: bigint;
      readonly surface: bigint;
    }
  | { readonly kind: "metal"; readonly layer: bigint };

/**
 * The surfaces that `MapSurfaceView`s lend. A view reports a surface's ID when
 * the platform provides it, and again when the platform takes it away. Release
 * a surface once its session no longer uses it, which frees it if the view has
 * already let go of it.
 */
export interface Surfaces {
  /** The surface's handles, or undefined once the view has withdrawn it. */
  acquire(surface: number): SurfaceHandles | undefined;
  release(surface: number): void;
}

export const surfaces = (
  globalThis as typeof globalThis & { __mlnExampleSurfaces: Surfaces }
).__mlnExampleSurfaces;
