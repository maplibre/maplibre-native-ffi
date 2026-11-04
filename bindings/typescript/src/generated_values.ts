// Generated from the C headers by tools/bindgen. Do not edit.

import type {
  MapHandle,
  ResourceRequestHandle,
  ResourceTransformRequestHandle,
} from "./generated_operations.ts";
import {
  type NativeRegistration,
  nativeRegistration,
} from "./internal/native_registrations.ts";

/**
 * An Android window to present to, for `MapHandle.attachAndroidSurface()`.
 *
 * Name the window by `window`, or by `jni_env` and `surface`. The session takes
 * its own reference to the window, so you can release yours once the call
 * returns. It uses the window until a replacement or release of the target
 * completes, a detach or release of the session completes, or an abandon of it
 * returns.
 *
 * See `mln_android_surface_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
 */
export interface AndroidSurfaceDescriptor {
  logicalSize: LogicalSize;
  /**
   * Your `ANativeWindow*`, or null to name the window by `surface`.
   */
  window?: bigint;
  /**
   * The calling thread's `JNIEnv*`, with `surface`.
   */
  jniEnv?: bigint;
  /**
   * An `android.view.Surface`, with `jni_env`.
   */
  surface?: bigint;
}

/**
 * A new Android window for `RenderSessionHandle.setAndroidSurfaceTarget()`.
 *
 * Name the window by `window`, or by `jni_env` and `surface`. The session takes
 * its own reference to the window, so you can release yours once the call
 * returns. It uses the window until a replacement or release of the target
 * completes, a detach or release of the session completes, or an abandon of it
 * returns.
 *
 * See `mln_android_surface_replacement` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
 */
export interface AndroidSurfaceReplacement {
  logicalSize: LogicalSize;
  /**
   * Your `ANativeWindow*`, or null to name the window by `surface`.
   */
  window?: bigint;
  /**
   * The calling thread's `JNIEnv*`, with `surface`.
   */
  jniEnv?: bigint;
  /**
   * An `android.view.Surface`, with `jni_env`.
   */
  surface?: bigint;
}

/**
 * How a camera change animates.
 *
 * See `mln_animation_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface AnimationOptions {
  durationMs?: number;
  /**
   * The average speed of a fly animation, in screens per second. `duration_ms`
   * overrides it.
   */
  velocity?: number;
  /**
   * The zoom level at the peak of a fly animation.
   */
  minZoom?: number;
  easing?: UnitBezier;
  /**
   * An ID that `MapHandle.cancelCameraTransitionsWithId()` cancels the change
   * by. Several changes can share one.
   */
  transitionId?: bigint;
  endHandler?: CameraTransitionHandler;
}

/**
 * Limits on the camera, for `MapHandle.setBounds()`.
 *
 * See `mln_bound_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface BoundOptions {
  /**
   * The area that the camera's center stays within.
   */
  bounds?: LatLngBounds;
  minZoom?: number;
  maxZoom?: number;
  /**
   * The lowest pitch, in degrees.
   */
  minPitch?: number;
  /**
   * The highest pitch, in degrees.
   */
  maxPitch?: number;
  /**
   * Removes the area that limits the camera's center.
   */
  unbounded?: boolean;
}

/**
 * A maintenance action on a runtime's cache database.
 *
 * The cache database is the file that `RuntimeOptions.cachePath` names. It
 * holds the ambient cache, meaning the resources that maps loaded, and the
 * offline regions.
 *
 * See `mln_cache_database_operation` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const CacheDatabaseOperation = {
  /**
   * Deletes the ambient cache's resources and frees the space they used.
   * Offline regions keep their resources.
   */
  clear: 1,
  /**
   * Keeps the ambient cache's resources, but makes maps check each one with the
   * server before they use it again. Offline regions keep their resources as
   * they are.
   */
  invalidate: 2,
  /**
   * Compacts the whole database file to free the disk space that it no longer
   * uses.
   */
  pack: 3,
  /**
   * Deletes the whole database, offline regions included, and starts an empty
   * one.
   */
  reset: 4,
} as const;
/** One of the CacheDatabaseOperation values, or a value this binding does not know. */
export type CacheDatabaseOperation =
  | (typeof CacheDatabaseOperation)[keyof typeof CacheDatabaseOperation]
  | (number & {});

/**
 * A map's whole camera.
 *
 * See `mln_camera` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface Camera {
  center: LatLng;
  /**
   * The altitude of the center, in meters above sea level.
   */
  centerAltitude: number;
  /**
   * Insets from the map's edges. The camera centers within the rest.
   */
  padding: EdgeInsets;
  zoom: number;
  /**
   * The rotation from north, in degrees.
   */
  bearing: number;
  /**
   * The tilt toward the horizon, in degrees. 0 looks straight down.
   */
  pitch: number;
  /**
   * The roll, in degrees.
   */
  roll: number;
  /**
   * The vertical field of view, in degrees.
   */
  fieldOfView: number;
}

/**
 * Converts this `Camera` to `CameraOptions` that set each of its members.
 */
export function cameraToOptions(camera: Camera): CameraOptions {
  return {
    center: camera.center,
    centerAltitude: camera.centerAltitude,
    padding: camera.padding,
    zoom: camera.zoom,
    bearing: camera.bearing,
    pitch: camera.pitch,
    roll: camera.roll,
    fieldOfView: camera.fieldOfView,
  };
}

/**
 * All the limits on a map's camera.
 *
 * Its members mean the same as those of `BoundOptions`.
 *
 * See `mln_camera_bounds` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface CameraBounds {
  /**
   * The area that the camera's center stays within. When `unbounded` is true,
   * it covers the whole world.
   */
  readonly bounds: LatLngBounds;
  /**
   * Whether no area limits the camera's center.
   */
  readonly unbounded: boolean;
  readonly minZoom: number;
  readonly maxZoom: number;
  /**
   * The lowest pitch, in degrees.
   */
  readonly minPitch: number;
  /**
   * The highest pitch, in degrees.
   */
  readonly maxPitch: number;
}

/**
 * Whether a camera change jumps or animates.
 *
 * See `mln_camera_change_mode` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const CameraChangeMode = {
  immediate: 0,
  animated: 1,
} as const;
/** One of the CameraChangeMode values, or a value this binding does not know. */
export type CameraChangeMode =
  | (typeof CameraChangeMode)[keyof typeof CameraChangeMode]
  | (number & {});

/**
 * A change to the camera from where it is now, for gestures.
 *
 * See `mln_camera_delta` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface CameraDelta {
  /**
   * Moves the map's content this far on the screen, as a drag does.
   */
  offset?: ScreenPoint;
  /**
   * Multiplies the map's scale. 2 zooms in by one level.
   */
  scale?: number;
  /**
   * Degrees to add to the bearing.
   */
  bearing?: number;
  /**
   * Degrees to add to the pitch.
   */
  pitch?: number;
  /**
   * The screen point that `scale`, `bearing`, and `pitch` keep in place. It
   * needs one of them, and it combines with `offset` only in a delta without a
   * duration.
   */
  anchor?: ScreenPoint;
  animation?: AnimationOptions;
  gesturePhase?: GesturePhase;
}

/**
 * Options for finding the camera that fits an area.
 *
 * See `mln_camera_fit_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface CameraFitOptions {
  /**
   * Space to leave between the area and each edge of the map.
   */
  padding?: EdgeInsets;
  /**
   * The rotation from north, in degrees.
   */
  bearing?: number;
  /**
   * The tilt toward the horizon, in degrees.
   */
  pitch?: number;
}

/**
 * A camera change that fits an area, for `MapHandle.fitCameraToLatLngBounds()`
 * and the other fit commands.
 *
 * A jump uses only the end handler of `animation`, and the handler runs as soon
 * as the camera moves.
 *
 * See `mln_camera_fit_update` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface CameraFitUpdate {
  mode?: CameraUpdateMode;
  fit?: CameraFitOptions;
  animation?: AnimationOptions;
}

/**
 * A camera, with only the members that `fields` selects.
 *
 * See `mln_camera_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface CameraOptions {
  center?: LatLng;
  /**
   * The altitude of the center, in meters above sea level.
   */
  centerAltitude?: number;
  /**
   * Insets from the map's edges. The camera centers within the rest.
   */
  padding?: EdgeInsets;
  /**
   * The screen point that a change to the zoom, bearing, or pitch keeps in
   * place. A `center` overrides it.
   */
  anchor?: ScreenPoint;
  zoom?: number;
  /**
   * The rotation from north, in degrees.
   */
  bearing?: number;
  /**
   * The tilt toward the horizon, in degrees. 0 looks straight down.
   */
  pitch?: number;
  /**
   * The roll, in degrees.
   */
  roll?: number;
  /**
   * The vertical field of view, in degrees.
   */
  fieldOfView?: number;
}

/**
 * A map's camera, with the snapshot generation that it matches.
 *
 * See `mln_camera_query_result` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface CameraQueryResult {
  readonly generation: bigint;
  readonly camera: Camera;
}

/**
 * The end of a camera change.
 *
 * See `mln_camera_transition_end` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface CameraTransitionEnd {
  readonly outcome: CameraTransitionOutcome;
  /**
   * The generation of the first map snapshot that shows where the change left
   * the camera, or 0 when the map closed first.
   */
  readonly generation: bigint;
}

/**
 * A callback for the end of a camera change, for `AnimationOptions`.
 *
 * The callback runs once for each camera command, after every part of the
 * change that the command started has ended.
 *
 * See `mln_camera_transition_handler` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface CameraTransitionHandler {
  callback?: (end: CameraTransitionEnd) => void;
}

/**
 * How a camera change ended.
 *
 * A camera delta that pans and also scales, rotates, or pitches animates as two
 * parts, and any other change as one. When the parts of a change end in
 * different ways, the change reports `CameraTransitionOutcome.cancelled` if any
 * part was cancelled, then `CameraTransitionOutcome.replaced` if any part was
 * replaced.
 *
 * See `mln_camera_transition_outcome` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const CameraTransitionOutcome = {
  /**
   * The camera finished the change.
   */
  completed: 0,
  /**
   * `MapHandle.cancelCameraTransitions()`,
   * `MapHandle.cancelCameraTransitionsWithId()`, a gesture's
   * `GesturePhase.begin` or `GesturePhase.cancel`,
   * `MapHandle.setFreeCameraOptions()`, or a resize that moves the camera back
   * inside its bounds stopped the change, or the command failed before it moved
   * the camera.
   */
  cancelled: 1,
  /**
   * The map closed before the change ended.
   */
  closed: 2,
  /**
   * A later camera change set every camera field that a part of the change
   * still animated, so that part stopped.
   *
   * A fly animates its center and zoom together, and an anchor animates the
   * center together with the fields it keeps in place, so a later change of any
   * of them replaces them all. A part that a later change took only some fields
   * from keeps animating the others, and ends as they end.
   */
  replaced: 3,
} as const;
/** One of the CameraTransitionOutcome values, or a value this binding does not know. */
export type CameraTransitionOutcome =
  | (typeof CameraTransitionOutcome)[keyof typeof CameraTransitionOutcome]
  | (number & {});

/**
 * A camera change for `MapHandle.updateCamera()`.
 *
 * A jump uses only the end handler of `animation`, and the handler runs as soon
 * as the camera moves.
 *
 * See `mln_camera_update` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface CameraUpdate {
  mode?: CameraUpdateMode;
  camera?: CameraOptions;
  animation?: AnimationOptions;
  gesturePhase?: GesturePhase;
}

/**
 * How a camera command moves the camera.
 *
 * See `mln_camera_update_mode` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const CameraUpdateMode = {
  /**
   * Moves the camera at once.
   */
  jump: 0,
  /**
   * Animates the camera straight to the target.
   */
  ease: 1,
  /**
   * Zooms out, travels to the target, and zooms back in.
   */
  fly: 2,
} as const;
/** One of the CameraUpdateMode values, or a value this binding does not know. */
export type CameraUpdateMode =
  | (typeof CameraUpdateMode)[keyof typeof CameraUpdateMode]
  | (number & {});

/**
 * A tile's zoom level `z`, column `x`, and row `y`, with row 0 at the north.
 *
 * See `mln_canonical_tile_id` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface CanonicalTileId {
  z: number;
  x: number;
  y: number;
}

/**
 * How the map keeps the camera within the world or within its bounds.
 *
 * See `mln_constrain_mode` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const ConstrainMode = {
  none: 0,
  /**
   * Keeps the world filling the view from top to bottom.
   */
  heightOnly: 1,
  /**
   * Keeps the world filling the view in both directions.
   */
  widthAndHeight: 2,
  /**
   * Keeps the whole view inside the bounds from `MapHandle.setBounds()`.
   */
  screen: 3,
} as const;
/** One of the ConstrainMode values, or a value this binding does not know. */
export type ConstrainMode =
  | (typeof ConstrainMode)[keyof typeof ConstrainMode]
  | (number & {});

/**
 * The callbacks and options of a custom geometry source.
 *
 * Zoom levels are whole numbers from 0 to 32.
 *
 * See `mln_custom_geometry_source_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface CustomGeometrySourceOptions {
  fetchTile: (
    map: MapHandle,
    sourceId: string,
    tileId: CanonicalTileId,
  ) => void;
  cancelTile?: (
    map: MapHandle,
    sourceId: string,
    tileId: CanonicalTileId,
  ) => void;
  minZoom?: number;
  maxZoom?: number;
  /**
   * How far, in pixels, simplified geometry can stray from the original.
   */
  tolerance?: number;
  /**
   * The width of a tile in pixels, a power of two no larger than 8192.
   */
  tileSize?: number;
  /**
   * The width in pixels of the margin around a tile that holds geometry from
   * beyond its edges. `buffer` times 8192 / `tile_size` must be at most 65535.
   */
  buffer?: number;
  /**
   * Whether to cut geometry at the outer edge of a tile's margin.
   */
  clip?: boolean;
  /**
   * Whether to wrap geometry that crosses the antimeridian.
   */
  wrap?: boolean;
}

/**
 * The callbacks and options of a custom vector source.
 *
 * Zoom levels are whole numbers from 0 to 32.
 *
 * See `mln_custom_vector_source_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface CustomVectorSourceOptions {
  fetchTile: (
    map: MapHandle,
    sourceId: string,
    tileId: CanonicalTileId,
  ) => void;
  cancelTile?: (
    map: MapHandle,
    sourceId: string,
    tileId: CanonicalTileId,
  ) => void;
  minZoom?: number;
  maxZoom?: number;
}

/**
 * Distances in logical pixels from each edge of the map.
 *
 * See `mln_edge_insets` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface EdgeInsets {
  top: number;
  left: number;
  bottom: number;
  right: number;
}

/**
 * The EGL objects that a render session makes its OpenGL context from.
 *
 * See `mln_egl_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface EglContextDescriptor {
  /**
   * An `EGLDisplay`.
   */
  display: bigint;
  /**
   * An `EGLConfig`.
   */
  config: bigint;
  /**
   * Your `EGLContext`, for a shared context.
   */
  shareContext?: bigint;
  /**
   * The API of a dedicated context. A shared context uses the API of yours.
   */
  clientApi?: OpenGlClientApi;
  /**
   * Your `eglGetProcAddress`, or null to use the system's.
   */
  getProcAddress?: bigint;
}

/**
 * The ID of a feature in a source, as the source's data gives it.
 *
 * The map compares feature IDs as text, so a string and a number with the same
 * text, such as "7" and 7, name the same feature. A number's text is its
 * shortest decimal form without a trailing ".0", so 7, 7.0, and "7" name the
 * same feature, and so do 1e21 and "1e21".
 *
 * See `mln_feature_id` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export type FeatureId =
  | { readonly kind: "stringValue"; readonly value: string }
  | { readonly kind: "uintValue"; readonly value: bigint }
  | { readonly kind: "intValue"; readonly value: bigint }
  | { readonly kind: "doubleValue"; readonly value: number };

/**
 * The features whose state to set, read, or remove.
 *
 * See `mln_feature_state_selector` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface FeatureStateSelector {
  sourceId: string;
  /**
   * The source layer, for a vector source. Empty for other sources.
   */
  sourceLayerId?: string;
  /**
   * The feature's ID. Setting or reading state needs it. Without it,
   * `MapHandle.removeFeatureState()` removes the state of every feature in the
   * source layer.
   */
  featureId?: FeatureId;
  /**
   * The one key that `MapHandle.removeFeatureState()` removes. It needs
   * `feature_id`. Empty removes every key.
   */
  stateKey?: string;
}

/**
 * A demand for one frame.
 *
 * See `mln_frame_demand` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
export interface FrameDemand {
  flags?: FrameDemandFlag;
  /**
   * A value of your choice that the demand's frame result carries.
   */
  token?: bigint;
}

/**
 * How a frame demand behaves.
 *
 * Without flags, a demand draws a frame when the map changed since the last
 * one, and otherwise finishes with `RenderResult.noUpdate`. A session with a
 * surface target shows each frame that it draws.
 *
 * See `mln_frame_demand_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
export const FrameDemandFlag = {
  /**
   * Draws a frame even when nothing changed since the last one. It can't be
   * combined with `FrameDemandFlag.waitForUpdate`.
   */
  force: 1,
  /**
   * When the map has nothing new to draw, waits for its next update instead of
   * finishing with `RenderResult.noUpdate`.
   */
  waitForUpdate: 4,
} as const;
/** A bitwise OR of FrameDemandFlag flags. */
export type FrameDemandFlag = number;

/**
 * Options for creating a frame pacer.
 *
 * See `mln_frame_pacer_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
export interface FramePacerOptions {
  /**
   * The most times per second that the pacer demands frames, greater than zero.
   * The pacer skips a display refresh that comes too soon after its last
   * demands, and a limit at the display's refresh rate or above skips none. By
   * default, it can demand frames at every refresh.
   */
  maximumFramesPerSecond?: number;
}

/**
 * A map camera's whole position and orientation in three dimensions.
 *
 * Its members mean the same as those of `FreeCameraOptions`.
 *
 * See `mln_free_camera` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface FreeCamera {
  readonly position: Vec3;
  readonly orientation: Quaternion;
}

/**
 * The camera's position and orientation in three dimensions.
 *
 * See `mln_free_camera_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface FreeCameraOptions {
  /**
   * The camera's position in Web Mercator units, where the world is 1 wide, (0,
   * 0) is its northwest corner, and (1, 1) is its southeast corner. `z` is the
   * altitude in the same units.
   */
  position?: Vec3;
  /**
   * The camera's rotation from looking straight down with north at the top of
   * the screen.
   */
  orientation?: Quaternion;
}

/**
 * Options for a GeoJSON
 * &#91;source&#93;(https://maplibre.org/maplibre-style-spec/sources/#geojson).
 *
 * Zoom levels are whole numbers.
 *
 * See `mln_geo_json_source_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface GeoJsonSourceOptions {
  minZoom?: number;
  maxZoom?: number;
  tolerance?: number;
  /**
   * The last zoom level that has clusters, at most 30.
   */
  clusterMaxZoom?: number;
  /**
   * The `clusterProperties` object, or empty for none.
   */
  clusterProperties?: unknown;
  buffer?: number;
  clusterRadius?: number;
  /**
   * At most 4294967295 on a 32-bit target.
   */
  clusterMinPoints?: bigint;
  lineMetrics?: boolean;
  cluster?: boolean;
}

/**
 * Where a camera change falls in a gesture that the user makes.
 *
 * `GesturePhase.begin` and `GesturePhase.update` mark a gesture in progress,
 * and `GesturePhase.end` and `GesturePhase.cancel` end it.
 *
 * See `mln_gesture_phase` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const GesturePhase = {
  /**
   * The change isn't part of a gesture.
   */
  none: 0,
  /**
   * The gesture starts. This cancels every running camera animation before the
   * change applies.
   */
  begin: 1,
  /**
   * The gesture continues. To start a gesture that keeps the running camera
   * animations, start it with this phase instead of `GesturePhase.begin`.
   */
  update: 2,
  end: 3,
  /**
   * The platform cancelled the gesture. This also cancels every running camera
   * animation after the change applies.
   */
  cancel: 4,
} as const;
/** One of the GesturePhase values, or a value this binding does not know. */
export type GesturePhase =
  | (typeof GesturePhase)[keyof typeof GesturePhase]
  | (number & {});

/**
 * A point in GPU work that the other side waits for before using a texture.
 *
 * See `mln_gpu_sync` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface GpuSync {
  kind: GpuSyncKind;
  /**
   * The object to wait on, for the kinds that name one.
   */
  handle: bigint;
  value: bigint;
}

/**
 * How one side waits for the other's GPU work on a shared texture.
 *
 * A sync file descriptor or an OpenGL fence signals once, so when
 * `AcquiredFrameHandle.release()` succeeds, the session takes the one you pass.
 * A Vulkan timeline semaphore, Metal shared event, or D3D12 fence counts up
 * with each frame, so the one you pass stays yours. Keep it valid until a
 * detach or release of the session completes.
 *
 * See `mln_gpu_sync_kind` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const GpuSyncKind = {
  /**
   * The work has already finished, so there is nothing to wait for.
   */
  cpuComplete: 0,
  /**
   * `handle` is a `GLsync` in the session's share group.
   */
  openGlFence: 1,
  /**
   * The work was submitted to the queue that both sides share, so work
   * submitted to that queue later runs after it.
   */
  queueOrder: 2,
  /**
   * `handle` is a `VkSemaphore`. Wait until it reaches `value`.
   */
  vulkanTimelineSemaphore: 3,
  /**
   * `handle` is a sync file descriptor.
   */
  syncFd: 4,
  /**
   * `handle` is an `MTLSharedEvent`. Wait until it reaches `value`.
   */
  metalSharedEvent: 5,
  /**
   * `handle` is a shared Windows handle to an `ID3D12Fence`. Wait until it
   * reaches `value`.
   */
  d3d12Fence: 6,
} as const;
/** One of the GpuSyncKind values, or a value this binding does not know. */
export type GpuSyncKind =
  | (typeof GpuSyncKind)[keyof typeof GpuSyncKind]
  | (number & {});

/**
 * A GPU sync kind, as a bit in a set.
 *
 * See `mln_gpu_sync_kind_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const GpuSyncKindFlag = {
  cpuComplete: 1,
  openGlFence: 2,
  queueOrder: 4,
  vulkanTimelineSemaphore: 8,
  syncFd: 16,
  metalSharedEvent: 32,
  d3d12Fence: 64,
} as const;
/** A bitwise OR of GpuSyncKindFlag flags. */
export type GpuSyncKindFlag = number;

/**
 * An HTTP header.
 *
 * See `mln_http_header` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface HttpHeader {
  name: string;
  value?: string;
}

/**
 * Headers to add to the HTTP requests whose URL matches a pattern.
 *
 * See `mln_http_header_rule` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface HttpHeaderRule {
  /**
   * The kinds of resource that the rule covers. Zero covers every kind.
   */
  kinds?: ResourceKindFlag;
  flags?: UrlMatchFlag;
  url?: string;
  headers?: readonly HttpHeader[];
}

/**
 * Headers to add to HTTP requests, for
 * `RuntimeHandle.setHttpHeaderTransform()`.
 *
 * See `mln_http_header_transform` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface HttpHeaderTransform {
  /**
   * The first rule that matches a request adds its headers, and no other.
   */
  rules?: readonly HttpHeaderRule[];
}

/**
 * The area of an image, in image pixels, that text fits inside.
 *
 * See `mln_image_content` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface ImageContent {
  left: number;
  top: number;
  right: number;
  bottom: number;
}

/**
 * A range of image pixels, from `from` to `to`, that stretches to fit text.
 *
 * See `mln_image_stretch` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface ImageStretch {
  from: number;
  to: number;
}

/**
 * A geographic coordinate, in degrees.
 *
 * See `mln_lat_lng` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface LatLng {
  latitude: number;
  longitude: number;
}

/**
 * A rectangle of coordinates, from its southwest to its northeast corner.
 *
 * See `mln_lat_lng_bounds` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface LatLngBounds {
  southwest: LatLng;
  northeast: LatLng;
}

/**
 * The part of the library that logged a message.
 *
 * See `mln_log_event` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
export const LogEvent = {
  general: 0,
  setup: 1,
  shader: 2,
  parseStyle: 3,
  parseTile: 4,
  render: 5,
  style: 6,
  database: 7,
  httpRequest: 8,
  sprite: 9,
  image: 10,
  graphicsBackend: 11,
  jni: 12,
  android: 13,
  crash: 14,
  glyph: 15,
  timing: 16,
} as const;
/** One of the LogEvent values, or a value this binding does not know. */
export type LogEvent = (typeof LogEvent)[keyof typeof LogEvent] | (number & {});

/**
 * A log callback, for `setLogCallback()`.
 *
 * The callback runs asynchronously on the JavaScript thread that registered it,
 * after native queues the call.
 *
 * See `mln_log_handler` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
export interface LogHandler {
  callback: (
    severity: LogSeverity,
    event: LogEvent,
    code: bigint,
    message: string,
  ) => void;
  /**
   * The severities that also go to the platform's log, as they do without a
   * callback. 0 sends none.
   *
   * A record that the handler's delivery queue can't accept is dropped. These
   * severities alone decide whether a record also goes to the platform's log,
   * whether or not the queue accepts it.
   */
  platformSeverities?: LogSeverityFlag;
}

/**
 * The severity of a log message.
 *
 * See `mln_log_severity` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
export const LogSeverity = {
  info: 1,
  warning: 2,
  error: 3,
} as const;
/** One of the LogSeverity values, or a value this binding does not know. */
export type LogSeverity =
  | (typeof LogSeverity)[keyof typeof LogSeverity]
  | (number & {});

/**
 * A log severity, as a bit in a set.
 *
 * See `mln_log_severity_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
export const LogSeverityFlag = {
  info: 2,
  warning: 4,
  error: 8,
  /**
   * The severities logged on a background thread until
   * `setLogAsyncSeverityMask()` changes them.
   */
  default: 6,
  all: 14,
} as const;
/** A bitwise OR of LogSeverityFlag flags. */
export type LogSeverityFlag = number;

/**
 * A size in logical pixels, and the physical pixels in each logical pixel.
 *
 * See `mln_logical_extent` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface LogicalExtent {
  width: number;
  height: number;
  scaleFactor: number;
}

/**
 * A size in logical pixels, which the map's scale factor turns into pixels.
 *
 * See `mln_logical_size` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface LogicalSize {
  width?: number;
  height?: number;
}

/**
 * A debug overlay that the map draws.
 *
 * See `mln_map_debug_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const MapDebugFlag = {
  tileBorders: 2,
  parseStatus: 4,
  timestamps: 8,
  collision: 16,
  overdraw: 32,
  stencilClip: 64,
  depthBuffer: 128,
} as const;
/** A bitwise OR of MapDebugFlag flags. */
export type MapDebugFlag = number;

/**
 * Why a map failed to load a style.
 *
 * See `mln_map_load_error` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const MapLoadError = {
  unknown: 0,
  /**
   * The style document isn't a valid style.
   */
  styleParse: 1,
  /**
   * The style's request failed, for any reason, including a 404 or a missing
   * file.
   */
  styleLoad: 2,
} as const;
/** One of the MapLoadError values, or a value this binding does not know. */
export type MapLoadError =
  | (typeof MapLoadError)[keyof typeof MapLoadError]
  | (number & {});

/**
 * When a map renders.
 *
 * See `mln_map_mode` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const MapMode = {
  /**
   * Renders whenever the map changes.
   */
  continuous: 0,
  /**
   * Renders only when `MapHandle.requestStillImage()` asks for an image.
   */
  static: 1,
  /**
   * Renders still images of single tiles, as `MapMode.static` does.
   */
  tile: 2,
} as const;
/** One of the MapMode values, or a value this binding does not know. */
export type MapMode = (typeof MapMode)[keyof typeof MapMode] | (number & {});

/**
 * Options for `RuntimeHandle.createMap()`.
 *
 * See `mln_map_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface MapOptions {
  /**
   * The map's first size, and its scale factor. The scale factor stays fixed
   * for the map's life, and every render session of the map renders at it. To
   * render at another density, create a new map and attach a new session.
   */
  initialExtent: LogicalExtent;
  mapMode?: MapMode;
  /**
   * Whether the map decodes MapLibre Tiles (MLT) that use FastPFOR integer
   * encoding. Without it, such tiles fail to load.
   */
  fastPforEnabled?: boolean;
  /**
   * The events that the map reports. Without it, the map reports them all.
   */
  eventMask?: RuntimeEventFlag;
  /**
   * Whether the map keeps tiles in memory after they leave the view, so they
   * show again without loading. Without it, the map keeps them.
   */
  tileCacheEnabled?: boolean;
}

/**
 * A projection's camera, as `MapProjectionHandle.getCamera()` reports it.
 *
 * See `mln_map_projection_camera_result` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
 */
export interface MapProjectionCameraResult {
  readonly camera: Camera;
}

/**
 * A map's state, as `MapHandle.getSnapshot()` reads it.
 *
 * See `mln_map_snapshot` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface MapSnapshot {
  readonly debugFlags: MapDebugFlag;
  /**
   * Grows with each change to the map's state.
   */
  readonly generation: bigint;
  /**
   * Whether the style and every resource that the view needs have loaded.
   */
  readonly fullyLoaded: boolean;
  readonly renderingStatsViewEnabled: boolean;
  readonly gestureInProgress: boolean;
  readonly transitionInProgress: boolean;
  readonly eventMask: RuntimeEventFlag;
  /**
   * Counts the render updates that the map has made for its render session.
   */
  readonly latestRenderUpdateGeneration: bigint;
  readonly tile: MapTileSettings;
  readonly bounds: CameraBounds;
  readonly freeCamera: FreeCamera;
  readonly view: MapView;
}

/**
 * How the map picks the tiles to load.
 *
 * The `lod_` members lower the level of detail away from the camera, so the map
 * loads fewer tiles when the pitch is high.
 *
 * See `mln_map_tile_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface MapTileOptions {
  /**
   * How many zoom levels lower the map also loads tiles for, so it can show a
   * coarse map while the current tiles load. 0 loads only the current tiles.
   */
  prefetchZoomDelta?: number;
  /**
   * The radius around the view point, in tiles, that keeps full detail.
   */
  lodMinRadius?: number;
  /**
   * Scales the distance to the view point. Above 1 lowers the detail.
   */
  lodScale?: number;
  /**
   * The pitch, in degrees, above which the map lowers the detail.
   */
  lodPitchThreshold?: number;
  /**
   * Zoom levels to add for picking the detail. Below 0 loads fewer tiles.
   */
  lodZoomShift?: number;
  /**
   * `TileLodMode.distance` ignores `lod_min_radius`.
   */
  lodMode?: TileLodMode;
}

/**
 * All the settings for how a map picks the tiles to load.
 *
 * Its members mean the same as those of `MapTileOptions`.
 *
 * See `mln_map_tile_settings` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface MapTileSettings {
  readonly prefetchZoomDelta: number;
  readonly lodMinRadius: number;
  readonly lodScale: number;
  /**
   * The pitch, in degrees, above which the map lowers the detail.
   */
  readonly lodPitchThreshold: number;
  readonly lodZoomShift: number;
  readonly lodMode: TileLodMode;
}

/**
 * One view of a map: its camera, size, and projection.
 *
 * See `mln_map_view` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface MapView {
  camera: Camera;
  extent: LogicalExtent;
  projectionMode: ProjectionMode;
  viewport: MapViewport;
}

/**
 * A map's whole viewport.
 *
 * Its members mean the same as those of `MapViewportOptions`.
 *
 * See `mln_map_viewport` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface MapViewport {
  northOrientation: NorthOrientation;
  constrainMode: ConstrainMode;
  viewportMode: ViewportMode;
  frustumOffset: EdgeInsets;
}

/**
 * How the map orients and constrains its view.
 *
 * See `mln_map_viewport_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface MapViewportOptions {
  northOrientation?: NorthOrientation;
  constrainMode?: ConstrainMode;
  viewportMode?: ViewportMode;
  frustumOffset?: EdgeInsets;
}

/**
 * A Metal texture of a borrowed ring.
 *
 * See `mln_metal_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface MetalBorrowedTexture {
  /**
   * Your MTLTexture.
   */
  texture: bigint;
}

/**
 * A ring of your Metal textures, for `MapHandle.attachMetalBorrowedTexture()`.
 *
 * `logical_size` sets the map's size. `physical_width` and `physical_height`
 * give the textures' size in pixels, which is usually `logical_size` times the
 * map's scale factor. The textures must all have that size, one device, one
 * pixel format, one sample, and MTLTextureUsageRenderTarget.
 *
 * The session uses the textures until a replacement or release of the target
 * completes, a detach or release of the session completes, or an abandon of it
 * returns.
 *
 * See `mln_metal_borrowed_texture_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface MetalBorrowedTextureDescriptor {
  logicalSize: LogicalSize;
  physicalWidth: number;
  physicalHeight: number;
  textures: readonly MetalBorrowedTexture[];
  /**
   * Your MTLCommandQueue on the textures' device, or null for a queue of the
   * session's own. With your queue, frames can use `GpuSyncKind.queueOrder`.
   */
  commandQueue?: bigint;
}

/**
 * New Metal textures for a borrowed ring, for
 * `RenderSessionHandle.setMetalBorrowedTextureTarget()`.
 *
 * The members mean what they mean in `MetalBorrowedTextureDescriptor`, and the
 * textures follow its rules. The ring keeps its depth, device, and command
 * queue, so give it as many textures as it attached with, on the same device.
 * The session uses the textures until a replacement or release of the target
 * completes, a detach or release of the session completes, or an abandon of it
 * returns.
 *
 * See `mln_metal_borrowed_texture_replacement` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface MetalBorrowedTextureReplacement {
  logicalSize: LogicalSize;
  physicalWidth: number;
  physicalHeight: number;
  textures: readonly MetalBorrowedTexture[];
}

/**
 * The Metal device that a render session uses.
 *
 * See `mln_metal_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface MetalContextDescriptor {
  /**
   * An `MTLDevice`.
   */
  device?: bigint;
}

/**
 * A Metal layer to render to, for `MapHandle.attachMetalSurface()`.
 *
 * The session sets the layer's device, pixel format, and drawable size. It uses
 * the layer until a replacement or release of the target completes, a detach or
 * disposal of the session completes, or an abandon of it returns.
 *
 * See `mln_metal_surface_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
export interface MetalSurfaceDescriptor {
  logicalSize: LogicalSize;
  /**
   * The MTLDevice to render with. A null device picks the system's default.
   */
  context?: MetalContextDescriptor;
  /**
   * Your CAMetalLayer.
   */
  layer: bigint;
}

/**
 * A new Metal layer for `RenderSessionHandle.setMetalSurfaceTarget()`.
 *
 * The session renders to it with the device that it attached with. It uses the
 * layer until a replacement or release of the target completes, a detach or
 * release of the session completes, or an abandon of it returns.
 *
 * See `mln_metal_surface_replacement` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
export interface MetalSurfaceReplacement {
  logicalSize: LogicalSize;
  /**
   * Your CAMetalLayer.
   */
  layer: bigint;
}

/**
 * The Metal texture of an acquired frame.
 *
 * See `mln_metal_texture_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface MetalTextureFrame {
  readonly width: number;
  readonly height: number;
  readonly scaleFactor: number;
  /**
   * The texture's index in the descriptor's `textures`.
   */
  readonly slot: number;
  /**
   * The MTLTexture.
   */
  readonly texture: bigint;
  /**
   * The texture's MTLDevice.
   */
  readonly device: bigint;
  /**
   * The texture's MTLPixelFormat.
   */
  readonly pixelFormat: bigint;
}

/**
 * Whether the library may use the network.
 *
 * See `mln_network_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const NetworkStatus = {
  online: 1,
  offline: 2,
} as const;
/** One of the NetworkStatus values, or a value this binding does not know. */
export type NetworkStatus =
  | (typeof NetworkStatus)[keyof typeof NetworkStatus]
  | (number & {});

/**
 * The screen direction that north points to when the bearing is 0.
 *
 * See `mln_north_orientation` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const NorthOrientation = {
  up: 0,
  right: 1,
  down: 2,
  left: 3,
} as const;
/** One of the NorthOrientation values, or a value this binding does not know. */
export type NorthOrientation =
  | (typeof NorthOrientation)[keyof typeof NorthOrientation]
  | (number & {});

/**
 * An offline region that covers a GeoJSON geometry.
 *
 * The members other than `geometry` mean the same as those of
 * `OfflineTilePyramidRegionDefinition`.
 *
 * See `mln_offline_geometry_region_definition` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface OfflineGeometryRegionDefinition {
  styleUrl: string;
  geometry?: unknown;
  minZoom: number;
  maxZoom: number;
  pixelRatio: number;
  includeIdeographs: boolean;
}

/**
 * The area, zoom range, and style that an offline region covers.
 *
 * See `mln_offline_region_definition` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export type OfflineRegionDefinition =
  | {
      readonly kind: "tilePyramid";
      readonly value: OfflineTilePyramidRegionDefinition;
    }
  | {
      readonly kind: "geometry";
      readonly value: OfflineGeometryRegionDefinition;
    }
  | {
      readonly kind: "unknown";
      readonly tag: number;
      readonly bytes: Uint8Array;
    };

/**
 * Whether an offline region is downloading.
 *
 * See `mln_offline_region_download_state` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const OfflineRegionDownloadState = {
  inactive: 0,
  active: 1,
} as const;
/** One of the OfflineRegionDownloadState values, or a value this binding does not know. */
export type OfflineRegionDownloadState =
  | (typeof OfflineRegionDownloadState)[keyof typeof OfflineRegionDownloadState]
  | (number & {});

/**
 * An offline region in the runtime's cache file.
 *
 * See `mln_offline_region_info` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface OfflineRegionInfo {
  readonly id: bigint;
  readonly definition: OfflineRegionDefinition;
  readonly metadata: Uint8Array;
}

/**
 * The download progress of an offline region.
 *
 * See `mln_offline_region_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface OfflineRegionStatus {
  readonly downloadState: OfflineRegionDownloadState;
  /**
   * The resources downloaded so far, tiles included.
   */
  readonly completedResourceCount: bigint;
  /**
   * The size of the downloaded resources in bytes, tiles included.
   */
  readonly completedResourceSize: bigint;
  readonly completedTileCount: bigint;
  readonly requiredTileCount: bigint;
  /**
   * The size of the downloaded tiles in bytes.
   */
  readonly completedTileSize: bigint;
  /**
   * The resources that the region needs, tiles included. Until
   * `required_resource_count_is_precise` is true, this is only a lower bound.
   */
  readonly requiredResourceCount: bigint;
  readonly requiredResourceCountIsPrecise: boolean;
  /**
   * Whether the region has downloaded every resource it needs.
   */
  readonly complete: boolean;
}

/**
 * An offline region that covers a rectangle of the map.
 *
 * See `mln_offline_tile_pyramid_region_definition` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface OfflineTilePyramidRegionDefinition {
  /**
   * The style whose resources the region holds. Maps use the region when they
   * load the same style URL.
   */
  styleUrl: string;
  bounds: LatLngBounds;
  minZoom: number;
  /**
   * INFINITY covers every zoom level that the style's sources have.
   */
  maxZoom: number;
  /**
   * The pixel ratio of the images to download, such as 2 for dense screens.
   */
  pixelRatio: number;
  /**
   * Whether the region holds the glyphs of CJK ideographs.
   */
  includeIdeographs: boolean;
}

/**
 * The image that `MapHandle.attachOffscreen()` renders. The session renders it
 * at the map's scale factor.
 *
 * See `mln_offscreen_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
 */
export interface OffscreenDescriptor {
  logicalSize: LogicalSize;
}

/**
 * Options for `OffscreenDeviceHandle.create()`.
 *
 * See `mln_offscreen_device_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
 */
export interface OffscreenDeviceOptions {
  /**
   * The GPU to use, as an index into the platform's list of GPUs.
   */
  adapterIndex?: number;
  /**
   * Whether the device's sessions take turns in one OpenGL context instead of
   * having one each. Only an EGL device reads it.
   */
  singleContext?: boolean;
  /**
   * The OpenGL API that the device's sessions render with. An EGL device
   * defaults to `OpenGlClientApi.gles`, and takes `OpenGlClientApi.gl` too on
   * Linux and FreeBSD. A WGL device takes only `OpenGlClientApi.gl`, its
   * default, and a WebGL device only `OpenGlClientApi.gles`, its default. Other
   * devices take neither.
   */
  clientApi?: OpenGlClientApi;
}

/**
 * An OpenHarmony window to present to, for `MapHandle.attachOhosSurface()`.
 *
 * Name the window by `window` or by `surface_id`. The session takes its own
 * reference to the window, so you can release yours once the call returns. It
 * uses the window until a replacement or release of the target completes, a
 * detach or release of the session completes, or an abandon of it returns.
 *
 * See `mln_ohos_surface_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/ohos_8h.html).
 */
export interface OhosSurfaceDescriptor {
  logicalSize: LogicalSize;
  /**
   * Your `OHNativeWindow*`, or null to name the window by `surface_id`.
   */
  window?: bigint;
  /**
   * The surface ID of a surface in this process, such as the one that
   * `XComponentController.getXComponentSurfaceId()` reports, or 0 to name the
   * window by `window`.
   */
  surfaceId?: bigint;
}

/**
 * A new OpenHarmony window for `RenderSessionHandle.setOhosSurfaceTarget()`.
 *
 * Name the window by `window` or by `surface_id`. The session takes its own
 * reference to the window, so you can release yours once the call returns. It
 * uses the window until a replacement or release of the target completes, a
 * detach or release of the session completes, or an abandon of it returns.
 *
 * See `mln_ohos_surface_replacement` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/ohos_8h.html).
 */
export interface OhosSurfaceReplacement {
  logicalSize: LogicalSize;
  /**
   * Your `OHNativeWindow*`, or null to name the window by `surface_id`.
   */
  window?: bigint;
  /**
   * The surface ID of a surface in this process, such as the one that
   * `XComponentController.getXComponentSurfaceId()` reports, or 0 to name the
   * window by `window`.
   */
  surfaceId?: bigint;
}

/**
 * An OpenGL texture of a borrowed ring.
 *
 * See `mln_open_gl_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface OpenGlBorrowedTexture {
  /**
   * Your texture name.
   */
  texture: number;
}

/**
 * A ring of your OpenGL textures, for
 * `MapHandle.attachOpenGlBorrowedTexture()`.
 *
 * `logical_size` sets the map's size. `physical_width` and `physical_height`
 * give the textures' size in pixels, which is usually `logical_size` times the
 * map's scale factor. The context must use `OpenGlContextOwnership.shared`.
 * Each texture must be in your context's share group, with storage of that
 * size.
 *
 * The session uses the textures until a replacement or release of the target
 * completes, a detach or release of the session completes, or an abandon of it
 * returns.
 *
 * See `mln_open_gl_borrowed_texture_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface OpenGlBorrowedTextureDescriptor {
  logicalSize: LogicalSize;
  physicalWidth: number;
  physicalHeight: number;
  context: OpenGlContextDescriptor;
  textures: readonly OpenGlBorrowedTexture[];
  /**
   * The textures' target. It must be GL_TEXTURE_2D.
   */
  target: number;
}

/**
 * New OpenGL textures for a borrowed ring, for
 * `RenderSessionHandle.setOpenGlBorrowedTextureTarget()`.
 *
 * The members mean what they mean in `OpenGlBorrowedTextureDescriptor`, and the
 * textures follow its rules. The ring keeps its depth, context, and texture
 * target, so give it as many textures as it attached with, in the same share
 * group. The session uses the textures until a replacement or release of the
 * target completes, a detach or release of the session completes, or an abandon
 * of it returns.
 *
 * See `mln_open_gl_borrowed_texture_replacement` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface OpenGlBorrowedTextureReplacement {
  logicalSize: LogicalSize;
  physicalWidth: number;
  physicalHeight: number;
  textures: readonly OpenGlBorrowedTexture[];
}

/**
 * The OpenGL API that a context renders with.
 *
 * See `mln_open_gl_client_api` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const OpenGlClientApi = {
  unspecified: 0,
  gl: 1,
  gles: 2,
} as const;
/** One of the OpenGlClientApi values, or a value this binding does not know. */
export type OpenGlClientApi =
  | (typeof OpenGlClientApi)[keyof typeof OpenGlClientApi]
  | (number & {});

/**
 * How a render session gets the OpenGL context that it renders with.
 *
 * Keep the objects that it names alive until a detach or disposal of the
 * session completes, or an abandon of it returns.
 *
 * See `mln_open_gl_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface OpenGlContextDescriptor {
  ownership?: OpenGlContextOwnership;
  data: OpenGlContextDescriptorData;
}

/**
 * The WGL, EGL, or WebGL part of an OpenGL context descriptor.
 *
 * See `mln_open_gl_context_descriptor_data` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export type OpenGlContextDescriptorData =
  | { readonly kind: "wgl"; readonly value: WglContextDescriptor }
  | { readonly kind: "egl"; readonly value: EglContextDescriptor }
  | { readonly kind: "webGl"; readonly value: WebGlContextDescriptor };

/**
 * Whether a render session's OpenGL context shares objects with yours.
 *
 * See `mln_open_gl_context_ownership` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const OpenGlContextOwnership = {
  /**
   * The session's context shares objects with your `share_context`.
   */
  shared: 0,
  /**
   * The session's context shares objects with no other context.
   */
  dedicated: 1,
} as const;
/** One of the OpenGlContextOwnership values, or a value this binding does not know. */
export type OpenGlContextOwnership =
  | (typeof OpenGlContextOwnership)[keyof typeof OpenGlContextOwnership]
  | (number & {});

/**
 * An OpenGL context platform, as a bit in a set.
 *
 * See `mln_open_gl_context_provider_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const OpenGlContextProviderFlag = {
  wgl: 1,
  egl: 2,
  webGl: 4,
} as const;
/** A bitwise OR of OpenGlContextProviderFlag flags. */
export type OpenGlContextProviderFlag = number;

/**
 * An OpenGL surface to present to, for `MapHandle.attachOpenGlSurface()`.
 *
 * The session uses the surface until a replacement or release of the target
 * completes, a detach or release of the session completes, or an abandon of it
 * returns.
 *
 * See `mln_open_gl_surface_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
export interface OpenGlSurfaceDescriptor {
  logicalSize: LogicalSize;
  context: OpenGlContextDescriptor;
  /**
   * Your EGLSurface with EGL, or your HDC with WGL. WebGL doesn't use it.
   */
  surface?: bigint;
}

/**
 * A new OpenGL surface for `RenderSessionHandle.setOpenGlSurfaceTarget()`.
 *
 * The session presents to it with the context that it attached with. It uses
 * the surface until a replacement or release of the target completes, a detach
 * or disposal of the session completes, or an abandon of it returns.
 *
 * See `mln_open_gl_surface_replacement` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
export interface OpenGlSurfaceReplacement {
  logicalSize: LogicalSize;
  /**
   * Your EGLSurface with EGL, or your HDC with WGL. WebGL doesn't use it, so
   * leave it null.
   */
  surface?: bigint;
}

/**
 * The OpenGL texture of an acquired frame.
 *
 * See `mln_open_gl_texture_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface OpenGlTextureFrame {
  readonly width: number;
  readonly height: number;
  readonly scaleFactor: number;
  /**
   * The texture's index in the descriptor's `textures`.
   */
  readonly slot: number;
  readonly texture: number;
  readonly target: number;
}

/**
 * An image of 8-bit RGBA pixels whose colors are multiplied by their alpha.
 *
 * See `mln_premultiplied_rgba8_image` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface PremultipliedRgba8Image {
  width: number;
  height: number;
  /**
   * The number of bytes from the start of one row to the start of the next, at
   * least `width` &#42; 4.
   */
  stride: number;
  /**
   * The rows of pixels, `stride` bytes apart, from the top row down.
   */
  pixels: Uint8Array;
}

/**
 * A point in the Web Mercator projection, in meters.
 *
 * See `mln_projected_meters` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface ProjectedMeters {
  northing: number;
  easting: number;
}

/**
 * A map's whole projection mode.
 *
 * Its members mean the same as those of `ProjectionModeOptions`.
 *
 * See `mln_projection_mode` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface ProjectionMode {
  axonometric: boolean;
  xSkew: number;
  ySkew: number;
}

/**
 * Settings that skew the map or draw it without perspective.
 *
 * See `mln_projection_mode_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface ProjectionModeOptions {
  /**
   * Whether the map draws without perspective.
   */
  axonometric?: boolean;
  xSkew?: number;
  ySkew?: number;
}

/**
 * A rotation, as a quaternion.
 *
 * See `mln_quaternion` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface Quaternion {
  x: number;
  y: number;
  z: number;
  w: number;
}

/**
 * Your lock on a Vulkan queue that you and a render session both submit to.
 *
 * The session holds the lock only while it submits or presents to the queue.
 * Set both `lock` and `unlock`, or neither when you don't submit to the queue
 * yourself.
 *
 * It can run on a library thread, and it can't call the library.
 *
 * See `mln_queue_lock` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export type QueueLock = NativeRegistration<"QueueLock">;
export const QueueLock = {
  /**
   * Creates a native registration from a native function pointer for
   * each callback, and an optional context and release. The binding
   * copies them into the C descriptor unchanged. The first call that
   * carries the registration consumes it: native code releases the
   * context of a call that it accepts, and the binding releases it for
   * any other call. A later call throws a `MaplibreError` with the
   * invalid-state status.
   */
  native(fields: NativeQueueLockFields): NativeRegistration<"QueueLock"> {
    return nativeRegistration("QueueLock", {
      lock: fields.lock,
      unlock: fields.unlock,
      userData: fields.userData ?? 0n,
      releaseUserData: fields.releaseUserData ?? 0n,
    });
  },
};

/** The fields of a native {@link QueueLock}, as native addresses. */
export interface NativeQueueLockFields {
  readonly lock: bigint;
  readonly unlock: bigint;
  readonly userData?: bigint;
  readonly releaseUserData?: bigint;
}

/**
 * What `RenderSessionHandle.abandon()` did with a session's graphics objects.
 *
 * See `mln_render_abandon_disposition` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
export const RenderAbandonDisposition = {
  clean: 0,
  /**
   * It kept some objects that it couldn't destroy safely. They stay allocated
   * until the process exits.
   */
  quarantined: 1,
} as const;
/** One of the RenderAbandonDisposition values, or a value this binding does not know. */
export type RenderAbandonDisposition =
  | (typeof RenderAbandonDisposition)[keyof typeof RenderAbandonDisposition]
  | (number & {});

/**
 * The outcome of `RenderSessionHandle.abandon()`.
 *
 * See `mln_render_abandon_result` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
export interface RenderAbandonResult {
  readonly disposition: RenderAbandonDisposition;
  readonly quarantinedResourceCount: number;
}

/**
 * A render backend, as a bit in a set.
 *
 * See `mln_render_backend_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export const RenderBackendFlag = {
  metal: 1,
  vulkan: 2,
  openGl: 4,
  webGpu: 8,
} as const;
/** A bitwise OR of RenderBackendFlag flags. */
export type RenderBackendFlag = number;

/**
 * The thread that runs a render session's graphics work.
 *
 * See `mln_render_driver_kind` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const RenderDriverKind = {
  /**
   * A library thread.
   */
  coreWorker: 1,
  /**
   * Your thread, when it calls `RenderSessionHandle.serviceDriverWork()`.
   */
  callerGraphicsThread: 2,
} as const;
/** One of the RenderDriverKind values, or a value this binding does not know. */
export type RenderDriverKind =
  | (typeof RenderDriverKind)[keyof typeof RenderDriverKind]
  | (number & {});

/**
 * How one frame demand finished.
 *
 * See `mln_render_frame_result` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
export interface RenderFrameResult {
  readonly disposition: RenderResult;
  readonly token: bigint;
  /**
   * The map update that a rendered frame shows. The frame includes a command's
   * change when this is at least the command's
   * `CommandCompletion.renderUpdateGeneration`.
   */
  readonly mapUpdateGeneration: bigint;
  readonly generation: bigint;
  readonly frameGeneration: bigint;
  readonly needsRepaint: boolean;
  /**
   * The camera and extent that a rendered frame shows.
   */
  readonly view: MapView;
  readonly mode: RenderMode;
}

/**
 * Whether a frame drew the map complete, or lacked resources that are still
 * loading.
 *
 * See `mln_render_mode` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const RenderMode = {
  partial: 0,
  full: 1,
} as const;
/** One of the RenderMode values, or a value this binding does not know. */
export type RenderMode =
  | (typeof RenderMode)[keyof typeof RenderMode]
  | (number & {});

/**
 * How a frame demand finished.
 *
 * See `mln_render_result` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
export const RenderResult = {
  rendered: 0,
  /**
   * The map had nothing new to draw.
   */
  noUpdate: 1,
  /**
   * The target couldn't take a frame, such as while a window is hidden or after
   * `RenderSessionHandle.releaseTarget()`. Demand a frame again later.
   */
  targetNotReady: 2,
  /**
   * A later demand with the same flags replaced this one.
   */
  superseded: 3,
  /**
   * The session failed, and `RenderSessionHandle.getFailure()` reports why.
   * Once `RenderSessionHandle.release()` completes, you can attach a new
   * session.
   */
  failed: 4,
} as const;
/** One of the RenderResult values, or a value this binding does not know. */
export type RenderResult =
  | (typeof RenderResult)[keyof typeof RenderResult]
  | (number & {});

/**
 * Options for attaching a render session to a map.
 *
 * See `mln_render_session_attach_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface RenderSessionAttachOptions {
  /**
   * The thread that runs the session's graphics work. Whoever owns the target's
   * graphics context drives it:
   *
   * - `RenderDriverKind.callerGraphicsThread` for an OpenGL or WebGPU target
   *   that you pass, since its context belongs to your thread: a shared or
   *   dedicated EGL or WGL context, an existing WebGL context, or a browser's
   *   WebGPU device.
   * - `RenderDriverKind.coreWorker` for a target whose context the library
   *   owns: a transferred WebGL canvas, an Android or OpenHarmony window, or an
   *   offscreen session.
   * - Either one for a Metal or Vulkan target.
   *
   * An attach with another driver fails with `Status.unsupported`.
   */
  driver: RenderDriverKind;
  /**
   * The kinds of `GpuSync` that you can wait on before you read an acquired
   * frame. The session uses one of them that its target supports. Zero lets the
   * session pick a kind that names no object to wait on.
   */
  producerSyncKinds?: GpuSyncKindFlag;
  /**
   * Called when frame results are waiting for
   * `RenderSessionHandle.drainFrameResults()`. It isn't called again until a
   * drain takes them.
   */
  frameWake?: Wake;
  /**
   * Called when the session has something new to draw, because the map or the
   * target changed since its last frame, or the target couldn't take that
   * frame. Demand a frame at your next display refresh. It isn't called again
   * until you demand a frame, and isn't called while the session has no target.
   */
  redrawWake?: Wake;
  /**
   * Called when a session driven by `RenderDriverKind.callerGraphicsThread` has
   * work for `RenderSessionHandle.serviceDriverWork()`. It isn't called again
   * until a service runs.
   */
  driverWorkWake?: Wake;
  /**
   * Your lock on the Vulkan queue, when you submit to it yourself.
   */
  queueLock?: QueueLock;
}

/**
 * What an attached render session supports.
 *
 * See `mln_render_session_capabilities` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface RenderSessionCapabilities {
  readonly driver: RenderDriverKind;
  /**
   * How many textures the session's texture ring has.
   */
  readonly textureRingDepth: number;
  readonly flags: RenderSessionCapabilityFlag;
  /**
   * The kind of `GpuSync` that `AcquiredFrameHandle.withProducerSync()`
   * reports.
   */
  readonly producerSyncKind: GpuSyncKind;
  /**
   * The kinds of `GpuSync` that `AcquiredFrameHandle.release()` accepts.
   *
   * They depend on the target and on what the device supports, so read this
   * field instead of assuming a kind.
   */
  readonly consumerSyncKinds: GpuSyncKindFlag;
}

/**
 * Something that a render session can do.
 *
 * See `mln_render_session_capability_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const RenderSessionCapabilityFlag = {
  /**
   * `RenderSessionHandle.acquireFrame()` can take the frames it renders.
   */
  frameAcquisition: 1,
  /**
   * `RenderSessionHandle.readTexture()` can copy its texture to memory.
   */
  readback: 2,
  /**
   * It shows frames on a surface.
   */
  presentation: 4,
} as const;
/** A bitwise OR of RenderSessionCapabilityFlag flags. */
export type RenderSessionCapabilityFlag = number;

/**
 * The state of a render session at one moment.
 *
 * See `mln_render_session_snapshot` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
export interface RenderSessionSnapshot {
  readonly state: RenderSessionState;
  /**
   * The target's logical size, and the map's scale factor.
   */
  readonly extent: LogicalExtent;
  readonly generation: bigint;
  readonly frameGeneration: bigint;
  readonly acquiredFrameCount: number;
  /**
   * `Status.ok` until the session fails. Then `Status.targetLost` when it
   * failed because it lost its target, or `Status.nativeError` when it failed
   * for another reason.
   */
  readonly failureStatus: Status;
  /**
   * Whether the session is attached and has a target to render to. It's false
   * from when `RenderSessionHandle.releaseTarget()` completes until you give
   * the session a new target.
   */
  readonly hasTarget: boolean;
}

/**
 * Where a render session is in its life.
 *
 * See `mln_render_session_state` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
export const RenderSessionState = {
  attaching: 1,
  attached: 2,
  detaching: 3,
  detached: 4,
  failed: 5,
  abandoned: 6,
} as const;
/** One of the RenderSessionState values, or a value this binding does not know. */
export type RenderSessionState =
  | (typeof RenderSessionState)[keyof typeof RenderSessionState]
  | (number & {});

/**
 * A feature that a rendered feature query found.
 *
 * See `mln_rendered_feature` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
export interface RenderedFeature {
  /**
   * The feature, as a GeoJSON feature.
   */
  readonly feature: unknown;
  /**
   * The layer that drew the feature.
   */
  readonly layerId: string;
  readonly sourceId: string;
  /**
   * The feature's source layer, or empty when its source has none.
   */
  readonly sourceLayerId: string;
  /**
   * The feature's state, as a JSON object.
   */
  readonly state: unknown;
}

/**
 * Options for a rendered feature query.
 *
 * See `mln_rendered_feature_query_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
export interface RenderedFeatureQueryOptions {
  /**
   * The layers to query. Without them, the query covers every layer.
   */
  layerIds?: readonly string[];
  /**
   * A &#91;filter&#93;(https://maplibre.org/maplibre-style-spec/expressions/)
   * that each feature must match. Empty matches every feature.
   */
  filter?: unknown;
}

/**
 * The screen area that a rendered feature query covers.
 *
 * See `mln_rendered_query_geometry` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
export type RenderedQueryGeometry =
  | { readonly kind: "point"; readonly value: ScreenPoint }
  | { readonly kind: "box"; readonly value: ScreenBox }
  | { readonly kind: "lineString"; readonly value: ScreenLineString };

/**
 * Statistics about the frames that a map renders.
 *
 * See `mln_rendering_stats` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RenderingStats {
  /**
   * The time that the frame took to encode, in milliseconds.
   */
  readonly encodingTimeMs: number;
  /**
   * The time that the frame took to render, in milliseconds.
   */
  readonly renderingTimeMs: number;
  /**
   * The frames rendered so far.
   */
  readonly frameCount: bigint;
  /**
   * The draw calls in the frame.
   */
  readonly drawCallCount: bigint;
  /**
   * The draw calls in every frame so far.
   */
  readonly totalDrawCallCount: bigint;
}

/**
 * Why a resource request failed.
 *
 * See `mln_resource_error_reason` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const ResourceErrorReason = {
  /**
   * A reason that no other value names.
   */
  other: 0,
  notFound: 1,
  /**
   * The server reported an error.
   */
  server: 2,
  /**
   * The server couldn't be reached.
   */
  connection: 3,
  rateLimit: 4,
} as const;
/** One of the ResourceErrorReason values, or a value this binding does not know. */
export type ResourceErrorReason =
  | (typeof ResourceErrorReason)[keyof typeof ResourceErrorReason]
  | (number & {});

/**
 * The kind of a resource that the library loads.
 *
 * See `mln_resource_kind` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const ResourceKind = {
  unknown: 0,
  style: 1,
  source: 2,
  tile: 3,
  glyphs: 4,
  spriteImage: 5,
  spriteJson: 6,
  image: 7,
} as const;
/** One of the ResourceKind values, or a value this binding does not know. */
export type ResourceKind =
  | (typeof ResourceKind)[keyof typeof ResourceKind]
  | (number & {});

/**
 * A resource kind, as a bit in a set.
 *
 * See `mln_resource_kind_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const ResourceKindFlag = {
  unknown: 1,
  style: 2,
  source: 4,
  tile: 8,
  glyphs: 16,
  spriteImage: 32,
  spriteJson: 64,
  image: 128,
  all: 255,
} as const;
/** A bitwise OR of ResourceKindFlag flags. */
export type ResourceKindFlag = number;

/**
 * Where a request may load its resource from.
 *
 * See `mln_resource_loading_method` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const ResourceLoadingMethod = {
  /**
   * The cache or the network.
   */
  all: 0,
  cacheOnly: 1,
  networkOnly: 2,
} as const;
/** One of the ResourceLoadingMethod values, or a value this binding does not know. */
export type ResourceLoadingMethod =
  | (typeof ResourceLoadingMethod)[keyof typeof ResourceLoadingMethod]
  | (number & {});

/**
 * The priority of a resource request.
 *
 * See `mln_resource_priority` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const ResourcePriority = {
  regular: 0,
  low: 1,
} as const;
/** One of the ResourcePriority values, or a value this binding does not know. */
export type ResourcePriority =
  | (typeof ResourcePriority)[keyof typeof ResourcePriority]
  | (number & {});

/**
 * A resource provider, for `RuntimeHandle.setResourceProvider()`.
 *
 * See `mln_resource_provider` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface ResourceProvider {
  callback: (
    request: ResourceRequestInfo,
    handle: ResourceRequestHandle,
  ) => void;
  /**
   * The requests that the provider serves. Without routes, it serves every
   * network request.
   */
  routes?: readonly ResourceRoute[];
}

/**
 * The bytes from `start` to `end` of a resource, both included.
 *
 * See `mln_resource_range` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface ResourceRange {
  readonly start: bigint;
  readonly end: bigint;
}

/**
 * A cancel callback, for `ResourceRequestHandle.setCancelCallback()`.
 *
 * See `mln_resource_request_cancel_handler` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface ResourceRequestCancelHandler {
  callback: () => void;
}

/**
 * A request that a resource provider serves.
 *
 * When the cache holds an earlier copy of the resource, the `prior_` fields
 * describe it.
 *
 * See `mln_resource_request_info` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface ResourceRequestInfo {
  /**
   * The URL as the style names it.
   */
  readonly requestedUrl: string;
  /**
   * The URL to load, after the library expands any shorthand in `requested_url`
   * and before any resource transform. Routes match this URL.
   */
  readonly resolvedUrl: string;
  readonly kind: ResourceKind;
  readonly loadingMethod: ResourceLoadingMethod;
  readonly priority: ResourcePriority;
  readonly usage: ResourceUsage;
  readonly storagePolicy: ResourceStoragePolicy;
  /**
   * The only bytes of the resource that the request wants.
   */
  readonly range?: ResourceRange;
  readonly priorModifiedUnixMs?: bigint;
  readonly priorExpiresUnixMs?: bigint;
  /**
   * Empty when the earlier copy has no ETag.
   */
  readonly priorEtag: string;
  readonly priorData: Uint8Array;
}

/**
 * The answer to a resource request.
 *
 * See `mln_resource_response` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface ResourceResponse {
  status?: ResourceResponseStatus;
  /**
   * Why the request failed, for `ResourceResponseStatus.error`.
   */
  errorReason?: ResourceErrorReason;
  /**
   * The resource, for `ResourceResponseStatus.ok`.
   */
  bytes?: Uint8Array;
  /**
   * A description of the error, for `ResourceResponseStatus.error`.
   */
  errorMessage?: string;
  /**
   * Whether the library must request the resource again before it reuses this
   * copy.
   */
  mustRevalidate?: boolean;
  modifiedUnixMs?: bigint;
  expiresUnixMs?: bigint;
  /**
   * Empty when the resource has no ETag.
   */
  etag?: string;
  /**
   * When to retry, for `ResourceResponseStatus.error`.
   */
  retryAfterUnixMs?: bigint;
}

/**
 * The outcome of a resource request.
 *
 * See `mln_resource_response_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const ResourceResponseStatus = {
  ok: 0,
  error: 1,
  /**
   * The resource exists and is empty.
   */
  noContent: 2,
  /**
   * The earlier copy that the request's `prior_` fields describe is current.
   */
  notModified: 3,
} as const;
/** One of the ResourceResponseStatus values, or a value this binding does not know. */
export type ResourceResponseStatus =
  | (typeof ResourceResponseStatus)[keyof typeof ResourceResponseStatus]
  | (number & {});

/**
 * The requests that a resource provider serves.
 *
 * See `mln_resource_route` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface ResourceRoute {
  /**
   * The kinds of resource that the route covers. Zero covers every kind.
   */
  kinds?: ResourceKindFlag;
  flags?: UrlMatchFlag;
  url?: string;
}

/**
 * Whether the library keeps a loaded resource in its cache.
 *
 * See `mln_resource_storage_policy` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const ResourceStoragePolicy = {
  permanent: 0,
  volatile: 1,
} as const;
/** One of the ResourceStoragePolicy values, or a value this binding does not know. */
export type ResourceStoragePolicy =
  | (typeof ResourceStoragePolicy)[keyof typeof ResourceStoragePolicy]
  | (number & {});

/**
 * How to rewrite request URLs, for `RuntimeHandle.setResourceTransform()`.
 *
 * Set either `callback` or `rules`, not both.
 *
 * See `mln_resource_transform` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface ResourceTransform {
  callback?: (
    kind: ResourceKind,
    url: string,
    request: ResourceTransformRequestHandle,
  ) => void;
  /**
   * The first rule that matches a request adds its parameters, and no other. A
   * request that no rule matches keeps its URL. Rules never fail a request.
   */
  rules?: readonly ResourceTransformRule[];
}

/**
 * Why a resource transform fails a request.
 *
 * See `mln_resource_transform_error` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface ResourceTransformError {
  reason?: ResourceErrorReason;
  message?: string;
  /**
   * When to retry, for `ResourceErrorReason.rateLimit`.
   */
  retryAfterUnixMs?: bigint;
}

/**
 * Query parameters to add to the URLs of requests that match a pattern.
 *
 * The library adds the parameters, in order, at the end of the URL's query and
 * before any fragment. It writes `?` before them when the URL has no `?`, or
 * `&` when the text before them ends with neither `?` nor `&`. It keeps the
 * parameters that the URL already has, even one with the same name.
 *
 * See `mln_resource_transform_rule` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface ResourceTransformRule {
  /**
   * The kinds of resource that the rule covers. Zero covers every kind.
   */
  kinds?: ResourceKindFlag;
  flags?: UrlMatchFlag;
  url?: string;
  parameters?: readonly UrlQueryParameter[];
}

/**
 * Whether a request loads a resource for a map or an offline region.
 *
 * See `mln_resource_usage` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const ResourceUsage = {
  online: 0,
  offline: 1,
} as const;
/** One of the ResourceUsage values, or a value this binding does not know. */
export type ResourceUsage =
  | (typeof ResourceUsage)[keyof typeof ResourceUsage]
  | (number & {});

/**
 * An event that a runtime or one of its maps raised.
 *
 * See `mln_runtime_event` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeEvent {
  readonly type: RuntimeEventType;
  readonly sourceType: RuntimeEventSourceType;
  /**
   * The runtime or map that raised the event, as `source_type` says.
   */
  readonly source: bigint;
  /**
   * For a map event, a map snapshot with this generation or a later one
   * includes what the event reports. Zero for a runtime event.
   */
  readonly generation: bigint;
  /**
   * The error's text for an event that reports a failure, or empty.
   */
  readonly message: string;
  readonly payload: RuntimeEventPayload;
}

/**
 * The details of `RuntimeEventType.mapCameraWillChange` and
 * `RuntimeEventType.mapCameraDidChange`.
 *
 * See `mln_runtime_event_camera_change` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeEventCameraChange {
  readonly mode: CameraChangeMode;
}

/**
 * An event type, as a bit in a set.
 *
 * See `mln_runtime_event_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const RuntimeEventFlag = {
  none: 0n,
  mapCameraWillChange: 2n,
  mapCameraIsChanging: 4n,
  mapCameraDidChange: 8n,
  mapStyleLoaded: 16n,
  mapLoadingStarted: 32n,
  mapLoadingFinished: 64n,
  mapLoadingFailed: 128n,
  mapIdle: 256n,
  mapRenderError: 512n,
  mapRenderFrameStarted: 1024n,
  mapRenderFrameFinished: 2048n,
  mapRenderMapStarted: 4096n,
  mapRenderMapFinished: 8192n,
  mapStyleImageMissing: 16384n,
  mapTileAction: 32768n,
  mapSourceChanged: 65536n,
  offlineRegionStatusChanged: 131072n,
  offlineRegionResponseError: 262144n,
  offlineRegionTileCountLimitExceeded: 524288n,
  mapGlyphsRequested: 1048576n,
  mapGlyphsLoaded: 2097152n,
  mapGlyphsError: 4194304n,
  mapSpriteRequested: 8388608n,
  mapSpriteLoaded: 16777216n,
  mapSpriteError: 33554432n,
  allMapEvents: 66191358n,
  allRuntimeEvents: 917504n,
  all: 67108862n,
} as const;
/** A bitwise OR of RuntimeEventFlag flags. */
export type RuntimeEventFlag = bigint;

/**
 * The details of a glyph event.
 *
 * The event covers the glyphs from `first` to `last`, both included.
 *
 * See `mln_runtime_event_glyphs` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeEventGlyphs {
  /**
   * The fonts that the glyphs come from, in order of preference.
   */
  readonly fontStack: readonly string[];
  readonly first: number;
  readonly last: number;
  readonly fontFace: boolean;
}

/**
 * An event callback, for `RuntimeHandle.setEventHandler()`.
 *
 * See `mln_runtime_event_handler` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeEventHandler {
  callback: (events: readonly RuntimeEvent[]) => void;
}

/**
 * The details of `RuntimeEventType.mapLoadingFailed`.
 *
 * See `mln_runtime_event_loading_failed` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeEventLoadingFailed {
  readonly error: MapLoadError;
}

/**
 * The details of `RuntimeEventType.offlineRegionResponseError`.
 *
 * See `mln_runtime_event_offline_region_response_error` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeEventOfflineRegionResponseError {
  readonly regionId: bigint;
  readonly reason: ResourceErrorReason;
}

/**
 * The details of `RuntimeEventType.offlineRegionStatusChanged`.
 *
 * See `mln_runtime_event_offline_region_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeEventOfflineRegionStatus {
  readonly regionId: bigint;
  readonly status: OfflineRegionStatus;
}

/**
 * The details of `RuntimeEventType.offlineRegionTileCountLimitExceeded`.
 *
 * See `mln_runtime_event_offline_region_tile_count_limit` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeEventOfflineRegionTileCountLimit {
  readonly regionId: bigint;
  /**
   * The number of demo server tiles that the cache file's offline regions may
   * hold together.
   */
  readonly limit: bigint;
}

/**
 * The details of an event, in the member that its `payload_type` names.
 *
 * See `mln_runtime_event_payload` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export type RuntimeEventPayload =
  | { readonly kind: "cameraChange"; readonly value: RuntimeEventCameraChange }
  | {
      readonly kind: "loadingFailed";
      readonly value: RuntimeEventLoadingFailed;
    }
  | { readonly kind: "renderFrame"; readonly value: RuntimeEventRenderFrame }
  | { readonly kind: "styleImage"; readonly value: RuntimeEventStyleImage }
  | { readonly kind: "tileAction"; readonly value: RuntimeEventTileAction }
  | {
      readonly kind: "sourceChanged";
      readonly value: RuntimeEventSourceChanged;
    }
  | {
      readonly kind: "offlineRegionStatus";
      readonly value: RuntimeEventOfflineRegionStatus;
    }
  | {
      readonly kind: "offlineRegionResponseError";
      readonly value: RuntimeEventOfflineRegionResponseError;
    }
  | {
      readonly kind: "offlineRegionTileCountLimit";
      readonly value: RuntimeEventOfflineRegionTileCountLimit;
    }
  | { readonly kind: "glyphs"; readonly value: RuntimeEventGlyphs }
  | { readonly kind: "sprite"; readonly value: RuntimeEventSprite }
  | { readonly kind: "none" }
  | {
      readonly kind: "unknown";
      readonly tag: number;
      readonly bytes: Uint8Array;
    };

/**
 * The details of `RuntimeEventType.mapRenderFrameFinished`.
 *
 * See `mln_runtime_event_render_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeEventRenderFrame {
  readonly mode: RenderMode;
  readonly needsRepaint: boolean;
  readonly placementChanged: boolean;
  readonly stats: RenderingStats;
}

/**
 * The details of `RuntimeEventType.mapSourceChanged`.
 *
 * See `mln_runtime_event_source_changed` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeEventSourceChanged {
  readonly sourceId: string;
}

/**
 * The kind of handle that raised an event.
 *
 * See `mln_runtime_event_source_type` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const RuntimeEventSourceType = {
  runtime: 0,
  map: 1,
} as const;
/** One of the RuntimeEventSourceType values, or a value this binding does not know. */
export type RuntimeEventSourceType =
  | (typeof RuntimeEventSourceType)[keyof typeof RuntimeEventSourceType]
  | (number & {});

/**
 * The details of a sprite event.
 *
 * `id` and `url` are both empty when the style has no sprite.
 *
 * See `mln_runtime_event_sprite` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeEventSprite {
  readonly id: string;
  readonly url: string;
}

/**
 * The details of `RuntimeEventType.mapStyleImageMissing`.
 *
 * See `mln_runtime_event_style_image` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeEventStyleImage {
  readonly imageId: string;
}

/**
 * The details of `RuntimeEventType.mapTileAction`.
 *
 * See `mln_runtime_event_tile_action` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeEventTileAction {
  /**
   * The source that the tile belongs to.
   */
  readonly sourceId: string;
  readonly operation: TileOperation;
  readonly tileId: TileId;
}

/**
 * The kind of an event.
 *
 * An event's `payload_type` names the member of its payload that holds the
 * event's details. An event that reports a failure carries the error's text in
 * `message`.
 *
 * Only maps in `MapMode.continuous` raise
 * `RuntimeEventType.mapLoadingFinished`, `RuntimeEventType.mapIdle`, and the
 * render frame and render map events. Offline region events come only for the
 * regions and types that `RuntimeHandle.setOfflineRegionObserved()` selects.
 *
 * See `mln_runtime_event_type` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const RuntimeEventType = {
  /**
   * The camera is about to change. Its details are in `camera_change`.
   */
  mapCameraWillChange: 1,
  mapCameraIsChanging: 2,
  /**
   * The camera changed. Its details are in `camera_change`.
   */
  mapCameraDidChange: 3,
  mapStyleLoaded: 4,
  /**
   * The map started to load a style.
   */
  mapLoadingStarted: 5,
  /**
   * The map loaded its style and everything that the current view needs, and
   * rendered the result.
   */
  mapLoadingFinished: 6,
  /**
   * The map failed to load a style. Its details are in `loading_failed`, and
   * the error's text is in `message`.
   */
  mapLoadingFailed: 7,
  /**
   * The map rendered a complete frame and has nothing left to load, animate, or
   * redraw.
   */
  mapIdle: 8,
  /**
   * The map failed to render. The error's text is in `message`.
   */
  mapRenderError: 9,
  mapRenderFrameStarted: 10,
  /**
   * The map rendered a frame. Its details are in `render_frame`.
   */
  mapRenderFrameFinished: 11,
  mapRenderMapStarted: 12,
  mapRenderMapFinished: 13,
  /**
   * The style uses an image that it doesn't have. Its details are in
   * `style_image`.
   */
  mapStyleImageMissing: 14,
  /**
   * A tile moved to another step of loading. Its details are in `tile_action`.
   */
  mapTileAction: 15,
  /**
   * A source changed. Its details are in `source_changed`.
   */
  mapSourceChanged: 16,
  /**
   * The region's status changed. Its details are in `offline_region_status`.
   */
  offlineRegionStatusChanged: 17,
  /**
   * A resource that the region needs failed to download. Its details are in
   * `offline_region_response_error`, and the error's text is in `message`.
   */
  offlineRegionResponseError: 18,
  /**
   * The region's download became inactive because the cache file's offline
   * regions reached their limit of 6000 tiles from MapLibre's demo tile server.
   * Its details are in `offline_region_tile_count_limit`.
   *
   * Only tiles of sources on the demo server count toward the limit: sources
   * with a `maplibre://` URL, which resolves to https://demotiles.maplibre.org.
   * Tiles from other servers never count.
   */
  offlineRegionTileCountLimitExceeded: 19,
  /**
   * The map requested a range of glyphs. Its details are in `glyphs`.
   */
  mapGlyphsRequested: 20,
  /**
   * A range of glyphs loaded. Its details are in `glyphs`.
   */
  mapGlyphsLoaded: 21,
  /**
   * A range of glyphs failed to load. Its details are in `glyphs`, and the
   * error's text is in `message`.
   */
  mapGlyphsError: 22,
  /**
   * The map requested a sprite. Its details are in `sprite`.
   */
  mapSpriteRequested: 23,
  /**
   * A sprite loaded. Its details are in `sprite`.
   */
  mapSpriteLoaded: 24,
  /**
   * A sprite failed to load. Its details are in `sprite`, and the error's text
   * is in `message`.
   */
  mapSpriteError: 25,
} as const;
/** One of the RuntimeEventType values, or a value this binding does not know. */
export type RuntimeEventType =
  | (typeof RuntimeEventType)[keyof typeof RuntimeEventType]
  | (number & {});

/**
 * Options for `RuntimeHandle.create()`.
 *
 * See `mln_runtime_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeOptions {
  /**
   * The directory that `asset://` URLs load from. Empty uses the working
   * directory, or the app's assets on Android.
   */
  assetPath?: string;
  /**
   * The cache database file, which holds the ambient cache and offline regions.
   * Empty keeps them in memory.
   */
  cachePath?: string;
}

/**
 * A rectangle of the screen, from its `min` corner to its `max` corner.
 *
 * See `mln_screen_box` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
export interface ScreenBox {
  min?: ScreenPoint;
  max?: ScreenPoint;
}

/**
 * A line through points on the screen.
 *
 * See `mln_screen_line_string` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
export interface ScreenLineString {
  points?: readonly ScreenPoint[];
}

/**
 * A point on the screen, in logical pixels from the map's top-left corner.
 *
 * See `mln_screen_point` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface ScreenPoint {
  x: number;
  y: number;
}

/**
 * A feature that a source feature query found.
 *
 * See `mln_source_feature` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
export interface SourceFeature {
  /**
   * The feature, as a GeoJSON feature.
   */
  readonly feature: unknown;
  /**
   * The feature's source layer, or empty when its source has none.
   */
  readonly sourceLayerId: string;
}

/**
 * Options for a source feature query.
 *
 * See `mln_source_feature_query_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
export interface SourceFeatureQueryOptions {
  /**
   * The source layers to query. A vector source finds features only in the
   * source layers named here.
   */
  sourceLayerIds?: readonly string[];
  /**
   * A &#91;filter&#93;(https://maplibre.org/maplibre-style-spec/expressions/)
   * that each feature must match. Empty matches every feature.
   */
  filter?: unknown;
}

/**
 * The result of a call or of asynchronous work.
 *
 * See `mln_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export const Status = {
  ok: 0,
  /**
   * An argument is invalid, such as malformed JSON or a value out of range.
   */
  invalidArgument: -1,
  /**
   * The call doesn't fit the current state, such as removing a source that a
   * layer still uses.
   */
  invalidState: -2,
  /**
   * The call came from a thread that can't make it.
   */
  wrongThread: -3,
  /**
   * The platform, the build, or the render target lacks the feature.
   */
  unsupported: -4,
  /**
   * The library or the platform hit an error, such as a failed graphics driver
   * call.
   */
  nativeError: -5,
  /**
   * The work stopped before it finished, such as when its handle was released.
   */
  cancelled: -6,
  /**
   * Another call is still using the same object.
   */
  busy: -7,
  /**
   * The render target can't be drawn to anymore.
   */
  targetLost: -8,
  /**
   * Nothing has the given ID.
   */
  notFound: -10,
  /**
   * Something already has the given ID.
   */
  alreadyExists: -11,
} as const;
/** One of the Status values, or a value this binding does not know. */
export type Status = (typeof Status)[keyof typeof Status] | (number & {});

/**
 * What a style expression's result depends on.
 *
 * See `mln_style_expression_info` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
 */
export interface StyleExpressionInfo {
  /**
   * True when the result doesn't depend on the feature or its state.
   */
  readonly isFeatureConstant: boolean;
  /**
   * True when the result doesn't depend on the zoom level.
   */
  readonly isZoomConstant: boolean;
}

/**
 * The inputs that a style expression reads while it evaluates.
 *
 * See `mln_style_expression_inputs` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
 */
export interface StyleExpressionInputs {
  zoom?: number;
  /**
   * The value of `heatmap-density`, `line-progress`, and `elevation`.
   */
  colorRampParameter?: number;
  /**
   * The feature to evaluate against, as a GeoJSON feature, or empty for none.
   */
  feature?: unknown;
  /**
   * The feature's state, as a JSON object, or empty for none.
   */
  featureState?: unknown;
  /**
   * The values that `global-state` reads, as a JSON object, or empty for none.
   */
  globalState?: unknown;
  /**
   * The tile that holds the feature. `within` and `distance` need it to read
   * the feature's geometry.
   */
  canonicalTileId?: CanonicalTileId;
  /**
   * The IDs of the images that `image` treats as available.
   */
  availableImages?: readonly string[];
}

/**
 * Options for parsing a style expression.
 *
 * See `mln_style_expression_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
 */
export interface StyleExpressionOptions {
  /**
   * The type that the result must have.
   */
  expectedType?: StyleExpressionType;
}

/**
 * The type of a style expression's result.
 *
 * See `mln_style_expression_type` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
 */
export interface StyleExpressionType {
  kind?: StyleExpressionTypeKind;
  /**
   * The type of each item when `kind` is `StyleExpressionTypeKind.array`. It
   * can't be an array itself.
   */
  itemKind?: StyleExpressionTypeKind;
  /**
   * The length of an array, or 0 for any length.
   */
  itemCount?: number;
}

/**
 * The type of a style expression's result, or of the items of an array result.
 *
 * See `mln_style_expression_type_kind` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
 */
export const StyleExpressionTypeKind = {
  /**
   * Any type.
   */
  value: 0,
  number: 1,
  string: 2,
  boolean: 3,
  color: 4,
  object: 5,
  formatted: 6,
  image: 7,
  padding: 8,
  variableAnchorOffsetCollection: 9,
  array: 10,
} as const;
/** One of the StyleExpressionTypeKind values, or a value this binding does not know. */
export type StyleExpressionTypeKind =
  | (typeof StyleExpressionTypeKind)[keyof typeof StyleExpressionTypeKind]
  | (number & {});

/**
 * An image in the map's style.
 *
 * See `mln_style_image_info` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface StyleImageInfo {
  readonly width: number;
  readonly height: number;
  /**
   * The premultiplied RGBA pixels, in rows of `width` &#42; 4 bytes, from the
   * top row down.
   */
  readonly pixels: Uint8Array;
  readonly stretchX: readonly ImageStretch[];
  readonly stretchY: readonly ImageStretch[];
  readonly content?: ImageContent;
  readonly textFitWidth?: StyleImageTextFit;
  readonly textFitHeight?: StyleImageTextFit;
  readonly pixelRatio: number;
  readonly sdf: boolean;
  readonly keepUnused: boolean;
}

/**
 * Options for a style image, as the
 * &#91;sprite&#93;(https://maplibre.org/maplibre-style-spec/sprite/) spec
 * describes them.
 *
 * See `mln_style_image_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface StyleImageOptions {
  /**
   * The ranges of columns that stretch, in increasing order without overlap.
   */
  stretchX?: readonly ImageStretch[];
  /**
   * The ranges of rows that stretch, in increasing order without overlap.
   */
  stretchY?: readonly ImageStretch[];
  content?: ImageContent;
  textFitWidth?: StyleImageTextFit;
  textFitHeight?: StyleImageTextFit;
  pixelRatio?: number;
  sdf?: boolean;
  /**
   * Keeps the image after no layer uses it. Without this, the map can remove an
   * image that it asked for through `RuntimeEventType.mapStyleImageMissing`.
   */
  keepUnused?: boolean;
}

/**
 * How an image stretches to fit text along one axis.
 *
 * See `mln_style_image_text_fit` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export const StyleImageTextFit = {
  stretchOrShrink: 0,
  stretchOnly: 1,
  proportional: 2,
} as const;
/** One of the StyleImageTextFit values, or a value this binding does not know. */
export type StyleImageTextFit =
  | (typeof StyleImageTextFit)[keyof typeof StyleImageTextFit]
  | (number & {});

/**
 * A layer in the map's style.
 *
 * See `mln_style_layer_info` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface StyleLayerInfo {
  readonly id: string;
  /**
   * The layer's
   * &#91;type&#93;(https://maplibre.org/maplibre-style-spec/layers/#type), such
   * as `fill`.
   */
  readonly type: string;
  readonly sourceId: string;
  readonly sourceLayer: string;
  /**
   * The zoom level where the layer starts to show, or negative infinity when
   * the layer doesn't set one.
   */
  readonly minZoom: number;
  /**
   * The zoom level where the layer stops showing, or infinity when the layer
   * doesn't set one.
   */
  readonly maxZoom: number;
  readonly visibility: StyleLayerVisibility;
}

/**
 * Whether a layer shows.
 *
 * See `mln_style_layer_visibility` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export const StyleLayerVisibility = {
  visible: 0,
  none: 1,
} as const;
/** One of the StyleLayerVisibility values, or a value this binding does not know. */
export type StyleLayerVisibility =
  | (typeof StyleLayerVisibility)[keyof typeof StyleLayerVisibility]
  | (number & {});

/**
 * How a raster DEM tile encodes elevation in its colors.
 *
 * See `mln_style_raster_dem_encoding` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export const StyleRasterDemEncoding = {
  mapbox: 0,
  terrarium: 1,
} as const;
/** One of the StyleRasterDemEncoding values, or a value this binding does not know. */
export type StyleRasterDemEncoding =
  | (typeof StyleRasterDemEncoding)[keyof typeof StyleRasterDemEncoding]
  | (number & {});

/**
 * A source in the map's style.
 *
 * See `mln_style_source_info` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface StyleSourceInfo {
  readonly id: string;
  readonly type: StyleSourceType;
  readonly isVolatile: boolean;
  readonly attribution: string;
  readonly url: string;
  /**
   * The tiles that the style lists in the source itself, instead of at a URL.
   */
  readonly tileJson?: StyleSourceTileInfo;
  readonly bounds?: LatLngBounds;
  readonly tileSize?: number;
  readonly vectorEncoding?: StyleVectorTileEncoding;
  readonly rasterEncoding?: StyleRasterDemEncoding;
  /**
   * A GeoJSON source's options, with every field set. Data prepared with them
   * can replace the source's data.
   */
  readonly geoJson?: GeoJsonSourceOptions;
}

/**
 * The tiles of a source, as the style lists them.
 *
 * See `mln_style_source_tile_info` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface StyleSourceTileInfo {
  readonly tileUrls: readonly string[];
  readonly minZoom: number;
  readonly maxZoom: number;
  readonly scheme: StyleTileScheme;
}

/**
 * The type of a style source.
 *
 * See `mln_style_source_type` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export const StyleSourceType = {
  unknown: 0,
  vector: 1,
  raster: 2,
  rasterDem: 3,
  geoJson: 4,
  image: 5,
  video: 6,
  annotations: 7,
  customGeometry: 8,
  customVector: 9,
} as const;
/** One of the StyleSourceType values, or a value this binding does not know. */
export type StyleSourceType =
  | (typeof StyleSourceType)[keyof typeof StyleSourceType]
  | (number & {});

/**
 * Whether a tileset counts tile rows from the north (XYZ) or the south (TMS).
 *
 * See `mln_style_tile_scheme` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export const StyleTileScheme = {
  xyz: 0,
  tms: 1,
} as const;
/** One of the StyleTileScheme values, or a value this binding does not know. */
export type StyleTileScheme =
  | (typeof StyleTileScheme)[keyof typeof StyleTileScheme]
  | (number & {});

/**
 * The default
 * &#91;transition&#93;(https://maplibre.org/maplibre-style-spec/transition/)
 * for changes to a style's paint properties.
 *
 * See `mln_style_transition_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface StyleTransitionOptions {
  durationMs?: number;
  delayMs?: number;
  /**
   * Whether symbols fade in and out as the map places them.
   */
  enablePlacementTransitions?: boolean;
}

/**
 * The format of vector tiles.
 *
 * See `mln_style_vector_tile_encoding` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export const StyleVectorTileEncoding = {
  mvt: 0,
  mlt: 1,
} as const;
/** One of the StyleVectorTileEncoding values, or a value this binding does not know. */
export type StyleVectorTileEncoding =
  | (typeof StyleVectorTileEncoding)[keyof typeof StyleVectorTileEncoding]
  | (number & {});

/**
 * The layout of the pixels that `RenderSessionHandle.readTexture()` reads.
 *
 * See `mln_texture_image_info` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface TextureImageInfo {
  readonly width: number;
  readonly height: number;
  /**
   * The bytes from the start of one row to the start of the next.
   */
  readonly stride: number;
  readonly byteLength: bigint;
}

/**
 * The pixels that `RenderSessionHandle.readTexture()` reads.
 *
 * See `mln_texture_readback_result` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface TextureReadbackResult {
  /**
   * Premultiplied RGBA pixels with 8 bits per channel, in rows from top to
   * bottom.
   */
  readonly data: Uint8Array;
  readonly info: TextureImageInfo;
}

/**
 * The ID of a tile.
 *
 * `canonical_x` and `canonical_y` place the tile in the source's grid at zoom
 * `canonical_z`. `overscaled_z` is the zoom that the map draws the tile at, and
 * is greater when the source has no tiles that deep. `wrap` says which copy of
 * the world holds the tile: zero for the main one, negative to the west, and
 * positive to the east.
 *
 * See `mln_tile_id` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface TileId {
  readonly overscaledZ: number;
  readonly wrap: number;
  readonly canonicalZ: number;
  readonly canonicalX: number;
  readonly canonicalY: number;
}

/**
 * How the map picks the level of detail of its tiles.
 *
 * See `mln_tile_lod_mode` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const TileLodMode = {
  /**
   * Uses the most detail at the center of the screen.
   */
  default: 0,
  /**
   * Uses the most detail closest to the camera.
   */
  distance: 1,
} as const;
/** One of the TileLodMode values, or a value this binding does not know. */
export type TileLodMode =
  | (typeof TileLodMode)[keyof typeof TileLodMode]
  | (number & {});

/**
 * A step in loading a tile.
 *
 * See `mln_tile_operation` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const TileOperation = {
  requestedFromCache: 0,
  requestedFromNetwork: 1,
  loadFromNetwork: 2,
  loadFromCache: 3,
  startParse: 4,
  endParse: 5,
  error: 6,
  cancelled: 7,
  null: 8,
} as const;
/** One of the TileOperation values, or a value this binding does not know. */
export type TileOperation =
  | (typeof TileOperation)[keyof typeof TileOperation]
  | (number & {});

/**
 * A timing curve from (0, 0) to (1, 1), with the control points (`x1`, `y1`)
 * and (`x2`, `y2`).
 *
 * `x1` and `x2` are between 0 and 1.
 *
 * See `mln_unit_bezier` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface UnitBezier {
  x1?: number;
  y1?: number;
  x2?: number;
  y2?: number;
}

/**
 * How a URL pattern matches a request's URL.
 *
 * A pattern matches the whole URL, query included. Without flags, it matches
 * only the URL that equals it exactly.
 *
 * See `mln_url_match_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const UrlMatchFlag = {
  /**
   * The URL matches the pattern as a glob. `*` matches any characters except
   * `/`, `**` matches any characters, and `?` matches one character except `/`.
   * `\` makes the next character match only itself. For example,
   * `https:// *.example.com/tiles/ **` matches every URL under `/tiles/` on each
   * subdomain of `example.com`.
   */
  glob: 1,
} as const;
/** A bitwise OR of UrlMatchFlag flags. */
export type UrlMatchFlag = number;

/**
 * A query parameter that a resource transform rule adds to a URL.
 *
 * Pass `name` and `value` unencoded. The library percent-encodes every byte of
 * them except the letters, digits, `-`, `.`, `_`, and `~`, and adds them as
 * `name=value`.
 *
 * See `mln_url_query_parameter` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface UrlQueryParameter {
  name: string;
  value?: string;
}

/**
 * A vector in three dimensions.
 *
 * See `mln_vec3` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface Vec3 {
  x: number;
  y: number;
  z: number;
}

/**
 * The vertical direction of the image that the map renders.
 *
 * See `mln_viewport_mode` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const ViewportMode = {
  default: 0,
  /**
   * Renders the image upside down.
   */
  flippedY: 1,
} as const;
/** One of the ViewportMode values, or a value this binding does not know. */
export type ViewportMode =
  | (typeof ViewportMode)[keyof typeof ViewportMode]
  | (number & {});

/**
 * A Vulkan image of a borrowed ring.
 *
 * See `mln_vulkan_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface VulkanBorrowedTexture {
  /**
   * Your VkImage.
   */
  image: bigint;
  /**
   * A 2D VkImageView of `image` with one mip level and one layer.
   */
  imageView: bigint;
}

/**
 * A ring of your Vulkan images, for `MapHandle.attachVulkanBorrowedTexture()`.
 *
 * `logical_size` sets the map's size. `physical_width` and `physical_height`
 * give the images' size in pixels, which is usually `logical_size` times the
 * map's scale factor. Each image must have that size, `format`, and
 * VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, and belong to the context's device.
 *
 * The session uses the images until a replacement or release of the target
 * completes, a detach or release of the session completes, or an abandon of it
 * returns.
 *
 * See `mln_vulkan_borrowed_texture_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface VulkanBorrowedTextureDescriptor {
  logicalSize: LogicalSize;
  physicalWidth: number;
  physicalHeight: number;
  context: VulkanContextDescriptor;
  textures: readonly VulkanBorrowedTexture[];
  /**
   * The images' VkFormat.
   */
  format: number;
  /**
   * The VkImageLayout that each frame leaves its image in.
   */
  finalLayout: number;
  owner?: VulkanImageOwner;
}

/**
 * New Vulkan images for a borrowed ring, for
 * `RenderSessionHandle.setVulkanBorrowedTextureTarget()`.
 *
 * The members mean what they mean in `VulkanBorrowedTextureDescriptor`, and the
 * images follow its rules. The ring keeps its depth, context, `format`,
 * `final_layout`, and `owner`, so give it as many images as it attached with,
 * in that format, on the same device. The session uses the images until a
 * replacement or release of the target completes, a detach or release of the
 * session completes, or an abandon of it returns.
 *
 * See `mln_vulkan_borrowed_texture_replacement` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface VulkanBorrowedTextureReplacement {
  logicalSize: LogicalSize;
  physicalWidth: number;
  physicalHeight: number;
  textures: readonly VulkanBorrowedTexture[];
}

/**
 * The Vulkan device and queue that a render session uses.
 *
 * Keep these objects alive until a detach or release of the session completes,
 * or an abandon of it returns.
 *
 * See `mln_vulkan_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface VulkanContextDescriptor {
  /**
   * A `VkInstance`.
   */
  instance: bigint;
  /**
   * A `VkPhysicalDevice`.
   */
  physicalDevice: bigint;
  /**
   * A `VkDevice`.
   */
  device: bigint;
  /**
   * A `VkQueue` from `graphics_queue_family_index` that supports graphics. If
   * you submit to it too, set `RenderSessionAttachOptions.queueLock`.
   */
  graphicsQueue: bigint;
  graphicsQueueFamilyIndex: number;
  /**
   * Your `vkGetInstanceProcAddr`, or null to use the Vulkan loader's.
   */
  getInstanceProcAddr: bigint;
  /**
   * Your `vkGetDeviceProcAddr`, or null to look it up from the instance.
   */
  getDeviceProcAddr: bigint;
}

/**
 * A Vulkan device's context, as `VulkanDeviceHandle.getContext()` reports it.
 *
 * See `mln_vulkan_device_context_result` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html).
 */
export interface VulkanDeviceContextResult {
  readonly context: VulkanContextDescriptor;
}

/**
 * Options for `VulkanDeviceHandle.create()`.
 *
 * See `mln_vulkan_device_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html).
 */
export interface VulkanDeviceOptions {
  /**
   * Whether the device can present to a window's surface. It enables
   * VK_KHR_surface and the platform's surface extensions on the instance, and
   * VK_KHR_swapchain on the device. Defaults to false.
   */
  presentation?: boolean;
  /**
   * The 16-byte `deviceUUID` of the GPU to use, as
   * `VkPhysicalDeviceIDProperties` reports it. Name a GPU to share textures
   * with another graphics API on it.
   */
  deviceUuid?: Uint8Array;
  /**
   * The 8-byte `deviceLUID` of the GPU to use, as
   * `VkPhysicalDeviceIDProperties` reports it. On Windows, this is the GPU's
   * DXGI adapter LUID.
   */
  deviceLuid?: Uint8Array;
}

/**
 * The queue family that a borrowed image goes to after each frame.
 *
 * With `VulkanImageOwner.external` or `VulkanImageOwner.foreign`, acquire the
 * image on your side before you read it. You don't need to release it back.
 *
 * See `mln_vulkan_image_owner` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export const VulkanImageOwner = {
  /**
   * The image stays with the queue family of `graphics_queue_family_index`.
   */
  graphicsQueue: 0,
  /**
   * Each frame releases the image to VK_QUEUE_FAMILY_EXTERNAL.
   */
  external: 1,
  /**
   * Each frame releases the image to VK_QUEUE_FAMILY_FOREIGN_EXT.
   */
  foreign: 2,
} as const;
/** One of the VulkanImageOwner values, or a value this binding does not know. */
export type VulkanImageOwner =
  | (typeof VulkanImageOwner)[keyof typeof VulkanImageOwner]
  | (number & {});

/**
 * A Vulkan surface to present to, for `MapHandle.attachVulkanSurface()`.
 *
 * The device must have VK_KHR_swapchain enabled, and the queue family of
 * `graphics_queue_family_index` must support presenting to the surface. The
 * session creates the swapchain. It uses the surface until a replacement or
 * release of the target completes, a detach or release of the session
 * completes, or an abandon of it returns.
 *
 * See `mln_vulkan_surface_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
export interface VulkanSurfaceDescriptor {
  logicalSize: LogicalSize;
  context: VulkanContextDescriptor;
  /**
   * Your VkSurfaceKHR.
   */
  surface: bigint;
}

/**
 * A new Vulkan surface for `RenderSessionHandle.setVulkanSurfaceTarget()`.
 *
 * The session presents to it with the context that it attached with, so the
 * queue family of that context's `graphics_queue_family_index` must support
 * presenting to the surface. It uses the surface until a replacement or release
 * of the target completes, a detach or release of the session completes, or an
 * abandon of it returns.
 *
 * See `mln_vulkan_surface_replacement` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
export interface VulkanSurfaceReplacement {
  logicalSize: LogicalSize;
  /**
   * Your VkSurfaceKHR.
   */
  surface: bigint;
}

/**
 * The Vulkan image of an acquired frame.
 *
 * See `mln_vulkan_texture_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface VulkanTextureFrame {
  readonly width: number;
  readonly height: number;
  readonly scaleFactor: number;
  /**
   * The image's index in the descriptor's `textures`.
   */
  readonly slot: number;
  /**
   * The VkImage.
   */
  readonly image: bigint;
  /**
   * The VkImageView.
   */
  readonly imageView: bigint;
  /**
   * The image's VkDevice.
   */
  readonly device: bigint;
  /**
   * The image's VkFormat.
   */
  readonly format: number;
  /**
   * The VkImageLayout that the frame left the image in.
   */
  readonly layout: number;
}

/**
 * A callback that tells the caller when work is ready.
 *
 * The callback runs asynchronously on the JavaScript thread that registered it,
 * after native queues the call.
 *
 * See `mln_wake` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wake_8h.html).
 */
export type Wake =
  | {
      /**
       * Null to turn the wake off.
       */
      callback?: () => void;
    }
  | NativeRegistration<"Wake">;
export const Wake = {
  /**
   * Creates a native registration from a native function pointer for
   * each callback, and an optional context and release. The binding
   * copies them into the C descriptor unchanged. The first call that
   * carries the registration consumes it: native code releases the
   * context of a call that it accepts, and the binding releases it for
   * any other call. A later call throws a `MaplibreError` with the
   * invalid-state status.
   */
  native(fields: NativeWakeFields): NativeRegistration<"Wake"> {
    return nativeRegistration("Wake", {
      callback: fields.callback,
      userData: fields.userData ?? 0n,
      releaseUserData: fields.releaseUserData ?? 0n,
    });
  },
};

/** The fields of a native {@link Wake}, as native addresses. */
export interface NativeWakeFields {
  /**
   * Null to turn the wake off.
   */
  readonly callback: bigint;
  readonly userData?: bigint;
  readonly releaseUserData?: bigint;
}

/**
 * The WebGL context or canvas that a render session uses.
 *
 * See `mln_web_gl_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface WebGlContextDescriptor {
  kind?: WebGlContextKind;
  /**
   * Your `EMSCRIPTEN_WEBGL_CONTEXT_HANDLE`, for an existing context.
   */
  context?: number;
  /**
   * The canvas's CSS selector, for a transferred canvas.
   */
  canvasSelector?: string;
}

/**
 * How a render session gets its WebGL context.
 *
 * See `mln_web_gl_context_kind` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const WebGlContextKind = {
  /**
   * The session uses a context that you created with
   * `emscripten_webgl_create_context()`.
   */
  existing: 0,
  /**
   * The library transfers the canvas that `canvas_selector` names to a worker
   * and creates the context there.
   */
  transferredCanvas: 1,
} as const;
/** One of the WebGlContextKind values, or a value this binding does not know. */
export type WebGlContextKind =
  | (typeof WebGlContextKind)[keyof typeof WebGlContextKind]
  | (number & {});

/**
 * A WebGPU texture of a borrowed ring.
 *
 * See `mln_web_gpu_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface WebGpuBorrowedTexture {
  /**
   * Your WGPUTexture.
   */
  texture: bigint;
  /**
   * A WGPUTextureView of `texture`.
   */
  textureView: bigint;
}

/**
 * A ring of your WebGPU textures, for
 * `MapHandle.attachWebGpuBorrowedTexture()`.
 *
 * `logical_size` sets the map's size. `physical_width` and `physical_height`
 * give the textures' size in pixels, which is usually `logical_size` times the
 * map's scale factor. Each texture must be 2D with that size, `format`, one
 * layer, one mip level, one sample, and WGPUTextureUsage_RenderAttachment.
 *
 * The session uses the textures until a replacement or release of the target
 * completes, a detach or release of the session completes, or an abandon of it
 * returns.
 *
 * See `mln_web_gpu_borrowed_texture_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface WebGpuBorrowedTextureDescriptor {
  logicalSize: LogicalSize;
  physicalWidth: number;
  physicalHeight: number;
  context: WebGpuContextDescriptor;
  textures: readonly WebGpuBorrowedTexture[];
  /**
   * The textures' WGPUTextureFormat.
   */
  format: number;
}

/**
 * New WebGPU textures for a borrowed ring, for
 * `RenderSessionHandle.setWebGpuBorrowedTextureTarget()`.
 *
 * The members mean what they mean in `WebGpuBorrowedTextureDescriptor`, and the
 * textures follow its rules. The ring keeps its depth, context, and `format`,
 * so give it as many textures as it attached with, in that format, on the same
 * device. The session uses the textures until a replacement or release of the
 * target completes, a detach or release of the session completes, or an abandon
 * of it returns.
 *
 * See `mln_web_gpu_borrowed_texture_replacement` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface WebGpuBorrowedTextureReplacement {
  logicalSize: LogicalSize;
  physicalWidth: number;
  physicalHeight: number;
  textures: readonly WebGpuBorrowedTexture[];
}

/**
 * The WebGPU device that a render session uses.
 *
 * See `mln_web_gpu_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface WebGpuContextDescriptor {
  /**
   * A `WGPUInstance`, or null.
   */
  instance?: bigint;
  /**
   * A `WGPUDevice`.
   */
  device: bigint;
  /**
   * A `WGPUQueue`, or null to use the device's queue.
   */
  queue?: bigint;
}

/**
 * A WebGPU surface to present to, for `MapHandle.attachWebGpuSurface()`.
 *
 * The session configures the surface. It uses the surface until a replacement
 * or release of the target completes, a detach or release of the session
 * completes, or an abandon of it returns.
 *
 * See `mln_web_gpu_surface_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
export interface WebGpuSurfaceDescriptor {
  logicalSize: LogicalSize;
  context: WebGpuContextDescriptor;
  /**
   * Your WGPUSurface.
   */
  surface: bigint;
  /**
   * The WGPUTextureFormat to configure the surface with.
   */
  format: number;
}

/**
 * A new WebGPU surface for `RenderSessionHandle.setWebGpuSurfaceTarget()`.
 *
 * The session configures it with the context and format that it attached with.
 * It uses the surface until a replacement or release of the target completes, a
 * detach or release of the session completes, or an abandon of it returns.
 *
 * See `mln_web_gpu_surface_replacement` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
export interface WebGpuSurfaceReplacement {
  logicalSize: LogicalSize;
  /**
   * Your WGPUSurface.
   */
  surface: bigint;
}

/**
 * The WebGPU texture of an acquired frame.
 *
 * See `mln_web_gpu_texture_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface WebGpuTextureFrame {
  readonly width: number;
  readonly height: number;
  readonly scaleFactor: number;
  /**
   * The texture's index in the descriptor's `textures`.
   */
  readonly slot: number;
  /**
   * The WGPUTexture.
   */
  readonly texture: bigint;
  /**
   * The WGPUTextureView.
   */
  readonly textureView: bigint;
  /**
   * The texture's WGPUDevice.
   */
  readonly device: bigint;
  /**
   * The texture's WGPUTextureFormat.
   */
  readonly format: number;
}

/**
 * The WGL objects that a render session makes its OpenGL context from.
 *
 * See `mln_wgl_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface WglContextDescriptor {
  /**
   * An `HDC`.
   */
  deviceContext: bigint;
  /**
   * Your `HGLRC`, for a shared context.
   */
  shareContext?: bigint;
  /**
   * Your `wglGetProcAddress`, or null to use the system's.
   */
  getProcAddress?: bigint;
}
