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
 * How a gesture's zoom, bearing, or pitch keeps changing after release.
 *
 * See `mln_anchored_momentum` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface AnchoredMomentum {
  /**
   * How fast the value changed at release: zoom levels per second for zoom, and
   * degrees per second for bearing and pitch. The part moves only when this is
   * set.
   */
  velocity?: number;
  /**
   * The screen point that the change keeps in place. Without one, the change
   * keeps the center of the view in place.
   */
  anchor?: ScreenPoint;
  decay?: MomentumDecay;
  /**
   * Where the part settles. Without this, it stays where the gesture and its
   * glide leave it.
   */
  snap?: MomentumSnap;
}

/**
 * The Android hardware buffer of an exported ring's acquired frame.
 *
 * See `mln_android_hardware_buffer_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface AndroidHardwareBufferFrame {
  readonly width: number;
  readonly height: number;
  readonly scaleFactor: number;
  /**
   * The buffer's index in the ring.
   */
  readonly slot: number;
  /**
   * The `AHardwareBuffer*`.
   */
  readonly buffer: bigint;
}

/**
 * An Android window to present to.
 *
 * The session takes its own reference to the window, so you can release yours
 * once the call returns. When the platform destroys the window, such as in
 * `SurfaceHolder.Callback.surfaceDestroyed`, release the target with
 * `RenderSessionHandle.releaseTarget()`, or give the session another window
 * with `RenderSessionHandle.setTarget()`, without waiting for that to complete.
 * A frame to the destroyed window finishes with `RenderResult.targetNotReady`,
 * or with `RenderResult.rendered` if the window went away while the frame was
 * presenting.
 *
 * See `mln_android_window_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface AndroidWindowTarget {
  logicalSize: LogicalSize;
  /**
   * Your `ANativeWindow*`.
   */
  window: bigint;
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
  /**
   * The longest a fly animation may take. A fly animation whose duration, set
   * or derived from `velocity`, is longer moves the camera at once instead, and
   * so does one too slow for the library to time, without this field. Other
   * animations ignore it.
   */
  maxDurationMs?: number;
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
 * What moved the camera.
 *
 * See `mln_camera_change_source` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const CameraChangeSource = {
  /**
   * A camera command outside a gesture, or the map itself, as when a resize
   * keeps the camera within its bounds.
   */
  command: 0,
  /**
   * A camera command with a gesture phase.
   */
  gesture: 1,
  /**
   * The momentum and bearing snap that a gesture's end started.
   */
  momentum: 2,
} as const;
/** One of the CameraChangeSource values, or a value this binding does not know. */
export type CameraChangeSource =
  | (typeof CameraChangeSource)[keyof typeof CameraChangeSource]
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
  /**
   * How the gesture keeps moving after this delta. Only a delta in the
   * `GesturePhase.end` phase carries it.
   */
  momentum?: GestureMomentum;
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
   * Space to leave between the area and each edge of the map. The left and
   * right insets must add up to less than the map's width, and the top and
   * bottom to less than its height.
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
 * A jump uses only the end handler of `animation`, as in `CameraUpdate`.
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
  /**
   * How fast a gesture's momentum still moved when it was cancelled, which the
   * next gesture can add to its own release velocities. Zero for every other
   * end.
   */
  readonly velocity: CameraVelocity;
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
 * A change that ends in more than one way reports
 * `CameraTransitionOutcome.cancelled` if any of it was cancelled, and otherwise
 * `CameraTransitionOutcome.replaced` if any of it was replaced.
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
   * `MapHandle.setFreeCameraOptions()`, or a resize that moved the camera back
   * inside its bounds stopped the change. A command that fails before it moves
   * the camera also reports this.
   */
  cancelled: 1,
  /**
   * The map closed before the change ended.
   */
  closed: 2,
  /**
   * A later camera change set every field that a part of the change was still
   * animating, so that part stopped. Its other parts keep animating, and the
   * change reports this outcome when they end, unless one was cancelled.
   *
   * A later change that sets only some of those fields leaves the change
   * animating the rest. A fly's center and zoom count as one field, and so do
   * an anchored change's center and the fields that the anchor holds in place.
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
 * How fast each part of a camera change moves.
 *
 * See `mln_camera_velocity` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface CameraVelocity {
  /**
   * How fast the map's content moves on the screen, in logical pixels per
   * second.
   */
  readonly pan: ScreenPoint;
  /**
   * In zoom levels per second.
   */
  readonly zoom: number;
  /**
   * In degrees per second.
   */
  readonly bearing: number;
  /**
   * In degrees per second.
   */
  readonly pitch: number;
}

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
 * How a graphics API's clip space holds depth.
 *
 * See `mln_clip_depth_range` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const ClipDepthRange = {
  /**
   * Clip z runs from 0 to w, as in Metal, Vulkan, WebGPU, Direct3D, and OpenGL
   * with `glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE)`.
   */
  zeroToOne: 1,
  /**
   * Clip z runs from -w to w, as in OpenGL by default.
   */
  negativeOneToOne: 2,
} as const;
/** One of the ClipDepthRange values, or a value this binding does not know. */
export type ClipDepthRange =
  | (typeof ClipDepthRange)[keyof typeof ClipDepthRange]
  | (number & {});

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
 * The callbacks of a custom layer, which draws with your own GPU commands in
 * the map's frame.
 *
 * Nor does a browser WebGPU session yet: the library's WebGPU (wgpu) exposes no
 * command encoder or render pass to hand you. Leave unset the callbacks that
 * you don't need.
 *
 * Runs on the thread that runs the session's graphics work: a library thread,
 * or yours with `RenderDriverKind.callerGraphicsThread`. It can't call the
 * library.
 *
 * It can't call the library.
 *
 * See `mln_custom_layer_host` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export type CustomLayerHost = NativeRegistration<"CustomLayerHost">;
export const CustomLayerHost = {
  /**
   * Creates a native registration from a native function pointer for
   * each callback, and an optional context and release. The binding
   * copies them into the C descriptor unchanged. The first call that
   * carries the registration consumes it: native code releases the
   * context of a call that it accepts, and the binding releases it for
   * any other call. A later call throws a `MaplibreError` with the
   * invalid-state status.
   */
  native(
    fields: NativeCustomLayerHostFields,
  ): NativeRegistration<"CustomLayerHost"> {
    return nativeRegistration("CustomLayerHost", {
      prerender: fields.prerender,
      render: fields.render,
      deinitialize: fields.deinitialize,
      contextLost: fields.contextLost,
      userData: fields.userData ?? 0n,
      releaseUserData: fields.releaseUserData ?? 0n,
    });
  },
};

/** The fields of a native {@link CustomLayerHost}, as native addresses. */
export interface NativeCustomLayerHostFields {
  readonly prerender: bigint;
  readonly render: bigint;
  readonly deinitialize: bigint;
  readonly contextLost: bigint;
  readonly userData?: bigint;
  readonly releaseUserData?: bigint;
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
 * A format of depth and stencil textures, as a bit in a set.
 *
 * See `mln_depth_stencil_format_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const DepthStencilFormatFlag = {
  /**
   * 24-bit normalized depth and 8-bit stencil: VK_FORMAT_D24_UNORM_S8_UINT,
   * MTLPixelFormatDepth24Unorm_Stencil8, GL_DEPTH24_STENCIL8, or
   * WGPUTextureFormat_Depth24PlusStencil8 (`depth24plus-stencil8`).
   */
  depth24Stencil8: 1,
  /**
   * 32-bit float depth and 8-bit stencil: VK_FORMAT_D32_SFLOAT_S8_UINT,
   * MTLPixelFormatDepth32Float_Stencil8, GL_DEPTH32F_STENCIL8, or
   * WGPUTextureFormat_Depth32FloatStencil8 (`depth32float-stencil8`).
   */
  depth32FloatStencil8: 2,
} as const;
/** A bitwise OR of DepthStencilFormatFlag flags. */
export type DepthStencilFormatFlag = number;

/**
 * A DRM format and modifier that you can import.
 *
 * See `mln_dma_buf_format` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface DmaBufFormat {
  /**
   * A DRM fourcc code, such as DRM_FORMAT_ABGR8888.
   */
  fourcc?: number;
  /**
   * A DRM format modifier, such as DRM_FORMAT_MOD_LINEAR.
   */
  modifier?: bigint;
}

/**
 * The dmabuf of an exported ring's acquired frame.
 *
 * See `mln_dma_buf_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface DmaBufFrame {
  readonly width: number;
  readonly height: number;
  readonly scaleFactor: number;
  /**
   * The buffer's index in the ring.
   */
  readonly slot: number;
  readonly fourcc: number;
  readonly modifier: bigint;
  /**
   * The buffer's planes, in the order that its fourcc and modifier define.
   * Import all of them, as EGL_EXT_image_dma_buf_import_modifiers planes 0 and
   * up, or with one `zwp_linux_buffer_params_v1.add` call each.
   */
  readonly planes: readonly DmaBufPlane[];
}

/**
 * Where one plane of a dmabuf keeps its data.
 *
 * See `mln_dma_buf_plane` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface DmaBufPlane {
  /**
   * The file descriptor of the dmabuf that holds the plane. The planes of a
   * buffer often share one.
   */
  readonly fd: number;
  /**
   * The bytes from the start of the dmabuf to the plane's first byte.
   */
  readonly offset: number;
  /**
   * The bytes from the start of one row to the start of the next.
   */
  readonly stride: number;
}

/**
 * The DXGI shared texture of an exported ring's acquired frame.
 *
 * See `mln_dxgi_shared_texture_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface DxgiSharedTextureFrame {
  readonly width: number;
  readonly height: number;
  readonly scaleFactor: number;
  /**
   * The texture's index in the ring.
   */
  readonly slot: number;
  /**
   * The texture's `DXGI_FORMAT`.
   */
  readonly format: number;
  /**
   * The texture's shared NT `HANDLE`.
   */
  readonly handle: bigint;
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
 * Your EGL context, which each session's context shares objects with.
 *
 * Each session makes its context on `display` with `config` and the client API
 * and version of `share_context`.
 *
 * See `mln_egl_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface EglContextDescriptor {
  /**
   * An `EGLDisplay`, which you initialized.
   */
  display: bigint;
  /**
   * The `EGLConfig` of `share_context`.
   */
  config: bigint;
  /**
   * Your `EGLContext`.
   */
  shareContext: bigint;
  /**
   * Your `eglGetProcAddress`, or null to use the system's.
   */
  getProcAddress?: bigint;
}

/**
 * A ring of textures that the session allocates on its device and shares with
 * you, in one of the ways that you can import.
 *
 * Each texture has the size in pixels of `logical_size` at the map's scale
 * factor. `RenderSessionHandle.resize()` gives the ring textures of the new
 * size, and a frame that you hold keeps its texture of the old size until you
 * release or discard it.
 *
 * The ring shares its textures in the first kind of `export_kinds` that the
 * device exports, in this order, and `RenderSessionCapabilities.exportKind`
 * reports it:
 *
 * - `GpuExportKind.ioSurface`: IOSurfaces of the pixel format
 *   kCVPixelFormatType_32BGRA, which Metal names MTLPixelFormatBGRA8Unorm. Get
 *   each frame's surface with `AcquiredFrameHandle.withIoSurface()`.
 * - `GpuExportKind.androidHardwareBuffer`: hardware buffers of the format
 *   AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM, with the usage
 *   AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT,
 *   AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE, and the bits of
 *   `android_hardware_buffer_usage`. Get each frame's buffer with
 *   `AcquiredFrameHandle.withAndroidHardwareBuffer()`.
 * - `GpuExportKind.dxgiSharedHandle`: D3D11 textures of the format
 *   DXGI_FORMAT_B8G8R8A8_UNORM on the device's GPU, which bind as shader
 *   resources and render targets. Open them on a D3D11 or D3D12 device of the
 *   same adapter, which you pick with `GpuDeviceOptions.adapterLuid`. Get each
 *   frame's texture with `AcquiredFrameHandle.withDxgiSharedTexture()`.
 * - `GpuExportKind.dmaBuf`: dmabufs of a format and modifier from
 *   `dma_buf_formats`. Get each frame's buffer with
 *   `AcquiredFrameHandle.withDmaBuf()`.
 * - `GpuExportKind.opaqueFd`: the memory of Vulkan images, which only a Vulkan
 *   device or an OpenGL context with GL_EXT_memory_object_fd on the same GPU
 *   and driver imports. To pick the GPU, create the device with
 *   `GpuDeviceOptions.adapterUuid` set to your context's GL_DEVICE_UUID_EXT.
 *   Get each frame's memory with `AcquiredFrameHandle.withOpaqueFd()`.
 *
 * `GpuDeviceCapabilities.exportKinds` reports the kinds that a device exports.
 * Attach with `RenderDriverKind.coreWorker`. Without a device, the session
 * renders on the first of the platform's backends whose device exports one of
 * `export_kinds`.
 *
 * Each frame hands over with `GpuSyncKind.cpuComplete` or with the sync kind
 * that crosses APIs on its platform: `GpuSyncKind.metalSharedEvent` for an
 * IOSurface, and `GpuSyncKind.syncFd` for the others, where the device has
 * them. For a reader that waits on neither, such as a Wayland compositor
 * without explicit synchronization or Flutter's `copyPixelBuffer`, leave them
 * out of `RenderSessionAttachOptions.producerSyncKinds`, so that each frame
 * finishes before you acquire it, and release each frame only once the reader
 * is done with its texture. Give the ring 3 textures, so the session can render
 * while you hold two frames.
 *
 * See `mln_exported_ring_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface ExportedRingTarget {
  logicalSize: LogicalSize;
  /**
   * How many textures the ring has, from 1 to `TEXTURE_RING_MAX_DEPTH`.
   */
  textureCount: number;
  /**
   * The ways of sharing a texture that you can import.
   */
  exportKinds: GpuExportKindFlag;
  /**
   * The dmabuf formats and modifiers that you import, most preferred first,
   * such as those that eglQueryDmaBufModifiersEXT() reports. A ring exports
   * dmabufs only of one of them. The library renders only DRM_FORMAT_ABGR8888
   * and DRM_FORMAT_ARGB8888. A modifier can give a buffer more planes than its
   * format has, such as a plane of compression metadata, and the library takes
   * any such modifier that its GPU can render into and share. The library takes
   * no implicit modifier: it skips DRM_FORMAT_MOD_INVALID, so a list of only
   * that one gets no dmabuf.
   */
  dmaBufFormats?: readonly DmaBufFormat[];
  /**
   * More `AHARDWAREBUFFER_USAGE_*` bits that your reads of a hardware buffer
   * need, such as AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, or zero.
   */
  androidHardwareBufferUsage?: bigint;
}

/**
 * The ID of a feature in a source, as the source's data gives it.
 *
 * The map compares feature IDs as text. A number names the same feature as the
 * string of its shortest decimal form, so 7, 7.0, and "7" name one feature.
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
 * Options for creating a frame pacer, or for `FramePacerHandle.setOptions()`.
 *
 * See `mln_frame_pacer_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
export interface FramePacerOptions {
  /**
   * The most frames per second that the pacer demands, greater than zero. The
   * pacer skips display refreshes to stay within it. By default, it can demand
   * frames at every refresh.
   */
  maximumFramesPerSecond?: number;
  /**
   * Removes the frame rate limit, so the pacer can demand frames at every
   * display refresh, as it does by default.
   */
  unlimitedFramesPerSecond?: boolean;
}

/**
 * Where a frame drew the map, and the depth that it wrote there.
 *
 * World coordinates are Web Mercator's. x runs from 0 at 180° west to 1 at 180°
 * east, and on past both into the copies of the world beside the first. y runs
 * from 0 at the north edge of the Mercator world, about 85.05° north, to 1 at
 * its south edge. z is the altitude in meters above sea level, at the scale of
 * the heights of the map's 3D layers, and the map draws its ground at 0.
 *
 * `matrix` takes a world point to the clip coordinates where the frame drew it,
 * with the camera's padding, roll, field of view, and center altitude, and the
 * map's viewport and projection mode. Clip y points up, as the map draws with
 * OpenGL, Metal, and WebGPU. The map negates clip y with Vulkan, so negate it
 * there too. At high zooms, single precision can't tell nearby world points
 * apart: subtract an origin near your geometry from its coordinates in double
 * precision, and fold the origin into the matrix, before you convert to float.
 *
 * Clip z follows `clip_depth_range`, the convention of the device's graphics
 * API. With the API's default depth range of 0 to 1, a fragment that you draw
 * through `matrix` gets the depth that the map's 3D layers, such as fill
 * extrusions, wrote at the same place: 0 at `near_distance` from the camera, up
 * to `far_depth` at `far_distance`. Depth-tested against the frame's depth,
 * your fragments hide behind the map's 3D layers where those stand in front of
 * them.
 *
 * The frame's depth starts at 1. The map's 3D layers write the depth of each of
 * their fragments. A 2D layer below every 3D layer of the style that draws
 * opaque fragments, such as an opaque fill, writes one depth over all of them,
 * wherever they are: the higher the layer, the lower its depth, and every one
 * between `far_depth` and 1. Backgrounds, translucent fragments, and the 2D
 * layers above a 3D layer, such as labels, write no depth. So what you draw
 * through `matrix`, even on the ground, lies in front of every 2D layer.
 *
 * While `globe` is true, the frame drew the map onto a globe, and its 2D layers
 * wrote no depth. `matrix` then is the map's Mercator one, which doesn't place
 * points on the globe.
 *
 * See `mln_frame_projection` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface FrameProjection {
  readonly matrix: Mat4;
  readonly clipDepthRange: ClipDepthRange;
  /**
   * The distance from the camera to the plane where the depth that you get
   * through `matrix` is 0, along the camera's view. Like `far_distance`, it is
   * in pixels of the map at the camera's zoom, where the world is 512 × 2^zoom
   * pixels wide.
   */
  readonly nearDistance: number;
  /**
   * The distance to the plane where the depth is `far_depth`.
   */
  readonly farDistance: number;
  /**
   * The depth at `far_distance`, below 1.
   */
  readonly farDepth: number;
  /**
   * Whether the frame drew the map as a globe, alone or blending into Mercator.
   */
  readonly globe: boolean;
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
 * How a gesture keeps moving the camera after the user lets go.
 *
 * A part moves only when its velocity is set, and each part slows on its own.
 * Every part defaults to `MomentumDecayModel.friction` with a half-life of 140
 * ms, and these other defaults:
 *
 * - Pan stops at 10 logical pixels per second, needs a release of at least 200,
 *   and counts a release faster than 10000 as 10000.
 * - Zoom stops at 0.01 levels per second and moves at most 2.5 levels. Its snap
 *   settles on the nearest whole level.
 * - Bearing stops at 1 degree per second and needs a release of at least 60.
 *   Its snap settles on north within 7 degrees.
 * - Pitch stops at 1 degree per second and needs a release of at least 5. Its
 *   snap settles flat within 7 degrees.
 *
 * See `mln_gesture_momentum` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface GestureMomentum {
  pan?: PanMomentum;
  zoom?: AnchoredMomentum;
  bearing?: AnchoredMomentum;
  pitch?: AnchoredMomentum;
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
 * What a GPU device can do.
 *
 * See `mln_gpu_device_capabilities` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
 */
export interface GpuDeviceCapabilities {
  readonly backend: RenderBackend;
  /**
   * The kinds of `RenderTarget` that sessions on the device render to.
   */
  readonly targetKinds: RenderTargetKindFlag;
  /**
   * The ways that the device can share the textures of an exported ring. A kind
   * is here only once the device has exported a texture of it, so a driver that
   * offers an extension but can't allocate the memory reports no kind of it.
   */
  readonly exportKinds: GpuExportKindFlag;
  /**
   * The kinds of `GpuSync` that the device can signal and wait on.
   */
  readonly syncKinds: GpuSyncKindFlag;
  /**
   * The formats of the depth textures that a borrowed ring takes.
   */
  readonly depthStencilFormats: DepthStencilFormatFlag;
  /**
   * The kinds of `export_kinds` whose rings also share each frame's depth
   * texture, with `RenderSessionAttachOptions.keepDepth`. An IOSurface or a
   * dmabuf holds no depth format, so only `GpuExportKind.opaqueFd` can be here.
   */
  readonly depthExportKinds: GpuExportKindFlag;
}

/**
 * Options for `GpuDeviceHandle.create()`.
 *
 * Name the GPU by at most one of `adapter_index`, `adapter_uuid`, and
 * `adapter_luid`. Without one, the device prefers a discrete GPU. An OpenGL
 * device takes none of them: it renders on the GPU that the platform picks.
 *
 * See `mln_gpu_device_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
 */
export interface GpuDeviceOptions {
  /**
   * The backend that the device renders with. By default, the platform's own:
   * Metal on Apple platforms, Vulkan elsewhere, or OpenGL where the system has
   * no Vulkan driver, and in a browser the build's one backend: WebGPU or
   * OpenGL (WebGL2).
   */
  backend?: RenderBackend;
  /**
   * The GPU to use, as an index into the backend's list of GPUs.
   */
  adapterIndex?: number;
  /**
   * The 16-byte UUID of the GPU to use: Vulkan's
   * `VkPhysicalDeviceIDProperties.deviceUUID`. Only a Vulkan device takes one.
   */
  adapterUuid?: Uint8Array;
  /**
   * The 8-byte LUID of the GPU to use: on Windows, its DXGI adapter LUID, which
   * Vulkan reports as `VkPhysicalDeviceIDProperties.deviceLUID`. Only a Vulkan
   * device takes one.
   */
  adapterLuid?: Uint8Array;
  /**
   * The OpenGL API that an OpenGL device renders with. An EGL device defaults
   * to `OpenGlClientApi.gles`, and takes `OpenGlClientApi.gl` too on Linux and
   * FreeBSD. A WGL device takes only `OpenGlClientApi.gl`, its default, and a
   * WebGL device only `OpenGlClientApi.gles`, its default.
   */
  openGlClientApi?: OpenGlClientApi;
  /**
   * Whether the sessions of an EGL device take turns in one OpenGL context
   * instead of having one each.
   */
  openGlSingleContext?: boolean;
  /**
   * Your lock on a Vulkan device's queue, when you submit to the queue that
   * `GpuDeviceHandle.getVulkanContext()` reports. Only a Vulkan device takes
   * one.
   */
  queueLock?: QueueLock;
}

/**
 * The Vulkan objects of a device, as `GpuDeviceHandle.getVulkanContext()`
 * reports them.
 *
 * See `mln_gpu_device_vulkan_context_result` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
 */
export interface GpuDeviceVulkanContextResult {
  readonly context: VulkanContextDescriptor;
}

/**
 * A way to share a texture's memory with another API or process.
 *
 * See `mln_gpu_export_kind` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const GpuExportKind = {
  /**
   * The texture isn't shared.
   */
  none: 0,
  /**
   * An `IOSurfaceRef`.
   */
  ioSurface: 1,
  /**
   * A Linux dmabuf, as file descriptors with a DRM format and modifier.
   */
  dmaBuf: 2,
  /**
   * An `AHardwareBuffer*`.
   */
  androidHardwareBuffer: 3,
  /**
   * A DXGI shared texture's NT handle.
   */
  dxgiSharedHandle: 4,
  /**
   * A Vulkan or OpenGL opaque memory file descriptor.
   */
  opaqueFd: 5,
} as const;
/** One of the GpuExportKind values, or a value this binding does not know. */
export type GpuExportKind =
  | (typeof GpuExportKind)[keyof typeof GpuExportKind]
  | (number & {});

/**
 * A way to share a texture's memory, as a bit in a set.
 *
 * See `mln_gpu_export_kind_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const GpuExportKindFlag = {
  ioSurface: 1,
  dmaBuf: 2,
  androidHardwareBuffer: 4,
  dxgiSharedHandle: 8,
  opaqueFd: 16,
} as const;
/** A bitwise OR of GpuExportKindFlag flags. */
export type GpuExportKindFlag = number;

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
   * `handle` is a sync file descriptor. A descriptor of -1, as Vulkan exports
   * for a semaphore that has already signaled, counts as
   * `GpuSyncKind.cpuComplete`.
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
 * The IOSurface of an exported ring's acquired frame.
 *
 * See `mln_io_surface_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface IoSurfaceFrame {
  readonly width: number;
  readonly height: number;
  readonly scaleFactor: number;
  /**
   * The surface's index in the ring.
   */
  readonly slot: number;
  /**
   * The `IOSurfaceRef`.
   */
  readonly surface: bigint;
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
   * callback. 0 sends none. A message of these severities goes to the
   * platform's log even when the handler's queue can't accept it.
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
   * The style's request failed, such as with a 404 or a missing file.
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
   * The map's first size, and its scale factor. Render sessions of the map
   * render at its scale factor. `MapHandle.resize()` changes both, or
   * `RenderSessionHandle.resize()` while a render session is attached.
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
  /**
   * Fonts for the text that a style's own fonts don't draw, such as scripts
   * that need complex shaping, like Myanmar or Devanagari. A JSON array of font
   * files, each an object with a `url` and an optional `unicode-range`, as in a
   * font face of a style's `font-faces`, which every font stack uses after the
   * style's own font faces. Empty for none.
   */
  fallbackFontFaces?: string;
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
  /**
   * Whether the map draws as a globe, alone or blending into Mercator.
   */
  readonly globeRendering: boolean;
  /**
   * The setting from `MapHandle.setGlobeFreeBearingEnabled()`.
   */
  readonly globeFreeBearingEnabled: boolean;
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
  /**
   * The style's projection at the camera's zoom.
   */
  projection: ProjectionDefinition;
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
 * A 4×4 matrix, by its columns: it takes a vector to `x` times the vector's x,
 * plus `y` times its y, `z` times its z, and `w` times its w. In memory it is
 * 16 doubles in column-major order, as OpenGL, Metal, Vulkan, and WebGPU
 * shaders take a matrix.
 *
 * See `mln_mat4` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface Mat4 {
  readonly x: Vec4;
  readonly y: Vec4;
  readonly z: Vec4;
  readonly w: Vec4;
}

/**
 * A Metal texture of a borrowed ring.
 *
 * See `mln_metal_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface MetalBorrowedTexture {
  /**
   * Your `MTLTexture`.
   */
  texture: bigint;
}

/**
 * A ring of your Metal textures to render into, on the Metal device that you
 * attach on.
 *
 * `physical_width` and `physical_height` give the textures' size in pixels,
 * which is usually `logical_size` times the map's scale factor. The textures
 * must all have that size, the device's `MTLDevice`, one pixel format, one
 * sample, and `MTLTextureUsageRenderTarget`. The ring holds up to
 * `TEXTURE_RING_MAX_DEPTH` distinct textures.
 *
 * Give the ring `depth_textures` to draw into each frame depth-tested against
 * the map, as `FrameProjection` says: one for each of `textures`, at the same
 * index, or none. Each frame clears the one at its slot, renders the map's
 * depth and stencil into it, and stores them. They must have the size of
 * `textures`, one pixel format of `GpuDeviceCapabilities.depthStencilFormats`,
 * one sample, `MTLTextureUsageRenderTarget`, and a storage mode other than
 * `MTLStorageModeMemoryless`.
 *
 * See `mln_metal_borrowed_textures_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface MetalBorrowedTexturesTarget {
  logicalSize: LogicalSize;
  physicalWidth: number;
  physicalHeight: number;
  textures: readonly MetalBorrowedTexture[];
  depthTextures?: readonly MetalBorrowedTexture[];
}

/**
 * Your Metal device and command queue, for `GpuDeviceHandle.wrapMetal()`.
 *
 * See `mln_metal_device_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
 */
export interface MetalDeviceDescriptor {
  /**
   * Your `MTLDevice`.
   */
  device: bigint;
  /**
   * Your `MTLCommandQueue` on `device`, or null for a queue of the library's.
   * With your queue, borrowed rings can use `GpuSyncKind.queueOrder`.
   */
  commandQueue?: bigint;
}

/**
 * A `CAMetalLayer` to present to.
 *
 * The session sets the layer's device, pixel format, and drawable size.
 *
 * See `mln_metal_layer_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface MetalLayerTarget {
  logicalSize: LogicalSize;
  /**
   * Your `CAMetalLayer*`.
   */
  layer: bigint;
}

/**
 * The Metal texture of an acquired frame.
 *
 * The depth members are set when the frame has a depth texture: one of a
 * borrowed ring's `depth_textures`, or an exported ring's with
 * `RenderSessionAttachOptions.keepDepth`. Otherwise they are zero.
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
  /**
   * The MTLTexture that holds the frame's depth and stencil.
   */
  readonly depthTexture: bigint;
  /**
   * The depth texture's MTLPixelFormat.
   */
  readonly depthPixelFormat: bigint;
}

/**
 * How one part of a gesture's momentum slows from its release speed.
 *
 * Speeds and distances are in the part's unit: logical pixels for pan, zoom
 * levels for zoom, and degrees for bearing and pitch. A setting whose bit isn't
 * set takes the part's default, which `GestureMomentum` lists. `min_speed` is
 * finite and not negative, and every other number is finite and positive. A
 * friction decay can serve every release, as a preset that matches a platform's
 * scrolling. An eased glide takes its direction from the part's velocity, and
 * its distance and duration from the caller's plan.
 *
 * See `mln_momentum_decay` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface MomentumDecay {
  model?: MomentumDecayModel;
  /**
   * For `MomentumDecayModel.friction`, how long the speed takes to halve.
   */
  halfLifeMs?: number;
  /**
   * For `MomentumDecayModel.friction`, the speed at which it ends.
   */
  stopSpeed?: number;
  /**
   * Required for `MomentumDecayModel.ease`.
   */
  distance?: number;
  /**
   * Required for `MomentumDecayModel.ease`.
   */
  durationMs?: number;
  /**
   * For `MomentumDecayModel.ease`, the share of `distance` covered over the
   * share of `duration_ms`. Its control points are between 0 and 1, so the
   * glide never passes its distance. Defaults to (0, 0, 0.25, 1), the curve
   * that camera animations ease along by default. Android's scroll fling
   * follows (0.175, 0.5, 0.35, 1).
   */
  easing?: UnitBezier;
  /**
   * A slower release starts no momentum.
   */
  minSpeed?: number;
  /**
   * For `MomentumDecayModel.friction`, a faster release counts as this.
   */
  maxSpeed?: number;
  /**
   * The farthest the part moves. A friction release that would move farther
   * starts slower, so it ends sooner, and a planned glide moves at most this
   * far over its duration.
   */
  maxDistance?: number;
}

/**
 * How a part of a gesture's momentum slows from its release speed.
 *
 * See `mln_momentum_decay_model` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const MomentumDecayModel = {
  /**
   * The speed halves every `half_life_ms`, until it drops to `stop_speed`.
   * UIScrollView's normal deceleration is a half-life of 346 ms, and its fast
   * deceleration 69 ms.
   */
  friction: 0,
  /**
   * The part eases `distance` over `duration_ms`, along `easing`, as a camera
   * animation does. The caller plans the glide, for example from the platform's
   * own fling physics, such as Android's OverScroller.
   */
  ease: 1,
} as const;
/** One of the MomentumDecayModel values, or a value this binding does not know. */
export type MomentumDecayModel =
  | (typeof MomentumDecayModel)[keyof typeof MomentumDecayModel]
  | (number & {});

/**
 * Where a part settles after a gesture that changed it.
 *
 * The targets are every `spacing` from `offset`, in the part's unit. Bearing
 * targets wrap around the circle. Once the part's glide ends, or right away
 * when it doesn't move, the part eases to the nearest target if it is within
 * `tolerance` of it, and otherwise stays where it is. A setting whose bit isn't
 * set takes the part's default, which `GestureMomentum` lists.
 *
 * See `mln_momentum_snap` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface MomentumSnap {
  /**
   * Positive.
   */
  spacing?: number;
  /**
   * Defaults to 0.
   */
  offset?: number;
  /**
   * Not negative.
   */
  tolerance?: number;
  /**
   * Zero settles without easing. Defaults to 300.
   */
  durationMs?: number;
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
 * An image that the session allocates, renders at the map's scale factor, and
 * reads back with `RenderSessionHandle.readTexture()`.
 *
 * See `mln_offscreen_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface OffscreenTarget {
  logicalSize: LogicalSize;
}

/**
 * An OpenHarmony window to present to.
 *
 * The session takes its own reference to the window, so you can release yours
 * once the call returns. When the platform destroys the window, such as in an
 * `XComponent`'s `onSurfaceDestroyed` callback, release the target or give the
 * session another window without waiting, as for an Android window.
 *
 * See `mln_ohos_window_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface OhosWindowTarget {
  logicalSize: LogicalSize;
  /**
   * Your `OHNativeWindow*`.
   */
  window: bigint;
}

/**
 * The memory of an exported ring's acquired frame, as an opaque file
 * descriptor.
 *
 * The memory holds a 2D Vulkan image of `format`, with one mip level, one
 * layer, optimal tiling, and the usage VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
 * VK_IMAGE_USAGE_SAMPLED_BIT, and VK_IMAGE_USAGE_TRANSFER_SRC_BIT, which a
 * Vulkan image that imports it has too. In OpenGL, import it with
 * glImportMemoryFdEXT() and GL_HANDLE_TYPE_OPAQUE_FD_EXT, and make the texture
 * with GL_TEXTURE_TILING_EXT set to GL_OPTIMAL_TILING_EXT and
 * glTexStorageMem2DEXT() at offset 0. Each frame leaves the image in
 * VK_IMAGE_LAYOUT_GENERAL, released to VK_QUEUE_FAMILY_EXTERNAL.
 *
 * With `RenderSessionAttachOptions.keepDepth`, on a device whose
 * `GpuDeviceCapabilities.depthExportKinds` has `GpuExportKind.opaqueFd`, the
 * depth members share the memory of the frame's depth image in the same way.
 * Otherwise `depth_fd` is -1 and the others are zero. The depth image is 2D, of
 * `depth_format`, with one mip level, one layer, optimal tiling, and the usage
 * VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_USAGE_SAMPLED_BIT, and
 * VK_IMAGE_USAGE_TRANSFER_SRC_BIT. Each frame leaves it in
 * VK_IMAGE_LAYOUT_GENERAL, released to VK_QUEUE_FAMILY_EXTERNAL. In OpenGL,
 * make its texture with the internal format that `DepthStencilFormatFlag` names
 * for `depth_format`.
 *
 * See `mln_opaque_fd_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface OpaqueFdFrame {
  readonly width: number;
  readonly height: number;
  readonly scaleFactor: number;
  /**
   * The texture's index in the ring.
   */
  readonly slot: number;
  /**
   * The file descriptor of the image's memory.
   */
  readonly fd: number;
  /**
   * The size of the memory in bytes.
   */
  readonly allocationSize: bigint;
  /**
   * Whether the memory is the image's own, which an import says, such as with
   * GL_DEDICATED_MEMORY_OBJECT_EXT.
   */
  readonly dedicated: boolean;
  /**
   * The image's `VkFormat`: VK_FORMAT_R8G8B8A8_UNORM, OpenGL's GL_RGBA8.
   */
  readonly format: number;
  /**
   * The file descriptor of the depth image's memory.
   */
  readonly depthFd: number;
  /**
   * The size of the depth image's memory in bytes.
   */
  readonly depthAllocationSize: bigint;
  /**
   * Whether the memory is the depth image's own.
   */
  readonly depthDedicated: boolean;
  /**
   * The depth image's `VkFormat`.
   */
  readonly depthFormat: number;
}

/**
 * An OpenGL texture of a borrowed ring.
 *
 * See `mln_open_gl_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface OpenGlBorrowedTexture {
  /**
   * Your texture name.
   */
  texture: number;
}

/**
 * A ring of your OpenGL textures to render into, on an OpenGL device that
 * `GpuDeviceHandle.wrapOpenGl()` made from your context.
 *
 * `physical_width` and `physical_height` give the textures' size in pixels,
 * which is usually `logical_size` times the map's scale factor. Each texture
 * must be in your context's share group, with RGBA8 storage of that size. The
 * ring holds up to `TEXTURE_RING_MAX_DEPTH` distinct textures.
 *
 * Give the ring `depth_textures` to draw into each frame depth-tested against
 * the map, as `FrameProjection` says: one for each of `textures`, at the same
 * index, or none. Each frame clears the one at its slot, and renders the map's
 * depth and stencil into it. They must be `GL_TEXTURE_2D` textures in the share
 * group, with storage of the size of `textures` in a format of
 * `GpuDeviceCapabilities.depthStencilFormats`.
 *
 * See `mln_open_gl_borrowed_textures_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface OpenGlBorrowedTexturesTarget {
  logicalSize: LogicalSize;
  physicalWidth: number;
  physicalHeight: number;
  textures: readonly OpenGlBorrowedTexture[];
  /**
   * The textures' target. It must be `GL_TEXTURE_2D`.
   */
  target: number;
  depthTextures?: readonly OpenGlBorrowedTexture[];
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
 * Your OpenGL context, for `GpuDeviceHandle.wrapOpenGl()`: the root of the
 * share group that the device's sessions join.
 *
 * See `mln_open_gl_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export type OpenGlContextDescriptor =
  | { readonly kind: "wgl"; readonly value: WglContextDescriptor }
  | { readonly kind: "egl"; readonly value: EglContextDescriptor }
  | { readonly kind: "webGl"; readonly value: WebGlContextDescriptor };

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
 * Your OpenGL surface to present to, with the context that
 * `GpuDeviceHandle.wrapOpenGl()` wrapped.
 *
 * The session makes its context current on the surface for each frame, and
 * swaps the surface's buffers to present.
 *
 * See `mln_open_gl_surface_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface OpenGlSurfaceTarget {
  logicalSize: LogicalSize;
  /**
   * Your `EGLSurface` on the context's display, with its config, or your `HDC`
   * with the context's pixel format.
   */
  surface: bigint;
}

/**
 * The OpenGL texture of an acquired frame.
 *
 * The depth members are set when the frame has a depth texture, as for
 * `MetalTextureFrame`. Otherwise they are zero.
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
  /**
   * The `GL_TEXTURE_2D` texture that holds the frame's depth and stencil.
   */
  readonly depthTexture: number;
  /**
   * The depth texture's internal format, such as GL_DEPTH24_STENCIL8, or zero
   * for a borrowed ring's.
   */
  readonly depthInternalFormat: number;
}

/**
 * How a gesture's pan keeps moving the map's content after release.
 *
 * See `mln_pan_momentum` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface PanMomentum {
  /**
   * How fast the map's content moved on the screen at release, in logical
   * pixels per second. The pan moves only when this is set.
   */
  velocity?: ScreenPoint;
  decay?: MomentumDecay;
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
 * A style's projection at one zoom: `from` turning into `to`.
 *
 * A zeroed definition is Mercator.
 *
 * See `mln_projection_definition` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface ProjectionDefinition {
  from: ProjectionType;
  to: ProjectionType;
  /**
   * How far `from` has turned into `to`, from 0 to 1. Ignored when `from`
   * equals `to`. A value outside that range holds at the nearer end.
   */
  transition: number;
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
 * A projection that a style's
 * &#91;projection&#93;(https://maplibre.org/maplibre-style-spec/projection/)
 * names.
 *
 * See `mln_projection_type` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const ProjectionType = {
  mercator: 0,
  /**
   * Draws the map as a globe.
   */
  verticalPerspective: 1,
  /**
   * Draws the map as a globe at low zooms, and hands over to Mercator between
   * zoom 11 and 12.
   */
  globe: 2,
} as const;
/** One of the ProjectionType values, or a value this binding does not know. */
export type ProjectionType =
  | (typeof ProjectionType)[keyof typeof ProjectionType]
  | (number & {});

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
 * Your lock on a Vulkan queue that you and the render sessions of a GPU device
 * all submit to.
 *
 * A session holds the lock only while it submits or presents to the queue, or
 * submits the empty batch that drains its work from the queue. It never holds
 * the lock while it waits. Set both `lock` and `unlock`, or neither when you
 * don't submit to the queue yourself.
 *
 * Don't hold the lock while you call the library.
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
 * A render backend.
 *
 * See `mln_render_backend` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export const RenderBackend = {
  metal: 1,
  vulkan: 2,
  openGl: 3,
  webGpu: 4,
} as const;
/** One of the RenderBackend values, or a value this binding does not know. */
export type RenderBackend =
  | (typeof RenderBackend)[keyof typeof RenderBackend]
  | (number & {});

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
  /**
   * Where a rendered frame drew the map, for drawing into its frame with depth.
   * All zero for a demand that rendered nothing.
   */
  readonly frameProjection: FrameProjection;
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
   * The target couldn't take a frame, such as while a window is hidden or
   * destroyed, or after `RenderSessionHandle.releaseTarget()`. Demand a frame
   * again later.
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
   * The thread that runs the session's graphics work, which depends on the
   * target:
   *
   * - `RenderDriverKind.callerGraphicsThread` for a borrowed ring of WebGPU
   *   textures.
   * - `RenderDriverKind.coreWorker` for an offscreen target, a window, or an
   *   exported ring.
   * - Either one for a web canvas. In a browser both run on the thread that
   *   loaded the library: `RenderDriverKind.coreWorker` in the library's own
   *   tasks, and `RenderDriverKind.callerGraphicsThread` when you service the
   *   session.
   * - Either one for a Metal layer, a Vulkan surface, an OpenGL surface, or a
   *   borrowed ring of Metal, Vulkan, or OpenGL textures. A build may drive
   *   these on `RenderDriverKind.coreWorker` only, and fails an attach with
   *   `RenderDriverKind.callerGraphicsThread` then.
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
   * Called when the session has something new to draw. Demand a frame at your
   * next display refresh. It isn't called again until you demand a frame, or
   * while the session has no target.
   */
  redrawWake?: Wake;
  /**
   * Called when a session driven by `RenderDriverKind.callerGraphicsThread` has
   * work for `RenderSessionHandle.serviceDriverWork()`. It isn't called again
   * until a service runs.
   */
  driverWorkWake?: Wake;
  /**
   * Whether an exported ring keeps the depth and stencil of each frame for you,
   * so that you can draw into the frame depth-tested against the map, as
   * `FrameProjection` says. The getter of the device's API, such as
   * `AcquiredFrameHandle.withVulkanTexture()`, gets a frame's depth texture on
   * the device, and `AcquiredFrameHandle.withOpaqueFd()` shares it across APIs
   * when `GpuDeviceCapabilities.depthExportKinds` has the ring's kind. Keeping
   * depth costs memory bandwidth, most on tile-based GPUs. A borrowed ring
   * keeps depth in the textures that you give it instead, and the other targets
   * keep none, so an attach to them with `keep_depth` fails with
   * `Status.unsupported`.
   */
  keepDepth?: boolean;
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
   * The kinds of `GpuSync` that `AcquiredFrameHandle.release()` accepts. They
   * depend on the target and on what the device supports.
   */
  readonly consumerSyncKinds: GpuSyncKindFlag;
  /**
   * How an exported ring shares its textures, which says which getter gets an
   * acquired frame's texture, or `GpuExportKind.none` for another target.
   */
  readonly exportKind: GpuExportKind;
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
 * What a render session renders into: one kind of target, and its descriptor.
 *
 * Every target has a size in logical pixels, which sets the map's size. The
 * session renders it at the map's scale factor. A borrowed or exported ring
 * hands each frame to you: acquire it with
 * `RenderSessionHandle.acquireFrame()`, and release it with your GPU's
 * synchronization once you have read it.
 *
 * The session uses the objects that the descriptor names until a replacement or
 * release of the target completes, a detach or release of the session
 * completes, or an abandon of it returns, except as a kind's descriptor says. A
 * window target presents without waiting for the display, so demand at most one
 * frame per display refresh.
 *
 * See `mln_render_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export type RenderTarget =
  | { readonly kind: "offscreen"; readonly value: OffscreenTarget }
  | { readonly kind: "metalLayer"; readonly value: MetalLayerTarget }
  | { readonly kind: "win32Window"; readonly value: Win32WindowTarget }
  | { readonly kind: "xlibWindow"; readonly value: XlibWindowTarget }
  | { readonly kind: "waylandSurface"; readonly value: WaylandSurfaceTarget }
  | { readonly kind: "androidWindow"; readonly value: AndroidWindowTarget }
  | { readonly kind: "ohosWindow"; readonly value: OhosWindowTarget }
  | { readonly kind: "vulkanSurface"; readonly value: VulkanSurfaceTarget }
  | { readonly kind: "openGlSurface"; readonly value: OpenGlSurfaceTarget }
  | {
      readonly kind: "metalBorrowedTextures";
      readonly value: MetalBorrowedTexturesTarget;
    }
  | {
      readonly kind: "vulkanBorrowedTextures";
      readonly value: VulkanBorrowedTexturesTarget;
    }
  | {
      readonly kind: "openGlBorrowedTextures";
      readonly value: OpenGlBorrowedTexturesTarget;
    }
  | {
      readonly kind: "webGpuBorrowedTextures";
      readonly value: WebGpuBorrowedTexturesTarget;
    }
  | { readonly kind: "exportedRing"; readonly value: ExportedRingTarget }
  | { readonly kind: "webCanvas"; readonly value: WebCanvasTarget };

/**
 * A render target kind, as a bit in a set.
 *
 * See `mln_render_target_kind_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const RenderTargetKindFlag = {
  offscreen: 1,
  metalLayer: 2,
  win32Window: 4,
  xlibWindow: 8,
  waylandSurface: 16,
  androidWindow: 32,
  ohosWindow: 64,
  vulkanSurface: 512,
  openGlSurface: 1024,
  metalBorrowedTextures: 2048,
  vulkanBorrowedTextures: 4096,
  openGlBorrowedTextures: 8192,
  webGpuBorrowedTextures: 16384,
  exportedRing: 32768,
  webCanvas: 65536,
} as const;
/** A bitwise OR of RenderTargetKindFlag flags. */
export type RenderTargetKindFlag = number;

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
   * request that no rule matches keeps its URL.
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
 * before any fragment. It keeps the parameters that the URL already has, even
 * one with the same name.
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
  readonly source: CameraChangeSource;
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
  mapGlyphsRequested: 524288n,
  mapGlyphsLoaded: 1048576n,
  mapGlyphsError: 2097152n,
  mapSpriteRequested: 4194304n,
  mapSpriteLoaded: 8388608n,
  mapSpriteError: 16777216n,
  allMapEvents: 33161214n,
  allRuntimeEvents: 393216n,
  all: 33554430n,
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
 * Only maps in `MapMode.continuous` raise
 * `RuntimeEventType.mapLoadingFinished`, `RuntimeEventType.mapIdle`, and the
 * render frame and render map events. Offline region events come only for the
 * regions and types that `RuntimeHandle.setOfflineRegionObserved()` selects.
 *
 * See `mln_runtime_event_type` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const RuntimeEventType = {
  mapCameraWillChange: 1,
  mapCameraIsChanging: 2,
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
   * The map failed to load a style.
   */
  mapLoadingFailed: 7,
  /**
   * The map rendered a complete frame and has nothing left to load, animate, or
   * redraw.
   */
  mapIdle: 8,
  mapRenderError: 9,
  mapRenderFrameStarted: 10,
  mapRenderFrameFinished: 11,
  mapRenderMapStarted: 12,
  mapRenderMapFinished: 13,
  /**
   * The style uses an image that it doesn't have.
   */
  mapStyleImageMissing: 14,
  /**
   * A tile moved to another step of loading.
   */
  mapTileAction: 15,
  /**
   * A source in the style changed, such as when its TileJSON loads.
   */
  mapSourceChanged: 16,
  offlineRegionStatusChanged: 17,
  /**
   * A resource that the region needs failed to download.
   */
  offlineRegionResponseError: 18,
  mapGlyphsRequested: 19,
  mapGlyphsLoaded: 20,
  mapGlyphsError: 21,
  mapSpriteRequested: 22,
  mapSpriteLoaded: 23,
  mapSpriteError: 24,
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
   * The path of the cache database file, which holds the ambient cache and
   * offline regions. Its directory must be writable. Empty keeps the cache in
   * memory.
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
   * A GeoJSON source's options, with every field set.
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
  customGeometry: 7,
  customVector: 8,
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
 * A vector in four dimensions, such as a point in homogeneous coordinates.
 *
 * See `mln_vec4` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface Vec4 {
  readonly x: number;
  readonly y: number;
  readonly z: number;
  readonly w: number;
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
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface VulkanBorrowedTexture {
  /**
   * Your `VkImage`.
   */
  image: bigint;
  /**
   * A 2D `VkImageView` of `image` with one mip level and one layer.
   */
  imageView: bigint;
}

/**
 * A ring of your Vulkan images to render into, on the Vulkan device that you
 * attach on.
 *
 * `physical_width` and `physical_height` give the images' size in pixels, which
 * is usually `logical_size` times the map's scale factor. Each image must have
 * that size, `format`, and `VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT`, and belong to
 * the device's `VkDevice`. The ring holds up to `TEXTURE_RING_MAX_DEPTH`
 * distinct images.
 *
 * Give the ring `depth_textures` to draw into each frame depth-tested against
 * the map, as `FrameProjection` says: one for each of `textures`, at the same
 * index, or none. Each frame clears the one at its slot, renders the map's
 * depth and stencil into it, stores them, and leaves it in
 * `depth_final_layout`, with `owner` as for the color image. They must have the
 * size of `textures`, `depth_format`, one sample, and
 * `VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT`, and their views both the depth
 * and the stencil aspect.
 *
 * See `mln_vulkan_borrowed_textures_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface VulkanBorrowedTexturesTarget {
  logicalSize: LogicalSize;
  physicalWidth: number;
  physicalHeight: number;
  textures: readonly VulkanBorrowedTexture[];
  /**
   * The images' `VkFormat`.
   */
  format: number;
  /**
   * The `VkImageLayout` that each frame leaves its image in.
   */
  finalLayout: number;
  owner?: VulkanImageOwner;
  depthTextures?: readonly VulkanBorrowedTexture[];
  /**
   * The depth images' `VkFormat`, of
   * `GpuDeviceCapabilities.depthStencilFormats`.
   */
  depthFormat?: number;
  /**
   * The `VkImageLayout` that each frame leaves its depth image in.
   */
  depthFinalLayout?: number;
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
   * you submit to it too, give the device your `QueueLock`.
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
 * Your Vulkan device and queue, for `GpuDeviceHandle.wrapVulkan()`.
 *
 * See `mln_vulkan_device_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
 */
export interface VulkanDeviceDescriptor {
  context: VulkanContextDescriptor;
  /**
   * Your lock on `context.graphics_queue`, when you submit to it yourself. Give
   * every device that wraps the queue the same lock.
   */
  queueLock?: QueueLock;
  /**
   * The names of the device extensions that you enabled on `context.device`,
   * such as "VK_KHR_external_semaphore_fd". Vulkan offers no way to ask a
   * device which extensions it has, so the device offers the export kinds and
   * `GpuSyncKind.syncFd` only when the extensions that they need are listed
   * here, and none of them when the list is empty.
   */
  enabledExtensions?: readonly string[];
}

/**
 * The queue family that a borrowed image goes to after each frame.
 *
 * With `VulkanImageOwner.external` or `VulkanImageOwner.foreign`, acquire the
 * image on your side before you read it. You don't need to release it back.
 *
 * See `mln_vulkan_image_owner` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const VulkanImageOwner = {
  /**
   * The image stays with the queue family of the device's queue.
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
 * Your Vulkan surface to present to.
 *
 * Make it with the instance of the Vulkan device that you attach on. The
 * device's queue family must support presenting to the surface. The session
 * creates the swapchain, and destroys it but not the surface.
 *
 * See `mln_vulkan_surface_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface VulkanSurfaceTarget {
  logicalSize: LogicalSize;
  /**
   * Your `VkSurfaceKHR`.
   */
  surface: bigint;
}

/**
 * The Vulkan image of an acquired frame.
 *
 * The depth members are set when the frame has a depth image, as for
 * `MetalTextureFrame`. Otherwise they are zero.
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
  /**
   * The VkImage that holds the frame's depth and stencil.
   */
  readonly depthImage: bigint;
  /**
   * A VkImageView of the depth image's depth and stencil aspects.
   */
  readonly depthImageView: bigint;
  /**
   * The depth image's VkFormat.
   */
  readonly depthFormat: number;
  /**
   * The VkImageLayout that the frame left the depth image in.
   */
  readonly depthLayout: number;
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
 * A Wayland surface to present to.
 *
 * Keep the display connected until the session's detach completes. On an OpenGL
 * device, if you also use EGL on the display, initialize it before you attach.
 *
 * See `mln_wayland_surface_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface WaylandSurfaceTarget {
  logicalSize: LogicalSize;
  /**
   * Your `wl_display*`.
   */
  display: bigint;
  /**
   * Your `wl_surface*`, on `display`.
   */
  surface: bigint;
}

/**
 * A canvas to present to.
 *
 * The session makes the canvas's "webgpu" or "webgl2" context with the device's
 * backend, and sets the canvas's size to `logical_size` at the map's scale
 * factor. Attached without a device, it renders with the build's backend: a
 * WebGPU build asks the browser for a device, and attaches once it has one. For
 * an OpenGL device that wraps your WebGL2 context, the canvas must be the
 * context's own.
 *
 * See `mln_web_canvas_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface WebCanvasTarget {
  logicalSize: LogicalSize;
  /**
   * An `OffscreenCanvas`, such as one from
   * `HTMLCanvasElement.transferControlToOffscreen()` that you posted to the
   * library's Web Worker, or an `HTMLCanvasElement` when the library runs on
   * the page's thread.
   */
  canvas: number;
  /**
   * For a WebGPU device, the `GPUTextureFormat` to configure the canvas with,
   * such as `bgra8unorm`. Empty for a format that the device's adapter prefers.
   */
  format?: string;
}

/**
 * The WebGL2 context that an OpenGL device wraps.
 *
 * See `mln_web_gl_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface WebGlContextDescriptor {
  /**
   * Your `WebGL2RenderingContext`.
   */
  context: number;
}

/**
 * A WebGPU texture of a borrowed ring.
 *
 * See `mln_web_gpu_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface WebGpuBorrowedTexture {
  /**
   * Your `GPUTexture`.
   */
  texture: number;
}

/**
 * A ring of your WebGPU textures to render into, on a WebGPU device that wraps
 * yours.
 *
 * `physical_width` and `physical_height` give the textures' size in pixels,
 * which is usually `logical_size` times the map's scale factor. Each texture
 * must be 2D with that size, `format`, one layer, one mip level, one sample,
 * and `GPUTextureUsage.RENDER_ATTACHMENT`. The ring holds up to
 * `TEXTURE_RING_MAX_DEPTH` distinct textures.
 *
 * Give the ring `depth_textures` to draw into each frame depth-tested against
 * the map, as `FrameProjection` says: one for each of `textures`, at the same
 * index, or none. Each frame clears the one at its slot, renders the map's
 * depth and stencil into it, and stores them. They must be 2D with the size of
 * `textures`, `depth_format`, one layer, one mip level, one sample, and
 * `GPUTextureUsage.RENDER_ATTACHMENT`.
 *
 * The ring renders on a device from `GpuDeviceHandle.wrapWebGpu()`, which no
 * build has yet: an attach with it fails with `Status.unsupported`.
 *
 * See `mln_web_gpu_borrowed_textures_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface WebGpuBorrowedTexturesTarget {
  logicalSize: LogicalSize;
  physicalWidth: number;
  physicalHeight: number;
  textures: readonly WebGpuBorrowedTexture[];
  /**
   * The textures' `GPUTextureFormat`, such as `rgba8unorm`.
   */
  format: string;
  depthTextures?: readonly WebGpuBorrowedTexture[];
  /**
   * The depth textures' `GPUTextureFormat`, of
   * `GpuDeviceCapabilities.depthStencilFormats`.
   */
  depthFormat?: string;
}

/**
 * The WebGPU device that a render session uses.
 *
 * See `mln_web_gpu_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface WebGpuContextDescriptor {
  /**
   * Your `GPUDevice`. Sessions submit to its `queue`.
   */
  device: number;
}

/**
 * The WebGPU texture of an acquired frame.
 *
 * The depth members are set when the frame has a depth texture, as for
 * `MetalTextureFrame`. Otherwise they are zero.
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
   * The `GPUTexture`.
   */
  readonly texture: number;
  /**
   * The texture's `GPUDevice`.
   */
  readonly device: number;
  /**
   * The texture's `GPUTextureFormat`.
   */
  readonly format: string;
  /**
   * The `GPUTexture` that holds the frame's depth and stencil.
   */
  readonly depthTexture: number;
  /**
   * The depth texture's `GPUTextureFormat`.
   */
  readonly depthFormat: string;
}

/**
 * Your WGL context, which each session's context shares objects with.
 *
 * See `mln_wgl_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface WglContextDescriptor {
  /**
   * An `HDC` with the pixel format of `share_context`.
   */
  deviceContext: bigint;
  /**
   * Your `HGLRC`.
   */
  shareContext: bigint;
  /**
   * Your `wglGetProcAddress`, or null to use the system's.
   */
  getProcAddress?: bigint;
}

/**
 * A Win32 window to present to.
 *
 * On an OpenGL device, the session sets the window's pixel format, so don't set
 * one yourself. While the window is minimized, frame demands finish with
 * `RenderResult.targetNotReady`.
 *
 * See `mln_win32_window_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface Win32WindowTarget {
  logicalSize: LogicalSize;
  /**
   * Your `HWND`.
   */
  window: bigint;
}

/**
 * An X11 window to present to, through Xlib.
 *
 * Keep the display connected until the session's detach completes.
 *
 * See `mln_xlib_window_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface XlibWindowTarget {
  logicalSize: LogicalSize;
  /**
   * Your `Display*`.
   */
  display: bigint;
  /**
   * Your `Window`, on `display`.
   */
  window: bigint;
}

/**
 * The size of a diagnostic message in bytes, with its terminator.
 *
 * See `MLN_DIAGNOSTIC_MESSAGE_CAPACITY` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export const DIAGNOSTIC_MESSAGE_CAPACITY: number = 4096;

/**
 * The token of the frame results of the demand that a session holds for an
 * image of `MapHandle.requestStillImage()`, which says how that demand runs.
 * Your own demands can't use it.
 *
 * See `MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
export const FRAME_DEMAND_TOKEN_STILL_IMAGE: bigint = 18446744073709551615n;

/**
 * The most textures that a render session's texture ring can have.
 *
 * See `MLN_TEXTURE_RING_MAX_DEPTH` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const TEXTURE_RING_MAX_DEPTH: number = 8;

/**
 * The null Vulkan non-dispatchable handle.
 *
 * See `MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const VULKAN_NON_DISPATCHABLE_HANDLE_NULL: bigint = 0n;
