/**
 * Materializing C structs, and reading the ones C fills.
 *
 * Every materializer starts from the C API's own default initializer, sets the
 * semantic fields the public value carries, and leaves the ABI bookkeeping —
 * `size`, field masks, nested storage — to this layer.
 */

import type { AnimationOptions, CameraOptions, UnitBezier } from "../camera.ts";
import { MaplibreError } from "../errors.ts";
import type { EdgeInsets, LatLng, ScreenPoint } from "../geo.ts";
import { EP } from "../raw/entrypoints.ts";
import {
  MLN_ANIMATION_OPTION_FIELD,
  MLN_CAMERA_OPTION_FIELD,
} from "../raw/enums.ts";
import type { Scope } from "./memory.ts";
import type { Native } from "./native.ts";
import type { Ptr } from "./transport.ts";

/** Writes an `mln_camera_options` a camera command reads. */
export function writeCameraOptions(
  native: Native,
  scope: Scope,
  camera: CameraOptions,
): Ptr {
  const layout = native.layout("mln_camera_options");
  const storage = scope.allocateZeroed(layout.size);
  native.structValue(scope, EP.mln_camera_options_default, storage);
  const view = native.memory.view(storage, layout.size);
  const fields = layout.fields;
  let mask = 0;

  if (camera.center !== undefined) {
    view.setFloat64(fields.latitude!.offset, camera.center.latitude, true);
    view.setFloat64(fields.longitude!.offset, camera.center.longitude, true);
    mask |= MLN_CAMERA_OPTION_FIELD.MLN_CAMERA_OPTION_CENTER;
  }
  if (camera.centerAltitude !== undefined) {
    view.setFloat64(
      fields.center_altitude!.offset,
      camera.centerAltitude,
      true,
    );
    mask |= MLN_CAMERA_OPTION_FIELD.MLN_CAMERA_OPTION_CENTER_ALTITUDE;
  }
  if (camera.padding !== undefined) {
    writeEdgeInsets(native, view, fields.padding!.offset, camera.padding);
    mask |= MLN_CAMERA_OPTION_FIELD.MLN_CAMERA_OPTION_PADDING;
  }
  if (camera.anchor !== undefined) {
    writeScreenPoint(native, view, fields.anchor!.offset, camera.anchor);
    mask |= MLN_CAMERA_OPTION_FIELD.MLN_CAMERA_OPTION_ANCHOR;
  }
  if (camera.zoom !== undefined) {
    view.setFloat64(fields.zoom!.offset, camera.zoom, true);
    mask |= MLN_CAMERA_OPTION_FIELD.MLN_CAMERA_OPTION_ZOOM;
  }
  if (camera.bearing !== undefined) {
    view.setFloat64(fields.bearing!.offset, camera.bearing, true);
    mask |= MLN_CAMERA_OPTION_FIELD.MLN_CAMERA_OPTION_BEARING;
  }
  if (camera.pitch !== undefined) {
    view.setFloat64(fields.pitch!.offset, camera.pitch, true);
    mask |= MLN_CAMERA_OPTION_FIELD.MLN_CAMERA_OPTION_PITCH;
  }
  if (camera.roll !== undefined) {
    view.setFloat64(fields.roll!.offset, camera.roll, true);
    mask |= MLN_CAMERA_OPTION_FIELD.MLN_CAMERA_OPTION_ROLL;
  }
  if (camera.fieldOfView !== undefined) {
    view.setFloat64(fields.field_of_view!.offset, camera.fieldOfView, true);
    mask |= MLN_CAMERA_OPTION_FIELD.MLN_CAMERA_OPTION_FOV;
  }
  view.setUint32(fields.fields!.offset, mask, true);
  return storage;
}

/** Reads an `mln_camera_options` the C API filled. */
export function readCameraOptions(native: Native, storage: Ptr): CameraOptions {
  const layout = native.layout("mln_camera_options");
  const view = native.memory.view(storage, layout.size);
  const fields = layout.fields;
  const mask = view.getUint32(fields.fields!.offset, true);
  const present = (flag: number): boolean => (mask & flag) !== 0;

  const center: LatLng | undefined = present(
    MLN_CAMERA_OPTION_FIELD.MLN_CAMERA_OPTION_CENTER,
  )
    ? {
        latitude: view.getFloat64(fields.latitude!.offset, true),
        longitude: view.getFloat64(fields.longitude!.offset, true),
      }
    : undefined;

  return {
    ...(center !== undefined && { center }),
    ...(present(MLN_CAMERA_OPTION_FIELD.MLN_CAMERA_OPTION_CENTER_ALTITUDE) && {
      centerAltitude: view.getFloat64(fields.center_altitude!.offset, true),
    }),
    ...(present(MLN_CAMERA_OPTION_FIELD.MLN_CAMERA_OPTION_PADDING) && {
      padding: readEdgeInsets(native, view, fields.padding!.offset),
    }),
    ...(present(MLN_CAMERA_OPTION_FIELD.MLN_CAMERA_OPTION_ANCHOR) && {
      anchor: readScreenPoint(native, view, fields.anchor!.offset),
    }),
    ...(present(MLN_CAMERA_OPTION_FIELD.MLN_CAMERA_OPTION_ZOOM) && {
      zoom: view.getFloat64(fields.zoom!.offset, true),
    }),
    ...(present(MLN_CAMERA_OPTION_FIELD.MLN_CAMERA_OPTION_BEARING) && {
      bearing: view.getFloat64(fields.bearing!.offset, true),
    }),
    ...(present(MLN_CAMERA_OPTION_FIELD.MLN_CAMERA_OPTION_PITCH) && {
      pitch: view.getFloat64(fields.pitch!.offset, true),
    }),
    ...(present(MLN_CAMERA_OPTION_FIELD.MLN_CAMERA_OPTION_ROLL) && {
      roll: view.getFloat64(fields.roll!.offset, true),
    }),
    ...(present(MLN_CAMERA_OPTION_FIELD.MLN_CAMERA_OPTION_FOV) && {
      fieldOfView: view.getFloat64(fields.field_of_view!.offset, true),
    }),
  };
}

/** Writes an `mln_animation_options` a transition command reads. */
export function writeAnimationOptions(
  native: Native,
  scope: Scope,
  animation: AnimationOptions,
): Ptr {
  const layout = native.layout("mln_animation_options");
  const storage = scope.allocateZeroed(layout.size);
  native.structValue(scope, EP.mln_animation_options_default, storage);
  const view = native.memory.view(storage, layout.size);
  const fields = layout.fields;
  let mask = 0;

  if (animation.durationMs !== undefined) {
    view.setFloat64(fields.duration_ms!.offset, animation.durationMs, true);
    mask |= MLN_ANIMATION_OPTION_FIELD.MLN_ANIMATION_OPTION_DURATION;
  }
  if (animation.velocity !== undefined) {
    view.setFloat64(fields.velocity!.offset, animation.velocity, true);
    mask |= MLN_ANIMATION_OPTION_FIELD.MLN_ANIMATION_OPTION_VELOCITY;
  }
  if (animation.minZoom !== undefined) {
    view.setFloat64(fields.min_zoom!.offset, animation.minZoom, true);
    mask |= MLN_ANIMATION_OPTION_FIELD.MLN_ANIMATION_OPTION_MIN_ZOOM;
  }
  if (animation.easing !== undefined) {
    writeUnitBezier(native, view, fields.easing!.offset, animation.easing);
    mask |= MLN_ANIMATION_OPTION_FIELD.MLN_ANIMATION_OPTION_EASING;
  }
  if (animation.transitionId !== undefined) {
    view.setBigUint64(
      fields.transition_id!.offset,
      animation.transitionId,
      true,
    );
    mask |= MLN_ANIMATION_OPTION_FIELD.MLN_ANIMATION_OPTION_TRANSITION_ID;
  }
  view.setUint32(fields.fields!.offset, mask, true);
  return storage;
}

function writeEdgeInsets(
  native: Native,
  view: DataView,
  offset: number,
  insets: EdgeInsets,
): void {
  const fields = native.layout("mln_edge_insets").fields;
  view.setFloat64(offset + fields.top!.offset, insets.top, true);
  view.setFloat64(offset + fields.left!.offset, insets.left, true);
  view.setFloat64(offset + fields.bottom!.offset, insets.bottom, true);
  view.setFloat64(offset + fields.right!.offset, insets.right, true);
}

function readEdgeInsets(
  native: Native,
  view: DataView,
  offset: number,
): EdgeInsets {
  const fields = native.layout("mln_edge_insets").fields;
  return {
    top: view.getFloat64(offset + fields.top!.offset, true),
    left: view.getFloat64(offset + fields.left!.offset, true),
    bottom: view.getFloat64(offset + fields.bottom!.offset, true),
    right: view.getFloat64(offset + fields.right!.offset, true),
  };
}

function writeScreenPoint(
  native: Native,
  view: DataView,
  offset: number,
  point: ScreenPoint,
): void {
  const fields = native.layout("mln_screen_point").fields;
  view.setFloat64(offset + fields.x!.offset, point.x, true);
  view.setFloat64(offset + fields.y!.offset, point.y, true);
}

function readScreenPoint(
  native: Native,
  view: DataView,
  offset: number,
): ScreenPoint {
  const fields = native.layout("mln_screen_point").fields;
  return {
    x: view.getFloat64(offset + fields.x!.offset, true),
    y: view.getFloat64(offset + fields.y!.offset, true),
  };
}

function writeUnitBezier(
  native: Native,
  view: DataView,
  offset: number,
  easing: UnitBezier,
): void {
  const fields = native.layout("mln_unit_bezier").fields;
  view.setFloat64(offset + fields.x1!.offset, easing.x1, true);
  view.setFloat64(offset + fields.y1!.offset, easing.y1, true);
  view.setFloat64(offset + fields.x2!.offset, easing.x2, true);
  view.setFloat64(offset + fields.y2!.offset, easing.y2, true);
}

/**
 * Runs a copy-out entry point.
 *
 * These report the length they need before they check the capacity they were
 * given, so a null buffer with zero capacity is a size probe. The probe runs
 * first, then the copy, so the caller never guesses a buffer size.
 */
export function copyOutText(
  native: Native,
  entrypoint: number,
  leading: readonly bigint[],
): string {
  return native.scope((scope) => {
    const outSize = scope.allocateZeroed(8);
    native.checked(scope, entrypoint, [...leading, 0n, 0n, outSize]);
    const required = native.readSize(outSize);
    if (required === 0) {
      return "";
    }
    const buffer = scope.allocateZeroed(required);
    native.checked(scope, entrypoint, [
      ...leading,
      buffer,
      BigInt(required),
      outSize,
    ]);
    const written = native.readSize(outSize);
    if (written > required) {
      throw new MaplibreError(
        "invalidState",
        `a copy-out entry point reported ${written} bytes after asking for ${required}`,
      );
    }
    return new TextDecoder().decode(native.memory.bytes(buffer, written));
  });
}
