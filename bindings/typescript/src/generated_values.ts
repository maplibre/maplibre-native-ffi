// Generated from the C headers by tools/bindgen. Do not edit.

import type {
  ResourceRequestHandle,
  ResourceTransformRequestHandle,
} from "./generated_operations.ts";

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
 * Field mask values for `mln_animation_options`.
 *
 * See `mln_animation_option_field` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const AnimationOptionField = {
  duration: 1,
  velocity: 2,
  minZoom: 4,
  easing: 8,
  transitionId: 16,
} as const;
/** A bitwise OR of AnimationOptionField flags. */
export type AnimationOptionField = number;

/**
 * Optional animation controls for camera transitions.
 *
 * See `mln_animation_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface AnimationOptions {
  /**
   * Duration in milliseconds. Must be finite and non-negative. Values that
   * would overflow MapLibre Native's internal duration are invalid.
   */
  durationMs?: number;
  /**
   * Average fly velocity in screenfuls per second. Must be positive and finite;
   * defaults to 1.2 when omitted. A flight whose derived duration exceeds
   * Native's nanosecond duration range fails its completion with
   * `MLN_STATUS_NATIVE_ERROR`. An explicit duration overrides velocity.
   */
  velocity?: number;
  /**
   * Peak zoom for flyTo transitions.
   */
  minZoom?: number;
  easing?: UnitBezier;
  /**
   * Caller-chosen identity of the command that these options animate, which
   * `mln_map_cancel_camera_transition()` matches.
   */
  transitionId?: bigint;
  /**
   * Reports the end of the command's transitions. Disabled by default.
   */
  endHandler?: CameraTransitionHandler;
}

/**
 * Field mask values for `mln_bound_options`.
 *
 * See `mln_bound_option_field` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const BoundOptionField = {
  /**
   * Selects `mln_bound_options.bounds` as a geographic constraint that the
   * camera center stays inside. Mutually exclusive with
   * `MLN_BOUND_OPTION_UNBOUNDED`.
   */
  bounds: 1,
  minZoom: 2,
  maxZoom: 4,
  minPitch: 8,
  maxPitch: 16,
  /**
   * Selects the unbounded geographic constraint, which leaves every camera
   * center unconstrained and lets the map pan freely across the antimeridian.
   * This differs from world bounds of -90/-180 to 90/180, which clamp longitude
   * to that range. Mutually exclusive with `MLN_BOUND_OPTION_BOUNDS`, and
   * leaves `mln_bound_options.bounds` unread.
   */
  unbounded: 32,
} as const;
/** A bitwise OR of BoundOptionField flags. */
export type BoundOptionField = number;

/**
 * Optional map camera constraint fields.
 *
 * See `mln_bound_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface BoundOptions {
  /**
   * Read when fields contains `MLN_BOUND_OPTION_BOUNDS`.
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
   * to that range. Mutually exclusive with `MLN_BOUND_OPTION_BOUNDS`, and
   * leaves `mln_bound_options.bounds` unread.
   */
  unbounded?: boolean;
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
 * Field mask values for `mln_camera_delta`.
 *
 * See `mln_camera_delta_field` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const CameraDeltaField = {
  offset: 1,
  scale: 2,
  bearing: 4,
  pitch: 8,
  anchor: 16,
} as const;
/** A bitwise OR of CameraDeltaField flags. */
export type CameraDeltaField = number;

/**
 * Field mask values for `mln_camera_fit_options`.
 *
 * See `mln_camera_fit_option_field` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const CameraFitOptionField = {
  padding: 1,
  bearing: 2,
  pitch: 4,
} as const;
/** A bitwise OR of CameraFitOptionField flags. */
export type CameraFitOptionField = number;

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
 * Field mask values for `mln_camera_options`.
 *
 * See `mln_camera_option_field` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const CameraOptionField = {
  center: 1,
  zoom: 2,
  bearing: 4,
  pitch: 8,
  centerAltitude: 16,
  padding: 32,
  anchor: 64,
  roll: 128,
  fov: 256,
} as const;
/** A bitwise OR of CameraOptionField flags. */
export type CameraOptionField = number;

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
 * Camera result borrowed for an ordered camera-query completion.
 *
 * See `mln_camera_query_result` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface CameraQueryResult {
  readonly generation: bigint;
  readonly camera: CameraOptions;
}

/**
 * The end of one camera command's transitions, borrowed for the callback.
 *
 * See `mln_camera_transition_end` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface CameraTransitionEnd {
  /**
   * One of `mln_camera_transition_outcome`.
   */
  readonly outcome: CameraTransitionOutcome;
  /**
   * Generation of the published snapshot that shows the camera where the
   * transitions left it. The map events of the change that ended them carry
   * this generation and are queued before the callback runs. Zero for
   * `MLN_CAMERA_TRANSITION_OUTCOME_CLOSED`.
   */
  readonly generation: bigint;
}

/**
 * Callback or queue state that one camera command copies to report the end of
 * its transitions.
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
   * `mln_map_cancel_transitions()`, `mln_map_cancel_camera_transition()`, or
   * `MLN_GESTURE_PHASE_CANCEL` ended the transitions, or the command failed
   * before it started them.
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
 * Camera transition behavior for `mln_camera_update`.
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
 * Terminal dispositions reported by command completions.
 *
 * See `mln_command_disposition` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/completion_8h.html).
 */
export const CommandDisposition = {
  committed: 0,
  superseded: 1,
  failed: 2,
  cancelled: 3,
} as const;
/** One of the CommandDisposition values, or a value this binding does not know. */
export type CommandDisposition =
  | (typeof CommandDisposition)[keyof typeof CommandDisposition]
  | (number & {});

/**
 * Map constraint modes used by `mln_map_viewport_options`.
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
 * Field mask values for `mln_custom_geometry_source_options`.
 *
 * See `mln_custom_geometry_source_option_field` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export const CustomGeometrySourceOptionField = {
  minZoom: 1,
  maxZoom: 2,
  tolerance: 4,
  tileSize: 8,
  buffer: 16,
  clip: 32,
  wrap: 64,
} as const;
/** A bitwise OR of CustomGeometrySourceOptionField flags. */
export type CustomGeometrySourceOptionField = number;

/**
 * Options for custom geometry sources.
 *
 * See `mln_custom_geometry_source_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface CustomGeometrySourceOptions {
  /**
   * Tile fetch callback. Required in callback mode; null in queue mode.
   */
  fetchTile?: (tileId: CanonicalTileId) => void;
  /**
   * Optional best-effort tile cancel callback. Null in queue mode.
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
 * Field mask values for `mln_custom_mvt_vector_source_options`.
 *
 * See `mln_custom_mvt_vector_source_option_field` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export const CustomMvtVectorSourceOptionField = {
  minZoom: 1,
  maxZoom: 2,
} as const;
/** A bitwise OR of CustomMvtVectorSourceOptionField flags. */
export type CustomMvtVectorSourceOptionField = number;

/**
 * Options for custom MVT vector sources.
 *
 * See `mln_custom_mvt_vector_source_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface CustomMvtVectorSourceOptions {
  /**
   * Tile fetch callback. Required in callback mode; null in queue mode.
   */
  fetchTile?: (tileId: CanonicalTileId) => void;
  /**
   * Optional best-effort tile cancel callback. Null in queue mode.
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
  display?: bigint;
  /**
   * Borrowed EGLConfig used to create the session context. Required. OpenGL
   * texture targets require EGL_SURFACE_TYPE to include EGL_PBUFFER_BIT.
   */
  config?: bigint;
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
  clientApi?: OpenglClientApi;
  /**
   * Optional eglGetProcAddress-compatible function for the host loader.
   */
  getProcAddress?: bigint;
}

/**
 * A borrowed span of runtime events in one delivery.
 *
 * See `mln_event_batch_view` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface EventBatchView {
  /**
   * Borrowed array of event_count events in queue order.
   */
  readonly events: readonly RuntimeEvent[];
}

/**
 * Feature-state source, feature, and key selector.
 *
 * See `mln_feature_state_selector` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface FeatureStateSelector {
  /**
   * Source ID. Required and borrowed for the duration of the call.
   */
  sourceId: string;
  /**
   * Optional source layer ID. Required for vector-source disambiguation.
   */
  sourceLayerId?: string;
  /**
   * Optional feature ID. Required by set and get, optional for remove.
   */
  featureId?: string;
  /**
   * Optional state key. Used only by remove and requires feature_id.
   */
  stateKey?: string;
}

/**
 * Optional fields for `mln_feature_state_selector`.
 *
 * See `mln_feature_state_selector_field` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const FeatureStateSelectorField = {
  sourceLayerId: 1,
  featureId: 2,
  stateKey: 4,
} as const;
/** A bitwise OR of FeatureStateSelectorField flags. */
export type FeatureStateSelectorField = number;

/**
 * One nonblocking request for a frame.
 *
 * See `mln_frame_demand` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
export interface FrameDemand {
  /**
   * A bitwise OR of `mln_frame_demand_flag` values. Defaults to
   * `MLN_FRAME_DEMAND_IF_NEEDED`.
   */
  flags?: FrameDemandFlag;
  /**
   * Host identity returned with the terminal frame result.
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
   * Render only when a newer map update exists.
   */
  ifNeeded: 1,
  /**
   * Present the rendered frame on a target that supports presentation. A
   * presenting target whose demand clears this bit still renders and keeps
   * whatever it presented last. Ignored by targets without presentation.
   */
  present: 2,
  /**
   * With `MLN_FRAME_DEMAND_IF_NEEDED`, a demand that would finish with
   * `MLN_RENDER_RESULT_NO_UPDATE` waits instead, and runs again after the map's
   * next update, a target replacement, or an applied resize. A waiting demand
   * holds no ring slot. A later demand with the same flags supersedes it, a
   * barrier ends its wait with `MLN_RENDER_RESULT_NO_UPDATE`, and detach,
   * abandon, or the quarantine of the ring's last usable slot end it with
   * `MLN_RENDER_RESULT_TARGET_NOT_READY`. A waiting demand's result can follow
   * the results of demands accepted after it. The flag does not pace: a host
   * that re-arms a waiting demand as each result arrives renders every update
   * the map publishes. The map publishes one update for the commands that its
   * runtime worker runs back to back, but a command that reaches the worker
   * while a frame renders appears in a later frame. To show several commands in
   * one frame, submit them in a command group, or arm demands once per display
   * frame.
   */
  waitForUpdate: 4,
} as const;
/** A bitwise OR of FrameDemandFlag flags. */
export type FrameDemandFlag = number;

/**
 * Field mask values for `mln_free_camera_options`.
 *
 * See `mln_free_camera_option_field` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const FreeCameraOptionField = {
  position: 1,
  orientation: 2,
} as const;
/** A bitwise OR of FreeCameraOptionField flags. */
export type FreeCameraOptionField = number;

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
 * Field mask values for `mln_geojson_source_options`.
 *
 * See `mln_geojson_source_option_field` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export const GeojsonSourceOptionField = {
  minZoom: 1,
  maxZoom: 2,
  tolerance: 4,
  clusterMaxZoom: 8,
  clusterProperties: 16,
  buffer: 32,
  clusterRadius: 64,
  clusterMinPoints: 128,
  lineMetrics: 256,
  cluster: 512,
  synchronousTiling: 1024,
} as const;
/** A bitwise OR of GeojsonSourceOptionField flags. */
export type GeojsonSourceOptionField = number;

/**
 * Options for GeoJSON sources, indexed and rendered at a fixed 512-pixel tile
 * size.
 *
 * See `mln_geojson_source_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface GeojsonSourceOptions {
  /**
   * Minimum tiling zoom. Defaults to 0.
   */
  minZoom?: number;
  /**
   * Maximum tiling zoom, an integer within &#91;0, 24&#93;. Defaults to 18.
   */
  maxZoom?: number;
  /**
   * Douglas-Peucker simplification tolerance. Defaults to 0.375.
   */
  tolerance?: number;
  /**
   * Highest zoom that clusters points. Defaults to 17.
   */
  clusterMaxZoom?: number;
  /**
   * Cluster aggregation expressions keyed by property name, as a JSON object
   * whose members follow the MapLibre Style Spec clusterProperties form. The
   * UTF-8 bytes are borrowed for the call.
   */
  clusterProperties?: string;
  /**
   * Tile buffer in pixels within &#91;0, 512&#93;. Defaults to 128.
   */
  buffer?: number;
  /**
   * Cluster radius in pixels within &#91;0, 4095&#93;. Defaults to 50.
   */
  clusterRadius?: number;
  /**
   * Points required to form a cluster. Defaults to 2.
   */
  clusterMinPoints?: number;
  /**
   * Adds line distance metrics to line features. Defaults to false.
   */
  lineMetrics?: boolean;
  /**
   * Clusters point features. Defaults to false.
   */
  cluster?: boolean;
  /**
   * Slices requested tiles inline during the update pass. Defaults to false.
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
   * running transitions; use `mln_map_cancel_transitions()` for that.
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
  /**
   * One `mln_gpu_sync_kind` value.
   */
  kind?: GpuSyncKind;
  /**
   * Bit pattern of the GLsync pointer.
   */
  object?: bigint;
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
   * caller-graphics-thread driver. object is nonzero.
   */
  openglFence: 3,
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
  name: string;
  value: string;
}

/**
 * One rule of a native HTTP header transform.
 *
 * See `mln_http_header_rule` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface HttpHeaderRule {
  /**
   * The resource kinds to match.
   */
  kinds?: ResourceKindMask;
  /**
   * A bitwise OR of `mln_url_match_flags` values.
   */
  flags?: UrlMatchFlags;
  /**
   * The URL or glob pattern to compare.
   */
  url?: string;
  /**
   * The headers to set. Registration validates each one as
   * `mln_http_header_transform_response_set_header()` does.
   */
  headers?: readonly HttpHeader[];
}

/**
 * HTTP header transform state: a callback, or native header rules.
 *
 * See `mln_http_header_transform` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface HttpHeaderTransform {
  /**
   * Header rules for a transform without a callback; empty with one.
   */
  rules: readonly HttpHeaderRule[];
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
 * Process-global log callback or queue state.
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
 * See `mln_log_severity_mask` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
export const LogSeverityMask = {
  info: 2,
  warning: 4,
  error: 8,
  defaultValue: 6,
  all: 14,
} as const;
/** A bitwise OR of LogSeverityMask flags. */
export type LogSeverityMask = number;

/**
 * Logical extent in UI pixels and the device-pixel scale.
 *
 * See `mln_logical_extent` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface LogicalExtent {
  /**
   * Width in UI pixels. Defaults to 256.
   */
  width?: number;
  /**
   * Height in UI pixels. Defaults to 256.
   */
  height?: number;
  /**
   * Device pixels per UI pixel. Defaults to 1.0.
   */
  scaleFactor?: number;
}

/**
 * Debug overlay mask values for `mln_map_set_debug_options()`.
 *
 * See `mln_map_debug_option` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const MapDebugOption = {
  tileBorders: 2,
  parseStatus: 4,
  timestamps: 8,
  collision: 16,
  overdraw: 32,
  stencilClip: 64,
  depthBuffer: 128,
} as const;
/** A bitwise OR of MapDebugOption flags. */
export type MapDebugOption = number;

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
   * Initial logical extent. Width and height must be nonzero, and scale_factor
   * must be finite and positive. scale_factor is fixed for the map's lifetime.
   */
  initialExtent?: LogicalExtent;
  /**
   * One of `mln_map_mode`. Defaults to `MLN_MAP_MODE_CONTINUOUS`.
   */
  mapMode?: MapMode;
  /**
   * Decodes MapLibre Tile (MLT) tiles whose integer streams use FastPFOR
   * encodings. Defaults to false.
   */
  fastPforEnabled?: boolean;
  /**
   * Map-originated event types this map queues, as a bitwise OR of
   * `mln_runtime_event_mask` values.
   */
  eventMask?: RuntimeEventMask;
}

/**
 * Immutable map state copied from the latest published generation.
 *
 * See `mln_map_snapshot` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface MapSnapshot {
  /**
   * Debug overlay mask of `mln_map_debug_option` values.
   */
  readonly debugOptions: MapDebugOption;
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
  readonly eventMask: RuntimeEventMask;
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
   * `mln_map_projection_create()` converts coordinates for this view.
   */
  readonly view: MapView;
}

/**
 * Field mask values for `mln_map_tile_options`.
 *
 * See `mln_map_tile_option_field` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const MapTileOptionField = {
  prefetchZoomDelta: 1,
  lodMinRadius: 2,
  lodScale: 4,
  lodPitchThreshold: 8,
  lodZoomShift: 16,
  lodMode: 32,
} as const;
/** A bitwise OR of MapTileOptionField flags. */
export type MapTileOptionField = number;

/**
 * Tile prefetch and LOD tuning controls.
 *
 * See `mln_map_tile_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface MapTileOptions {
  /**
   * Native uint8_t prefetch zoom delta.
   */
  prefetchZoomDelta?: number;
  lodMinRadius?: number;
  lodScale?: number;
  lodPitchThreshold?: number;
  lodZoomShift?: number;
  /**
   * One of `mln_tile_lod_mode`.
   */
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
   * copy and may fall outside -180 to 180. Every field except
   * `MLN_CAMERA_OPTION_ANCHOR` is set.
   */
  camera?: CameraOptions;
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
 * Field mask values for `mln_map_viewport_options`.
 *
 * See `mln_map_viewport_option_field` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const MapViewportOptionField = {
  northOrientation: 1,
  constrainMode: 2,
  viewportMode: 4,
  frustumOffset: 8,
} as const;
/** A bitwise OR of MapViewportOptionField flags. */
export type MapViewportOptionField = number;

/**
 * Live map viewport and render-transform controls.
 *
 * See `mln_map_viewport_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface MapViewportOptions {
  /**
   * One of `mln_north_orientation`.
   */
  northOrientation?: NorthOrientation;
  /**
   * One of `mln_constrain_mode`.
   */
  constrainMode?: ConstrainMode;
  /**
   * One of `mln_viewport_mode`.
   */
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
  texture?: bigint;
}

/**
 * Metal attachment options for a borrowed texture target.
 *
 * See `mln_metal_borrowed_texture_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface MetalBorrowedTextureDescriptor {
  /**
   * Logical texture extent. The map viewport uses width and height and the
   * renderer uses scale_factor; the physical size is stated separately below. A
   * scale_factor that differs from the map's is accepted and logged as a
   * warning.
   */
  extent?: LogicalExtent;
  /**
   * Physical texture width in device pixels. Must be positive. Defaults to 256.
   */
  physicalWidth?: number;
  /**
   * Physical texture height in device pixels. Must be positive. Defaults to
   * 256.
   */
  physicalHeight?: number;
  /**
   * The ring's textures, one per slot, in slot order. Required.
   */
  textures?: readonly MetalBorrowedTexture[];
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
   * Logical texture extent. A scale_factor that differs from the map's is
   * accepted and logged as a warning.
   */
  extent?: LogicalExtent;
  /**
   * Metal backend context. device is required.
   */
  context?: MetalContextDescriptor;
  /**
   * Backend-native MTLPixelFormat value of the ring's textures: either
   * MTLPixelFormatRGBA8Unorm or MTLPixelFormatBGRA8Unorm. Defaults to
   * MTLPixelFormatRGBA8Unorm. Readback returns RGBA8 for either format.
   */
  pixelFormat?: bigint;
}

/**
 * Metal attachment options for a native surface.
 *
 * See `mln_metal_surface_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
export interface MetalSurfaceDescriptor {
  /**
   * Logical surface extent. A scale_factor that differs from the map's is
   * accepted and logged as a warning.
   */
  extent?: LogicalExtent;
  /**
   * Metal backend context. device is optional for Metal surfaces.
   */
  context?: MetalContextDescriptor;
  /**
   * `CAMetalLayer*` / `CA::MetalLayer*` retained by the session. Required.
   */
  layer?: bigint;
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
   * Opaque frame identity used to reject stale releases.
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
 * Map north orientation values used by `mln_map_viewport_options`.
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
  styleUrl: string;
  /**
   * UTF-8 GeoJSON Geometry bytes. Borrowed during region creation.
   */
  geometry: string;
  minZoom?: number;
  /**
   * Maximum zoom. Positive infinity follows MapLibre Native behavior and lets
   * each tile source use its own maximum zoom.
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
export interface OfflineRegionDefinition {
  data: OfflineRegionDefinitionData;
}

/**
 * Offline region definition data.
 *
 * See `mln_offline_region_definition_data` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export type OfflineRegionDefinitionData =
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

export const OfflineRegionDefinitionType = {
  tilePyramid: 1,
  geometry: 2,
} as const;
/** One of the OfflineRegionDefinitionType values, or a value this binding does not know. */
export type OfflineRegionDefinitionType =
  | (typeof OfflineRegionDefinitionType)[keyof typeof OfflineRegionDefinitionType]
  | (number & {});

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
  /**
   * One of `mln_offline_region_download_state`.
   */
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
  styleUrl: string;
  bounds?: LatLngBounds;
  minZoom?: number;
  /**
   * Maximum zoom. Positive infinity follows MapLibre Native behavior and lets
   * each tile source use its own maximum zoom.
   */
  maxZoom?: number;
  pixelRatio?: number;
  includeIdeographs?: boolean;
}

/**
 * One caller-owned OpenGL texture of a borrowed texture ring.
 *
 * See `mln_opengl_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface OpenglBorrowedTexture {
  /**
   * Borrowed OpenGL texture object name. Required.
   */
  texture?: number;
}

/**
 * OpenGL attachment options for a borrowed texture target.
 *
 * See `mln_opengl_borrowed_texture_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface OpenglBorrowedTextureDescriptor {
  /**
   * Logical texture extent. The map viewport uses width and height and the
   * renderer uses scale_factor; the physical size is stated separately below. A
   * scale_factor that differs from the map's is accepted and logged as a
   * warning.
   */
  extent?: LogicalExtent;
  /**
   * Physical texture width in device pixels. Must be positive. Defaults to 256.
   */
  physicalWidth?: number;
  /**
   * Physical texture height in device pixels. Must be positive. Defaults to
   * 256.
   */
  physicalHeight?: number;
  /**
   * Borrowed OpenGL context provider data. The textures must belong to this
   * context or a context in the same share group.
   */
  context?: OpenglContextDescriptor;
  /**
   * The ring's textures, one per slot, in slot order. Required.
   */
  textures?: readonly OpenglBorrowedTexture[];
  /**
   * OpenGL texture target of every texture. Must be GL_TEXTURE_2D.
   */
  target?: number;
}

/**
 * OpenGL client API a dedicated EGL session creates its context for.
 *
 * See `mln_opengl_client_api` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const OpenglClientApi = {
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
/** One of the OpenglClientApi values, or a value this binding does not know. */
export type OpenglClientApi =
  | (typeof OpenglClientApi)[keyof typeof OpenglClientApi]
  | (number & {});

/**
 * OpenGL backend context fields shared by OpenGL render targets.
 *
 * See `mln_opengl_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface OpenglContextDescriptor {
  /**
   * Whether the session shares its driver thread and graphics objects with the
   * host. A private EGL owned texture and a transferred WebGL canvas are
   * dedicated to their core worker.
   */
  ownership?: OpenglContextOwnership;
  data: OpenglContextDescriptorData;
}

/**
 * Backend-specific OpenGL context data.
 *
 * See `mln_opengl_context_descriptor_data` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export type OpenglContextDescriptorData =
  | { readonly kind: "wgl"; readonly value: WglContextDescriptor }
  | { readonly kind: "egl"; readonly value: EglContextDescriptor }
  | { readonly kind: "webgl"; readonly value: WebglContextDescriptor }
  | {
      readonly kind: "unknown";
      readonly tag: number;
      readonly bytes: Uint8Array;
    };

/**
 * How a session's OpenGL context relates to its driver thread and host graphics
 * state.
 *
 * See `mln_opengl_context_ownership` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const OpenglContextOwnership = {
  /**
   * The session shares its thread with host graphics work.
   */
  shared: 0,
  /**
   * The session owns its thread's OpenGL context.
   */
  dedicated: 1,
} as const;
/** One of the OpenglContextOwnership values, or a value this binding does not know. */
export type OpenglContextOwnership =
  | (typeof OpenglContextOwnership)[keyof typeof OpenglContextOwnership]
  | (number & {});

/**
 * OpenGL platform context provider used by a context descriptor.
 *
 * See `mln_opengl_context_platform` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const OpenglContextPlatform = {
  /**
   * No OpenGL context provider is selected.
   */
  unspecified: 0,
  wgl: 1,
  egl: 2,
  /**
   * Emscripten WebGL context handle.
   */
  webgl: 3,
} as const;
/** One of the OpenglContextPlatform values, or a value this binding does not know. */
export type OpenglContextPlatform =
  | (typeof OpenglContextPlatform)[keyof typeof OpenglContextPlatform]
  | (number & {});

/**
 * OpenGL context providers supported by this build.
 *
 * See `mln_opengl_context_provider_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const OpenglContextProviderFlag = {
  wgl: 1,
  egl: 2,
  /**
   * Browser WebGL context imported into an Emscripten module.
   */
  webgl: 4,
} as const;
/** A bitwise OR of OpenglContextProviderFlag flags. */
export type OpenglContextProviderFlag = number;

/**
 * OpenGL attachment options for an owned texture target.
 *
 * See `mln_opengl_owned_texture_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface OpenglOwnedTextureDescriptor {
  /**
   * Logical texture extent. A scale_factor that differs from the map's is
   * accepted and logged as a warning.
   */
  extent?: LogicalExtent;
  /**
   * Borrowed OpenGL context provider data. Shared ownership creates a context
   * whose texture frames the host can acquire. Dedicated EGL or transferred
   * WebGL ownership creates a private core-worker context for CPU readback.
   */
  context?: OpenglContextDescriptor;
}

/**
 * OpenGL attachment options for a native surface.
 *
 * See `mln_opengl_surface_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
export interface OpenglSurfaceDescriptor {
  /**
   * Logical surface extent. A scale_factor that differs from the map's is
   * accepted and logged as a warning.
   */
  extent?: LogicalExtent;
  /**
   * Borrowed OpenGL context provider data.
   */
  context?: OpenglContextDescriptor;
  /**
   * Borrowed platform surface handle: an HDC for WGL and an EGLSurface for EGL,
   * both required. Null for WebGL, whose context carries its canvas binding.
   */
  surface?: bigint;
}

/**
 * OpenGL frame acquired from a texture ring.
 *
 * See `mln_opengl_texture_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface OpenglTextureFrame {
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
   * Opaque frame identity used to reject stale releases.
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
  width?: number;
  height?: number;
  /**
   * Bytes per image row. Must be at least width &#42; 4.
   */
  stride?: number;
  /**
   * Premultiplied RGBA8 pixels. Must not be null for a non-empty image.
   */
  pixels?: Uint8Array;
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
   * Native x-skew factor used by the axonometric transform.
   */
  xSkew?: number;
  /**
   * Native y-skew factor used by the axonometric transform.
   */
  ySkew?: number;
}

/**
 * Field mask values for MapLibre axonometric rendering options.
 *
 * See `mln_projection_mode_field` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export const ProjectionModeField = {
  axonometric: 1,
  xSkew: 2,
  ySkew: 4,
} as const;
/** A bitwise OR of ProjectionModeField flags. */
export type ProjectionModeField = number;

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
 * One query hit borrowed for a completion callback.
 *
 * See `mln_queried_feature` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
export interface QueriedFeature {
  readonly feature: string;
  readonly sourceId?: string;
  readonly sourceLayerId?: string;
  readonly state?: string;
}

/**
 * Optional fields for `mln_queried_feature`.
 *
 * See `mln_queried_feature_field` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
export const QueriedFeatureField = {
  sourceId: 1,
  sourceLayerId: 2,
  state: 4,
} as const;
/** A bitwise OR of QueriedFeatureField flags. */
export type QueriedFeatureField = number;

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
  /**
   * One `mln_render_abandon_disposition` value.
   */
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
  opengl: 4,
  webgpu: 8,
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
 * A borrowed view of one owned frame-result batch.
 *
 * See `mln_render_frame_batch_view` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
export interface RenderFrameBatchView {
  /**
   * Borrowed array of result_count terminal frame results in completion order.
   */
  readonly results: readonly RenderFrameResult[];
}

/**
 * Terminal result of one frame demand, held by an owned frame-result batch and
 * copied by `mln_acquired_frame_get_result()`.
 *
 * See `mln_render_frame_result` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
export interface RenderFrameResult {
  /**
   * One `mln_render_result` value.
   */
  readonly disposition: RenderResult;
  readonly token: bigint;
  /**
   * Generation of the map render update the demand evaluated. When disposition
   * is `MLN_RENDER_RESULT_RENDERED`, the frame drew that update; compare it
   * with `mln_completion_result.render_update_generation` to find the first
   * frame that includes a command.
   */
  readonly mapUpdateGeneration: bigint;
  /**
   * The session's target generation when the demand reached its result. See
   * `mln_render_session_snapshot.generation`.
   */
  readonly generation: bigint;
  /**
   * Position of the frame among the frames that the session rendered, counting
   * from one. Zero unless disposition is `MLN_RENDER_RESULT_RENDERED`.
   */
  readonly frameGeneration: bigint;
  /**
   * Whether the map asked for another frame while it rendered this one, as
   * during an ongoing paint transition. Set only when disposition is
   * `MLN_RENDER_RESULT_RENDERED`, and false for every other outcome. This is
   * the same signal that `MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED` carries
   * in its needs_repaint field, delivered with the frame result so a host can
   * re-arm its frame loop without the runtime event round trip. A camera
   * transition does not set it by itself: the map publishes a new update after
   * each of the transition's frames instead, which a render-if-needed demand
   * renders. A demand with `MLN_FRAME_DEMAND_WAIT_FOR_UPDATE` renders each
   * transition update without a runtime-event round trip; the host re-arms the
   * demand as each result arrives.
   */
  readonly needsRepaint: boolean;
  /**
   * The view that this frame drew, from the transform of the map update that it
   * rendered. A host that displays the frame builds its projection with
   * `mln_map_projection_create()` to place overlays where the frame drew their
   * coordinates. Zero, with a size of zero, unless disposition is
   * `MLN_RENDER_RESULT_RENDERED`.
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
   * update when it has one. A demand with `MLN_FRAME_DEMAND_WAIT_FOR_UPDATE`
   * waits for that update instead, and finishes with this result only when a
   * barrier ends its wait.
   */
  noUpdate: 1,
  /**
   * The target could not produce a frame. The attempt consumes nothing, so a
   * later demand with the same flags renders what this one would have. This
   * result does not cause a map update, so the host demands again when the
   * target can be ready, such as after a paced delay.
   */
  targetNotReady: 3,
  /**
   * A newer IF_NEEDED demand with the same flags replaced this demand.
   */
  superseded: 4,
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
   * One `mln_render_driver_kind` value. Defaults to
   * `MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD`.
   */
  driver?: RenderDriverKind;
  /**
   * Requested slot count of a session-owned texture ring, from one to
   * `MLN_TEXTURE_RING_MAX_DEPTH`. Private targets grant one slot regardless of
   * this value. A borrowed texture ring's depth is its texture count, so
   * borrowed and other targets ignore this value. Defaults to 1.
   */
  requestedTextureRingDepth?: number;
  /**
   * Producer synchronization for acquired texture frames. Defaults to
   * `MLN_GPU_SYNC_CPU_COMPLETE`. `MLN_GPU_SYNC_OPENGL_FENCE` requires a shared
   * EGL or WGL texture target and a caller-graphics-thread driver. Other
   * targets support only CPU completion and fail an unsupported request.
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
}

/**
 * Driver and target capabilities fixed for one attached render session.
 *
 * See `mln_render_session_capabilities` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface RenderSessionCapabilities {
  /**
   * One `mln_render_driver_kind` value.
   */
  readonly driver: RenderDriverKind;
  /**
   * Granted texture ring depth: the slot count of a session-owned ring, or the
   * texture count of a borrowed one. Zero for a surface.
   */
  readonly textureRingDepth: number;
  /**
   * A bitwise OR of `mln_render_session_capability_flag` values.
   */
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
  /**
   * One `mln_render_session_state` value.
   */
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
   * Acquired frames that the host has not released or disposed.
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
  targetLost: 5,
  abandoned: 6,
} as const;
/** One of the RenderSessionState values, or a value this binding does not know. */
export type RenderSessionState =
  | (typeof RenderSessionState)[keyof typeof RenderSessionState]
  | (number & {});

/**
 * Optional fields for `mln_rendered_feature_query_options`.
 *
 * See `mln_rendered_feature_query_option_field` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
export const RenderedFeatureQueryOptionField = {
  layerIds: 1,
  filter: 2,
} as const;
/** A bitwise OR of RenderedFeatureQueryOptionField flags. */
export type RenderedFeatureQueryOptionField = number;

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
   * Optional UTF-8 MapLibre style-spec filter JSON. When absent, no filter.
   */
  filter?: string;
}

/**
 * Rendered feature query geometry descriptor.
 *
 * See `mln_rendered_query_geometry` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
export interface RenderedQueryGeometry {
  data: RenderedQueryGeometryData;
}

/**
 * Screen-space query geometry data.
 *
 * See `mln_rendered_query_geometry_data` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
export type RenderedQueryGeometryData =
  | { readonly kind: "point"; readonly value: ScreenPoint }
  | { readonly kind: "box"; readonly value: ScreenBox }
  | { readonly kind: "lineString"; readonly value: ScreenLineString }
  | {
      readonly kind: "unknown";
      readonly tag: number;
      readonly bytes: Uint8Array;
    };

/**
 * Rendered feature query geometry variants.
 *
 * See `mln_rendered_query_geometry_type` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
export const RenderedQueryGeometryType = {
  point: 1,
  box: 2,
  lineString: 3,
} as const;
/** One of the RenderedQueryGeometryType values, or a value this binding does not know. */
export type RenderedQueryGeometryType =
  | (typeof RenderedQueryGeometryType)[keyof typeof RenderedQueryGeometryType]
  | (number & {});

/**
 * Rendering statistics reported in `MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME`.
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
 * The resource kinds that a route or rule matches, one bit per
 * `mln_resource_kind` value. Registration requires at least one known bit.
 * `MLN_RESOURCE_KIND_MASK_ALL` selects every resource kind.
 *
 * See `mln_resource_kind_mask` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const ResourceKindMask = {
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
/** A bitwise OR of ResourceKindMask flags. */
export type ResourceKindMask = number;

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
 * Resource provider state.
 *
 * See `mln_resource_provider` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface ResourceProvider {
  /**
   * Decides each request that the routes leave to the provider.
   */
  callback: (
    request: ResourceRequestInfo,
    handle: ResourceRequestHandle,
  ) => void;
  /**
   * The requests that reach the provider; empty to claim every request.
   */
  routes: readonly ResourceRoute[];
}

export const ResourceProviderDecision = {
  passThrough: 0,
  handle: 1,
} as const;
/** One of the ResourceProviderDecision values, or a value this binding does not know. */
export type ResourceProviderDecision =
  | (typeof ResourceProviderDecision)[keyof typeof ResourceProviderDecision]
  | (number & {});

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
 * Cancel callback or queue state for one handled resource request.
 *
 * See `mln_resource_request_cancel_handler` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export type ResourceRequestCancelHandler = () => void;

/**
 * Field mask values for `mln_resource_request_info`.
 *
 * See `mln_resource_request_field` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const ResourceRequestField = {
  /**
   * The request asks only for the bytes in range.
   */
  range: 1,
  /**
   * The cached copy being revalidated carries a modification time.
   */
  priorModified: 2,
  /**
   * The cached copy being revalidated carries an expiration time.
   */
  priorExpires: 4,
} as const;
/** A bitwise OR of ResourceRequestField flags. */
export type ResourceRequestField = number;

export interface ResourceRequestInfo {
  /**
   * URL entering the network layer, before tile server normalization.
   */
  readonly requestedUrl?: string;
  /**
   * URL to fetch, after resource-kind normalization against the runtime's tile
   * server options and API key.
   */
  readonly resolvedUrl?: string;
  readonly kind: ResourceKind;
  readonly loadingMethod: ResourceLoadingMethod;
  readonly priority: ResourcePriority;
  readonly usage: ResourceUsage;
  readonly storagePolicy: ResourceStoragePolicy;
  readonly range?: ResourceRange;
  readonly priorModifiedUnixMs?: bigint;
  readonly priorExpiresUnixMs?: bigint;
  readonly priorEtag?: string;
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
   * Response bytes. May be null only when byte_count is 0.
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
 * Field mask values for `mln_resource_response`.
 *
 * See `mln_resource_response_field` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const ResourceResponseField = {
  /**
   * The response carries a modification time.
   */
  modified: 1,
  /**
   * The response carries an expiration time.
   */
  expires: 2,
  /**
   * An ERROR response carries the earliest time to retry the request.
   */
  retryAfter: 4,
} as const;
/** A bitwise OR of ResourceResponseField flags. */
export type ResourceResponseField = number;

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
   * The resource kinds to match.
   */
  kinds?: ResourceKindMask;
  /**
   * A bitwise OR of `mln_url_match_flags` values.
   */
  flags?: UrlMatchFlags;
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
 * Resource transform state.
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
  /**
   * One of `mln_runtime_event_type`.
   */
  readonly type: RuntimeEventType;
  /**
   * One of `mln_runtime_event_source_type`.
   */
  readonly sourceType: RuntimeEventSourceType;
  /**
   * Source handle selected by source_type: an `mln_runtime` or an `mln_map`.
   */
  readonly source: bigint;
  /**
   * Map snapshot generation that the event reports, or zero when source_type is
   * `MLN_RUNTIME_EVENT_SOURCE_RUNTIME`.
   */
  readonly generation: bigint;
  /**
   * Secondary event detail whose meaning type selects. Depending on type it
   * carries an `mln_camera_change_mode`, a MapLibre Native error ordinal, or 0.
   * See `mln_runtime_event_type` for the per-type meaning.
   */
  readonly code: number;
  /**
   * Borrowed UTF-8 message bytes, empty when this event carries no message.
   */
  readonly message: string;
  /**
   * Typed payload selected by payload_type.
   */
  readonly payload: RuntimeEventPayload;
}

/**
 * Runtime and map event delivery. Set either callback or queue.
 *
 * See `mln_runtime_event_handler` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export type RuntimeEventHandler = (events: EventBatchView) => void;

/**
 * Bit values for the map and runtime event subscription masks.
 *
 * See `mln_runtime_event_mask` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const RuntimeEventMask = {
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
  mapRenderError: 1024n,
  mapRenderFrameStarted: 8192n,
  mapRenderFrameFinished: 16384n,
  mapRenderMapStarted: 32768n,
  mapRenderMapFinished: 65536n,
  mapStyleImageMissing: 131072n,
  mapTileAction: 262144n,
  offlineRegionStatusChanged: 524288n,
  offlineRegionResponseError: 1048576n,
  offlineRegionTileCountLimitExceeded: 2097152n,
  mapSourceChanged: 8388608n,
  /**
   * Selects every map-originated event type this version defines.
   */
  allMapEvents: 8906238n,
  /**
   * Selects every runtime-originated event type this version defines.
   */
  allRuntimeEvents: 3670016n,
  /**
   * Selects every event type this version defines.
   */
  all: 12576254n,
} as const;
/** A bitwise OR of RuntimeEventMask flags. */
export type RuntimeEventMask = bigint;

/**
 * Payload for `MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR`.
 *
 * See `mln_runtime_event_offline_region_response_error` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeEventOfflineRegionResponseError {
  readonly regionId: bigint;
  /**
   * One of `mln_resource_error_reason`.
   */
  readonly reason: ResourceErrorReason;
}

/**
 * Payload for `MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED`.
 *
 * See `mln_runtime_event_offline_region_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeEventOfflineRegionStatus {
  readonly regionId: bigint;
  readonly status: OfflineRegionStatus;
}

/**
 * Payload for `MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED`.
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
 * Payload kinds used by `mln_runtime_event.payload_type`.
 *
 * See `mln_runtime_event_payload_type` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const RuntimeEventPayloadType = {
  none: 0,
  renderFrame: 1,
  renderMap: 2,
  tileAction: 4,
  offlineRegionStatus: 5,
  offlineRegionResponseError: 6,
  offlineRegionTileCountLimit: 7,
} as const;
/** One of the RuntimeEventPayloadType values, or a value this binding does not know. */
export type RuntimeEventPayloadType =
  | (typeof RuntimeEventPayloadType)[keyof typeof RuntimeEventPayloadType]
  | (number & {});

/**
 * Payload for `MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED`.
 *
 * See `mln_runtime_event_render_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeEventRenderFrame {
  /**
   * One of `mln_render_mode`.
   */
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
 * Payload for `MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED`.
 *
 * See `mln_runtime_event_render_map` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeEventRenderMap {
  /**
   * One of `mln_render_mode`.
   */
  readonly mode: RenderMode;
}

/**
 * Source kinds used by `mln_runtime_event.source_type`.
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
 * Payload for `MLN_RUNTIME_EVENT_MAP_TILE_ACTION`.
 *
 * See `mln_runtime_event_tile_action` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export interface RuntimeEventTileAction {
  /**
   * One of `mln_tile_operation`.
   */
  readonly operation: TileOperation;
  readonly tileId: TileId;
}

/**
 * Runtime event types carried by `mln_runtime_event.type`.
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
  mapRenderError: 10,
  mapRenderFrameStarted: 13,
  mapRenderFrameFinished: 14,
  mapRenderMapStarted: 15,
  mapRenderMapFinished: 16,
  mapStyleImageMissing: 17,
  mapTileAction: 18,
  offlineRegionStatusChanged: 19,
  offlineRegionResponseError: 20,
  offlineRegionTileCountLimitExceeded: 21,
  mapSourceChanged: 23,
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
   * Cache database path. Copied during runtime creation.
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
   * Points. Null only when point_count is 0.
   */
  points: readonly ScreenPoint[];
}

/**
 * Screen-space point in logical map pixels.
 *
 * See `mln_screen_point` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
export interface ScreenPoint {
  x?: number;
  y?: number;
}

/**
 * Optional fields for `mln_source_feature_query_options`.
 *
 * See `mln_source_feature_query_option_field` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
export const SourceFeatureQueryOptionField = {
  sourceLayerIds: 1,
  filter: 2,
} as const;
/** A bitwise OR of SourceFeatureQueryOptionField flags. */
export type SourceFeatureQueryOptionField = number;

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
   * Optional UTF-8 MapLibre style-spec filter JSON. When absent, no filter.
   */
  filter?: string;
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
   * A native MapLibre error or C++ exception was converted to status.
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
 * One complete runtime style image, borrowed for a completion callback.
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
 * Field mask values for `mln_style_image_info`.
 *
 * See `mln_style_image_info_field` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export const StyleImageInfoField = {
  /**
   * The image declares a content box.
   */
  content: 1,
  /**
   * The image declares a horizontal text-fit mode.
   */
  textFitWidth: 2,
  /**
   * The image declares a vertical text-fit mode.
   */
  textFitHeight: 4,
} as const;
/** A bitwise OR of StyleImageInfoField flags. */
export type StyleImageInfoField = number;

/**
 * Field mask values for `mln_style_image_options`.
 *
 * See `mln_style_image_option_field` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export const StyleImageOptionField = {
  pixelRatio: 1,
  sdf: 2,
  stretchX: 4,
  stretchY: 8,
  content: 16,
  textFitWidth: 32,
  textFitHeight: 64,
} as const;
/** A bitwise OR of StyleImageOptionField flags. */
export type StyleImageOptionField = number;

/**
 * Options for runtime style images.
 *
 * See `mln_style_image_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface StyleImageOptions {
  /**
   * Horizontally stretchable intervals. Borrowed for the call and copied before
   * return. May be null only when stretch_x_count is 0.
   */
  stretchX?: readonly ImageStretch[];
  /**
   * Vertically stretchable intervals. Borrowed for the call and copied before
   * return. May be null only when stretch_y_count is 0.
   */
  stretchY?: readonly ImageStretch[];
  /**
   * Content box used when icon-text-fit applies.
   */
  content?: ImageContent;
  /**
   * One of `mln_style_image_text_fit`. Defaults to STRETCH_OR_SHRINK.
   */
  textFitWidth?: StyleImageTextFit;
  /**
   * One of `mln_style_image_text_fit`. Defaults to STRETCH_OR_SHRINK.
   */
  textFitHeight?: StyleImageTextFit;
  /**
   * Sprite pixel ratio. Defaults to 1.
   */
  pixelRatio?: number;
  /**
   * Whether the image is a signed distance field icon. Defaults to false.
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
 * One style layer, borrowed for a completion callback.
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
  readonly sourceId?: string;
  /**
   * Source-layer ID. Empty when the layer sets none.
   */
  readonly sourceLayer?: string;
  /**
   * Lowest zoom at which the layer draws; -INFINITY with no lower bound.
   */
  readonly minZoom: number;
  /**
   * Highest zoom at which the layer draws; INFINITY with no upper bound.
   */
  readonly maxZoom: number;
  /**
   * One of `mln_style_layer_visibility`.
   */
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
 * Complete metadata of one style source, borrowed for a completion callback.
 *
 * See `mln_style_source_info` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface StyleSourceInfo {
  /**
   * Source ID.
   */
  readonly id: string;
  /**
   * One of `mln_style_source_type`.
   */
  readonly type: StyleSourceType;
  /**
   * Whether the source is marked volatile.
   */
  readonly isVolatile: boolean;
  /**
   * Attribution string, when the source sets one. It may be empty.
   */
  readonly attribution?: string;
  /**
   * URL that the source loads from, when it has one.
   */
  readonly url?: string;
  /**
   * Inline TileJSON metadata, when the source was defined with it.
   */
  readonly tilejson?: StyleSourceTileInfo;
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
 * Fields available in `mln_style_source_info`.
 *
 * See `mln_style_source_info_field` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export const StyleSourceInfoField = {
  /**
   * The source retains a URL.
   */
  url: 1,
  /**
   * The tile source was defined with an inline TileJSON description.
   */
  tilejson: 2,
  /**
   * The inline TileJSON description contains geographic bounds.
   */
  bounds: 4,
  /**
   * The source exposes a tile size.
   */
  tileSize: 8,
  /**
   * The source exposes a vector tile encoding.
   */
  vectorEncoding: 16,
  /**
   * The source exposes a DEM raster encoding.
   */
  rasterEncoding: 32,
  /**
   * The source carries an attribution string, which may be empty.
   */
  attribution: 64,
} as const;
/** A bitwise OR of StyleSourceInfoField flags. */
export type StyleSourceInfoField = number;

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
  /**
   * One of `mln_style_tile_scheme`.
   */
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
  geojson: 4,
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
 * Field mask values for `mln_style_transition_options`.
 *
 * See `mln_style_transition_option_field` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export const StyleTransitionOptionField = {
  duration: 1,
  delay: 2,
  enablePlacementTransitions: 4,
} as const;
/** A bitwise OR of StyleTransitionOptionField flags. */
export type StyleTransitionOptionField = number;

/**
 * Global style transition options.
 *
 * See `mln_style_transition_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
export interface StyleTransitionOptions {
  /**
   * Transition duration in milliseconds. Must be finite and non-negative.
   * Values that would overflow MapLibre Native's internal duration are invalid.
   */
  durationMs?: number;
  /**
   * Transition delay in milliseconds. Must be finite and non-negative. Values
   * that would overflow MapLibre Native's internal duration are invalid.
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
 * Texture readback borrowed for a completion callback.
 *
 * See `mln_texture_readback_result` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface TextureReadbackResult {
  /**
   * Borrowed pixel bytes, valid only during the callback.
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
 * Tile LOD algorithms used by `mln_map_tile_options`.
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
 * See `mln_url_match_flags` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
export const UrlMatchFlags = {
  exact: 0,
  /**
   * Reads url as a glob pattern.
   */
  glob: 1,
} as const;
/** A bitwise OR of UrlMatchFlags flags. */
export type UrlMatchFlags = number;

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
 * Viewport orientation modes used by `mln_map_viewport_options`.
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
  image?: bigint;
  /**
   * Borrowed VkImageView for image. Required. The view must be a 2D color view
   * that matches image and the descriptor's format.
   */
  imageView?: bigint;
}

/**
 * Vulkan attachment options for a borrowed texture target.
 *
 * See `mln_vulkan_borrowed_texture_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface VulkanBorrowedTextureDescriptor {
  /**
   * Logical texture extent. The map viewport uses width and height and the
   * renderer uses scale_factor; the physical size is stated separately below. A
   * scale_factor that differs from the map's is accepted and logged as a
   * warning.
   */
  extent?: LogicalExtent;
  /**
   * Physical image width in device pixels. Must be positive. Defaults to 256.
   */
  physicalWidth?: number;
  /**
   * Physical image height in device pixels. Must be positive. Defaults to 256.
   */
  physicalHeight?: number;
  /**
   * Borrowed Vulkan context. All handles are required.
   */
  context?: VulkanContextDescriptor;
  /**
   * The ring's images, one per slot, in slot order. Required.
   */
  textures?: readonly VulkanBorrowedTexture[];
  /**
   * Backend-native VkFormat value of every image. VK_FORMAT_UNDEFINED is
   * invalid.
   */
  format?: number;
  /**
   * Backend-native VkImageLayout value expected at render-pass begin.
   */
  initialLayout?: number;
  /**
   * Backend-native VkImageLayout value left after rendering succeeds. Defaults
   * to 5, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
   */
  finalLayout?: number;
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
  instance?: bigint;
  /**
   * Borrowed VkPhysicalDevice. Required.
   */
  physicalDevice?: bigint;
  /**
   * Borrowed VkDevice. Required.
   */
  device?: bigint;
  /**
   * Borrowed graphics VkQueue. Required. The session's driver submits to it
   * from its own thread, so a host that uses the same queue passes
   * `mln_render_session_attach_options.queue_lock` at attach.
   */
  graphicsQueue?: bigint;
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
   * Logical texture extent. A scale_factor that differs from the map's is
   * accepted and logged as a warning.
   */
  extent?: LogicalExtent;
  /**
   * Borrowed Vulkan context. All handles are required.
   */
  context?: VulkanContextDescriptor;
}

/**
 * Vulkan attachment options for a native surface.
 *
 * See `mln_vulkan_surface_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
export interface VulkanSurfaceDescriptor {
  /**
   * Logical surface extent. A scale_factor that differs from the map's is
   * accepted and logged as a warning.
   */
  extent?: LogicalExtent;
  /**
   * Borrowed Vulkan context. All handles are required. The device must support
   * VK_KHR_swapchain, and the queue family must support graphics and
   * presentation to this descriptor's surface.
   */
  context?: VulkanContextDescriptor;
  /**
   * Borrowed VkSurfaceKHR bit pattern. Required.
   */
  surface?: bigint;
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
   * Opaque frame identity used to reject stale releases.
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
 * thread.
 *
 * See `mln_wake` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wake_8h.html).
 */
export type Wake = () => void;

/**
 * WebGL context fields shared by OpenGL render targets in the browser.
 *
 * See `mln_webgl_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface WebglContextDescriptor {
  /**
   * One `mln_webgl_context_kind` value.
   */
  kind?: WebglContextKind;
  /**
   * Borrowed EMSCRIPTEN_WEBGL_CONTEXT_HANDLE for EXISTING. Must be positive.
   */
  context?: number;
  /**
   * Copied UTF-8 Emscripten target selector for TRANSFERRED_CANVAS. The HTML
   * canvas must still be transferable when attachment starts.
   */
  canvasSelector: string;
}

/**
 * WebGL context placement.
 *
 * See `mln_webgl_context_kind` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export const WebglContextKind = {
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
/** One of the WebglContextKind values, or a value this binding does not know. */
export type WebglContextKind =
  | (typeof WebglContextKind)[keyof typeof WebglContextKind]
  | (number & {});

/**
 * One caller-owned WebGPU texture of a borrowed texture ring.
 *
 * See `mln_webgpu_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface WebgpuBorrowedTexture {
  /**
   * Borrowed WGPUTexture. Required.
   */
  texture?: bigint;
  /**
   * Borrowed WGPUTextureView for texture. Required. The view must be a 2D color
   * view compatible with texture and the descriptor's format.
   */
  textureView?: bigint;
}

/**
 * WebGPU attachment options for a borrowed texture target.
 *
 * See `mln_webgpu_borrowed_texture_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface WebgpuBorrowedTextureDescriptor {
  /**
   * Logical texture extent. The map viewport uses width and height and the
   * renderer uses scale_factor; the physical size is stated separately below. A
   * scale_factor that differs from the map's is accepted and logged as a
   * warning.
   */
  extent?: LogicalExtent;
  /**
   * Physical texture width in device pixels. Defaults to 256.
   */
  physicalWidth?: number;
  /**
   * Physical texture height in device pixels. Defaults to 256.
   */
  physicalHeight?: number;
  /**
   * Borrowed WebGPU context. device is required. Rendering is submitted through
   * context.queue or that device's default queue.
   */
  context?: WebgpuContextDescriptor;
  /**
   * The ring's textures, one per slot, in slot order. Required.
   */
  textures?: readonly WebgpuBorrowedTexture[];
  /**
   * Backend-native WGPUTextureFormat value of every texture. Undefined is
   * invalid.
   */
  format?: number;
}

/**
 * WebGPU backend context fields shared by WebGPU render targets.
 *
 * See `mln_webgpu_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface WebgpuContextDescriptor {
  /**
   * Borrowed WGPUInstance. Optional for texture targets.
   */
  instance?: bigint;
  /**
   * Borrowed WGPUDevice. Required.
   */
  device?: bigint;
  /**
   * Borrowed WGPUQueue. Optional; null uses the device default queue. A
   * non-null queue must belong to device.
   */
  queue?: bigint;
}

/**
 * WebGPU attachment options for an owned texture target.
 *
 * See `mln_webgpu_owned_texture_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface WebgpuOwnedTextureDescriptor {
  /**
   * Logical texture extent. A scale_factor that differs from the map's is
   * accepted and logged as a warning.
   */
  extent?: LogicalExtent;
  /**
   * Borrowed WebGPU context. device is required.
   */
  context?: WebgpuContextDescriptor;
}

/**
 * WebGPU attachment options for a native surface.
 *
 * See `mln_webgpu_surface_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
export interface WebgpuSurfaceDescriptor {
  /**
   * Logical surface extent. A scale_factor that differs from the map's is
   * accepted and logged as a warning.
   */
  extent?: LogicalExtent;
  /**
   * Borrowed WebGPU context. device is required.
   */
  context?: WebgpuContextDescriptor;
  /**
   * Borrowed WGPUSurface. Required, and must stay alive for the session. The
   * session configures it for this device and extent, and unconfigures it when
   * the session ends.
   */
  surface?: bigint;
  /**
   * WGPUTextureFormat to configure the surface with. Required. A browser host
   * takes it from navigator.gpu.getPreferredCanvasFormat().
   */
  format?: number;
}

/**
 * WebGPU frame acquired from a texture ring.
 *
 * See `mln_webgpu_texture_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
export interface WebgpuTextureFrame {
  /**
   * Session generation that produced this frame.
   */
  readonly generation: bigint;
  /**
   * Physical WebGPU texture width in device pixels.
   */
  readonly width: number;
  /**
   * Physical WebGPU texture height in device pixels.
   */
  readonly height: number;
  /**
   * UI-to-device pixel scale used for this frame.
   */
  readonly scaleFactor: number;
  /**
   * Opaque frame identity used to reject stale releases.
   */
  readonly frameId: bigint;
  /**
   * Ring slot that holds this frame. For a borrowed target, the index of its
   * texture in the descriptor's textures array.
   */
  readonly slot: number;
  /**
   * Borrowed WGPUTexture. Valid until frame release.
   */
  readonly texture: bigint;
  /**
   * Borrowed WGPUTextureView. Valid until frame release.
   */
  readonly textureView: bigint;
  /**
   * Borrowed WGPUDevice. Valid until frame release.
   */
  readonly device: bigint;
  /**
   * Backend-native WGPUTextureFormat value.
   */
  readonly format: number;
}

/**
 * WGL context fields shared by OpenGL render targets on Windows.
 *
 * See `mln_wgl_context_descriptor` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
export interface WglContextDescriptor {
  /**
   * Borrowed HDC used to create the session context. Required.
   */
  deviceContext?: bigint;
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
