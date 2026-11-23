// The messages between the demo page and the Web Worker that runs its map
// (map-worker.ts). The library runs on the thread that loads it and never
// blocks it, so the map's work stays off the page's thread, and the page
// sends the worker the canvas and the user's input.

import type { CameraOptions, GestureInput } from "@maplibre/native-ffi";

/** The camera, as the map last reported it. */
export interface View {
  readonly camera: Required<
    Pick<CameraOptions, "center" | "zoom" | "bearing" | "pitch">
  >;
}

export interface LogicalSize {
  readonly width: number;
  readonly height: number;
}

export type ToWorker =
  | {
      readonly type: "start";
      /** The URL of the browser module's `maplibre_native_ffi.js`. */
      readonly module: string;
      /** The page's canvas, transferred, which the map's session presents to. */
      readonly canvas: OffscreenCanvas;
      readonly size: LogicalSize;
      readonly scaleFactor: number;
      readonly camera: CameraOptions;
      readonly styleId: string;
    }
  | { readonly type: "resize"; readonly size: LogicalSize }
  /**
   * The inputs of one event for the map's gesture recognizer, at the event's
   * time as `performance.timeOrigin + timeStamp`.
   */
  | {
      readonly type: "gesture";
      readonly inputs: readonly GestureInput[];
      readonly time: number;
    }
  | { readonly type: "setStyle"; readonly styleId: string };

export type FromWorker =
  | { readonly type: "status"; readonly message: string }
  | {
      readonly type: "error";
      readonly message: string;
      readonly fatal: boolean;
    }
  | { readonly type: "attributions"; readonly attributions: string[] }
  /** The map is on the canvas, and takes input. */
  | { readonly type: "ready"; readonly view: View }
  /** The camera moved; `settled` once it stopped. */
  | { readonly type: "camera"; readonly view: View; readonly settled: boolean };
