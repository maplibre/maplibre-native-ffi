/**
 * Camera values.
 *
 * The C API's camera struct carries a field mask, so an omitted field and a
 * field set to zero are different things. These types keep that distinction:
 * a field this object leaves `undefined` is absent, and every other value is
 * present even when it is zero.
 */

import type { EdgeInsets, LatLng, ScreenPoint } from "./geo.ts";
import { edgeInsetsEquals, latLngEquals, screenPointEquals } from "./geo.ts";

/** Where the camera is, or where a command should take it. */
export interface CameraOptions {
  readonly center?: LatLng;
  readonly centerAltitude?: number;
  readonly padding?: EdgeInsets;
  /**
   * Screen-space focal point for a camera command.
   *
   * This field is input-only: MapLibre Native applies it to commands and leaves
   * it out of every snapshot it reports.
   */
  readonly anchor?: ScreenPoint;
  readonly zoom?: number;
  readonly bearing?: number;
  readonly pitch?: number;
  readonly roll?: number;
  readonly fieldOfView?: number;
}

/** A cubic easing curve for an animated transition. */
export interface UnitBezier {
  readonly x1: number;
  readonly y1: number;
  readonly x2: number;
  readonly y2: number;
}

/** Optional controls for an animated camera transition. */
export interface AnimationOptions {
  /** Duration in milliseconds. */
  readonly durationMs?: number;
  /** Average fly-to velocity in screenfuls per second. */
  readonly velocity?: number;
  /** Lowest zoom a fly-to transition passes through. */
  readonly minZoom?: number;
  readonly easing?: UnitBezier;
  /**
   * Identifies the transition, so the event that reports its end names it.
   *
   * A caller that never sets one still gets the event; it just carries the id
   * MapLibre chose.
   */
  readonly transitionId?: bigint;
}

export function cameraOptionsEquals(
  left: CameraOptions,
  right: CameraOptions,
): boolean {
  return (
    optionalEquals(left.center, right.center, latLngEquals) &&
    left.centerAltitude === right.centerAltitude &&
    optionalEquals(left.padding, right.padding, edgeInsetsEquals) &&
    optionalEquals(left.anchor, right.anchor, screenPointEquals) &&
    left.zoom === right.zoom &&
    left.bearing === right.bearing &&
    left.pitch === right.pitch &&
    left.roll === right.roll &&
    left.fieldOfView === right.fieldOfView
  );
}

/** Produces an independent copy, so a later mutation of one is not the other. */
export function copyCameraOptions(camera: CameraOptions): CameraOptions {
  return {
    ...(camera.center !== undefined && { center: { ...camera.center } }),
    ...(camera.centerAltitude !== undefined && {
      centerAltitude: camera.centerAltitude,
    }),
    ...(camera.padding !== undefined && { padding: { ...camera.padding } }),
    ...(camera.anchor !== undefined && { anchor: { ...camera.anchor } }),
    ...(camera.zoom !== undefined && { zoom: camera.zoom }),
    ...(camera.bearing !== undefined && { bearing: camera.bearing }),
    ...(camera.pitch !== undefined && { pitch: camera.pitch }),
    ...(camera.roll !== undefined && { roll: camera.roll }),
    ...(camera.fieldOfView !== undefined && {
      fieldOfView: camera.fieldOfView,
    }),
  };
}

function optionalEquals<T>(
  left: T | undefined,
  right: T | undefined,
  equals: (left: T, right: T) => boolean,
): boolean {
  if (left === undefined || right === undefined) {
    return left === right;
  }
  return equals(left, right);
}
