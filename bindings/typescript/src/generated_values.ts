// Generated from the C headers by tools/bindgen. Do not edit.

import type {
  ResourceRequestHandle,
  ResourceTransformRequestHandle,
} from "./generated_operations.ts";
import {
  type NativeRegistration,
  nativeRegistration,
} from "./internal/native_registrations.ts";

export const AmbientCacheOperation = {
  resetDatabase: 1,
  packDatabase: 2,
  invalidate: 3,
  clear: 4,
} as const;
/** One of the AmbientCacheOperation values, or a value this binding does not know. */
export type AmbientCacheOperation =
  | (typeof AmbientCacheOperation)[keyof typeof AmbientCacheOperation]
  | (number & {});

/**
 * Optional animation controls for camera transitions.
 *
 * See `mln_animation_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface AnimationOptions {
  /**
   * Duration in milliseconds. Must be finite, non-negative, and short enough to
   * represent as a duration.
   */
  durationMs?: number;
  /**
   * Average fly velocity in screenfuls per second. Must be positive and finite;
   * absent selects 1.2. A flight whose derived duration is too long to
   * represent fails its completion with `Status.nativeError`. An explicit
   * duration overrides velocity.
   */
  velocity?: number;
  /**
   * Peak zoom for flyTo transitions.
   */
  minZoom?: number;
  easing?: UnitBezier;
  /**
   * Caller-chosen identity of the command that these options animate, which
   * `MapHandle.cancelCameraTransition()` matches.
   */
  transitionId?: bigint;
  /**
   * Reports the end of the command's transitions. Disabled by default.
   */
  endHandler?: CameraTransitionHandler;
}

/**
 * Optional map camera constraint fields.
 *
 * See `mln_bound_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface BoundOptions {
  /**
   * Geographic bounds that the camera center stays inside.
   */
  bounds?: LatLngBounds;
  minZoom?: number;
  maxZoom?: number;
  minPitch?: number;
  maxPitch?: number;
  /**
   * Selects the unbounded geographic constraint, which leaves every camera
   * center unconstrained and lets the map pan freely across the antimeridian.
   * This differs from world bounds of -90/-180 to 90/180, which clamp longitude
   * to that range. Mutually exclusive with `BoundOptions.bounds`, which it
   * leaves unread.
   */
  unbounded?: boolean;
}

/**
 * A complete camera, as native reports it.
 *
 * See `mln_camera` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface Camera {
  center?: LatLng;
  centerAltitude?: number;
  padding?: EdgeInsets;
  zoom?: number;
  bearing?: number;
  pitch?: number;
  roll?: number;
  fieldOfView?: number;
}

/**
 * Camera change kinds reported by camera will-change and did-change events.
 *
 * See `mln_camera_change_mode` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const CameraChangeMode = {
  /**
   * The camera reached its new value without an animated transition.
   */
  immediate: 0,
  /**
   * The camera moved as part of an animated transition.
   */
  animated: 1,
} as const;
/** One of the CameraChangeMode values, or a value this binding does not know. */
export type CameraChangeMode =
  | (typeof CameraChangeMode)[keyof typeof CameraChangeMode]
  | (number & {});

/**
 * One atomic relative camera update.
 *
 * See `mln_camera_delta` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface CameraDelta {
  /**
   * Pan in logical map pixels; the content moves by this offset.
   */
  offset?: ScreenPoint;
  /**
   * Positive zoom factor; 2 zooms in one level.
   */
  scale?: number;
  /**
   * Degrees added to the bearing.
   */
  bearing?: number;
  /**
   * Degrees added to the pitch; positive tilts further from straight down.
   */
  pitch?: number;
  /**
   * Screen point in logical map pixels that scale, bearing, and pitch keep
   * fixed.
   */
  anchor?: ScreenPoint;
  animation?: AnimationOptions;
  gesturePhase?: GesturePhase;
}

/**
 * Optional fitting controls for camera-for-viewport queries.
 *
 * See `mln_camera_fit_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface CameraFitOptions {
  padding?: EdgeInsets;
  bearing?: number;
  pitch?: number;
}

/**
 * Camera fields used by snapshots and camera updates.
 *
 * See `mln_camera_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface CameraOptions {
  center?: LatLng;
  centerAltitude?: number;
  padding?: EdgeInsets;
  /**
   * Optional screen-space focal point in logical map pixels.
   */
  anchor?: ScreenPoint;
  zoom?: number;
  bearing?: number;
  pitch?: number;
  roll?: number;
  fieldOfView?: number;
}

/**
 * Camera result of an ordered camera query.
 *
 * See `mln_camera_query_result` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface CameraQueryResult {
  readonly generation: bigint;
  readonly camera: Camera;
}

/**
 * The end of one camera command's transitions.
 *
 * See `mln_camera_transition_end` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface CameraTransitionEnd {
  readonly outcome: CameraTransitionOutcome;
  /**
   * Generation of the published snapshot that shows the camera where the
   * transitions left it. The map events of the change that ended them carry
   * this generation and are queued before the callback runs. Zero for
   * `CameraTransitionOutcome.closed`.
   */
  readonly generation: bigint;
}

/**
 * Callback that one camera command copies to report the end of its transitions.
 * The callback runs asynchronously on the JavaScript thread that registered it,
 * after native queues the call.
 *
 * See `mln_camera_transition_handler` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export type CameraTransitionHandler = (end: CameraTransitionEnd) => void;

/**
 * How the transitions of one camera command ended.
 *
 * See `mln_camera_transition_outcome` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const CameraTransitionOutcome = {
  /**
   * Every property of the command reached its target, or a later camera write
   * replaced it.
   */
  completed: 0,
  /**
   * `MapHandle.cancelTransitions()`, `MapHandle.cancelCameraTransition()`, or
   * `GesturePhase.cancel` ended the transitions, or the command failed before
   * it started them.
   */
  cancelled: 1,
  /**
   * The map closed before the transitions ended.
   */
  closed: 2,
} as const;
/** One of the CameraTransitionOutcome values, or a value this binding does not know. */
export type CameraTransitionOutcome =
  | (typeof CameraTransitionOutcome)[keyof typeof CameraTransitionOutcome]
  | (number & {});

/**
 * One atomic absolute camera update.
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
 * Camera transition behavior for `CameraUpdate`.
 *
 * See `mln_camera_update_mode` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const CameraUpdateMode = {
  jump: 0,
  ease: 1,
  fly: 2,
} as const;
/** One of the CameraUpdateMode values, or a value this binding does not know. */
export type CameraUpdateMode =
  | (typeof CameraUpdateMode)[keyof typeof CameraUpdateMode]
  | (number & {});

/**
 * Canonical tile identity used by custom geometry and custom MVT vector source
 * callbacks.
 *
 * See `mln_canonical_tile_id` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface CanonicalTileId {
  z?: number;
  x?: number;
  y?: number;
}

/**
 * Map constraint modes used by `MapViewportOptions`.
 *
 * See `mln_constrain_mode` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const ConstrainMode = {
  none: 0,
  heightOnly: 1,
  widthAndHeight: 2,
  screen: 3,
} as const;
/** One of the ConstrainMode values, or a value this binding does not know. */
export type ConstrainMode =
  | (typeof ConstrainMode)[keyof typeof ConstrainMode]
  | (number & {});

/**
 * Options for custom geometry sources. The callback runs asynchronously on the
 * JavaScript thread that registered it, after native queues the call.
 *
 * See `mln_custom_geometry_source_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface CustomGeometrySourceOptions {
  /**
   * Tile fetch callback.
   */
  fetchTile: (tileId: CanonicalTileId) => void;
  /**
   * Optional best-effort tile cancel callback.
   */
  cancelTile?: (tileId: CanonicalTileId) => void;
  minZoom?: number;
  maxZoom?: number;
  tolerance?: number;
  tileSize?: number;
  buffer?: number;
  clip?: boolean;
  wrap?: boolean;
}

/**
 * Options for custom MVT vector sources. The callback runs asynchronously on
 * the JavaScript thread that registered it, after native queues the call.
 *
 * See `mln_custom_mvt_vector_source_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface CustomMvtVectorSourceOptions {
  /**
   * Tile fetch callback.
   */
  fetchTile: (tileId: CanonicalTileId) => void;
  /**
   * Optional best-effort tile cancel callback.
   */
  cancelTile?: (tileId: CanonicalTileId) => void;
  minZoom?: number;
  maxZoom?: number;
}

/**
 * Screen-space inset in logical map pixels.
 *
 * See `mln_edge_insets` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface EdgeInsets {
  top?: number;
  left?: number;
  bottom?: number;
  right?: number;
}

/**
 * EGL context fields shared by OpenGL render targets.
 *
 * See `mln_egl_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface EglContextDescriptor {
  /**
   * Borrowed EGLDisplay. Required and kept initialized through teardown.
   * eglGetDisplay(EGL_DEFAULT_DISPLAY) returns the same display for the whole
   * process, so a shared display must stay initialized until every session
   * using it has detached or been destroyed. The host must not call
   * eglTerminate per session.
   */
  display: bigint;
  /**
   * Borrowed EGLConfig used to create the session context. Required. OpenGL
   * texture targets require EGL_SURFACE_TYPE to include EGL_PBUFFER_BIT.
   */
  config: bigint;
  /**
   * Borrowed EGLContext whose share group the session context joins. Required
   * under shared ownership, where the session also takes its client API from
   * this context. A dedicated session joins no share group, so it must be null
   * there and names client_api instead.
   */
  shareContext?: bigint;
  /**
   * Client API the session creates its context for. Required under dedicated
   * ownership. A shared session queries share_context for it, so this is
   * ignored there.
   */
  clientApi?: OpenGlClientApi;
  /**
   * Optional eglGetProcAddress-compatible function for the host loader.
   */
  getProcAddress?: bigint;
}

/**
 * Feature-state source, feature, and key selector.
 *
 * See `mln_feature_state_selector` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface FeatureStateSelector {
  /**
   * Source ID. Required.
   */
  sourceId: string;
  /**
   * Source layer ID, or empty for none. Required for vector-source
   * disambiguation.
   */
  sourceLayerId?: string;
  /**
   * Optional feature ID. Required by set and get, optional for remove. A
   * numeric feature ID matches its decimal text, such as "7" for the GeoJSON id
   * 7. Its field bit tells an absent ID from the empty ID "".
   */
  featureId?: string;
  /**
   * State key, or empty to select every key. Used only by remove, and requires
   * feature_id.
   */
  stateKey?: string;
}

/**
 * One nonblocking request for a frame.
 *
 * See `mln_frame_demand` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
export interface FrameDemand {
  /**
   * Frame-demand policy. Zero renders only when a newer map update exists,
   * without presenting.
   */
  flags?: FrameDemandFlag;
  /**
   * Host identity returned with the terminal frame result. Any value except the
   * maximum uint64_t value.
   */
  token?: bigint;
}

/**
 * Frame-demand policy bits.
 *
 * See `mln_frame_demand_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
export const FrameDemandFlag = {
  /**
   * Render even when no map update is newer than the last rendered one.
   */
  force: 1,
  /**
   * Present the rendered frame on a target that supports presentation. A
   * presenting target whose demand clears this bit still renders and keeps
   * whatever it presented last. Ignored by targets without presentation.
   */
  present: 2,
  /**
   * Without `FrameDemandFlag.force`, a demand that would finish with
   * `RenderResult.noUpdate` waits instead, and runs again after the map's next
   * update, a target replacement, or an applied resize. A waiting demand holds
   * no ring slot. A later demand with the same flags supersedes it, a barrier
   * ends its wait with `RenderResult.noUpdate`, and detach, abandon, or the
   * quarantine of the ring's last usable slot end it with
   * `RenderResult.targetNotReady`. A waiting demand's result can follow the
   * results of demands accepted after it. The flag does not pace: a host that
   * re-arms a waiting demand as each result arrives renders every update the
   * map publishes, and a command that arrives while a frame renders appears in
   * a later frame. To show several commands in one frame, submit them in a
   * command group, or arm demands once per display frame.
   */
  waitForUpdate: 4,
} as const;
/** A bitwise OR of FrameDemandFlag flags. */
export type FrameDemandFlag = number;

/**
 * Free camera position and orientation in MapLibre Native camera space.
 *
 * See `mln_free_camera_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface FreeCameraOptions {
  position?: Vec3;
  orientation?: Quaternion;
}

/**
 * Options for GeoJSON sources, indexed and rendered at a fixed 512-pixel tile
 * size.
 *
 * See `mln_geo_json_source_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface GeoJsonSourceOptions {
  /**
   * Minimum tiling zoom. Absent selects 0.
   */
  minZoom?: number;
  /**
   * Maximum tiling zoom, an integer within &#91;0, 24&#93;. Absent selects 18.
   */
  maxZoom?: number;
  /**
   * Douglas-Peucker simplification tolerance. Absent selects 0.375.
   */
  tolerance?: number;
  /**
   * Highest zoom that clusters points. Absent selects 17.
   */
  clusterMaxZoom?: number;
  /**
   * Cluster aggregation expressions keyed by property name, as a JSON object
   * whose members follow the MapLibre Style Spec clusterProperties form, or
   * empty for none.
   */
  clusterProperties?: unknown;
  /**
   * Tile buffer in pixels within &#91;0, 512&#93;. Absent selects 128.
   */
  buffer?: number;
  /**
   * Cluster radius in pixels within &#91;0, 4095&#93;. Absent selects 50.
   */
  clusterRadius?: number;
  /**
   * Points required to form a cluster. Absent selects 2.
   */
  clusterMinPoints?: number;
  /**
   * Adds line distance metrics to line features. Absent selects false.
   */
  lineMetrics?: boolean;
  /**
   * Clusters point features. Absent selects false.
   */
  cluster?: boolean;
  /**
   * Tiles the data for the next render update instead of in the background.
   * Absent selects false.
   */
  synchronousTiling?: boolean;
}

/**
 * Gesture boundary carried atomically with a camera update or delta.
 *
 * See `mln_gesture_phase` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const GesturePhase = {
  /**
   * The update carries no gesture boundary and leaves the flag as it is.
   */
  none: 0,
  /**
   * Marks a gesture as in progress before the camera write. It does not cancel
   * running transitions; use `MapHandle.cancelTransitions()` for that.
   */
  begin: 1,
  /**
   * Keeps the gesture marked as in progress before the camera write.
   */
  update: 2,
  /**
   * Clears the gesture flag after the camera write.
   */
  end: 3,
  /**
   * Cancels transitions running after the camera write, then clears the gesture
   * flag.
   */
  cancel: 4,
} as const;
/** One of the GesturePhase values, or a value this binding does not know. */
export type GesturePhase =
  | (typeof GesturePhase)[keyof typeof GesturePhase]
  | (number & {});

/**
 * Backend synchronization copied by frame access and release calls.
 *
 * See `mln_gpu_sync` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface GpuSync {
  kind?: GpuSyncKind;
  /**
   * Bit pattern of the GLsync pointer.
   */
  handle?: bigint;
}

/**
 * Synchronization payload kind for acquired texture frames.
 *
 * See `mln_gpu_sync_kind` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const GpuSyncKind = {
  /**
   * The host needs no synchronization object. The work completed, or on WebGPU
   * was submitted to the device's queue, before the frame became acquirable or
   * before the release call.
   */
  cpuComplete: 0,
  /**
   * GLsync in the shared EGL or WGL context's share group, used only by a
   * caller-graphics-thread driver. handle is nonzero.
   */
  openGlFence: 1,
} as const;
/** One of the GpuSyncKind values, or a value this binding does not know. */
export type GpuSyncKind =
  | (typeof GpuSyncKind)[keyof typeof GpuSyncKind]
  | (number & {});

/**
 * One HTTP header that a header rule supplies.
 *
 * See `mln_http_header` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface HttpHeader {
  /**
   * Header field name. Required.
   */
  name: string;
  value?: string;
}

/**
 * One rule of a native HTTP header transform.
 *
 * See `mln_http_header_rule` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface HttpHeaderRule {
  /**
   * The resource kinds to match. Zero selects every kind.
   */
  kinds?: ResourceKindFlag;
  flags?: UrlMatchFlag;
  /**
   * The URL or glob pattern to compare.
   */
  url?: string;
  /**
   * The headers to set.
   */
  headers?: readonly HttpHeader[];
}

/**
 * HTTP header transform state: a callback, or native header rules. The callback
 * runs asynchronously on the JavaScript thread that registered it, after native
 * queues the call.
 *
 * See `mln_http_header_transform` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface HttpHeaderTransform {
  /**
   * Header rules for a transform without a callback; empty with one.
   */
  rules?: readonly HttpHeaderRule[];
}

/**
 * Content-box insets in image pixels, measured from the image's top-left.
 *
 * See `mln_image_content` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface ImageContent {
  left?: number;
  top?: number;
  right?: number;
  bottom?: number;
}

/**
 * One stretchable interval along an image axis, in image pixels.
 *
 * See `mln_image_stretch` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface ImageStretch {
  from?: number;
  to?: number;
}

/**
 * Geographic coordinate in degrees used by map and projection APIs.
 *
 * See `mln_lat_lng` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface LatLng {
  /**
   * Latitude in degrees, finite and within &#91;-90, 90&#93; on input.
   */
  latitude?: number;
  /**
   * Longitude in degrees. Input longitude must be finite.
   */
  longitude?: number;
}

/**
 * Geographic bounds in degrees.
 *
 * See `mln_lat_lng_bounds` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface LatLngBounds {
  southwest?: LatLng;
  northeast?: LatLng;
}

/**
 * Log event categories emitted by MapLibre Native.
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
 * Process-global log callback. The callback runs asynchronously on the
 * JavaScript thread that registered it, after native queues the call.
 *
 * See `mln_log_handler` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
export type LogHandler = (
  severity: LogSeverity,
  event: LogEvent,
  code: bigint,
  message: string,
) => void;

/**
 * Log severity values emitted by MapLibre Native.
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
 * Bitmask values for log severities dispatched asynchronously.
 *
 * See `mln_log_severity_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
export const LogSeverityFlag = {
  info: 2,
  warning: 4,
  error: 8,
  defaultValue: 6,
  all: 14,
} as const;
/** A bitwise OR of LogSeverityFlag flags. */
export type LogSeverityFlag = number;

/**
 * Logical extent in UI pixels and the device-pixel scale.
 *
 * See `mln_logical_extent` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface LogicalExtent {
  /**
   * Width in UI pixels.
   */
  width?: number;
  /**
   * Height in UI pixels.
   */
  height?: number;
  /**
   * Device pixels per UI pixel.
   */
  scaleFactor?: number;
}

/**
 * Debug overlay mask values for `MapHandle.setDebugOptions()`.
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
 * Map rendering modes used when creating a map.
 *
 * See `mln_map_mode` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const MapMode = {
  /**
   * Continuously updates as data arrives and map state changes.
   */
  continuous: 0,
  /**
   * Produces one-off still images of an arbitrary viewport.
   */
  staticValue: 1,
  /**
   * Produces one-off still images for a single tile.
   */
  tile: 2,
} as const;
/** One of the MapMode values, or a value this binding does not know. */
export type MapMode = (typeof MapMode)[keyof typeof MapMode] | (number & {});

/**
 * Options used when creating a map.
 *
 * See `mln_map_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface MapOptions {
  /**
   * Initial logical extent. Required. Width and height must be nonzero, and
   * scale_factor must be finite and positive. scale_factor is fixed for the
   * map's lifetime.
   */
  initialExtent: LogicalExtent;
  mapMode?: MapMode;
  /**
   * Decodes MapLibre Tile (MLT) tiles whose integer streams use FastPFOR
   * encodings.
   */
  fastPforEnabled?: boolean;
  /**
   * Map-originated event types this map queues.
   */
  eventMask?: RuntimeEventFlag;
}

/**
 * Immutable map state copied from the latest published generation.
 *
 * See `mln_map_snapshot` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface MapSnapshot {
  /**
   * Debug overlays that the map draws.
   */
  readonly debugOptions: MapDebugFlag;
  readonly generation: bigint;
  /**
   * True once every requested style and tile resource finished loading.
   */
  readonly fullyLoaded: boolean;
  readonly renderingStatsViewEnabled: boolean;
  /**
   * True while the map is inside a gesture.
   */
  readonly gestureInProgress: boolean;
  /**
   * True while a camera command with a duration is still animating.
   */
  readonly transitionInProgress: boolean;
  readonly eventMask: RuntimeEventFlag;
  /**
   * Generation of the latest render update the map published. A rendered frame
   * at or past it draws map state that includes every command this snapshot
   * observes, though animations and resource loads finish in later frames.
   */
  readonly latestRenderUpdateGeneration: bigint;
  readonly tile: MapTileOptions;
  readonly bounds: BoundOptions;
  readonly freeCamera: FreeCameraOptions;
  /**
   * The map's camera, extent, projection mode, and viewport options.
   * `MapProjectionHandle.create()` converts coordinates for this view.
   */
  readonly view: MapView;
}

/**
 * Tile prefetch and LOD tuning controls.
 *
 * See `mln_map_tile_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface MapTileOptions {
  /**
   * Prefetch zoom delta, at most 255.
   */
  prefetchZoomDelta?: number;
  lodMinRadius?: number;
  lodScale?: number;
  lodPitchThreshold?: number;
  lodZoomShift?: number;
  lodMode?: TileLodMode;
}

/**
 * Everything that a projection depends on: the transform that a published
 * snapshot or a rendered frame describes.
 *
 * See `mln_map_view` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface MapView {
  /**
   * The camera. Its center is unwrapped: the longitude keeps the visible world
   * copy and may fall outside -180 to 180.
   */
  camera?: Camera;
  /**
   * Logical size and device-pixel scale. A snapshot carries the map's scale
   * factor, and a frame carries the scale factor of the target that drew it.
   */
  extent?: LogicalExtent;
  /**
   * Axonometric mode and skew, with every field set.
   */
  projectionMode?: ProjectionMode;
  /**
   * North orientation, constrain mode, viewport mode, and frustum offset, with
   * every field set.
   */
  viewport?: MapViewportOptions;
}

/**
 * Live map viewport and render-transform controls.
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
 * One caller-owned Metal texture of a borrowed texture ring.
 *
 * See `mln_metal_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface MetalBorrowedTexture {
  /**
   * Borrowed `id<MTLTexture>` / `MTL::Texture*`. Required.
   */
  texture: bigint;
}

/**
 * Metal attachment options for a borrowed texture target.
 *
 * See `mln_metal_borrowed_texture_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface MetalBorrowedTextureDescriptor {
  /**
   * Logical texture extent. Required. physical_width and physical_height give
   * the texture's size. A scale_factor that differs from the map's is accepted
   * and logged as a warning.
   */
  extent: LogicalExtent;
  /**
   * Physical texture width in device pixels. Required.
   */
  physicalWidth: number;
  /**
   * Physical texture height in device pixels. Required.
   */
  physicalHeight: number;
  /**
   * The ring's textures, one per slot, in slot order. Required.
   */
  textures: readonly MetalBorrowedTexture[];
}

/**
 * Metal backend context fields shared by Metal render targets.
 *
 * See `mln_metal_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface MetalContextDescriptor {
  /**
   * `id<MTLDevice>` / `MTL::Device*`. Retained when the target requires it.
   */
  device?: bigint;
}

/**
 * Metal attachment options for an owned texture target.
 *
 * See `mln_metal_owned_texture_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface MetalOwnedTextureDescriptor {
  /**
   * Logical texture extent. Required. A scale_factor that differs from the
   * map's is accepted and logged as a warning.
   */
  extent: LogicalExtent;
  /**
   * Metal backend context. device is required.
   */
  context?: MetalContextDescriptor;
  /**
   * Backend-native MTLPixelFormat value of the ring's textures: either
   * MTLPixelFormatRGBA8Unorm or MTLPixelFormatBGRA8Unorm. Required. Readback
   * returns RGBA8 for either format.
   */
  pixelFormat: bigint;
}

/**
 * Metal attachment options for a native surface.
 *
 * See `mln_metal_surface_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
export interface MetalSurfaceDescriptor {
  /**
   * Logical surface extent. Required. A scale_factor that differs from the
   * map's is accepted and logged as a warning.
   */
  extent: LogicalExtent;
  /**
   * Metal backend context. device is optional for Metal surfaces.
   */
  context?: MetalContextDescriptor;
  /**
   * `CAMetalLayer*` / `CA::MetalLayer*` retained by the session. Required.
   */
  layer: bigint;
}

/**
 * Metal frame acquired from a texture ring.
 *
 * See `mln_metal_texture_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface MetalTextureFrame {
  /**
   * Session generation that produced this frame.
   */
  readonly generation: bigint;
  /**
   * Physical Metal texture width in device pixels.
   */
  readonly width: number;
  /**
   * Physical Metal texture height in device pixels.
   */
  readonly height: number;
  /**
   * UI-to-device pixel scale used for this frame.
   */
  readonly scaleFactor: number;
  /**
   * Opaque identity of this frame.
   */
  readonly frameId: bigint;
  /**
   * Ring slot that holds this frame. For a borrowed target, the index of its
   * texture in the descriptor's textures array.
   */
  readonly slot: number;
  /**
   * Borrowed `id<MTLTexture>` / `MTL::Texture*`. Valid until frame release.
   */
  readonly texture: bigint;
  /**
   * Borrowed `id<MTLDevice>` / `MTL::Device*`. Valid until frame release.
   */
  readonly device: bigint;
  /**
   * Backend-native pixel format value. Metal uses MTLPixelFormat.
   */
  readonly pixelFormat: bigint;
}

export const NetworkStatus = {
  online: 1,
  offline: 2,
} as const;
/** One of the NetworkStatus values, or a value this binding does not know. */
export type NetworkStatus =
  | (typeof NetworkStatus)[keyof typeof NetworkStatus]
  | (number & {});

/**
 * Map north orientation values used by `MapViewportOptions`.
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
 * Geometry offline region definition.
 *
 * See `mln_offline_geometry_region_definition` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface OfflineGeometryRegionDefinition {
  /**
   * Style URL. Copied during region creation.
   */
  styleUrl?: string;
  /**
   * UTF-8 GeoJSON Geometry. Required.
   */
  geometry: unknown;
  minZoom?: number;
  /**
   * Maximum zoom. Positive infinity lets each tile source use its own maximum
   * zoom.
   */
  maxZoom?: number;
  pixelRatio?: number;
  includeIdeographs?: boolean;
}

/**
 * Tagged offline region definition.
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

export const OfflineRegionDownloadState = {
  inactive: 0,
  active: 1,
} as const;
/** One of the OfflineRegionDownloadState values, or a value this binding does not know. */
export type OfflineRegionDownloadState =
  | (typeof OfflineRegionDownloadState)[keyof typeof OfflineRegionDownloadState]
  | (number & {});

/**
 * Region data delivered by an offline completion.
 *
 * See `mln_offline_region_info` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface OfflineRegionInfo {
  readonly id: bigint;
  readonly definition: OfflineRegionDefinition;
  /**
   * Metadata bytes.
   */
  readonly metadata: Uint8Array;
}

/**
 * Offline region status snapshot.
 *
 * See `mln_offline_region_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface OfflineRegionStatus {
  readonly downloadState: OfflineRegionDownloadState;
  readonly completedResourceCount: bigint;
  readonly completedResourceSize: bigint;
  readonly completedTileCount: bigint;
  readonly requiredTileCount: bigint;
  readonly completedTileSize: bigint;
  readonly requiredResourceCount: bigint;
  readonly requiredResourceCountIsPrecise: boolean;
  readonly complete: boolean;
}

/**
 * Tile-pyramid offline region definition.
 *
 * See `mln_offline_tile_pyramid_region_definition` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface OfflineTilePyramidRegionDefinition {
  /**
   * Style URL. Copied during region creation.
   */
  styleUrl?: string;
  bounds?: LatLngBounds;
  minZoom?: number;
  /**
   * Maximum zoom. Positive infinity lets each tile source use its own maximum
   * zoom.
   */
  maxZoom?: number;
  pixelRatio?: number;
  includeIdeographs?: boolean;
}

/**
 * One caller-owned OpenGL texture of a borrowed texture ring.
 *
 * See `mln_open_gl_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface OpenGlBorrowedTexture {
  /**
   * Borrowed OpenGL texture object name. Required.
   */
  texture: number;
}

/**
 * OpenGL attachment options for a borrowed texture target.
 *
 * See `mln_open_gl_borrowed_texture_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface OpenGlBorrowedTextureDescriptor {
  /**
   * Logical texture extent. Required. physical_width and physical_height give
   * the texture's size. A scale_factor that differs from the map's is accepted
   * and logged as a warning.
   */
  extent: LogicalExtent;
  /**
   * Physical texture width in device pixels. Required.
   */
  physicalWidth: number;
  /**
   * Physical texture height in device pixels. Required.
   */
  physicalHeight: number;
  /**
   * Borrowed OpenGL context provider data. The textures must belong to this
   * context or a context in the same share group.
   */
  context: OpenGlContextDescriptor;
  /**
   * The ring's textures, one per slot, in slot order. Required.
   */
  textures: readonly OpenGlBorrowedTexture[];
  /**
   * OpenGL texture target of every texture. Required: GL_TEXTURE_2D.
   */
  target: number;
}

/**
 * OpenGL client API a dedicated EGL session creates its context for.
 *
 * See `mln_open_gl_client_api` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const OpenGlClientApi = {
  /**
   * No client API is named.
   */
  unspecified: 0,
  /**
   * Desktop OpenGL, as EGL_OPENGL_API names it.
   */
  gl: 1,
  /**
   * OpenGL ES, as EGL_OPENGL_ES_API names it.
   */
  gles: 2,
} as const;
/** One of the OpenGlClientApi values, or a value this binding does not know. */
export type OpenGlClientApi =
  | (typeof OpenGlClientApi)[keyof typeof OpenGlClientApi]
  | (number & {});

/**
 * OpenGL backend context fields shared by OpenGL render targets.
 *
 * See `mln_open_gl_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface OpenGlContextDescriptor {
  /**
   * Whether the session shares its driver thread and graphics objects with the
   * host. A private EGL or WGL owned texture and a transferred WebGL canvas are
   * dedicated to their core worker.
   */
  ownership?: OpenGlContextOwnership;
  data: OpenGlContextDescriptorData;
}

/**
 * Backend-specific OpenGL context data.
 *
 * See `mln_open_gl_context_descriptor_data` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export type OpenGlContextDescriptorData =
  | { readonly kind: "wgl"; readonly value: WglContextDescriptor }
  | { readonly kind: "egl"; readonly value: EglContextDescriptor }
  | { readonly kind: "webGl"; readonly value: WebGlContextDescriptor };

/**
 * How a session's OpenGL context relates to its driver thread and host graphics
 * state.
 *
 * See `mln_open_gl_context_ownership` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const OpenGlContextOwnership = {
  /**
   * The session shares its thread with host graphics work.
   */
  shared: 0,
  /**
   * The session keeps its own OpenGL context current on its thread.
   */
  dedicated: 1,
} as const;
/** One of the OpenGlContextOwnership values, or a value this binding does not know. */
export type OpenGlContextOwnership =
  | (typeof OpenGlContextOwnership)[keyof typeof OpenGlContextOwnership]
  | (number & {});

/**
 * OpenGL context providers supported by this build.
 *
 * See `mln_open_gl_context_provider_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const OpenGlContextProviderFlag = {
  wgl: 1,
  egl: 2,
  /**
   * Browser WebGL context imported into an Emscripten module.
   */
  webGl: 4,
} as const;
/** A bitwise OR of OpenGlContextProviderFlag flags. */
export type OpenGlContextProviderFlag = number;

/**
 * OpenGL attachment options for an owned texture target.
 *
 * See `mln_open_gl_owned_texture_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface OpenGlOwnedTextureDescriptor {
  /**
   * Logical texture extent. Required. A scale_factor that differs from the
   * map's is accepted and logged as a warning.
   */
  extent: LogicalExtent;
  /**
   * Borrowed OpenGL context provider data. Shared ownership creates a context
   * whose texture frames the host can acquire. Dedicated EGL, WGL, or
   * transferred WebGL ownership creates a private core-worker context for CPU
   * readback.
   */
  context: OpenGlContextDescriptor;
}

/**
 * OpenGL attachment options for a native surface.
 *
 * See `mln_open_gl_surface_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
export interface OpenGlSurfaceDescriptor {
  /**
   * Logical surface extent. Required. A scale_factor that differs from the
   * map's is accepted and logged as a warning.
   */
  extent: LogicalExtent;
  /**
   * Borrowed OpenGL context provider data.
   */
  context: OpenGlContextDescriptor;
  /**
   * Borrowed platform surface handle: an HDC for WGL and an EGLSurface for EGL,
   * both required. Null for WebGL, whose context carries its canvas binding.
   */
  surface?: bigint;
}

/**
 * OpenGL frame acquired from a texture ring.
 *
 * See `mln_open_gl_texture_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface OpenGlTextureFrame {
  /**
   * Session generation that produced this frame.
   */
  readonly generation: bigint;
  /**
   * Physical OpenGL texture width in device pixels.
   */
  readonly width: number;
  /**
   * Physical OpenGL texture height in device pixels.
   */
  readonly height: number;
  /**
   * UI-to-device pixel scale used for this frame.
   */
  readonly scaleFactor: number;
  /**
   * Opaque identity of this frame.
   */
  readonly frameId: bigint;
  /**
   * Ring slot that holds this frame. For a borrowed target, the index of its
   * texture in the descriptor's textures array.
   */
  readonly slot: number;
  /**
   * Borrowed OpenGL texture object name. Valid until frame release.
   */
  readonly texture: number;
  /**
   * OpenGL texture target. GL_TEXTURE_2D is the expected target.
   */
  readonly target: number;
  /**
   * OpenGL internal format, such as GL_RGBA8. Zero for a borrowed texture,
   * whose format the host chose.
   */
  readonly internalFormat: number;
  /**
   * OpenGL pixel format, such as GL_RGBA. Zero for a borrowed texture.
   */
  readonly format: number;
  /**
   * OpenGL pixel type, such as GL_UNSIGNED_BYTE. Zero for a borrowed texture.
   */
  readonly type: number;
}

/**
 * Caller-owned premultiplied RGBA8 image pixels.
 *
 * See `mln_premultiplied_rgba8_image` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface PremultipliedRgba8Image {
  /**
   * Image width in pixels. Required.
   */
  width: number;
  /**
   * Image height in pixels. Required.
   */
  height: number;
  /**
   * Bytes per image row. Required, and at least width &#42; 4.
   */
  stride: number;
  /**
   * Premultiplied RGBA8 pixels. Required.
   */
  pixels: Uint8Array;
}

/**
 * Lower-level Spherical Mercator projected-meter coordinate.
 *
 * See `mln_projected_meters` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface ProjectedMeters {
  /**
   * Distance measured northward from the equator, in meters.
   */
  northing?: number;
  /**
   * Distance measured eastward from the prime meridian, in meters.
   */
  easting?: number;
}

/**
 * MapLibre axonometric rendering options used for snapshots and commands.
 *
 * See `mln_projection_mode` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface ProjectionMode {
  /**
   * Enables a non-perspective axonometric render transform.
   */
  axonometric?: boolean;
  /**
   * X-skew factor of the axonometric transform.
   */
  xSkew?: number;
  /**
   * Y-skew factor of the axonometric transform.
   */
  ySkew?: number;
}

/**
 * Quaternion stored as x, y, z, w components.
 *
 * See `mln_quaternion` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface Quaternion {
  x?: number;
  y?: number;
  z?: number;
  w?: number;
}

/**
 * Host lock on the graphics queue that a session shares with its host, copied
 * by a successful attach. The callback runs asynchronously on the JavaScript
 * thread that registered it, after native queues the call.
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
  /**
   * Optional. Releases user_data once, after all callback invocations have
   * returned and the session can no longer invoke the callbacks. A disabled
   * lock leaves it null.
   */
  readonly releaseUserData?: bigint;
}

/**
 * What abandon did with a session's graphics resources.
 *
 * See `mln_render_abandon_disposition` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
export const RenderAbandonDisposition = {
  /**
   * Abandon destroyed every graphics resource, or none remained.
   */
  clean: 0,
  /**
   * Abandon kept graphics resources that it could not safely destroy.
   */
  quarantined: 1,
} as const;
/** One of the RenderAbandonDisposition values, or a value this binding does not know. */
export type RenderAbandonDisposition =
  | (typeof RenderAbandonDisposition)[keyof typeof RenderAbandonDisposition]
  | (number & {});

export interface RenderAbandonResult {
  readonly disposition: RenderAbandonDisposition;
  /**
   * Backend resource groups intentionally retained until process exit.
   */
  readonly quarantinedResourceCount: number;
}

/**
 * Render backend support flags reported by this native library build.
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
 * Execution placement for one render session.
 *
 * See `mln_render_driver_kind` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const RenderDriverKind = {
  /**
   * Native code initializes, services, and tears down transferable graphics
   * state on a library-owned thread.
   */
  coreWorker: 1,
  /**
   * The host services driver work on the thread or realm where its graphics
   * context is current.
   */
  callerGraphicsThread: 2,
} as const;
/** One of the RenderDriverKind values, or a value this binding does not know. */
export type RenderDriverKind =
  | (typeof RenderDriverKind)[keyof typeof RenderDriverKind]
  | (number & {});

/**
 * Terminal result of one frame demand, held by an owned frame-result batch and
 * copied by `AcquiredFrameHandle.getResult()`.
 *
 * See `mln_render_frame_result` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
export interface RenderFrameResult {
  readonly disposition: RenderResult;
  readonly token: bigint;
  /**
   * Generation of the map render update the demand evaluated. When disposition
   * is `RenderResult.rendered`, the frame drew that update; compare it with
   * `CommandCompletion.renderUpdateGeneration` to find the first frame that
   * includes a command.
   */
  readonly mapUpdateGeneration: bigint;
  /**
   * The session's target generation when the demand reached its result. See
   * `RenderSessionSnapshot.generation`.
   */
  readonly generation: bigint;
  /**
   * Position of the frame among the frames that the session rendered, counting
   * from one. Zero unless disposition is `RenderResult.rendered`.
   */
  readonly frameGeneration: bigint;
  /**
   * Whether the map asked for another frame while it rendered this one, as
   * during an ongoing paint transition. Set only when disposition is
   * `RenderResult.rendered`, and false for every other outcome. It matches the
   * needs_repaint field of `RuntimeEventType.mapRenderFrameFinished`. A camera
   * transition does not set it: the map publishes an update for each of the
   * transition's frames instead, which a render-if-needed demand renders.
   */
  readonly needsRepaint: boolean;
  /**
   * The view that this frame drew, from the transform of the map update that it
   * rendered. A host that displays the frame builds its projection with
   * `MapProjectionHandle.create()` to place overlays where the frame drew their
   * coordinates. Zero, with a size of zero, unless disposition is
   * `RenderResult.rendered`.
   */
  readonly view: MapView;
}

/**
 * Render modes reported by render observer events.
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
 * Terminal disposition of one accepted frame demand.
 *
 * See `mln_render_result` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
export const RenderResult = {
  /**
   * A frame was rendered for acquisition, presentation, or ordered readback.
   */
  rendered: 0,
  /**
   * No newer map update was available, the map had no complete frame to draw,
   * or the update extent differed from the target. The map publishes another
   * update when it has one. A demand with `FrameDemandFlag.waitForUpdate` waits
   * for that update instead, and finishes with this result only when a barrier
   * ends its wait.
   */
  noUpdate: 1,
  /**
   * The target could not produce a frame. The attempt consumes nothing, so a
   * later demand with the same flags renders what this one would have. This
   * result does not cause a map update, so the host demands again when the
   * target can be ready, such as after a paced delay.
   */
  targetNotReady: 2,
  /**
   * A newer demand with the same flags, neither of them forced, replaced this
   * demand. A still image's demand is never replaced, and never replaces one.
   */
  superseded: 3,
} as const;
/** One of the RenderResult values, or a value this binding does not know. */
export type RenderResult =
  | (typeof RenderResult)[keyof typeof RenderResult]
  | (number & {});

/**
 * Common attachment policy copied before an attach call returns.
 *
 * See `mln_render_session_attach_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface RenderSessionAttachOptions {
  /**
   * Where the session's driver runs. Required.
   */
  driver: RenderDriverKind;
  /**
   * Requested slot count of a session-owned texture ring, up to 8. Zero selects
   * 2, so the host can retain one frame while the session renders its
   * replacement. Private targets grant one slot regardless of this value. A
   * borrowed texture ring's depth is its texture count, so borrowed and other
   * targets ignore this value.
   */
  requestedTextureRingDepth?: number;
  /**
   * Producer synchronization for acquired texture frames.
   * `GpuSyncKind.openGlFence` requires a shared EGL or WGL texture target and a
   * caller-graphics-thread driver. Other targets support only CPU completion
   * and fail an unsupported request.
   */
  producerSyncKind?: GpuSyncKind;
  /**
   * Wakes the receiver when the frame-result queue becomes nonempty.
   */
  frameWake?: Wake;
  /**
   * Wakes the graphics receiver when caller-driver work is available, including
   * work that a finished driver result releases.
   */
  driverWorkWake?: Wake;
  /**
   * Host lock on the graphics queue, disabled by default. Only Vulkan targets
   * accept an enabled lock; other backends fail the attach with
   * `Status.unsupported`.
   */
  queueLock?: QueueLock;
}

/**
 * Driver and target capabilities fixed for one attached render session.
 *
 * See `mln_render_session_capabilities` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface RenderSessionCapabilities {
  readonly driver: RenderDriverKind;
  /**
   * Granted texture ring depth: the slot count of a session-owned ring, or the
   * texture count of a borrowed one. Zero for a surface.
   */
  readonly textureRingDepth: number;
  readonly flags: RenderSessionCapabilityFlag;
}

/**
 * Optional render-session capabilities.
 *
 * See `mln_render_session_capability_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const RenderSessionCapabilityFlag = {
  frameAcquisition: 1,
  readback: 2,
  consumerSync: 4,
  presentation: 8,
} as const;
/** A bitwise OR of RenderSessionCapabilityFlag flags. */
export type RenderSessionCapabilityFlag = number;

/**
 * Any-thread render-session snapshot.
 *
 * See `mln_render_session_snapshot` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
export interface RenderSessionSnapshot {
  readonly state: RenderSessionState;
  /**
   * Logical extent applied by the driver.
   */
  readonly extent: LogicalExtent;
  /**
   * Increases when the session's lifecycle state, target, or applied extent
   * changes. An acquired texture frame reports the value it rendered under.
   */
  readonly generation: bigint;
  /**
   * Generation of the latest map render update that the session has seen.
   */
  readonly mapUpdateGeneration: bigint;
  /**
   * Map render update generation that the latest rendered frame drew. Zero
   * before the first frame after attachment or a target replacement.
   */
  readonly renderedUpdateGeneration: bigint;
  /**
   * Frame generation of the latest rendered frame, zero before the first.
   */
  readonly frameGeneration: bigint;
  /**
   * Acquired frames that the host has not released or discarded.
   */
  readonly acquiredFrameCount: number;
  /**
   * True when a map update or an accepted resize arrived after the latest
   * rendered frame.
   */
  readonly pendingChanges: boolean;
}

/**
 * Render-session lifecycle visible in snapshots.
 *
 * See `mln_render_session_state` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
export const RenderSessionState = {
  attaching: 1,
  attached: 2,
  detaching: 3,
  detached: 4,
  /**
   * A frame or driver task failed and the session can no longer render. The map
   * reports the failure as `RuntimeEventType.mapRenderError`. A failed attach
   * reports it through its completion instead.
   */
  targetLost: 5,
  abandoned: 6,
} as const;
/** One of the RenderSessionState values, or a value this binding does not know. */
export type RenderSessionState =
  | (typeof RenderSessionState)[keyof typeof RenderSessionState]
  | (number & {});

/**
 * One rendered-feature query hit.
 *
 * See `mln_rendered_feature` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
export interface RenderedFeature {
  readonly feature: unknown;
  readonly layerId: string;
  readonly sourceId: string;
  readonly sourceLayerId: string;
  readonly state: unknown;
}

/**
 * Options for rendered feature queries.
 *
 * See `mln_rendered_feature_query_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
export interface RenderedFeatureQueryOptions {
  /**
   * Optional style layer IDs. When absent, all rendered layers are queried.
   */
  layerIds?: readonly string[];
  /**
   * UTF-8 MapLibre style-spec filter JSON, or empty for no filter.
   */
  filter?: unknown;
}

/**
 * Rendered feature query geometry descriptor.
 *
 * See `mln_rendered_query_geometry` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
export type RenderedQueryGeometry =
  | { readonly kind: "point"; readonly value: ScreenPoint }
  | { readonly kind: "box"; readonly value: ScreenBox }
  | { readonly kind: "lineString"; readonly value: ScreenLineString };

/**
 * Rendering statistics of one finished frame.
 *
 * See `mln_rendering_stats` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RenderingStats {
  /**
   * Frame CPU encoding time in seconds.
   */
  readonly encodingTime: number;
  /**
   * Frame CPU rendering time in seconds.
   */
  readonly renderingTime: number;
  /**
   * Number of frames rendered by the native renderer.
   */
  readonly frameCount: bigint;
  /**
   * Draw calls executed during the most recent frame.
   */
  readonly drawCallCount: bigint;
  /**
   * Total draw calls executed by the native renderer.
   */
  readonly totalDrawCallCount: bigint;
}

export const ResourceErrorReason = {
  none: 0,
  notFound: 1,
  server: 2,
  connection: 3,
  rateLimit: 4,
  other: 5,
} as const;
/** One of the ResourceErrorReason values, or a value this binding does not know. */
export type ResourceErrorReason =
  | (typeof ResourceErrorReason)[keyof typeof ResourceErrorReason]
  | (number & {});

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
 * The resource kinds that a route or rule matches, one bit per `ResourceKind`
 * value. Registration rejects unknown bits. `ResourceKindFlag.all`, like zero,
 * selects every resource kind.
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

export const ResourceLoadingMethod = {
  all: 0,
  cacheOnly: 1,
  networkOnly: 2,
} as const;
/** One of the ResourceLoadingMethod values, or a value this binding does not know. */
export type ResourceLoadingMethod =
  | (typeof ResourceLoadingMethod)[keyof typeof ResourceLoadingMethod]
  | (number & {});

export const ResourcePriority = {
  regular: 0,
  low: 1,
} as const;
/** One of the ResourcePriority values, or a value this binding does not know. */
export type ResourcePriority =
  | (typeof ResourcePriority)[keyof typeof ResourcePriority]
  | (number & {});

/**
 * Resource provider state. The callback runs asynchronously on the JavaScript
 * thread that registered it, after native queues the call.
 *
 * See `mln_resource_provider` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface ResourceProvider {
  /**
   * Answers each request that the routes leave to the provider.
   */
  callback: (
    request: ResourceRequestInfo,
    handle: ResourceRequestHandle,
  ) => void;
  /**
   * The requests that reach the provider; empty to claim every request.
   */
  routes?: readonly ResourceRoute[];
}

/**
 * Inclusive byte range of a resource request.
 *
 * See `mln_resource_range` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface ResourceRange {
  /**
   * First byte offset of the requested range.
   */
  readonly start: bigint;
  /**
   * Last byte offset of the requested range, inclusive.
   */
  readonly end: bigint;
}

/**
 * Cancel callback for one handled resource request. The callback runs
 * asynchronously on the JavaScript thread that registered it, after native
 * queues the call.
 *
 * See `mln_resource_request_cancel_handler` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export type ResourceRequestCancelHandler = () => void;

export interface ResourceRequestInfo {
  /**
   * URL entering the network layer, before tile server normalization.
   */
  readonly requestedUrl: string;
  /**
   * URL to fetch, after resource-kind normalization against the runtime's tile
   * server options and API key.
   */
  readonly resolvedUrl: string;
  readonly kind: ResourceKind;
  readonly loadingMethod: ResourceLoadingMethod;
  readonly priority: ResourcePriority;
  readonly usage: ResourceUsage;
  readonly storagePolicy: ResourceStoragePolicy;
  readonly range?: ResourceRange;
  readonly priorModifiedUnixMs?: bigint;
  readonly priorExpiresUnixMs?: bigint;
  /**
   * Entity tag of the cached copy, or empty when there is none.
   */
  readonly priorEtag: string;
  readonly priorData: Uint8Array;
}

/**
 * A resource provider's answer to one request.
 *
 * See `mln_resource_response` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface ResourceResponse {
  status?: ResourceResponseStatus;
  errorReason?: ResourceErrorReason;
  /**
   * Response bytes.
   */
  bytes?: Uint8Array;
  errorMessage?: string;
  mustRevalidate?: boolean;
  modifiedUnixMs?: bigint;
  expiresUnixMs?: bigint;
  etag?: string;
  retryAfterUnixMs?: bigint;
}

/**
 * How a resource provider answered a request.
 *
 * See `mln_resource_response_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const ResourceResponseStatus = {
  ok: 0,
  error: 1,
  noContent: 2,
  notModified: 3,
} as const;
/** One of the ResourceResponseStatus values, or a value this binding does not know. */
export type ResourceResponseStatus =
  | (typeof ResourceResponseStatus)[keyof typeof ResourceResponseStatus]
  | (number & {});

/**
 * One request pattern that a resource provider claims.
 *
 * See `mln_resource_route` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface ResourceRoute {
  /**
   * The resource kinds to match. Zero selects every kind.
   */
  kinds?: ResourceKindFlag;
  flags?: UrlMatchFlag;
  /**
   * The URL or glob pattern to compare.
   */
  url?: string;
}

export const ResourceStoragePolicy = {
  permanent: 0,
  volatile: 1,
} as const;
/** One of the ResourceStoragePolicy values, or a value this binding does not know. */
export type ResourceStoragePolicy =
  | (typeof ResourceStoragePolicy)[keyof typeof ResourceStoragePolicy]
  | (number & {});

/**
 * Resource transform state. The callback runs asynchronously on the JavaScript
 * thread that registered it, after native queues the call.
 *
 * See `mln_resource_transform` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export type ResourceTransform = (
  kind: ResourceKind,
  url: string,
  request: ResourceTransformRequestHandle,
) => void;

export const ResourceUsage = {
  online: 0,
  offline: 1,
} as const;
/** One of the ResourceUsage values, or a value this binding does not know. */
export type ResourceUsage =
  | (typeof ResourceUsage)[keyof typeof ResourceUsage]
  | (number & {});

/**
 * One delivered runtime event.
 *
 * See `mln_runtime_event` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeEvent {
  readonly type: RuntimeEventType;
  readonly sourceType: RuntimeEventSourceType;
  /**
   * Source handle: the `RuntimeHandle` or `MapHandle` that source_type selects.
   */
  readonly source: bigint;
  /**
   * Map snapshot generation that the event reports, or zero when source_type is
   * `RuntimeEventSourceType.runtime`.
   */
  readonly generation: bigint;
  /**
   * Secondary event detail whose meaning type selects; see `RuntimeEventType`.
   */
  readonly code: number;
  /**
   * UTF-8 message, empty when this event carries no message.
   */
  readonly message: string;
  /**
   * Typed payload selected by payload_type.
   */
  readonly payload: RuntimeEventPayload;
}

/**
 * Bit values for the map and runtime event subscription masks.
 *
 * See `mln_runtime_event_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const RuntimeEventFlag = {
  /**
   * Selects no event type.
   */
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
  /**
   * Selects every map-originated event type this version defines.
   */
  allMapEvents: 131070n,
  /**
   * Selects every runtime-originated event type this version defines.
   */
  allRuntimeEvents: 917504n,
  /**
   * Selects every event type this version defines.
   */
  all: 1048574n,
} as const;
/** A bitwise OR of RuntimeEventFlag flags. */
export type RuntimeEventFlag = bigint;

/**
 * Runtime and map event callback. The callback runs asynchronously on the
 * JavaScript thread that registered it, after native queues the call.
 *
 * See `mln_runtime_event_handler` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export type RuntimeEventHandler = (events: readonly RuntimeEvent[]) => void;

/**
 * Payload for `RuntimeEventType.offlineRegionResponseError`.
 *
 * See `mln_runtime_event_offline_region_response_error` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeEventOfflineRegionResponseError {
  readonly regionId: bigint;
  readonly reason: ResourceErrorReason;
}

/**
 * Payload for `RuntimeEventType.offlineRegionStatusChanged`.
 *
 * See `mln_runtime_event_offline_region_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeEventOfflineRegionStatus {
  readonly regionId: bigint;
  readonly status: OfflineRegionStatus;
}

/**
 * Payload for `RuntimeEventType.offlineRegionTileCountLimitExceeded`.
 *
 * See `mln_runtime_event_offline_region_tile_count_limit` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeEventOfflineRegionTileCountLimit {
  readonly regionId: bigint;
  readonly limit: bigint;
}

/**
 * Typed event payload carried inline by every event.
 *
 * See `mln_runtime_event_payload` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export type RuntimeEventPayload =
  | { readonly kind: "renderFrame"; readonly value: RuntimeEventRenderFrame }
  | { readonly kind: "renderMap"; readonly value: RuntimeEventRenderMap }
  | { readonly kind: "tileAction"; readonly value: RuntimeEventTileAction }
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
  | { readonly kind: "none" }
  | {
      readonly kind: "unknown";
      readonly tag: number;
      readonly bytes: Uint8Array;
    };

/**
 * Payload for `RuntimeEventType.mapRenderFrameFinished`.
 *
 * See `mln_runtime_event_render_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeEventRenderFrame {
  readonly mode: RenderMode;
  /**
   * Whether MapLibre needs another frame after this one.
   */
  readonly needsRepaint: boolean;
  /**
   * Whether symbol placement changed during this frame.
   */
  readonly placementChanged: boolean;
  readonly stats: RenderingStats;
}

/**
 * Payload for `RuntimeEventType.mapRenderMapFinished`.
 *
 * See `mln_runtime_event_render_map` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeEventRenderMap {
  readonly mode: RenderMode;
}

/**
 * Source kinds used by `RuntimeEvent.sourceType`.
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
 * Payload for `RuntimeEventType.mapTileAction`.
 *
 * See `mln_runtime_event_tile_action` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeEventTileAction {
  readonly operation: TileOperation;
  readonly tileId: TileId;
}

/**
 * Runtime event types carried by `RuntimeEvent.type`.
 *
 * See `mln_runtime_event_type` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const RuntimeEventType = {
  mapCameraWillChange: 1,
  mapCameraIsChanging: 2,
  mapCameraDidChange: 3,
  mapStyleLoaded: 4,
  mapLoadingStarted: 5,
  mapLoadingFinished: 6,
  mapLoadingFailed: 7,
  mapIdle: 8,
  mapRenderError: 9,
  mapRenderFrameStarted: 10,
  mapRenderFrameFinished: 11,
  mapRenderMapStarted: 12,
  mapRenderMapFinished: 13,
  mapStyleImageMissing: 14,
  mapTileAction: 15,
  mapSourceChanged: 16,
  offlineRegionStatusChanged: 17,
  offlineRegionResponseError: 18,
  offlineRegionTileCountLimitExceeded: 19,
} as const;
/** One of the RuntimeEventType values, or a value this binding does not know. */
export type RuntimeEventType =
  | (typeof RuntimeEventType)[keyof typeof RuntimeEventType]
  | (number & {});

/**
 * Options used when creating a runtime.
 *
 * See `mln_runtime_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeOptions {
  /**
   * Directory root for asset:// URLs. Copied during runtime creation. Empty
   * selects `/android_asset` on Android and `.` elsewhere.
   */
  assetPath?: string;
  /**
   * Cache database path. Copied during runtime creation. Empty selects an
   * in-memory cache.
   */
  cachePath?: string;
}

/**
 * Screen-space box in logical map pixels.
 *
 * See `mln_screen_box` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
export interface ScreenBox {
  min?: ScreenPoint;
  max?: ScreenPoint;
}

/**
 * Screen-space line string in logical map pixels.
 *
 * See `mln_screen_line_string` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
export interface ScreenLineString {
  /**
   * Points.
   */
  points?: readonly ScreenPoint[];
}

/**
 * Screen-space point in logical map pixels, from the top-left of the viewport
 * with y growing downward.
 *
 * See `mln_screen_point` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface ScreenPoint {
  x?: number;
  y?: number;
}

/**
 * One source-feature query hit.
 *
 * See `mln_source_feature` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
export interface SourceFeature {
  readonly feature: unknown;
  readonly sourceLayerId: string;
}

/**
 * Options for source feature queries.
 *
 * See `mln_source_feature_query_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
export interface SourceFeatureQueryOptions {
  /**
   * Optional source-layer IDs. Required by vector sources; ignored by GeoJSON.
   */
  sourceLayerIds?: readonly string[];
  /**
   * UTF-8 MapLibre style-spec filter JSON, or empty for no filter.
   */
  filter?: unknown;
}

/**
 * Status values returned by status-returning functions.
 *
 * See `mln_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
export const Status = {
  ok: 0,
  /**
   * A pointer, size field, mask, or handle argument was invalid.
   */
  invalidArgument: -1,
  /**
   * The object is valid but not currently in a state that permits the call.
   */
  invalidState: -2,
  /**
   * The handle is thread-affine and the call came from the wrong thread.
   */
  wrongThread: -3,
  /**
   * The entry point or requested behavior is unavailable in this build.
   */
  unsupported: -4,
  /**
   * Native code failed; the diagnostic describes the error.
   */
  nativeError: -5,
  /**
   * The operation reached its terminal cancelled disposition.
   */
  cancelled: -6,
  /**
   * A conflicting driver call or lifecycle transition is in flight.
   */
  busy: -7,
  /**
   * The render target or graphics receiver was irreversibly lost.
   */
  targetLost: -8,
  /**
   * A nonblocking drain or acquisition has nothing to return yet.
   */
  notReady: -9,
  /**
   * A call named an ID with no live object behind it.
   */
  notFound: -10,
} as const;
/** One of the Status values, or a value this binding does not know. */
export type Status = (typeof Status)[keyof typeof Status] | (number & {});

/**
 * What a parsed expression depends on.
 *
 * See `mln_style_expression_info` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
 */
export interface StyleExpressionInfo {
  /**
   * Whether the result is the same for every feature and feature state.
   */
  readonly isFeatureConstant: boolean;
  /**
   * Whether the result is the same at every zoom.
   */
  readonly isZoomConstant: boolean;
}

/**
 * What one evaluation reads.
 *
 * See `mln_style_expression_inputs` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
 */
export interface StyleExpressionInputs {
  /**
   * The value of the zoom expression.
   */
  zoom?: number;
  /**
   * The value of the heatmap-density and line-progress expressions, and of the
   * elevation expression in a color-relief layer.
   */
  colorRampParameter?: number;
  /**
   * The value of the elevation expression outside a color-relief layer.
   */
  elevation?: number;
  /**
   * One UTF-8 GeoJSON Feature that feature data expressions read.
   */
  feature?: unknown;
  /**
   * A UTF-8 JSON object that the feature-state expression reads.
   */
  featureState?: unknown;
  /**
   * A UTF-8 JSON object that the global-state expression reads.
   */
  globalState?: unknown;
  /**
   * The tile that holds the feature, which the within and distance expressions
   * need to place its geometry.
   */
  canonicalTileId?: CanonicalTileId;
  /**
   * The image IDs that the image expression reports as available. The
   * expression reports any other ID as unavailable.
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
   * The type that the expression must produce. A string type coerces the result
   * to a string, as a string layer property does; every other type asserts it.
   * Without it, the expression keeps the type it infers.
   */
  expectedType?: StyleExpressionType;
}

/**
 * A style expression type.
 *
 * See `mln_style_expression_type` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
 */
export interface StyleExpressionType {
  kind?: StyleExpressionTypeKind;
  /**
   * For an array, its items' kind; ignored for any other kind.
   */
  itemKind?: StyleExpressionTypeKind;
  /**
   * For an array, its fixed length, or zero for any length.
   */
  itemCount?: number;
}

/**
 * Style expression types that a parsed expression may be required to have.
 *
 * See `mln_style_expression_type_kind` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
 */
export const StyleExpressionTypeKind = {
  /**
   * Any value.
   */
  value: 0,
  number: 1,
  string: 2,
  boolean: 3,
  color: 4,
  object: 5,
  formatted: 6,
  /**
   * A resolved image, as the image expression returns.
   */
  image: 7,
  padding: 8,
  variableAnchorOffsetCollection: 9,
  /**
   * An array whose items have item_kind, which is not itself an array.
   */
  array: 10,
} as const;
/** One of the StyleExpressionTypeKind values, or a value this binding does not know. */
export type StyleExpressionTypeKind =
  | (typeof StyleExpressionTypeKind)[keyof typeof StyleExpressionTypeKind]
  | (number & {});

/**
 * One complete runtime style image.
 *
 * See `mln_style_image_info` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface StyleImageInfo {
  /**
   * Image width in pixels.
   */
  readonly width: number;
  /**
   * Image height in pixels.
   */
  readonly height: number;
  /**
   * Premultiplied RGBA8 pixels in tightly packed rows of width &#42; 4 bytes,
   * top row first.
   */
  readonly pixels: Uint8Array;
  /**
   * Horizontally stretchable intervals.
   */
  readonly stretchX: readonly ImageStretch[];
  /**
   * Vertically stretchable intervals.
   */
  readonly stretchY: readonly ImageStretch[];
  /**
   * Content box, when the image sets one.
   */
  readonly content?: ImageContent;
  /**
   * How the image fits text horizontally, when it sets this.
   */
  readonly textFitWidth?: StyleImageTextFit;
  /**
   * How the image fits text vertically, when it sets this.
   */
  readonly textFitHeight?: StyleImageTextFit;
  /**
   * Sprite pixel ratio.
   */
  readonly pixelRatio: number;
  /**
   * Whether the image is a signed distance field icon.
   */
  readonly sdf: boolean;
}

/**
 * Options for runtime style images.
 *
 * See `mln_style_image_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface StyleImageOptions {
  /**
   * Horizontally stretchable intervals.
   */
  stretchX?: readonly ImageStretch[];
  /**
   * Vertically stretchable intervals.
   */
  stretchY?: readonly ImageStretch[];
  /**
   * Content box used when icon-text-fit applies.
   */
  content?: ImageContent;
  /**
   * Absent selects STRETCH_OR_SHRINK.
   */
  textFitWidth?: StyleImageTextFit;
  /**
   * Absent selects STRETCH_OR_SHRINK.
   */
  textFitHeight?: StyleImageTextFit;
  /**
   * Sprite pixel ratio. Absent selects 1.
   */
  pixelRatio?: number;
  /**
   * Whether the image is a signed distance field icon. Absent selects false.
   */
  sdf?: boolean;
}

/**
 * How a stretchable image fits text along one axis.
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
 * One style layer.
 *
 * See `mln_style_layer_info` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface StyleLayerInfo {
  /**
   * Layer ID.
   */
  readonly id: string;
  /**
   * The style-spec layer type string.
   */
  readonly type: string;
  /**
   * Source ID. Empty for a layer type that takes no source.
   */
  readonly sourceId: string;
  /**
   * Source-layer ID. Empty when the layer sets none.
   */
  readonly sourceLayer: string;
  /**
   * Lowest zoom at which the layer draws; -INFINITY with no lower bound.
   */
  readonly minZoom: number;
  /**
   * Highest zoom at which the layer draws; INFINITY with no upper bound.
   */
  readonly maxZoom: number;
  readonly visibility: StyleLayerVisibility;
}

/**
 * Layer visibility values used by the visibility setter and layer info.
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
 * DEM raster encoding values.
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
 * Complete metadata of one style source.
 *
 * See `mln_style_source_info` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface StyleSourceInfo {
  /**
   * Source ID.
   */
  readonly id: string;
  readonly type: StyleSourceType;
  /**
   * Whether the source is marked volatile.
   */
  readonly isVolatile: boolean;
  /**
   * Attribution string, or empty when the source sets none.
   */
  readonly attribution: string;
  /**
   * URL that the source loads from, or empty when it has none.
   */
  readonly url: string;
  /**
   * Inline TileJSON metadata, when the source was defined with it.
   */
  readonly tileJson?: StyleSourceTileInfo;
  /**
   * Geographic bounds, when inline TileJSON sets them.
   */
  readonly bounds?: LatLngBounds;
  /**
   * Tile size in pixels, for a tile source.
   */
  readonly tileSize?: number;
  /**
   * Vector tile encoding, for a vector source.
   */
  readonly vectorEncoding?: StyleVectorTileEncoding;
  /**
   * DEM raster encoding, when inline TileJSON sets one.
   */
  readonly rasterEncoding?: StyleRasterDemEncoding;
}

/**
 * Inline TileJSON metadata of a tile source.
 *
 * See `mln_style_source_tile_info` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface StyleSourceTileInfo {
  /**
   * Tile URL templates in TileJSON order.
   */
  readonly tileUrls: readonly string[];
  /**
   * Lowest zoom level the TileJSON provides tiles for.
   */
  readonly minZoom: number;
  /**
   * Highest zoom level the TileJSON provides tiles for.
   */
  readonly maxZoom: number;
  readonly scheme: StyleTileScheme;
}

/**
 * Style source type values returned by source metadata queries.
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
  customVector: 8,
  customMvtVector: 9,
} as const;
/** One of the StyleSourceType values, or a value this binding does not know. */
export type StyleSourceType =
  | (typeof StyleSourceType)[keyof typeof StyleSourceType]
  | (number & {});

/**
 * Tile URL coordinate scheme values.
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
 * Global style transition options.
 *
 * See `mln_style_transition_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface StyleTransitionOptions {
  /**
   * Transition duration in milliseconds. Must be finite, non-negative, and
   * short enough to represent as a duration.
   */
  durationMs?: number;
  /**
   * Transition delay in milliseconds. Must be finite, non-negative, and short
   * enough to represent as a duration.
   */
  delayMs?: number;
  /**
   * Whether symbol placement changes cross-fade.
   */
  enablePlacementTransitions?: boolean;
}

/**
 * Vector tile encoding values.
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
 * CPU image readback metadata for a texture target frame.
 *
 * See `mln_texture_image_info` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface TextureImageInfo {
  /**
   * Physical image width in device pixels.
   */
  readonly width: number;
  /**
   * Physical image height in device pixels.
   */
  readonly height: number;
  /**
   * Bytes per image row.
   */
  readonly stride: number;
  /**
   * Required output buffer byte length.
   */
  readonly byteLength: bigint;
}

/**
 * Texture readback.
 *
 * See `mln_texture_readback_result` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface TextureReadbackResult {
  /**
   * Pixel bytes.
   */
  readonly data: Uint8Array;
  readonly info: TextureImageInfo;
}

/**
 * Overscaled tile identity reported in tile observer events.
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
 * Tile LOD algorithms used by `MapTileOptions`.
 *
 * See `mln_tile_lod_mode` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const TileLodMode = {
  defaultValue: 0,
  distance: 1,
} as const;
/** One of the TileLodMode values, or a value this binding does not know. */
export type TileLodMode =
  | (typeof TileLodMode)[keyof typeof TileLodMode]
  | (number & {});

/**
 * Tile operations reported by tile observer events.
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
  nullValue: 8,
} as const;
/** One of the TileOperation values, or a value this binding does not know. */
export type TileOperation =
  | (typeof TileOperation)[keyof typeof TileOperation]
  | (number & {});

/**
 * Cubic easing curve for animated camera transitions.
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
 * How a rule or route compares its url against a request URL.
 *
 * See `mln_url_match_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const UrlMatchFlag = {
  exact: 0,
  /**
   * Reads url as a glob pattern.
   */
  glob: 1,
} as const;
/** A bitwise OR of UrlMatchFlag flags. */
export type UrlMatchFlag = number;

/**
 * Three-component vector used by free camera options.
 *
 * See `mln_vec3` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface Vec3 {
  x?: number;
  y?: number;
  z?: number;
}

/**
 * Viewport orientation modes used by `MapViewportOptions`.
 *
 * See `mln_viewport_mode` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const ViewportMode = {
  defaultValue: 0,
  flippedY: 1,
} as const;
/** One of the ViewportMode values, or a value this binding does not know. */
export type ViewportMode =
  | (typeof ViewportMode)[keyof typeof ViewportMode]
  | (number & {});

/**
 * One caller-owned Vulkan image of a borrowed texture ring.
 *
 * See `mln_vulkan_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface VulkanBorrowedTexture {
  /**
   * Borrowed VkImage. Required.
   */
  image: bigint;
  /**
   * Borrowed VkImageView for image. Required. The view must be a 2D color view
   * that matches image and the descriptor's format.
   */
  imageView: bigint;
}

/**
 * Vulkan attachment options for a borrowed texture target.
 *
 * See `mln_vulkan_borrowed_texture_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface VulkanBorrowedTextureDescriptor {
  /**
   * Logical texture extent. Required. physical_width and physical_height give
   * the texture's size. A scale_factor that differs from the map's is accepted
   * and logged as a warning.
   */
  extent: LogicalExtent;
  /**
   * Physical image width in device pixels. Required.
   */
  physicalWidth: number;
  /**
   * Physical image height in device pixels. Required.
   */
  physicalHeight: number;
  /**
   * Borrowed Vulkan context. All handles are required.
   */
  context: VulkanContextDescriptor;
  /**
   * The ring's images, one per slot, in slot order. Required.
   */
  textures: readonly VulkanBorrowedTexture[];
  /**
   * Backend-native VkFormat value of every image. Required, so
   * VK_FORMAT_UNDEFINED is invalid.
   */
  format: number;
  /**
   * Backend-native VkImageLayout value expected at render-pass begin.
   */
  initialLayout?: number;
  /**
   * Backend-native VkImageLayout value left after rendering succeeds, such as
   * VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL. Required, so
   * VK_IMAGE_LAYOUT_UNDEFINED is invalid.
   */
  finalLayout: number;
}

/**
 * Vulkan backend context fields shared by Vulkan render targets.
 *
 * See `mln_vulkan_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface VulkanContextDescriptor {
  /**
   * Borrowed VkInstance. Required.
   */
  instance: bigint;
  /**
   * Borrowed VkPhysicalDevice. Required.
   */
  physicalDevice: bigint;
  /**
   * Borrowed VkDevice. Required.
   */
  device: bigint;
  /**
   * Borrowed graphics VkQueue. Required. The session's driver submits to it
   * from its own thread, so a host that uses the same queue passes
   * `RenderSessionAttachOptions.queueLock` at attach.
   */
  graphicsQueue: bigint;
  /**
   * Queue family index for graphics_queue. Must support graphics commands.
   */
  graphicsQueueFamilyIndex?: number;
  /**
   * PFN_vkGetInstanceProcAddr for the loader that created the instance.
   */
  getInstanceProcAddr?: bigint;
  /**
   * PFN_vkGetDeviceProcAddr for the loader that created the Vulkan device.
   */
  getDeviceProcAddr?: bigint;
}

/**
 * Vulkan attachment options for an owned texture target.
 *
 * See `mln_vulkan_owned_texture_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface VulkanOwnedTextureDescriptor {
  /**
   * Logical texture extent. Required. A scale_factor that differs from the
   * map's is accepted and logged as a warning.
   */
  extent: LogicalExtent;
  /**
   * Borrowed Vulkan context. All handles are required.
   */
  context: VulkanContextDescriptor;
}

/**
 * Vulkan attachment options for a native surface.
 *
 * See `mln_vulkan_surface_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
export interface VulkanSurfaceDescriptor {
  /**
   * Logical surface extent. Required. A scale_factor that differs from the
   * map's is accepted and logged as a warning.
   */
  extent: LogicalExtent;
  /**
   * Borrowed Vulkan context. All handles are required. The device must support
   * VK_KHR_swapchain, and the queue family must support graphics and
   * presentation to this descriptor's surface.
   */
  context: VulkanContextDescriptor;
  /**
   * Borrowed VkSurfaceKHR bit pattern. Required.
   */
  surface: bigint;
}

/**
 * Vulkan frame acquired from a texture ring.
 *
 * See `mln_vulkan_texture_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface VulkanTextureFrame {
  /**
   * Session generation that produced this frame.
   */
  readonly generation: bigint;
  /**
   * Physical Vulkan image width in device pixels.
   */
  readonly width: number;
  /**
   * Physical Vulkan image height in device pixels.
   */
  readonly height: number;
  /**
   * UI-to-device pixel scale used for this frame.
   */
  readonly scaleFactor: number;
  /**
   * Opaque identity of this frame.
   */
  readonly frameId: bigint;
  /**
   * Ring slot that holds this frame. For a borrowed target, the index of its
   * image in the descriptor's textures array.
   */
  readonly slot: number;
  /**
   * Borrowed VkImage bit pattern. Valid until frame release.
   */
  readonly image: bigint;
  /**
   * Borrowed VkImageView bit pattern. Valid until frame release.
   */
  readonly imageView: bigint;
  /**
   * Borrowed VkDevice. Valid until frame release.
   */
  readonly device: bigint;
  /**
   * Backend-native VkFormat value.
   */
  readonly format: number;
  /**
   * Backend-native VkImageLayout value that the image is in:
   * VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL for a session-owned ring, and the
   * descriptor's final_layout for a borrowed one.
   */
  readonly layout: number;
}

/**
 * Receiver wake callback copied by a successful owning call. The callback
 * schedules service by the receiver and returns without waiting for another
 * thread. The callback runs asynchronously on the JavaScript thread that
 * registered it, after native queues the call.
 *
 * See `mln_wake` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wake_8h.html).
 */
export type Wake = (() => void) | NativeRegistration<"Wake">;
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
  readonly callback: bigint;
  readonly userData?: bigint;
  /**
   * Optional. Releases user_data after all callback invocations have returned.
   * A disabled wake leaves it null.
   */
  readonly releaseUserData?: bigint;
}

/**
 * WebGL context fields shared by OpenGL render targets in the browser.
 *
 * See `mln_web_gl_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface WebGlContextDescriptor {
  kind?: WebGlContextKind;
  /**
   * Borrowed EMSCRIPTEN_WEBGL_CONTEXT_HANDLE for EXISTING. Must be positive.
   */
  context?: number;
  /**
   * Copied UTF-8 Emscripten target selector for TRANSFERRED_CANVAS. The HTML
   * canvas must still be transferable when attachment starts.
   */
  canvasSelector?: string;
}

/**
 * WebGL context placement.
 *
 * See `mln_web_gl_context_kind` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const WebGlContextKind = {
  /**
   * Use a host-created context on its current browser agent.
   */
  existing: 0,
  /**
   * Transfer the canvas named by canvas_selector to a native worker and create
   * a WebGL 2 context there.
   */
  transferredCanvas: 1,
} as const;
/** One of the WebGlContextKind values, or a value this binding does not know. */
export type WebGlContextKind =
  | (typeof WebGlContextKind)[keyof typeof WebGlContextKind]
  | (number & {});

/**
 * WGL context fields shared by OpenGL render targets on Windows.
 *
 * See `mln_wgl_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface WglContextDescriptor {
  /**
   * Borrowed HDC used to create the session context. Required. A dedicated
   * owned texture makes its context current on this HDC from its core worker.
   */
  deviceContext: bigint;
  /**
   * Borrowed HGLRC whose share group the session context joins. Required under
   * shared ownership. A dedicated session joins no share group, so it must be
   * null there.
   */
  shareContext?: bigint;
  /**
   * Optional wglGetProcAddress-compatible function for the host loader.
   */
  getProcAddress?: bigint;
}
