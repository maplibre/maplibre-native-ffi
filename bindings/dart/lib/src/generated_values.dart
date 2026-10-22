// Generated from the C headers by tools/bindgen. Do not edit.
part of 'values.dart';

final class AmbientCacheOperation extends _Enum {
  const AmbientCacheOperation.fromRawValue(super.rawValue);
  static const resetDatabase = AmbientCacheOperation.fromRawValue(1);
  static const packDatabase = AmbientCacheOperation.fromRawValue(2);
  static const invalidate = AmbientCacheOperation.fromRawValue(3);
  static const clear = AmbientCacheOperation.fromRawValue(4);
  @override
  String toString() => switch (rawValue) {
    1 => 'AmbientCacheOperation.resetDatabase',
    2 => 'AmbientCacheOperation.packDatabase',
    3 => 'AmbientCacheOperation.invalidate',
    4 => 'AmbientCacheOperation.clear',
    _ => 'AmbientCacheOperation($rawValue)',
  };
}

/// Field mask values for `mln_animation_options`.
///
/// See `mln_animation_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class AnimationOptionField extends _Flags<AnimationOptionField> {
  const AnimationOptionField.fromRawValue(super.rawValue);
  static const duration = AnimationOptionField.fromRawValue(1);
  static const velocity = AnimationOptionField.fromRawValue(2);
  static const minZoom = AnimationOptionField.fromRawValue(4);
  static const easing = AnimationOptionField.fromRawValue(8);
  static const transitionId = AnimationOptionField.fromRawValue(16);
  @override
  AnimationOptionField _of(int rawValue) =>
      AnimationOptionField.fromRawValue(rawValue);
}

/// Field mask values for `mln_bound_options`.
///
/// See `mln_bound_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class BoundOptionField extends _Flags<BoundOptionField> {
  const BoundOptionField.fromRawValue(super.rawValue);

  /// Selects `mln_bound_options.bounds` as a geographic constraint that the
  /// camera center stays inside. Mutually exclusive with
  /// `MLN_BOUND_OPTION_UNBOUNDED`.
  static const bounds = BoundOptionField.fromRawValue(1);
  static const minZoom = BoundOptionField.fromRawValue(2);
  static const maxZoom = BoundOptionField.fromRawValue(4);
  static const minPitch = BoundOptionField.fromRawValue(8);
  static const maxPitch = BoundOptionField.fromRawValue(16);

  /// Selects the unbounded geographic constraint, which leaves every camera
  /// center unconstrained and lets the map pan freely across the antimeridian.
  /// This differs from world bounds of -90/-180 to 90/180, which clamp
  /// longitude to that range. Mutually exclusive with
  /// `MLN_BOUND_OPTION_BOUNDS`, and leaves `mln_bound_options.bounds` unread.
  static const unbounded = BoundOptionField.fromRawValue(32);
  @override
  BoundOptionField _of(int rawValue) => BoundOptionField.fromRawValue(rawValue);
}

/// Camera change kinds reported by camera will-change and did-change events.
///
/// See `mln_camera_change_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class CameraChangeMode extends _Enum {
  const CameraChangeMode.fromRawValue(super.rawValue);

  /// The camera reached its new value without an animated transition.
  static const immediate = CameraChangeMode.fromRawValue(0);

  /// The camera moved as part of an animated transition.
  static const animated = CameraChangeMode.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'CameraChangeMode.immediate',
    1 => 'CameraChangeMode.animated',
    _ => 'CameraChangeMode($rawValue)',
  };
}

/// Field mask values for `mln_camera_delta`.
///
/// See `mln_camera_delta_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraDeltaField extends _Flags<CameraDeltaField> {
  const CameraDeltaField.fromRawValue(super.rawValue);
  static const offset = CameraDeltaField.fromRawValue(1);
  static const scale = CameraDeltaField.fromRawValue(2);
  static const bearing = CameraDeltaField.fromRawValue(4);
  static const pitch = CameraDeltaField.fromRawValue(8);
  static const anchor = CameraDeltaField.fromRawValue(16);
  @override
  CameraDeltaField _of(int rawValue) => CameraDeltaField.fromRawValue(rawValue);
}

/// Field mask values for `mln_camera_fit_options`.
///
/// See `mln_camera_fit_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraFitOptionField extends _Flags<CameraFitOptionField> {
  const CameraFitOptionField.fromRawValue(super.rawValue);
  static const padding = CameraFitOptionField.fromRawValue(1);
  static const bearing = CameraFitOptionField.fromRawValue(2);
  static const pitch = CameraFitOptionField.fromRawValue(4);
  @override
  CameraFitOptionField _of(int rawValue) =>
      CameraFitOptionField.fromRawValue(rawValue);
}

/// Field mask values for `mln_camera_options`.
///
/// See `mln_camera_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraOptionField extends _Flags<CameraOptionField> {
  const CameraOptionField.fromRawValue(super.rawValue);
  static const center = CameraOptionField.fromRawValue(1);
  static const zoom = CameraOptionField.fromRawValue(2);
  static const bearing = CameraOptionField.fromRawValue(4);
  static const pitch = CameraOptionField.fromRawValue(8);
  static const centerAltitude = CameraOptionField.fromRawValue(16);
  static const padding = CameraOptionField.fromRawValue(32);
  static const anchor = CameraOptionField.fromRawValue(64);
  static const roll = CameraOptionField.fromRawValue(128);
  static const fov = CameraOptionField.fromRawValue(256);
  @override
  CameraOptionField _of(int rawValue) =>
      CameraOptionField.fromRawValue(rawValue);
}

/// How the transitions of one camera command ended.
///
/// See `mln_camera_transition_outcome` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraTransitionOutcome extends _Enum {
  const CameraTransitionOutcome.fromRawValue(super.rawValue);

  /// Every property of the command reached its target, or a later camera write
  /// replaced it.
  static const completed = CameraTransitionOutcome.fromRawValue(0);

  /// `mln_map_cancel_transitions()`, `mln_map_cancel_camera_transition()`, or
  /// `MLN_GESTURE_PHASE_CANCEL` ended the transitions, or the command failed
  /// before it started them.
  static const cancelled = CameraTransitionOutcome.fromRawValue(1);

  /// The map closed before the transitions ended.
  static const closed = CameraTransitionOutcome.fromRawValue(2);
  @override
  String toString() => switch (rawValue) {
    0 => 'CameraTransitionOutcome.completed',
    1 => 'CameraTransitionOutcome.cancelled',
    2 => 'CameraTransitionOutcome.closed',
    _ => 'CameraTransitionOutcome($rawValue)',
  };
}

/// Camera transition behavior for `mln_camera_update`.
///
/// See `mln_camera_update_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraUpdateMode extends _Enum {
  const CameraUpdateMode.fromRawValue(super.rawValue);
  static const jump = CameraUpdateMode.fromRawValue(0);
  static const ease = CameraUpdateMode.fromRawValue(1);
  static const fly = CameraUpdateMode.fromRawValue(2);
  @override
  String toString() => switch (rawValue) {
    0 => 'CameraUpdateMode.jump',
    1 => 'CameraUpdateMode.ease',
    2 => 'CameraUpdateMode.fly',
    _ => 'CameraUpdateMode($rawValue)',
  };
}

/// Terminal dispositions reported by command completions.
///
/// See `mln_command_disposition` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/completion_8h.html).
final class CommandDisposition extends _Enum {
  const CommandDisposition.fromRawValue(super.rawValue);
  static const committed = CommandDisposition.fromRawValue(0);
  static const superseded = CommandDisposition.fromRawValue(1);
  static const failed = CommandDisposition.fromRawValue(2);
  static const cancelled = CommandDisposition.fromRawValue(3);
  @override
  String toString() => switch (rawValue) {
    0 => 'CommandDisposition.committed',
    1 => 'CommandDisposition.superseded',
    2 => 'CommandDisposition.failed',
    3 => 'CommandDisposition.cancelled',
    _ => 'CommandDisposition($rawValue)',
  };
}

/// Map constraint modes used by `mln_map_viewport_options`.
///
/// See `mln_constrain_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class ConstrainMode extends _Enum {
  const ConstrainMode.fromRawValue(super.rawValue);
  static const none = ConstrainMode.fromRawValue(0);
  static const heightOnly = ConstrainMode.fromRawValue(1);
  static const widthAndHeight = ConstrainMode.fromRawValue(2);
  static const screen = ConstrainMode.fromRawValue(3);
  @override
  String toString() => switch (rawValue) {
    0 => 'ConstrainMode.none',
    1 => 'ConstrainMode.heightOnly',
    2 => 'ConstrainMode.widthAndHeight',
    3 => 'ConstrainMode.screen',
    _ => 'ConstrainMode($rawValue)',
  };
}

/// Field mask values for `mln_custom_geometry_source_options`.
///
/// See `mln_custom_geometry_source_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class CustomGeometrySourceOptionField
    extends _Flags<CustomGeometrySourceOptionField> {
  const CustomGeometrySourceOptionField.fromRawValue(super.rawValue);
  static const minZoom = CustomGeometrySourceOptionField.fromRawValue(1);
  static const maxZoom = CustomGeometrySourceOptionField.fromRawValue(2);
  static const tolerance = CustomGeometrySourceOptionField.fromRawValue(4);
  static const tileSize = CustomGeometrySourceOptionField.fromRawValue(8);
  static const buffer = CustomGeometrySourceOptionField.fromRawValue(16);
  static const clip = CustomGeometrySourceOptionField.fromRawValue(32);
  static const wrap = CustomGeometrySourceOptionField.fromRawValue(64);
  @override
  CustomGeometrySourceOptionField _of(int rawValue) =>
      CustomGeometrySourceOptionField.fromRawValue(rawValue);
}

/// Field mask values for `mln_custom_mvt_vector_source_options`.
///
/// See `mln_custom_mvt_vector_source_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class CustomMvtVectorSourceOptionField
    extends _Flags<CustomMvtVectorSourceOptionField> {
  const CustomMvtVectorSourceOptionField.fromRawValue(super.rawValue);
  static const minZoom = CustomMvtVectorSourceOptionField.fromRawValue(1);
  static const maxZoom = CustomMvtVectorSourceOptionField.fromRawValue(2);
  @override
  CustomMvtVectorSourceOptionField _of(int rawValue) =>
      CustomMvtVectorSourceOptionField.fromRawValue(rawValue);
}

/// Optional fields for `mln_feature_state_selector`.
///
/// See `mln_feature_state_selector_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class FeatureStateSelectorField
    extends _Flags<FeatureStateSelectorField> {
  const FeatureStateSelectorField.fromRawValue(super.rawValue);
  static const sourceLayerId = FeatureStateSelectorField.fromRawValue(1);
  static const featureId = FeatureStateSelectorField.fromRawValue(2);
  static const stateKey = FeatureStateSelectorField.fromRawValue(4);
  @override
  FeatureStateSelectorField _of(int rawValue) =>
      FeatureStateSelectorField.fromRawValue(rawValue);
}

/// Frame-demand policy bits.
///
/// See `mln_frame_demand_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
final class FrameDemandFlag extends _Flags<FrameDemandFlag> {
  const FrameDemandFlag.fromRawValue(super.rawValue);

  /// Render only when a newer map update exists.
  static const ifNeeded = FrameDemandFlag.fromRawValue(1);

  /// Present the rendered frame on a target that supports presentation. A
  /// presenting target whose demand clears this bit still renders and keeps
  /// whatever it presented last. Ignored by targets without presentation.
  static const present = FrameDemandFlag.fromRawValue(2);

  /// With `MLN_FRAME_DEMAND_IF_NEEDED`, a demand that would finish with
  /// `MLN_RENDER_RESULT_NO_UPDATE` waits instead, and runs again after the
  /// map's next update, a target replacement, or an applied resize. A waiting
  /// demand holds no ring slot. A later demand with the same flags supersedes
  /// it, a barrier ends its wait with `MLN_RENDER_RESULT_NO_UPDATE`, and
  /// detach, abandon, or the quarantine of the ring's last usable slot end it
  /// with `MLN_RENDER_RESULT_TARGET_NOT_READY`. A waiting demand's result can
  /// follow the results of demands accepted after it. The flag does not pace: a
  /// host that re-arms a waiting demand as each result arrives renders every
  /// update the map publishes. The map publishes one update for the commands
  /// that its runtime worker runs back to back, but a command that reaches the
  /// worker while a frame renders appears in a later frame. To show several
  /// commands in one frame, submit them in a command group, or arm demands once
  /// per display frame.
  static const waitForUpdate = FrameDemandFlag.fromRawValue(4);
  @override
  FrameDemandFlag _of(int rawValue) => FrameDemandFlag.fromRawValue(rawValue);
}

/// Field mask values for `mln_free_camera_options`.
///
/// See `mln_free_camera_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class FreeCameraOptionField extends _Flags<FreeCameraOptionField> {
  const FreeCameraOptionField.fromRawValue(super.rawValue);
  static const position = FreeCameraOptionField.fromRawValue(1);
  static const orientation = FreeCameraOptionField.fromRawValue(2);
  @override
  FreeCameraOptionField _of(int rawValue) =>
      FreeCameraOptionField.fromRawValue(rawValue);
}

/// Field mask values for `mln_geojson_source_options`.
///
/// See `mln_geojson_source_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class GeojsonSourceOptionField extends _Flags<GeojsonSourceOptionField> {
  const GeojsonSourceOptionField.fromRawValue(super.rawValue);
  static const minZoom = GeojsonSourceOptionField.fromRawValue(1);
  static const maxZoom = GeojsonSourceOptionField.fromRawValue(2);
  static const tolerance = GeojsonSourceOptionField.fromRawValue(4);
  static const clusterMaxZoom = GeojsonSourceOptionField.fromRawValue(8);
  static const clusterProperties = GeojsonSourceOptionField.fromRawValue(16);
  static const buffer = GeojsonSourceOptionField.fromRawValue(32);
  static const clusterRadius = GeojsonSourceOptionField.fromRawValue(64);
  static const clusterMinPoints = GeojsonSourceOptionField.fromRawValue(128);
  static const lineMetrics = GeojsonSourceOptionField.fromRawValue(256);
  static const cluster = GeojsonSourceOptionField.fromRawValue(512);
  static const synchronousTiling = GeojsonSourceOptionField.fromRawValue(1024);
  @override
  GeojsonSourceOptionField _of(int rawValue) =>
      GeojsonSourceOptionField.fromRawValue(rawValue);
}

/// Gesture boundary carried atomically with a camera update or delta.
///
/// See `mln_gesture_phase` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class GesturePhase extends _Enum {
  const GesturePhase.fromRawValue(super.rawValue);

  /// The update carries no gesture boundary and leaves the flag as it is.
  static const none = GesturePhase.fromRawValue(0);

  /// Marks a gesture as in progress before the camera write. It does not cancel
  /// running transitions; use `mln_map_cancel_transitions()` for that.
  static const begin = GesturePhase.fromRawValue(1);

  /// Keeps the gesture marked as in progress before the camera write.
  static const update = GesturePhase.fromRawValue(2);

  /// Clears the gesture flag after the camera write.
  static const end = GesturePhase.fromRawValue(3);

  /// Cancels transitions running after the camera write, then clears the
  /// gesture flag.
  static const cancel = GesturePhase.fromRawValue(4);
  @override
  String toString() => switch (rawValue) {
    0 => 'GesturePhase.none',
    1 => 'GesturePhase.begin',
    2 => 'GesturePhase.update',
    3 => 'GesturePhase.end',
    4 => 'GesturePhase.cancel',
    _ => 'GesturePhase($rawValue)',
  };
}

/// Synchronization payload kind for acquired texture frames.
///
/// See `mln_gpu_sync_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class GpuSyncKind extends _Enum {
  const GpuSyncKind.fromRawValue(super.rawValue);

  /// The host needs no synchronization object. The work completed, or on WebGPU
  /// was submitted to the device's queue, before the frame became acquirable or
  /// before the release call.
  static const cpuComplete = GpuSyncKind.fromRawValue(0);

  /// GLsync in the shared EGL or WGL context's share group, used only by a
  /// caller-graphics-thread driver. object is nonzero.
  static const openglFence = GpuSyncKind.fromRawValue(3);
  @override
  String toString() => switch (rawValue) {
    0 => 'GpuSyncKind.cpuComplete',
    3 => 'GpuSyncKind.openglFence',
    _ => 'GpuSyncKind($rawValue)',
  };
}

/// Log event categories emitted by MapLibre Native.
///
/// See `mln_log_event` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
final class LogEvent extends _Enum {
  const LogEvent.fromRawValue(super.rawValue);
  static const general = LogEvent.fromRawValue(0);
  static const setup = LogEvent.fromRawValue(1);
  static const shader = LogEvent.fromRawValue(2);
  static const parseStyle = LogEvent.fromRawValue(3);
  static const parseTile = LogEvent.fromRawValue(4);
  static const render = LogEvent.fromRawValue(5);
  static const style = LogEvent.fromRawValue(6);
  static const database = LogEvent.fromRawValue(7);
  static const httpRequest = LogEvent.fromRawValue(8);
  static const sprite = LogEvent.fromRawValue(9);
  static const image = LogEvent.fromRawValue(10);
  static const graphicsBackend = LogEvent.fromRawValue(11);
  static const jni = LogEvent.fromRawValue(12);
  static const android = LogEvent.fromRawValue(13);
  static const crash = LogEvent.fromRawValue(14);
  static const glyph = LogEvent.fromRawValue(15);
  static const timing = LogEvent.fromRawValue(16);
  @override
  String toString() => switch (rawValue) {
    0 => 'LogEvent.general',
    1 => 'LogEvent.setup',
    2 => 'LogEvent.shader',
    3 => 'LogEvent.parseStyle',
    4 => 'LogEvent.parseTile',
    5 => 'LogEvent.render',
    6 => 'LogEvent.style',
    7 => 'LogEvent.database',
    8 => 'LogEvent.httpRequest',
    9 => 'LogEvent.sprite',
    10 => 'LogEvent.image',
    11 => 'LogEvent.graphicsBackend',
    12 => 'LogEvent.jni',
    13 => 'LogEvent.android',
    14 => 'LogEvent.crash',
    15 => 'LogEvent.glyph',
    16 => 'LogEvent.timing',
    _ => 'LogEvent($rawValue)',
  };
}

/// Log severity values emitted by MapLibre Native.
///
/// See `mln_log_severity` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
final class LogSeverity extends _Enum {
  const LogSeverity.fromRawValue(super.rawValue);
  static const info = LogSeverity.fromRawValue(1);
  static const warning = LogSeverity.fromRawValue(2);
  static const error = LogSeverity.fromRawValue(3);
  @override
  String toString() => switch (rawValue) {
    1 => 'LogSeverity.info',
    2 => 'LogSeverity.warning',
    3 => 'LogSeverity.error',
    _ => 'LogSeverity($rawValue)',
  };
}

/// Bitmask values for log severities dispatched asynchronously.
///
/// See `mln_log_severity_mask` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
final class LogSeverityMask extends _Flags<LogSeverityMask> {
  const LogSeverityMask.fromRawValue(super.rawValue);
  static const info = LogSeverityMask.fromRawValue(2);
  static const warning = LogSeverityMask.fromRawValue(4);
  static const error = LogSeverityMask.fromRawValue(8);
  static const defaultValue = LogSeverityMask.fromRawValue(6);
  static const all = LogSeverityMask.fromRawValue(14);
  @override
  LogSeverityMask _of(int rawValue) => LogSeverityMask.fromRawValue(rawValue);
}

/// Debug overlay mask values for `mln_map_set_debug_options()`.
///
/// See `mln_map_debug_option` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class MapDebugOption extends _Flags<MapDebugOption> {
  const MapDebugOption.fromRawValue(super.rawValue);
  static const tileBorders = MapDebugOption.fromRawValue(2);
  static const parseStatus = MapDebugOption.fromRawValue(4);
  static const timestamps = MapDebugOption.fromRawValue(8);
  static const collision = MapDebugOption.fromRawValue(16);
  static const overdraw = MapDebugOption.fromRawValue(32);
  static const stencilClip = MapDebugOption.fromRawValue(64);
  static const depthBuffer = MapDebugOption.fromRawValue(128);
  @override
  MapDebugOption _of(int rawValue) => MapDebugOption.fromRawValue(rawValue);
}

/// Map rendering modes used when creating a map.
///
/// See `mln_map_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class MapMode extends _Enum {
  const MapMode.fromRawValue(super.rawValue);

  /// Continuously updates as data arrives and map state changes.
  static const continuous = MapMode.fromRawValue(0);

  /// Produces one-off still images of an arbitrary viewport.
  static const static = MapMode.fromRawValue(1);

  /// Produces one-off still images for a single tile.
  static const tile = MapMode.fromRawValue(2);
  @override
  String toString() => switch (rawValue) {
    0 => 'MapMode.continuous',
    1 => 'MapMode.static',
    2 => 'MapMode.tile',
    _ => 'MapMode($rawValue)',
  };
}

/// Field mask values for `mln_map_tile_options`.
///
/// See `mln_map_tile_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class MapTileOptionField extends _Flags<MapTileOptionField> {
  const MapTileOptionField.fromRawValue(super.rawValue);
  static const prefetchZoomDelta = MapTileOptionField.fromRawValue(1);
  static const lodMinRadius = MapTileOptionField.fromRawValue(2);
  static const lodScale = MapTileOptionField.fromRawValue(4);
  static const lodPitchThreshold = MapTileOptionField.fromRawValue(8);
  static const lodZoomShift = MapTileOptionField.fromRawValue(16);
  static const lodMode = MapTileOptionField.fromRawValue(32);
  @override
  MapTileOptionField _of(int rawValue) =>
      MapTileOptionField.fromRawValue(rawValue);
}

/// Field mask values for `mln_map_viewport_options`.
///
/// See `mln_map_viewport_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class MapViewportOptionField extends _Flags<MapViewportOptionField> {
  const MapViewportOptionField.fromRawValue(super.rawValue);
  static const northOrientation = MapViewportOptionField.fromRawValue(1);
  static const constrainMode = MapViewportOptionField.fromRawValue(2);
  static const viewportMode = MapViewportOptionField.fromRawValue(4);
  static const frustumOffset = MapViewportOptionField.fromRawValue(8);
  @override
  MapViewportOptionField _of(int rawValue) =>
      MapViewportOptionField.fromRawValue(rawValue);
}

final class NetworkStatus extends _Enum {
  const NetworkStatus.fromRawValue(super.rawValue);
  static const online = NetworkStatus.fromRawValue(1);
  static const offline = NetworkStatus.fromRawValue(2);
  @override
  String toString() => switch (rawValue) {
    1 => 'NetworkStatus.online',
    2 => 'NetworkStatus.offline',
    _ => 'NetworkStatus($rawValue)',
  };
}

/// Map north orientation values used by `mln_map_viewport_options`.
///
/// See `mln_north_orientation` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class NorthOrientation extends _Enum {
  const NorthOrientation.fromRawValue(super.rawValue);
  static const up = NorthOrientation.fromRawValue(0);
  static const right = NorthOrientation.fromRawValue(1);
  static const down = NorthOrientation.fromRawValue(2);
  static const left = NorthOrientation.fromRawValue(3);
  @override
  String toString() => switch (rawValue) {
    0 => 'NorthOrientation.up',
    1 => 'NorthOrientation.right',
    2 => 'NorthOrientation.down',
    3 => 'NorthOrientation.left',
    _ => 'NorthOrientation($rawValue)',
  };
}

final class OfflineRegionDefinitionType extends _Enum {
  const OfflineRegionDefinitionType.fromRawValue(super.rawValue);
  static const tilePyramid = OfflineRegionDefinitionType.fromRawValue(1);
  static const geometry = OfflineRegionDefinitionType.fromRawValue(2);
  @override
  String toString() => switch (rawValue) {
    1 => 'OfflineRegionDefinitionType.tilePyramid',
    2 => 'OfflineRegionDefinitionType.geometry',
    _ => 'OfflineRegionDefinitionType($rawValue)',
  };
}

final class OfflineRegionDownloadState extends _Enum {
  const OfflineRegionDownloadState.fromRawValue(super.rawValue);
  static const inactive = OfflineRegionDownloadState.fromRawValue(0);
  static const active = OfflineRegionDownloadState.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'OfflineRegionDownloadState.inactive',
    1 => 'OfflineRegionDownloadState.active',
    _ => 'OfflineRegionDownloadState($rawValue)',
  };
}

/// OpenGL client API a dedicated EGL session creates its context for.
///
/// See `mln_opengl_client_api` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class OpenglClientApi extends _Enum {
  const OpenglClientApi.fromRawValue(super.rawValue);

  /// No client API is named.
  static const unspecified = OpenglClientApi.fromRawValue(0);

  /// Desktop OpenGL, as EGL_OPENGL_API names it.
  static const gl = OpenglClientApi.fromRawValue(1);

  /// OpenGL ES, as EGL_OPENGL_ES_API names it.
  static const gles = OpenglClientApi.fromRawValue(2);
  @override
  String toString() => switch (rawValue) {
    0 => 'OpenglClientApi.unspecified',
    1 => 'OpenglClientApi.gl',
    2 => 'OpenglClientApi.gles',
    _ => 'OpenglClientApi($rawValue)',
  };
}

/// How a session's OpenGL context relates to its driver thread and host
/// graphics state.
///
/// See `mln_opengl_context_ownership` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class OpenglContextOwnership extends _Enum {
  const OpenglContextOwnership.fromRawValue(super.rawValue);

  /// The session shares its thread with host graphics work.
  static const shared = OpenglContextOwnership.fromRawValue(0);

  /// The session owns its thread's OpenGL context.
  static const dedicated = OpenglContextOwnership.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'OpenglContextOwnership.shared',
    1 => 'OpenglContextOwnership.dedicated',
    _ => 'OpenglContextOwnership($rawValue)',
  };
}

/// OpenGL platform context provider used by a context descriptor.
///
/// See `mln_opengl_context_platform` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class OpenglContextPlatform extends _Enum {
  const OpenglContextPlatform.fromRawValue(super.rawValue);

  /// No OpenGL context provider is selected.
  static const unspecified = OpenglContextPlatform.fromRawValue(0);
  static const wgl = OpenglContextPlatform.fromRawValue(1);
  static const egl = OpenglContextPlatform.fromRawValue(2);

  /// Emscripten WebGL context handle.
  static const webgl = OpenglContextPlatform.fromRawValue(3);
  @override
  String toString() => switch (rawValue) {
    0 => 'OpenglContextPlatform.unspecified',
    1 => 'OpenglContextPlatform.wgl',
    2 => 'OpenglContextPlatform.egl',
    3 => 'OpenglContextPlatform.webgl',
    _ => 'OpenglContextPlatform($rawValue)',
  };
}

/// OpenGL context providers supported by this build.
///
/// See `mln_opengl_context_provider_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class OpenglContextProviderFlag
    extends _Flags<OpenglContextProviderFlag> {
  const OpenglContextProviderFlag.fromRawValue(super.rawValue);
  static const wgl = OpenglContextProviderFlag.fromRawValue(1);
  static const egl = OpenglContextProviderFlag.fromRawValue(2);

  /// Browser WebGL context imported into an Emscripten module.
  static const webgl = OpenglContextProviderFlag.fromRawValue(4);
  @override
  OpenglContextProviderFlag _of(int rawValue) =>
      OpenglContextProviderFlag.fromRawValue(rawValue);
}

/// Field mask values for MapLibre axonometric rendering options.
///
/// See `mln_projection_mode_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class ProjectionModeField extends _Flags<ProjectionModeField> {
  const ProjectionModeField.fromRawValue(super.rawValue);
  static const axonometric = ProjectionModeField.fromRawValue(1);
  static const xSkew = ProjectionModeField.fromRawValue(2);
  static const ySkew = ProjectionModeField.fromRawValue(4);
  @override
  ProjectionModeField _of(int rawValue) =>
      ProjectionModeField.fromRawValue(rawValue);
}

/// Optional fields for `mln_queried_feature`.
///
/// See `mln_queried_feature_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
final class QueriedFeatureField extends _Flags<QueriedFeatureField> {
  const QueriedFeatureField.fromRawValue(super.rawValue);
  static const sourceId = QueriedFeatureField.fromRawValue(1);
  static const sourceLayerId = QueriedFeatureField.fromRawValue(2);
  static const state = QueriedFeatureField.fromRawValue(4);
  @override
  QueriedFeatureField _of(int rawValue) =>
      QueriedFeatureField.fromRawValue(rawValue);
}

/// What abandon did with a session's graphics resources.
///
/// See `mln_render_abandon_disposition` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
final class RenderAbandonDisposition extends _Enum {
  const RenderAbandonDisposition.fromRawValue(super.rawValue);

  /// Abandon destroyed every graphics resource, or none remained.
  static const clean = RenderAbandonDisposition.fromRawValue(0);

  /// Abandon kept graphics resources that it could not safely destroy.
  static const quarantined = RenderAbandonDisposition.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'RenderAbandonDisposition.clean',
    1 => 'RenderAbandonDisposition.quarantined',
    _ => 'RenderAbandonDisposition($rawValue)',
  };
}

/// Render backend support flags reported by this native library build.
///
/// See `mln_render_backend_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class RenderBackendFlag extends _Flags<RenderBackendFlag> {
  const RenderBackendFlag.fromRawValue(super.rawValue);
  static const metal = RenderBackendFlag.fromRawValue(1);
  static const vulkan = RenderBackendFlag.fromRawValue(2);
  static const opengl = RenderBackendFlag.fromRawValue(4);
  static const webgpu = RenderBackendFlag.fromRawValue(8);
  @override
  RenderBackendFlag _of(int rawValue) =>
      RenderBackendFlag.fromRawValue(rawValue);
}

/// Execution placement for one render session.
///
/// See `mln_render_driver_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class RenderDriverKind extends _Enum {
  const RenderDriverKind.fromRawValue(super.rawValue);

  /// Native code initializes, services, and tears down transferable graphics
  /// state on a library-owned thread.
  static const coreWorker = RenderDriverKind.fromRawValue(1);

  /// The host services driver work on the thread or realm where its graphics
  /// context is current.
  static const callerGraphicsThread = RenderDriverKind.fromRawValue(2);
  @override
  String toString() => switch (rawValue) {
    1 => 'RenderDriverKind.coreWorker',
    2 => 'RenderDriverKind.callerGraphicsThread',
    _ => 'RenderDriverKind($rawValue)',
  };
}

/// Render modes reported by render observer events.
///
/// See `mln_render_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RenderMode extends _Enum {
  const RenderMode.fromRawValue(super.rawValue);
  static const partial = RenderMode.fromRawValue(0);
  static const full = RenderMode.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'RenderMode.partial',
    1 => 'RenderMode.full',
    _ => 'RenderMode($rawValue)',
  };
}

/// Terminal disposition of one accepted frame demand.
///
/// See `mln_render_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
final class RenderResult extends _Enum {
  const RenderResult.fromRawValue(super.rawValue);

  /// A frame was rendered for acquisition, presentation, or ordered readback.
  static const rendered = RenderResult.fromRawValue(0);

  /// No newer map update was available, the map had no complete frame to draw,
  /// or the update extent differed from the target. The map publishes another
  /// update when it has one. A demand with `MLN_FRAME_DEMAND_WAIT_FOR_UPDATE`
  /// waits for that update instead, and finishes with this result only when a
  /// barrier ends its wait.
  static const noUpdate = RenderResult.fromRawValue(1);

  /// The target could not produce a frame. The attempt consumes nothing, so a
  /// later demand with the same flags renders what this one would have. This
  /// result does not cause a map update, so the host demands again when the
  /// target can be ready, such as after a paced delay.
  static const targetNotReady = RenderResult.fromRawValue(3);

  /// A newer IF_NEEDED demand with the same flags replaced this demand.
  static const superseded = RenderResult.fromRawValue(4);
  @override
  String toString() => switch (rawValue) {
    0 => 'RenderResult.rendered',
    1 => 'RenderResult.noUpdate',
    3 => 'RenderResult.targetNotReady',
    4 => 'RenderResult.superseded',
    _ => 'RenderResult($rawValue)',
  };
}

/// Optional render-session capabilities.
///
/// See `mln_render_session_capability_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class RenderSessionCapabilityFlag
    extends _Flags<RenderSessionCapabilityFlag> {
  const RenderSessionCapabilityFlag.fromRawValue(super.rawValue);
  static const frameAcquisition = RenderSessionCapabilityFlag.fromRawValue(1);
  static const readback = RenderSessionCapabilityFlag.fromRawValue(2);
  static const consumerSync = RenderSessionCapabilityFlag.fromRawValue(4);
  static const presentation = RenderSessionCapabilityFlag.fromRawValue(8);
  @override
  RenderSessionCapabilityFlag _of(int rawValue) =>
      RenderSessionCapabilityFlag.fromRawValue(rawValue);
}

/// Render-session lifecycle visible in snapshots.
///
/// See `mln_render_session_state` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
final class RenderSessionState extends _Enum {
  const RenderSessionState.fromRawValue(super.rawValue);
  static const attaching = RenderSessionState.fromRawValue(1);
  static const attached = RenderSessionState.fromRawValue(2);
  static const detaching = RenderSessionState.fromRawValue(3);
  static const detached = RenderSessionState.fromRawValue(4);
  static const targetLost = RenderSessionState.fromRawValue(5);
  static const abandoned = RenderSessionState.fromRawValue(6);
  @override
  String toString() => switch (rawValue) {
    1 => 'RenderSessionState.attaching',
    2 => 'RenderSessionState.attached',
    3 => 'RenderSessionState.detaching',
    4 => 'RenderSessionState.detached',
    5 => 'RenderSessionState.targetLost',
    6 => 'RenderSessionState.abandoned',
    _ => 'RenderSessionState($rawValue)',
  };
}

/// Optional fields for `mln_rendered_feature_query_options`.
///
/// See `mln_rendered_feature_query_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
final class RenderedFeatureQueryOptionField
    extends _Flags<RenderedFeatureQueryOptionField> {
  const RenderedFeatureQueryOptionField.fromRawValue(super.rawValue);
  static const layerIds = RenderedFeatureQueryOptionField.fromRawValue(1);
  static const filter = RenderedFeatureQueryOptionField.fromRawValue(2);
  @override
  RenderedFeatureQueryOptionField _of(int rawValue) =>
      RenderedFeatureQueryOptionField.fromRawValue(rawValue);
}

/// Rendered feature query geometry variants.
///
/// See `mln_rendered_query_geometry_type` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
final class RenderedQueryGeometryType extends _Enum {
  const RenderedQueryGeometryType.fromRawValue(super.rawValue);
  static const point = RenderedQueryGeometryType.fromRawValue(1);
  static const box = RenderedQueryGeometryType.fromRawValue(2);
  static const lineString = RenderedQueryGeometryType.fromRawValue(3);
  @override
  String toString() => switch (rawValue) {
    1 => 'RenderedQueryGeometryType.point',
    2 => 'RenderedQueryGeometryType.box',
    3 => 'RenderedQueryGeometryType.lineString',
    _ => 'RenderedQueryGeometryType($rawValue)',
  };
}

final class ResourceErrorReason extends _Enum {
  const ResourceErrorReason.fromRawValue(super.rawValue);
  static const none = ResourceErrorReason.fromRawValue(0);
  static const notFound = ResourceErrorReason.fromRawValue(1);
  static const server = ResourceErrorReason.fromRawValue(2);
  static const connection = ResourceErrorReason.fromRawValue(3);
  static const rateLimit = ResourceErrorReason.fromRawValue(4);
  static const other = ResourceErrorReason.fromRawValue(5);
  @override
  String toString() => switch (rawValue) {
    0 => 'ResourceErrorReason.none',
    1 => 'ResourceErrorReason.notFound',
    2 => 'ResourceErrorReason.server',
    3 => 'ResourceErrorReason.connection',
    4 => 'ResourceErrorReason.rateLimit',
    5 => 'ResourceErrorReason.other',
    _ => 'ResourceErrorReason($rawValue)',
  };
}

final class ResourceKind extends _Enum {
  const ResourceKind.fromRawValue(super.rawValue);
  static const unknown = ResourceKind.fromRawValue(0);
  static const style = ResourceKind.fromRawValue(1);
  static const source = ResourceKind.fromRawValue(2);
  static const tile = ResourceKind.fromRawValue(3);
  static const glyphs = ResourceKind.fromRawValue(4);
  static const spriteImage = ResourceKind.fromRawValue(5);
  static const spriteJson = ResourceKind.fromRawValue(6);
  static const image = ResourceKind.fromRawValue(7);
  @override
  String toString() => switch (rawValue) {
    0 => 'ResourceKind.unknown',
    1 => 'ResourceKind.style',
    2 => 'ResourceKind.source',
    3 => 'ResourceKind.tile',
    4 => 'ResourceKind.glyphs',
    5 => 'ResourceKind.spriteImage',
    6 => 'ResourceKind.spriteJson',
    7 => 'ResourceKind.image',
    _ => 'ResourceKind($rawValue)',
  };
}

/// The resource kinds that a route or rule matches, one bit per
/// `mln_resource_kind` value. Registration requires at least one known bit.
/// `MLN_RESOURCE_KIND_MASK_ALL` selects every resource kind.
///
/// See `mln_resource_kind_mask` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class ResourceKindMask extends _Flags<ResourceKindMask> {
  const ResourceKindMask.fromRawValue(super.rawValue);
  static const unknown = ResourceKindMask.fromRawValue(1);
  static const style = ResourceKindMask.fromRawValue(2);
  static const source = ResourceKindMask.fromRawValue(4);
  static const tile = ResourceKindMask.fromRawValue(8);
  static const glyphs = ResourceKindMask.fromRawValue(16);
  static const spriteImage = ResourceKindMask.fromRawValue(32);
  static const spriteJson = ResourceKindMask.fromRawValue(64);
  static const image = ResourceKindMask.fromRawValue(128);
  static const all = ResourceKindMask.fromRawValue(255);
  @override
  ResourceKindMask _of(int rawValue) => ResourceKindMask.fromRawValue(rawValue);
}

final class ResourceLoadingMethod extends _Enum {
  const ResourceLoadingMethod.fromRawValue(super.rawValue);
  static const all = ResourceLoadingMethod.fromRawValue(0);
  static const cacheOnly = ResourceLoadingMethod.fromRawValue(1);
  static const networkOnly = ResourceLoadingMethod.fromRawValue(2);
  @override
  String toString() => switch (rawValue) {
    0 => 'ResourceLoadingMethod.all',
    1 => 'ResourceLoadingMethod.cacheOnly',
    2 => 'ResourceLoadingMethod.networkOnly',
    _ => 'ResourceLoadingMethod($rawValue)',
  };
}

final class ResourcePriority extends _Enum {
  const ResourcePriority.fromRawValue(super.rawValue);
  static const regular = ResourcePriority.fromRawValue(0);
  static const low = ResourcePriority.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'ResourcePriority.regular',
    1 => 'ResourcePriority.low',
    _ => 'ResourcePriority($rawValue)',
  };
}

final class ResourceProviderDecision extends _Enum {
  const ResourceProviderDecision.fromRawValue(super.rawValue);
  static const passThrough = ResourceProviderDecision.fromRawValue(0);
  static const handle = ResourceProviderDecision.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'ResourceProviderDecision.passThrough',
    1 => 'ResourceProviderDecision.handle',
    _ => 'ResourceProviderDecision($rawValue)',
  };
}

/// Field mask values for `mln_resource_request_info`.
///
/// See `mln_resource_request_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class ResourceRequestField extends _Flags<ResourceRequestField> {
  const ResourceRequestField.fromRawValue(super.rawValue);

  /// The request asks only for the bytes in range.
  static const range = ResourceRequestField.fromRawValue(1);

  /// The cached copy being revalidated carries a modification time.
  static const priorModified = ResourceRequestField.fromRawValue(2);

  /// The cached copy being revalidated carries an expiration time.
  static const priorExpires = ResourceRequestField.fromRawValue(4);
  @override
  ResourceRequestField _of(int rawValue) =>
      ResourceRequestField.fromRawValue(rawValue);
}

/// Field mask values for `mln_resource_response`.
///
/// See `mln_resource_response_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class ResourceResponseField extends _Flags<ResourceResponseField> {
  const ResourceResponseField.fromRawValue(super.rawValue);

  /// The response carries a modification time.
  static const modified = ResourceResponseField.fromRawValue(1);

  /// The response carries an expiration time.
  static const expires = ResourceResponseField.fromRawValue(2);

  /// An ERROR response carries the earliest time to retry the request.
  static const retryAfter = ResourceResponseField.fromRawValue(4);
  @override
  ResourceResponseField _of(int rawValue) =>
      ResourceResponseField.fromRawValue(rawValue);
}

/// How a resource provider answered a request.
///
/// See `mln_resource_response_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class ResourceResponseStatus extends _Enum {
  const ResourceResponseStatus.fromRawValue(super.rawValue);
  static const ok = ResourceResponseStatus.fromRawValue(0);
  static const error = ResourceResponseStatus.fromRawValue(1);
  static const noContent = ResourceResponseStatus.fromRawValue(2);
  static const notModified = ResourceResponseStatus.fromRawValue(3);
  @override
  String toString() => switch (rawValue) {
    0 => 'ResourceResponseStatus.ok',
    1 => 'ResourceResponseStatus.error',
    2 => 'ResourceResponseStatus.noContent',
    3 => 'ResourceResponseStatus.notModified',
    _ => 'ResourceResponseStatus($rawValue)',
  };
}

final class ResourceStoragePolicy extends _Enum {
  const ResourceStoragePolicy.fromRawValue(super.rawValue);
  static const permanent = ResourceStoragePolicy.fromRawValue(0);
  static const volatile = ResourceStoragePolicy.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'ResourceStoragePolicy.permanent',
    1 => 'ResourceStoragePolicy.volatile',
    _ => 'ResourceStoragePolicy($rawValue)',
  };
}

final class ResourceUsage extends _Enum {
  const ResourceUsage.fromRawValue(super.rawValue);
  static const online = ResourceUsage.fromRawValue(0);
  static const offline = ResourceUsage.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'ResourceUsage.online',
    1 => 'ResourceUsage.offline',
    _ => 'ResourceUsage($rawValue)',
  };
}

/// Bit values for the map and runtime event subscription masks.
///
/// See `mln_runtime_event_mask` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventMask extends _Flags<RuntimeEventMask> {
  const RuntimeEventMask.fromRawValue(super.rawValue);

  /// Selects no event type.
  static const none = RuntimeEventMask.fromRawValue(0);
  static const mapCameraWillChange = RuntimeEventMask.fromRawValue(2);
  static const mapCameraIsChanging = RuntimeEventMask.fromRawValue(4);
  static const mapCameraDidChange = RuntimeEventMask.fromRawValue(8);
  static const mapStyleLoaded = RuntimeEventMask.fromRawValue(16);
  static const mapLoadingStarted = RuntimeEventMask.fromRawValue(32);
  static const mapLoadingFinished = RuntimeEventMask.fromRawValue(64);
  static const mapLoadingFailed = RuntimeEventMask.fromRawValue(128);
  static const mapIdle = RuntimeEventMask.fromRawValue(256);
  static const mapRenderError = RuntimeEventMask.fromRawValue(1024);
  static const mapRenderFrameStarted = RuntimeEventMask.fromRawValue(8192);
  static const mapRenderFrameFinished = RuntimeEventMask.fromRawValue(16384);
  static const mapRenderMapStarted = RuntimeEventMask.fromRawValue(32768);
  static const mapRenderMapFinished = RuntimeEventMask.fromRawValue(65536);
  static const mapStyleImageMissing = RuntimeEventMask.fromRawValue(131072);
  static const mapTileAction = RuntimeEventMask.fromRawValue(262144);
  static const offlineRegionStatusChanged = RuntimeEventMask.fromRawValue(
    524288,
  );
  static const offlineRegionResponseError = RuntimeEventMask.fromRawValue(
    1048576,
  );
  static const offlineRegionTileCountLimitExceeded =
      RuntimeEventMask.fromRawValue(2097152);
  static const mapSourceChanged = RuntimeEventMask.fromRawValue(8388608);

  /// Selects every map-originated event type this version defines.
  static const allMapEvents = RuntimeEventMask.fromRawValue(8906238);

  /// Selects every runtime-originated event type this version defines.
  static const allRuntimeEvents = RuntimeEventMask.fromRawValue(3670016);

  /// Selects every event type this version defines.
  static const all = RuntimeEventMask.fromRawValue(12576254);
  @override
  RuntimeEventMask _of(int rawValue) => RuntimeEventMask.fromRawValue(rawValue);
}

/// Payload kinds used by `mln_runtime_event.payload_type`.
///
/// See `mln_runtime_event_payload_type` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventPayloadType extends _Enum {
  const RuntimeEventPayloadType.fromRawValue(super.rawValue);
  static const none = RuntimeEventPayloadType.fromRawValue(0);
  static const renderFrame = RuntimeEventPayloadType.fromRawValue(1);
  static const renderMap = RuntimeEventPayloadType.fromRawValue(2);
  static const tileAction = RuntimeEventPayloadType.fromRawValue(4);
  static const offlineRegionStatus = RuntimeEventPayloadType.fromRawValue(5);
  static const offlineRegionResponseError =
      RuntimeEventPayloadType.fromRawValue(6);
  static const offlineRegionTileCountLimit =
      RuntimeEventPayloadType.fromRawValue(7);
  @override
  String toString() => switch (rawValue) {
    0 => 'RuntimeEventPayloadType.none',
    1 => 'RuntimeEventPayloadType.renderFrame',
    2 => 'RuntimeEventPayloadType.renderMap',
    4 => 'RuntimeEventPayloadType.tileAction',
    5 => 'RuntimeEventPayloadType.offlineRegionStatus',
    6 => 'RuntimeEventPayloadType.offlineRegionResponseError',
    7 => 'RuntimeEventPayloadType.offlineRegionTileCountLimit',
    _ => 'RuntimeEventPayloadType($rawValue)',
  };
}

/// Source kinds used by `mln_runtime_event.source_type`.
///
/// See `mln_runtime_event_source_type` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventSourceType extends _Enum {
  const RuntimeEventSourceType.fromRawValue(super.rawValue);
  static const runtime = RuntimeEventSourceType.fromRawValue(0);
  static const map = RuntimeEventSourceType.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'RuntimeEventSourceType.runtime',
    1 => 'RuntimeEventSourceType.map',
    _ => 'RuntimeEventSourceType($rawValue)',
  };
}

/// Runtime event types carried by `mln_runtime_event.type`.
///
/// See `mln_runtime_event_type` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventType extends _Enum {
  const RuntimeEventType.fromRawValue(super.rawValue);
  static const mapCameraWillChange = RuntimeEventType.fromRawValue(1);
  static const mapCameraIsChanging = RuntimeEventType.fromRawValue(2);
  static const mapCameraDidChange = RuntimeEventType.fromRawValue(3);
  static const mapStyleLoaded = RuntimeEventType.fromRawValue(4);
  static const mapLoadingStarted = RuntimeEventType.fromRawValue(5);
  static const mapLoadingFinished = RuntimeEventType.fromRawValue(6);
  static const mapLoadingFailed = RuntimeEventType.fromRawValue(7);
  static const mapIdle = RuntimeEventType.fromRawValue(8);
  static const mapRenderError = RuntimeEventType.fromRawValue(10);
  static const mapRenderFrameStarted = RuntimeEventType.fromRawValue(13);
  static const mapRenderFrameFinished = RuntimeEventType.fromRawValue(14);
  static const mapRenderMapStarted = RuntimeEventType.fromRawValue(15);
  static const mapRenderMapFinished = RuntimeEventType.fromRawValue(16);
  static const mapStyleImageMissing = RuntimeEventType.fromRawValue(17);
  static const mapTileAction = RuntimeEventType.fromRawValue(18);
  static const offlineRegionStatusChanged = RuntimeEventType.fromRawValue(19);
  static const offlineRegionResponseError = RuntimeEventType.fromRawValue(20);
  static const offlineRegionTileCountLimitExceeded =
      RuntimeEventType.fromRawValue(21);
  static const mapSourceChanged = RuntimeEventType.fromRawValue(23);
  @override
  String toString() => switch (rawValue) {
    1 => 'RuntimeEventType.mapCameraWillChange',
    2 => 'RuntimeEventType.mapCameraIsChanging',
    3 => 'RuntimeEventType.mapCameraDidChange',
    4 => 'RuntimeEventType.mapStyleLoaded',
    5 => 'RuntimeEventType.mapLoadingStarted',
    6 => 'RuntimeEventType.mapLoadingFinished',
    7 => 'RuntimeEventType.mapLoadingFailed',
    8 => 'RuntimeEventType.mapIdle',
    10 => 'RuntimeEventType.mapRenderError',
    13 => 'RuntimeEventType.mapRenderFrameStarted',
    14 => 'RuntimeEventType.mapRenderFrameFinished',
    15 => 'RuntimeEventType.mapRenderMapStarted',
    16 => 'RuntimeEventType.mapRenderMapFinished',
    17 => 'RuntimeEventType.mapStyleImageMissing',
    18 => 'RuntimeEventType.mapTileAction',
    19 => 'RuntimeEventType.offlineRegionStatusChanged',
    20 => 'RuntimeEventType.offlineRegionResponseError',
    21 => 'RuntimeEventType.offlineRegionTileCountLimitExceeded',
    23 => 'RuntimeEventType.mapSourceChanged',
    _ => 'RuntimeEventType($rawValue)',
  };
}

/// Optional fields for `mln_source_feature_query_options`.
///
/// See `mln_source_feature_query_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
final class SourceFeatureQueryOptionField
    extends _Flags<SourceFeatureQueryOptionField> {
  const SourceFeatureQueryOptionField.fromRawValue(super.rawValue);
  static const sourceLayerIds = SourceFeatureQueryOptionField.fromRawValue(1);
  static const filter = SourceFeatureQueryOptionField.fromRawValue(2);
  @override
  SourceFeatureQueryOptionField _of(int rawValue) =>
      SourceFeatureQueryOptionField.fromRawValue(rawValue);
}

/// Status values returned by status-returning functions.
///
/// See `mln_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class Status extends _Enum {
  const Status.fromRawValue(super.rawValue);
  static const ok = Status.fromRawValue(0);

  /// A pointer, size field, mask, or handle argument was invalid.
  static const invalidArgument = Status.fromRawValue(-1);

  /// The object is valid but not currently in a state that permits the call.
  static const invalidState = Status.fromRawValue(-2);

  /// The handle is thread-affine and the call came from the wrong thread.
  static const wrongThread = Status.fromRawValue(-3);

  /// The entry point or requested behavior is unavailable in this build.
  static const unsupported = Status.fromRawValue(-4);

  /// A native MapLibre error or C++ exception was converted to status.
  static const nativeError = Status.fromRawValue(-5);

  /// The operation reached its terminal cancelled disposition.
  static const cancelled = Status.fromRawValue(-6);

  /// A conflicting driver call or lifecycle transition is in flight.
  static const busy = Status.fromRawValue(-7);

  /// The render target or graphics receiver was irreversibly lost.
  static const targetLost = Status.fromRawValue(-8);

  /// A nonblocking drain or acquisition has nothing to return yet.
  static const notReady = Status.fromRawValue(-9);

  /// A call named an ID with no live object behind it.
  static const notFound = Status.fromRawValue(-10);
  @override
  String toString() => switch (rawValue) {
    0 => 'Status.ok',
    -1 => 'Status.invalidArgument',
    -2 => 'Status.invalidState',
    -3 => 'Status.wrongThread',
    -4 => 'Status.unsupported',
    -5 => 'Status.nativeError',
    -6 => 'Status.cancelled',
    -7 => 'Status.busy',
    -8 => 'Status.targetLost',
    -9 => 'Status.notReady',
    -10 => 'Status.notFound',
    _ => 'Status($rawValue)',
  };
}

/// Field mask values for `mln_style_image_info`.
///
/// See `mln_style_image_info_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleImageInfoField extends _Flags<StyleImageInfoField> {
  const StyleImageInfoField.fromRawValue(super.rawValue);

  /// The image declares a content box.
  static const content = StyleImageInfoField.fromRawValue(1);

  /// The image declares a horizontal text-fit mode.
  static const textFitWidth = StyleImageInfoField.fromRawValue(2);

  /// The image declares a vertical text-fit mode.
  static const textFitHeight = StyleImageInfoField.fromRawValue(4);
  @override
  StyleImageInfoField _of(int rawValue) =>
      StyleImageInfoField.fromRawValue(rawValue);
}

/// Field mask values for `mln_style_image_options`.
///
/// See `mln_style_image_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleImageOptionField extends _Flags<StyleImageOptionField> {
  const StyleImageOptionField.fromRawValue(super.rawValue);
  static const pixelRatio = StyleImageOptionField.fromRawValue(1);
  static const sdf = StyleImageOptionField.fromRawValue(2);
  static const stretchX = StyleImageOptionField.fromRawValue(4);
  static const stretchY = StyleImageOptionField.fromRawValue(8);
  static const content = StyleImageOptionField.fromRawValue(16);
  static const textFitWidth = StyleImageOptionField.fromRawValue(32);
  static const textFitHeight = StyleImageOptionField.fromRawValue(64);
  @override
  StyleImageOptionField _of(int rawValue) =>
      StyleImageOptionField.fromRawValue(rawValue);
}

/// How a stretchable image fits text along one axis.
///
/// See `mln_style_image_text_fit` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleImageTextFit extends _Enum {
  const StyleImageTextFit.fromRawValue(super.rawValue);
  static const stretchOrShrink = StyleImageTextFit.fromRawValue(0);
  static const stretchOnly = StyleImageTextFit.fromRawValue(1);
  static const proportional = StyleImageTextFit.fromRawValue(2);
  @override
  String toString() => switch (rawValue) {
    0 => 'StyleImageTextFit.stretchOrShrink',
    1 => 'StyleImageTextFit.stretchOnly',
    2 => 'StyleImageTextFit.proportional',
    _ => 'StyleImageTextFit($rawValue)',
  };
}

/// Layer visibility values used by the visibility setter and layer info.
///
/// See `mln_style_layer_visibility` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleLayerVisibility extends _Enum {
  const StyleLayerVisibility.fromRawValue(super.rawValue);
  static const visible = StyleLayerVisibility.fromRawValue(0);
  static const none = StyleLayerVisibility.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'StyleLayerVisibility.visible',
    1 => 'StyleLayerVisibility.none',
    _ => 'StyleLayerVisibility($rawValue)',
  };
}

/// DEM raster encoding values.
///
/// See `mln_style_raster_dem_encoding` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleRasterDemEncoding extends _Enum {
  const StyleRasterDemEncoding.fromRawValue(super.rawValue);
  static const mapbox = StyleRasterDemEncoding.fromRawValue(0);
  static const terrarium = StyleRasterDemEncoding.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'StyleRasterDemEncoding.mapbox',
    1 => 'StyleRasterDemEncoding.terrarium',
    _ => 'StyleRasterDemEncoding($rawValue)',
  };
}

/// Fields available in `mln_style_source_info`.
///
/// See `mln_style_source_info_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleSourceInfoField extends _Flags<StyleSourceInfoField> {
  const StyleSourceInfoField.fromRawValue(super.rawValue);

  /// The source retains a URL.
  static const url = StyleSourceInfoField.fromRawValue(1);

  /// The tile source was defined with an inline TileJSON description.
  static const tilejson = StyleSourceInfoField.fromRawValue(2);

  /// The inline TileJSON description contains geographic bounds.
  static const bounds = StyleSourceInfoField.fromRawValue(4);

  /// The source exposes a tile size.
  static const tileSize = StyleSourceInfoField.fromRawValue(8);

  /// The source exposes a vector tile encoding.
  static const vectorEncoding = StyleSourceInfoField.fromRawValue(16);

  /// The source exposes a DEM raster encoding.
  static const rasterEncoding = StyleSourceInfoField.fromRawValue(32);

  /// The source carries an attribution string, which may be empty.
  static const attribution = StyleSourceInfoField.fromRawValue(64);
  @override
  StyleSourceInfoField _of(int rawValue) =>
      StyleSourceInfoField.fromRawValue(rawValue);
}

/// Style source type values returned by source metadata queries.
///
/// See `mln_style_source_type` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleSourceType extends _Enum {
  const StyleSourceType.fromRawValue(super.rawValue);
  static const unknown = StyleSourceType.fromRawValue(0);
  static const vector = StyleSourceType.fromRawValue(1);
  static const raster = StyleSourceType.fromRawValue(2);
  static const rasterDem = StyleSourceType.fromRawValue(3);
  static const geojson = StyleSourceType.fromRawValue(4);
  static const image = StyleSourceType.fromRawValue(5);
  static const video = StyleSourceType.fromRawValue(6);
  static const annotations = StyleSourceType.fromRawValue(7);
  static const customVector = StyleSourceType.fromRawValue(8);
  static const customMvtVector = StyleSourceType.fromRawValue(9);
  @override
  String toString() => switch (rawValue) {
    0 => 'StyleSourceType.unknown',
    1 => 'StyleSourceType.vector',
    2 => 'StyleSourceType.raster',
    3 => 'StyleSourceType.rasterDem',
    4 => 'StyleSourceType.geojson',
    5 => 'StyleSourceType.image',
    6 => 'StyleSourceType.video',
    7 => 'StyleSourceType.annotations',
    8 => 'StyleSourceType.customVector',
    9 => 'StyleSourceType.customMvtVector',
    _ => 'StyleSourceType($rawValue)',
  };
}

/// Tile URL coordinate scheme values.
///
/// See `mln_style_tile_scheme` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleTileScheme extends _Enum {
  const StyleTileScheme.fromRawValue(super.rawValue);
  static const xyz = StyleTileScheme.fromRawValue(0);
  static const tms = StyleTileScheme.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'StyleTileScheme.xyz',
    1 => 'StyleTileScheme.tms',
    _ => 'StyleTileScheme($rawValue)',
  };
}

/// Field mask values for `mln_style_transition_options`.
///
/// See `mln_style_transition_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleTransitionOptionField
    extends _Flags<StyleTransitionOptionField> {
  const StyleTransitionOptionField.fromRawValue(super.rawValue);
  static const duration = StyleTransitionOptionField.fromRawValue(1);
  static const delay = StyleTransitionOptionField.fromRawValue(2);
  static const enablePlacementTransitions =
      StyleTransitionOptionField.fromRawValue(4);
  @override
  StyleTransitionOptionField _of(int rawValue) =>
      StyleTransitionOptionField.fromRawValue(rawValue);
}

/// Vector tile encoding values.
///
/// See `mln_style_vector_tile_encoding` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleVectorTileEncoding extends _Enum {
  const StyleVectorTileEncoding.fromRawValue(super.rawValue);
  static const mvt = StyleVectorTileEncoding.fromRawValue(0);
  static const mlt = StyleVectorTileEncoding.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'StyleVectorTileEncoding.mvt',
    1 => 'StyleVectorTileEncoding.mlt',
    _ => 'StyleVectorTileEncoding($rawValue)',
  };
}

/// Tile LOD algorithms used by `mln_map_tile_options`.
///
/// See `mln_tile_lod_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class TileLodMode extends _Enum {
  const TileLodMode.fromRawValue(super.rawValue);
  static const defaultValue = TileLodMode.fromRawValue(0);
  static const distance = TileLodMode.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'TileLodMode.defaultValue',
    1 => 'TileLodMode.distance',
    _ => 'TileLodMode($rawValue)',
  };
}

/// Tile operations reported by tile observer events.
///
/// See `mln_tile_operation` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class TileOperation extends _Enum {
  const TileOperation.fromRawValue(super.rawValue);
  static const requestedFromCache = TileOperation.fromRawValue(0);
  static const requestedFromNetwork = TileOperation.fromRawValue(1);
  static const loadFromNetwork = TileOperation.fromRawValue(2);
  static const loadFromCache = TileOperation.fromRawValue(3);
  static const startParse = TileOperation.fromRawValue(4);
  static const endParse = TileOperation.fromRawValue(5);
  static const error = TileOperation.fromRawValue(6);
  static const cancelled = TileOperation.fromRawValue(7);
  static const nullValue = TileOperation.fromRawValue(8);
  @override
  String toString() => switch (rawValue) {
    0 => 'TileOperation.requestedFromCache',
    1 => 'TileOperation.requestedFromNetwork',
    2 => 'TileOperation.loadFromNetwork',
    3 => 'TileOperation.loadFromCache',
    4 => 'TileOperation.startParse',
    5 => 'TileOperation.endParse',
    6 => 'TileOperation.error',
    7 => 'TileOperation.cancelled',
    8 => 'TileOperation.nullValue',
    _ => 'TileOperation($rawValue)',
  };
}

/// How a rule or route compares its url against a request URL.
///
/// See `mln_url_match_flags` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class UrlMatchFlags extends _Flags<UrlMatchFlags> {
  const UrlMatchFlags.fromRawValue(super.rawValue);
  static const exact = UrlMatchFlags.fromRawValue(0);

  /// Reads url as a glob pattern.
  static const glob = UrlMatchFlags.fromRawValue(1);
  @override
  UrlMatchFlags _of(int rawValue) => UrlMatchFlags.fromRawValue(rawValue);
}

/// Viewport orientation modes used by `mln_map_viewport_options`.
///
/// See `mln_viewport_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class ViewportMode extends _Enum {
  const ViewportMode.fromRawValue(super.rawValue);
  static const defaultValue = ViewportMode.fromRawValue(0);
  static const flippedY = ViewportMode.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'ViewportMode.defaultValue',
    1 => 'ViewportMode.flippedY',
    _ => 'ViewportMode($rawValue)',
  };
}

/// WebGL context placement.
///
/// See `mln_webgl_context_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class WebglContextKind extends _Enum {
  const WebglContextKind.fromRawValue(super.rawValue);

  /// Use a host-created context on its current browser agent.
  static const existing = WebglContextKind.fromRawValue(0);

  /// Transfer the canvas named by canvas_selector to a native worker and create
  /// a WebGL 2 context there.
  static const transferredCanvas = WebglContextKind.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'WebglContextKind.existing',
    1 => 'WebglContextKind.transferredCanvas',
    _ => 'WebglContextKind($rawValue)',
  };
}

/// Metal frame acquired from a texture ring.
///
/// See `mln_metal_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class MetalTextureFrame extends _Value {
  const MetalTextureFrame({
    required this.generation,
    this.width = 0,
    this.height = 0,
    this.scaleFactor = 0,
    required this.frameId,
    this.slot = 0,
    this.texture = NativePointer.nullPointer,
    this.device = NativePointer.nullPointer,
    required this.pixelFormat,
  });

  /// Session generation that produced this frame.
  final BigInt generation;

  /// Physical Metal texture width in device pixels.
  final int width;

  /// Physical Metal texture height in device pixels.
  final int height;

  /// UI-to-device pixel scale used for this frame.
  final double scaleFactor;

  /// Opaque frame identity used to reject stale releases.
  final BigInt frameId;

  /// Ring slot that holds this frame. For a borrowed target, the index of its
  /// texture in the descriptor's textures array.
  final int slot;

  /// Borrowed `id<MTLTexture>` / `MTL::Texture*`. Valid until frame release.
  final NativePointer texture;

  /// Borrowed `id<MTLDevice>` / `MTL::Device*`. Valid until frame release.
  final NativePointer device;

  /// Backend-native pixel format value. Metal uses MTLPixelFormat.
  final BigInt pixelFormat;

  @override
  List<Object?> get _members => [
    generation,
    width,
    height,
    scaleFactor,
    frameId,
    slot,
    texture,
    device,
    pixelFormat,
  ];
}

/// OpenGL frame acquired from a texture ring.
///
/// See `mln_opengl_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class OpenglTextureFrame extends _Value {
  const OpenglTextureFrame({
    required this.generation,
    this.width = 0,
    this.height = 0,
    this.scaleFactor = 0,
    required this.frameId,
    this.slot = 0,
    this.texture = 0,
    this.target = 0,
    this.internalFormat = 0,
    this.format = 0,
    this.type = 0,
  });

  /// Session generation that produced this frame.
  final BigInt generation;

  /// Physical OpenGL texture width in device pixels.
  final int width;

  /// Physical OpenGL texture height in device pixels.
  final int height;

  /// UI-to-device pixel scale used for this frame.
  final double scaleFactor;

  /// Opaque frame identity used to reject stale releases.
  final BigInt frameId;

  /// Ring slot that holds this frame. For a borrowed target, the index of its
  /// texture in the descriptor's textures array.
  final int slot;

  /// Borrowed OpenGL texture object name. Valid until frame release.
  final int texture;

  /// OpenGL texture target. GL_TEXTURE_2D is the expected target.
  final int target;

  /// OpenGL internal format, such as GL_RGBA8. Zero for a borrowed texture,
  /// whose format the host chose.
  final int internalFormat;

  /// OpenGL pixel format, such as GL_RGBA. Zero for a borrowed texture.
  final int format;

  /// OpenGL pixel type, such as GL_UNSIGNED_BYTE. Zero for a borrowed texture.
  final int type;

  @override
  List<Object?> get _members => [
    generation,
    width,
    height,
    scaleFactor,
    frameId,
    slot,
    texture,
    target,
    internalFormat,
    format,
    type,
  ];
}

/// Backend synchronization copied by frame access and release calls.
///
/// See `mln_gpu_sync` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class GpuSync extends _Value {
  GpuSync({GpuSyncKind? kind, BigInt? object})
    : kind = kind ?? gpuSyncDefault().kind,
      object = object ?? gpuSyncDefault().object;

  /// One `mln_gpu_sync_kind` value.
  final GpuSyncKind kind;

  /// Bit pattern of the GLsync pointer.
  final BigInt object;

  @override
  List<Object?> get _members => [kind, object];
}

/// Geographic coordinate in degrees used by map and projection APIs.
///
/// See `mln_lat_lng` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class LatLng extends _Value {
  const LatLng({this.latitude = 0, this.longitude = 0});

  /// Latitude in degrees, finite and within \[-90, 90\] on input.
  final double latitude;

  /// Longitude in degrees. Input longitude must be finite.
  final double longitude;

  @override
  List<Object?> get _members => [latitude, longitude];
}

/// Screen-space inset in logical map pixels.
///
/// See `mln_edge_insets` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class EdgeInsets extends _Value {
  const EdgeInsets({
    this.top = 0,
    this.left = 0,
    this.bottom = 0,
    this.right = 0,
  });
  final double top;
  final double left;
  final double bottom;
  final double right;

  @override
  List<Object?> get _members => [top, left, bottom, right];
}

/// Screen-space point in logical map pixels.
///
/// See `mln_screen_point` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class ScreenPoint extends _Value {
  const ScreenPoint({this.x = 0, this.y = 0});
  final double x;
  final double y;

  @override
  List<Object?> get _members => [x, y];
}

/// Camera fields used by snapshots and camera updates.
///
/// See `mln_camera_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraOptions extends _Value {
  CameraOptions({
    this.center,
    this.centerAltitude,
    this.padding,
    this.anchor,
    this.zoom,
    this.bearing,
    this.pitch,
    this.roll,
    this.fieldOfView,
  });
  final LatLng? center;
  final double? centerAltitude;
  final EdgeInsets? padding;

  /// Optional screen-space focal point in logical map pixels.
  final ScreenPoint? anchor;
  final double? zoom;
  final double? bearing;
  final double? pitch;
  final double? roll;
  final double? fieldOfView;

  @override
  List<Object?> get _members => [
    center,
    centerAltitude,
    padding,
    anchor,
    zoom,
    bearing,
    pitch,
    roll,
    fieldOfView,
  ];
}

/// Logical extent in UI pixels and the device-pixel scale.
///
/// See `mln_logical_extent` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class LogicalExtent extends _Value {
  LogicalExtent({int? width, int? height, double? scaleFactor})
    : width = width ?? logicalExtentDefault().width,
      height = height ?? logicalExtentDefault().height,
      scaleFactor = scaleFactor ?? logicalExtentDefault().scaleFactor;

  /// Width in UI pixels. Defaults to 256.
  final int width;

  /// Height in UI pixels. Defaults to 256.
  final int height;

  /// Device pixels per UI pixel. Defaults to 1.0.
  final double scaleFactor;

  @override
  List<Object?> get _members => [width, height, scaleFactor];
}

/// MapLibre axonometric rendering options used for snapshots and commands.
///
/// See `mln_projection_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class ProjectionMode extends _Value {
  ProjectionMode({this.axonometric, this.xSkew, this.ySkew});

  /// Enables a non-perspective axonometric render transform.
  final bool? axonometric;

  /// Native x-skew factor used by the axonometric transform.
  final double? xSkew;

  /// Native y-skew factor used by the axonometric transform.
  final double? ySkew;

  @override
  List<Object?> get _members => [axonometric, xSkew, ySkew];
}

/// Live map viewport and render-transform controls.
///
/// See `mln_map_viewport_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class MapViewportOptions extends _Value {
  MapViewportOptions({
    this.northOrientation,
    this.constrainMode,
    this.viewportMode,
    this.frustumOffset,
  });

  /// One of `mln_north_orientation`.
  final NorthOrientation? northOrientation;

  /// One of `mln_constrain_mode`.
  final ConstrainMode? constrainMode;

  /// One of `mln_viewport_mode`.
  final ViewportMode? viewportMode;
  final EdgeInsets? frustumOffset;

  @override
  List<Object?> get _members => [
    northOrientation,
    constrainMode,
    viewportMode,
    frustumOffset,
  ];
}

/// Everything that a projection depends on: the transform that a published
/// snapshot or a rendered frame describes.
///
/// See `mln_map_view` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class MapView extends _Value {
  const MapView({
    required this.camera,
    required this.extent,
    required this.projectionMode,
    required this.viewport,
  });

  /// The camera. Its center is unwrapped: the longitude keeps the visible world
  /// copy and may fall outside -180 to 180. Every field except
  /// `MLN_CAMERA_OPTION_ANCHOR` is set.
  final CameraOptions camera;

  /// Logical size and device-pixel scale. A snapshot carries the map's scale
  /// factor, and a frame carries the scale factor of the target that drew it.
  final LogicalExtent extent;

  /// Axonometric mode and skew, with every field set.
  final ProjectionMode projectionMode;

  /// North orientation, constrain mode, viewport mode, and frustum offset, with
  /// every field set.
  final MapViewportOptions viewport;

  @override
  List<Object?> get _members => [camera, extent, projectionMode, viewport];
}

/// Terminal result of one frame demand, held by an owned frame-result batch and
/// copied by `mln_acquired_frame_get_result()`.
///
/// See `mln_render_frame_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
final class RenderFrameResult extends _Value {
  const RenderFrameResult({
    this.disposition = const RenderResult.fromRawValue(0),
    required this.token,
    required this.mapUpdateGeneration,
    required this.generation,
    required this.frameGeneration,
    this.needsRepaint = false,
    required this.view,
  });

  /// One `mln_render_result` value.
  final RenderResult disposition;
  final BigInt token;

  /// Generation of the map render update the demand evaluated. When disposition
  /// is `MLN_RENDER_RESULT_RENDERED`, the frame drew that update; compare it
  /// with `mln_map_snapshot.latest_render_update_generation` to find the first
  /// frame that includes a command.
  final BigInt mapUpdateGeneration;

  /// The session's target generation when the demand reached its result. See
  /// `mln_render_session_snapshot.generation`.
  final BigInt generation;

  /// Position of the frame among the frames that the session rendered, counting
  /// from one. Zero unless disposition is `MLN_RENDER_RESULT_RENDERED`.
  final BigInt frameGeneration;

  /// Whether the map asked for another frame while it rendered this one, as
  /// during an ongoing paint transition. Set only when disposition is
  /// `MLN_RENDER_RESULT_RENDERED`, and false for every other outcome. This is
  /// the same signal that `MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED` carries
  /// in its needs_repaint field, delivered with the frame result so a host can
  /// re-arm its frame loop without the runtime event round trip. A camera
  /// transition does not set it by itself: the map publishes a new update after
  /// each of the transition's frames instead, which a render-if-needed demand
  /// renders. A demand with `MLN_FRAME_DEMAND_WAIT_FOR_UPDATE` renders each
  /// transition update without a runtime-event round trip; the host re-arms the
  /// demand as each result arrives.
  final bool needsRepaint;

  /// The view that this frame drew, from the transform of the map update that
  /// it rendered. A host that displays the frame builds its projection with
  /// `mln_map_projection_create()` to place overlays where the frame drew their
  /// coordinates. Zero, with a size of zero, unless disposition is
  /// `MLN_RENDER_RESULT_RENDERED`.
  final MapView view;

  @override
  List<Object?> get _members => [
    disposition,
    token,
    mapUpdateGeneration,
    generation,
    frameGeneration,
    needsRepaint,
    view,
  ];
}

/// Vulkan frame acquired from a texture ring.
///
/// See `mln_vulkan_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class VulkanTextureFrame extends _Value {
  const VulkanTextureFrame({
    required this.generation,
    this.width = 0,
    this.height = 0,
    this.scaleFactor = 0,
    required this.frameId,
    this.slot = 0,
    required this.image,
    required this.imageView,
    this.device = NativePointer.nullPointer,
    this.format = 0,
    this.layout = 0,
  });

  /// Session generation that produced this frame.
  final BigInt generation;

  /// Physical Vulkan image width in device pixels.
  final int width;

  /// Physical Vulkan image height in device pixels.
  final int height;

  /// UI-to-device pixel scale used for this frame.
  final double scaleFactor;

  /// Opaque frame identity used to reject stale releases.
  final BigInt frameId;

  /// Ring slot that holds this frame. For a borrowed target, the index of its
  /// image in the descriptor's textures array.
  final int slot;

  /// Borrowed VkImage bit pattern. Valid until frame release.
  final BigInt image;

  /// Borrowed VkImageView bit pattern. Valid until frame release.
  final BigInt imageView;

  /// Borrowed VkDevice. Valid until frame release.
  final NativePointer device;

  /// Backend-native VkFormat value.
  final int format;

  /// Backend-native VkImageLayout value that the image is in:
  /// VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL for a session-owned ring, and the
  /// descriptor's final_layout for a borrowed one.
  final int layout;

  @override
  List<Object?> get _members => [
    generation,
    width,
    height,
    scaleFactor,
    frameId,
    slot,
    image,
    imageView,
    device,
    format,
    layout,
  ];
}

/// WebGPU frame acquired from a texture ring.
///
/// See `mln_webgpu_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class WebgpuTextureFrame extends _Value {
  const WebgpuTextureFrame({
    required this.generation,
    this.width = 0,
    this.height = 0,
    this.scaleFactor = 0,
    required this.frameId,
    this.slot = 0,
    this.texture = NativePointer.nullPointer,
    this.textureView = NativePointer.nullPointer,
    this.device = NativePointer.nullPointer,
    this.format = 0,
  });

  /// Session generation that produced this frame.
  final BigInt generation;

  /// Physical WebGPU texture width in device pixels.
  final int width;

  /// Physical WebGPU texture height in device pixels.
  final int height;

  /// UI-to-device pixel scale used for this frame.
  final double scaleFactor;

  /// Opaque frame identity used to reject stale releases.
  final BigInt frameId;

  /// Ring slot that holds this frame. For a borrowed target, the index of its
  /// texture in the descriptor's textures array.
  final int slot;

  /// Borrowed WGPUTexture. Valid until frame release.
  final NativePointer texture;

  /// Borrowed WGPUTextureView. Valid until frame release.
  final NativePointer textureView;

  /// Borrowed WGPUDevice. Valid until frame release.
  final NativePointer device;

  /// Backend-native WGPUTextureFormat value.
  final int format;

  @override
  List<Object?> get _members => [
    generation,
    width,
    height,
    scaleFactor,
    frameId,
    slot,
    texture,
    textureView,
    device,
    format,
  ];
}

/// Cubic easing curve for animated camera transitions.
///
/// See `mln_unit_bezier` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class UnitBezier extends _Value {
  const UnitBezier({this.x1 = 0, this.y1 = 0, this.x2 = 0, this.y2 = 0});
  final double x1;
  final double y1;
  final double x2;
  final double y2;

  @override
  List<Object?> get _members => [x1, y1, x2, y2];
}

/// The end of one camera command's transitions, borrowed for the callback.
///
/// See `mln_camera_transition_end` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraTransitionEnd extends _Value {
  const CameraTransitionEnd({
    this.outcome = const CameraTransitionOutcome.fromRawValue(0),
    required this.generation,
  });

  /// One of `mln_camera_transition_outcome`.
  final CameraTransitionOutcome outcome;

  /// Generation of the published snapshot that shows the camera where the
  /// transitions left it. The map events of the change that ended them carry
  /// this generation and are queued before the callback runs. Zero for
  /// `MLN_CAMERA_TRANSITION_OUTCOME_CLOSED`.
  final BigInt generation;

  @override
  List<Object?> get _members => [outcome, generation];
}

/// Receives the end of one camera command's transitions.
///
/// See `mln_camera_transition_end_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
typedef CameraTransitionEndCallback = void Function(CameraTransitionEnd);

/// Callback or queue state that one camera command copies to report the end of
/// its transitions.
///
/// See `mln_camera_transition_handler` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraTransitionHandler {
  const CameraTransitionHandler({this.callback});
  final CameraTransitionEndCallback? callback;
}

/// Optional animation controls for camera transitions.
///
/// See `mln_animation_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class AnimationOptions extends _Value {
  AnimationOptions({
    this.durationMs,
    this.velocity,
    this.minZoom,
    this.easing,
    this.transitionId,
    CameraTransitionHandler? endHandler,
  }) : endHandler = endHandler ?? animationOptionsDefault().endHandler;

  /// Duration in milliseconds. Must be finite and non-negative. Values that
  /// would overflow MapLibre Native's internal duration are invalid.
  final double? durationMs;

  /// Average fly velocity in screenfuls per second. Must be positive and
  /// finite; defaults to 1.2 when omitted. A flight whose derived duration
  /// exceeds Native's nanosecond duration range fails its completion with
  /// `MLN_STATUS_NATIVE_ERROR`. An explicit duration overrides velocity.
  final double? velocity;

  /// Peak zoom for flyTo transitions.
  final double? minZoom;
  final UnitBezier? easing;

  /// Caller-chosen identity of the command that these options animate, which
  /// `mln_map_cancel_camera_transition()` matches.
  final BigInt? transitionId;

  /// Reports the end of the command's transitions. Disabled by default.
  final CameraTransitionHandler endHandler;

  @override
  List<Object?> get _members => [
    durationMs,
    velocity,
    minZoom,
    easing,
    transitionId,
    endHandler,
  ];
}

/// Geographic bounds in degrees.
///
/// See `mln_lat_lng_bounds` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class LatLngBounds extends _Value {
  const LatLngBounds({
    this.southwest = const LatLng(),
    this.northeast = const LatLng(),
  });
  final LatLng southwest;
  final LatLng northeast;

  @override
  List<Object?> get _members => [southwest, northeast];
}

/// Optional map camera constraint fields.
///
/// See `mln_bound_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class BoundOptions extends _Value {
  BoundOptions({
    this.bounds,
    this.minZoom,
    this.maxZoom,
    this.minPitch,
    this.maxPitch,
    this.unbounded = false,
  });

  /// Read when fields contains `MLN_BOUND_OPTION_BOUNDS`.
  final LatLngBounds? bounds;
  final double? minZoom;
  final double? maxZoom;
  final double? minPitch;
  final double? maxPitch;

  /// Selects the unbounded geographic constraint, which leaves every camera
  /// center unconstrained and lets the map pan freely across the antimeridian.
  /// This differs from world bounds of -90/-180 to 90/180, which clamp
  /// longitude to that range. Mutually exclusive with
  /// `MLN_BOUND_OPTION_BOUNDS`, and leaves `mln_bound_options.bounds` unread.
  final bool unbounded;
  @override
  List<Object?> get _members => [
    bounds,
    minZoom,
    maxZoom,
    minPitch,
    maxPitch,
    unbounded,
  ];
}

/// One atomic relative camera update.
///
/// See `mln_camera_delta` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraDelta extends _Value {
  CameraDelta({
    this.offset,
    this.scale,
    this.bearing,
    this.pitch,
    this.anchor,
    AnimationOptions? animation,
    GesturePhase? gesturePhase,
  }) : animation = animation ?? cameraDeltaDefault().animation,
       gesturePhase = gesturePhase ?? cameraDeltaDefault().gesturePhase;

  /// Pan in logical map pixels; the content moves by this offset.
  final ScreenPoint? offset;

  /// Positive zoom factor; 2 zooms in one level.
  final double? scale;

  /// Degrees added to the bearing.
  final double? bearing;

  /// Degrees added to the pitch; positive tilts further from straight down.
  final double? pitch;

  /// Screen point in logical map pixels that scale, bearing, and pitch keep
  /// fixed.
  final ScreenPoint? anchor;
  final AnimationOptions animation;
  final GesturePhase gesturePhase;

  @override
  List<Object?> get _members => [
    offset,
    scale,
    bearing,
    pitch,
    anchor,
    animation,
    gesturePhase,
  ];
}

/// Optional fitting controls for camera-for-viewport queries.
///
/// See `mln_camera_fit_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraFitOptions extends _Value {
  CameraFitOptions({this.padding, this.bearing, this.pitch});
  final EdgeInsets? padding;
  final double? bearing;
  final double? pitch;

  @override
  List<Object?> get _members => [padding, bearing, pitch];
}

/// One atomic absolute camera update.
///
/// See `mln_camera_update` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraUpdate extends _Value {
  CameraUpdate({
    CameraUpdateMode? mode,
    CameraOptions? camera,
    AnimationOptions? animation,
    GesturePhase? gesturePhase,
  }) : mode = mode ?? cameraUpdateDefault().mode,
       camera = camera ?? cameraUpdateDefault().camera,
       animation = animation ?? cameraUpdateDefault().animation,
       gesturePhase = gesturePhase ?? cameraUpdateDefault().gesturePhase;
  final CameraUpdateMode mode;
  final CameraOptions camera;
  final AnimationOptions animation;
  final GesturePhase gesturePhase;

  @override
  List<Object?> get _members => [mode, camera, animation, gesturePhase];
}

/// Canonical tile identity used by custom geometry and custom MVT vector source
/// callbacks.
///
/// See `mln_canonical_tile_id` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class CanonicalTileId extends _Value {
  const CanonicalTileId({this.z = 0, this.x = 0, this.y = 0});
  final int z;
  final int x;
  final int y;

  @override
  List<Object?> get _members => [z, x, y];
}

/// Requests one tile of a custom geometry or custom MVT vector source.
///
/// See `mln_custom_source_fetch_tile_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
typedef CustomSourceFetchTileCallback = void Function(CanonicalTileId);

/// Cancels a request for one tile of a custom geometry or custom MVT vector
/// source, best effort.
///
/// See `mln_custom_source_cancel_tile_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
typedef CustomSourceCancelTileCallback = void Function(CanonicalTileId);

/// Options for custom geometry sources.
///
/// See `mln_custom_geometry_source_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class CustomGeometrySourceOptions {
  const CustomGeometrySourceOptions({
    this.fetchTile,
    this.cancelTile,
    this.minZoom,
    this.maxZoom,
    this.tolerance,
    this.tileSize,
    this.buffer,
    this.clip,
    this.wrap,
  });

  /// Tile fetch callback. Required in callback mode; null in queue mode.
  final CustomSourceFetchTileCallback? fetchTile;

  /// Optional best-effort tile cancel callback. Null in queue mode.
  final CustomSourceCancelTileCallback? cancelTile;
  final double? minZoom;
  final double? maxZoom;
  final double? tolerance;
  final int? tileSize;
  final int? buffer;
  final bool? clip;
  final bool? wrap;
}

/// Options for custom MVT vector sources.
///
/// See `mln_custom_mvt_vector_source_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class CustomMvtVectorSourceOptions {
  const CustomMvtVectorSourceOptions({
    this.fetchTile,
    this.cancelTile,
    this.minZoom,
    this.maxZoom,
  });

  /// Tile fetch callback. Required in callback mode; null in queue mode.
  final CustomSourceFetchTileCallback? fetchTile;

  /// Optional best-effort tile cancel callback. Null in queue mode.
  final CustomSourceCancelTileCallback? cancelTile;
  final double? minZoom;
  final double? maxZoom;
}

/// One nonblocking request for a frame.
///
/// See `mln_frame_demand` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
final class FrameDemand extends _Value {
  FrameDemand({FrameDemandFlag? flags, BigInt? token})
    : flags = flags ?? frameDemandDefault().flags,
      token = token ?? frameDemandDefault().token;

  /// A bitwise OR of `mln_frame_demand_flag` values. Defaults to
  /// `MLN_FRAME_DEMAND_IF_NEEDED`.
  final FrameDemandFlag flags;

  /// Host identity returned with the terminal frame result.
  final BigInt token;

  @override
  List<Object?> get _members => [flags, token];
}

/// Three-component vector used by free camera options.
///
/// See `mln_vec3` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class Vec3 extends _Value {
  const Vec3({this.x = 0, this.y = 0, this.z = 0});
  final double x;
  final double y;
  final double z;

  @override
  List<Object?> get _members => [x, y, z];
}

/// Quaternion stored as x, y, z, w components.
///
/// See `mln_quaternion` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class Quaternion extends _Value {
  const Quaternion({this.x = 0, this.y = 0, this.z = 0, this.w = 0});
  final double x;
  final double y;
  final double z;
  final double w;

  @override
  List<Object?> get _members => [x, y, z, w];
}

/// Free camera position and orientation in MapLibre Native camera space.
///
/// See `mln_free_camera_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class FreeCameraOptions extends _Value {
  FreeCameraOptions({this.position, this.orientation});
  final Vec3? position;
  final Quaternion? orientation;

  @override
  List<Object?> get _members => [position, orientation];
}

/// Options for GeoJSON sources, indexed and rendered at a fixed 512-pixel tile
/// size.
///
/// See `mln_geojson_source_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class GeojsonSourceOptions extends _Value {
  GeojsonSourceOptions({
    this.minZoom,
    this.maxZoom,
    this.tolerance,
    this.clusterMaxZoom,
    Uint8List? clusterProperties,
    this.buffer,
    this.clusterRadius,
    this.clusterMinPoints,
    this.lineMetrics,
    this.cluster,
    this.synchronousTiling,
  }) : clusterProperties = clusterProperties == null
           ? null
           : Uint8List.fromList(clusterProperties).asUnmodifiableView();

  /// Minimum tiling zoom. Defaults to 0.
  final double? minZoom;

  /// Maximum tiling zoom, an integer within \[0, 24\]. Defaults to 18.
  final double? maxZoom;

  /// Douglas-Peucker simplification tolerance. Defaults to 0.375.
  final double? tolerance;

  /// Highest zoom that clusters points. Defaults to 17.
  final double? clusterMaxZoom;

  /// Cluster aggregation expressions keyed by property name, as a JSON object
  /// whose members follow the MapLibre Style Spec clusterProperties form. The
  /// UTF-8 bytes are borrowed for the call.
  final Uint8List? clusterProperties;

  /// Tile buffer in pixels within \[0, 512\]. Defaults to 128.
  final int? buffer;

  /// Cluster radius in pixels within \[0, 4095\]. Defaults to 50.
  final int? clusterRadius;

  /// Points required to form a cluster. Defaults to 2.
  final int? clusterMinPoints;

  /// Adds line distance metrics to line features. Defaults to false.
  final bool? lineMetrics;

  /// Clusters point features. Defaults to false.
  final bool? cluster;

  /// Slices requested tiles inline during the update pass. Defaults to false.
  final bool? synchronousTiling;

  @override
  List<Object?> get _members => [
    minZoom,
    maxZoom,
    tolerance,
    clusterMaxZoom,
    clusterProperties,
    buffer,
    clusterRadius,
    clusterMinPoints,
    lineMetrics,
    cluster,
    synchronousTiling,
  ];
}

/// One HTTP header that a header rule supplies.
///
/// See `mln_http_header` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class HttpHeader extends _Value {
  const HttpHeader({required this.name, required this.value});
  final String name;
  final String value;

  @override
  List<Object?> get _members => [name, value];
}

/// One rule of a native HTTP header transform.
///
/// See `mln_http_header_rule` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class HttpHeaderRule extends _Value {
  HttpHeaderRule({
    ResourceKindMask? kinds,
    UrlMatchFlags? flags,
    String? url,
    List<HttpHeader>? headers,
  }) : kinds = kinds ?? httpHeaderRuleDefault().kinds,
       flags = flags ?? httpHeaderRuleDefault().flags,
       url = url ?? httpHeaderRuleDefault().url,
       headers = List.unmodifiable(headers ?? httpHeaderRuleDefault().headers);

  /// The resource kinds to match.
  final ResourceKindMask kinds;

  /// A bitwise OR of `mln_url_match_flags` values.
  final UrlMatchFlags flags;

  /// The URL or glob pattern to compare.
  final String url;

  /// The headers to set. Registration validates each one as
  /// `mln_http_header_transform_response_set_header()` does.
  final List<HttpHeader> headers;

  @override
  List<Object?> get _members => [kinds, flags, url, headers];
}

/// Lower-level Spherical Mercator projected-meter coordinate.
///
/// See `mln_projected_meters` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class ProjectedMeters extends _Value {
  const ProjectedMeters({this.northing = 0, this.easting = 0});

  /// Distance measured northward from the equator, in meters.
  final double northing;

  /// Distance measured eastward from the prime meridian, in meters.
  final double easting;

  @override
  List<Object?> get _members => [northing, easting];
}

/// Receives a MapLibre Native log record.
///
/// See `mln_log_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
typedef LogCallback = void Function(LogSeverity, LogEvent, int, String);

/// Process-global log callback or queue state.
///
/// See `mln_log_handler` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
final class LogHandler {
  const LogHandler({required this.callback});

  /// Receives each record.
  final LogCallback callback;
}

/// Caller-owned premultiplied RGBA8 image pixels.
///
/// See `mln_premultiplied_rgba8_image` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class PremultipliedRgba8Image extends _Value {
  PremultipliedRgba8Image({
    int? width,
    int? height,
    int? stride,
    Uint8List? pixels,
  }) : width = width ?? premultipliedRgba8ImageDefault().width,
       height = height ?? premultipliedRgba8ImageDefault().height,
       stride = stride ?? premultipliedRgba8ImageDefault().stride,
       pixels = Uint8List.fromList(
         pixels ?? premultipliedRgba8ImageDefault().pixels,
       ).asUnmodifiableView();
  final int width;
  final int height;

  /// Bytes per image row. Must be at least width \* 4.
  final int stride;

  /// Premultiplied RGBA8 pixels. Must not be null for a non-empty image.
  final Uint8List pixels;

  @override
  List<Object?> get _members => [width, height, stride, pixels];
}

/// One caller-owned Metal texture of a borrowed texture ring.
///
/// See `mln_metal_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class MetalBorrowedTexture extends _Value {
  const MetalBorrowedTexture({this.texture = NativePointer.nullPointer});

  /// Borrowed `id<MTLTexture>` / `MTL::Texture*`. Required.
  final NativePointer texture;

  @override
  List<Object?> get _members => [texture];
}

/// Metal attachment options for a borrowed texture target.
///
/// See `mln_metal_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class MetalBorrowedTextureDescriptor extends _Value {
  MetalBorrowedTextureDescriptor({
    LogicalExtent? extent,
    int? physicalWidth,
    int? physicalHeight,
    List<MetalBorrowedTexture>? textures,
  }) : extent = extent ?? metalBorrowedTextureDescriptorDefault().extent,
       physicalWidth =
           physicalWidth ??
           metalBorrowedTextureDescriptorDefault().physicalWidth,
       physicalHeight =
           physicalHeight ??
           metalBorrowedTextureDescriptorDefault().physicalHeight,
       textures = List.unmodifiable(
         textures ?? metalBorrowedTextureDescriptorDefault().textures,
       );

  /// Logical texture extent. The map viewport uses width and height and the
  /// renderer uses scale_factor; the physical size is stated separately below.
  /// A scale_factor that differs from the map's is accepted and logged as a
  /// warning.
  final LogicalExtent extent;

  /// Physical texture width in device pixels. Must be positive. Defaults to
  /// 256.
  final int physicalWidth;

  /// Physical texture height in device pixels. Must be positive. Defaults to
  /// 256.
  final int physicalHeight;

  /// The ring's textures, one per slot, in slot order. Required.
  final List<MetalBorrowedTexture> textures;

  @override
  List<Object?> get _members => [
    extent,
    physicalWidth,
    physicalHeight,
    textures,
  ];
}

/// Schedules service by the receiver that owns a queue or driver.
///
/// See `mln_wake_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wake_8h.html).
typedef WakeCallback = void Function();

/// Receiver wake callback copied by a successful owning call. The callback
/// schedules service by the receiver and returns without waiting for another
/// thread.
///
/// See `mln_wake` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wake_8h.html).
final class Wake {
  const Wake({this.callback});
  final WakeCallback? callback;
}

/// Common attachment policy copied before an attach call returns.
///
/// See `mln_render_session_attach_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class RenderSessionAttachOptions extends _Value {
  RenderSessionAttachOptions({
    RenderDriverKind? driver,
    int? requestedTextureRingDepth,
    GpuSyncKind? producerSyncKind,
    Wake? frameWake,
    Wake? driverWorkWake,
  }) : driver = driver ?? renderSessionAttachOptionsDefault().driver,
       requestedTextureRingDepth =
           requestedTextureRingDepth ??
           renderSessionAttachOptionsDefault().requestedTextureRingDepth,
       producerSyncKind =
           producerSyncKind ??
           renderSessionAttachOptionsDefault().producerSyncKind,
       frameWake = frameWake ?? renderSessionAttachOptionsDefault().frameWake,
       driverWorkWake =
           driverWorkWake ?? renderSessionAttachOptionsDefault().driverWorkWake;

  /// One `mln_render_driver_kind` value. Defaults to
  /// `MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD`.
  final RenderDriverKind driver;

  /// Requested slot count of a session-owned texture ring, from one to
  /// `MLN_TEXTURE_RING_MAX_DEPTH`. Private targets grant one slot regardless of
  /// this value. A borrowed texture ring's depth is its texture count, so
  /// borrowed and other targets ignore this value. Defaults to 1.
  final int requestedTextureRingDepth;

  /// Producer synchronization for acquired texture frames. Defaults to
  /// `MLN_GPU_SYNC_CPU_COMPLETE`. `MLN_GPU_SYNC_OPENGL_FENCE` requires a shared
  /// EGL or WGL texture target and a caller-graphics-thread driver. Other
  /// targets support only CPU completion and fail an unsupported request.
  final GpuSyncKind producerSyncKind;

  /// Wakes the receiver when the frame-result queue becomes nonempty.
  final Wake frameWake;

  /// Wakes the graphics receiver when caller-driver work is available,
  /// including work that a finished driver result releases.
  final Wake driverWorkWake;

  @override
  List<Object?> get _members => [
    driver,
    requestedTextureRingDepth,
    producerSyncKind,
    frameWake,
    driverWorkWake,
  ];
}

/// Metal backend context fields shared by Metal render targets.
///
/// See `mln_metal_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class MetalContextDescriptor extends _Value {
  const MetalContextDescriptor({this.device = NativePointer.nullPointer});

  /// `id<MTLDevice>` / `MTL::Device*`. Retained when the target requires it.
  final NativePointer device;

  @override
  List<Object?> get _members => [device];
}

/// Metal attachment options for an owned texture target.
///
/// See `mln_metal_owned_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class MetalOwnedTextureDescriptor extends _Value {
  MetalOwnedTextureDescriptor({
    LogicalExtent? extent,
    MetalContextDescriptor? context,
  }) : extent = extent ?? metalOwnedTextureDescriptorDefault().extent,
       context = context ?? metalOwnedTextureDescriptorDefault().context;

  /// Logical texture extent. A scale_factor that differs from the map's is
  /// accepted and logged as a warning.
  final LogicalExtent extent;

  /// Metal backend context. device is required.
  final MetalContextDescriptor context;

  @override
  List<Object?> get _members => [extent, context];
}

/// Metal attachment options for a native surface.
///
/// See `mln_metal_surface_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
final class MetalSurfaceDescriptor extends _Value {
  MetalSurfaceDescriptor({
    LogicalExtent? extent,
    MetalContextDescriptor? context,
    NativePointer? layer,
  }) : extent = extent ?? metalSurfaceDescriptorDefault().extent,
       context = context ?? metalSurfaceDescriptorDefault().context,
       layer = layer ?? metalSurfaceDescriptorDefault().layer;

  /// Logical surface extent. A scale_factor that differs from the map's is
  /// accepted and logged as a warning.
  final LogicalExtent extent;

  /// Metal backend context. device is optional for Metal surfaces.
  final MetalContextDescriptor context;

  /// `CAMetalLayer*` / `CA::MetalLayer*` retained by the session. Required.
  final NativePointer layer;

  @override
  List<Object?> get _members => [extent, context, layer];
}

/// WGL context fields shared by OpenGL render targets on Windows.
///
/// See `mln_wgl_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class WglContextDescriptor extends _Value {
  const WglContextDescriptor({
    this.deviceContext = NativePointer.nullPointer,
    this.shareContext = NativePointer.nullPointer,
    this.getProcAddress = NativePointer.nullPointer,
  });

  /// Borrowed HDC used to create the session context. Required.
  final NativePointer deviceContext;

  /// Borrowed HGLRC whose share group the session context joins. Required under
  /// shared ownership. A dedicated session joins no share group, so it must be
  /// null there.
  final NativePointer shareContext;

  /// Optional wglGetProcAddress-compatible function for the host loader.
  final NativePointer getProcAddress;

  @override
  List<Object?> get _members => [deviceContext, shareContext, getProcAddress];
}

/// EGL context fields shared by OpenGL render targets.
///
/// See `mln_egl_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class EglContextDescriptor extends _Value {
  const EglContextDescriptor({
    this.display = NativePointer.nullPointer,
    this.config = NativePointer.nullPointer,
    this.shareContext = NativePointer.nullPointer,
    this.clientApi = const OpenglClientApi.fromRawValue(0),
    this.getProcAddress = NativePointer.nullPointer,
  });

  /// Borrowed EGLDisplay. Required and kept initialized through teardown.
  /// eglGetDisplay(EGL_DEFAULT_DISPLAY) returns the same display for the whole
  /// process, so a shared display must stay initialized until every session
  /// using it has detached or been destroyed. The host must not call
  /// eglTerminate per session.
  final NativePointer display;

  /// Borrowed EGLConfig used to create the session context. Required. OpenGL
  /// texture targets require EGL_SURFACE_TYPE to include EGL_PBUFFER_BIT.
  final NativePointer config;

  /// Borrowed EGLContext whose share group the session context joins. Required
  /// under shared ownership, where the session also takes its client API from
  /// this context. A dedicated session joins no share group, so it must be null
  /// there and names client_api instead.
  final NativePointer shareContext;

  /// Client API the session creates its context for. Required under dedicated
  /// ownership. A shared session queries share_context for it, so this is
  /// ignored there.
  final OpenglClientApi clientApi;

  /// Optional eglGetProcAddress-compatible function for the host loader.
  final NativePointer getProcAddress;

  @override
  List<Object?> get _members => [
    display,
    config,
    shareContext,
    clientApi,
    getProcAddress,
  ];
}

/// WebGL context fields shared by OpenGL render targets in the browser.
///
/// See `mln_webgl_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class WebglContextDescriptor extends _Value {
  const WebglContextDescriptor({
    this.kind = const WebglContextKind.fromRawValue(0),
    this.context = 0,
    required this.canvasSelector,
  });

  /// One `mln_webgl_context_kind` value.
  final WebglContextKind kind;

  /// Borrowed EMSCRIPTEN_WEBGL_CONTEXT_HANDLE for EXISTING. Must be positive.
  final int context;

  /// Copied UTF-8 Emscripten target selector for TRANSFERRED_CANVAS. The HTML
  /// canvas must still be transferable when attachment starts.
  final String canvasSelector;

  @override
  List<Object?> get _members => [kind, context, canvasSelector];
}

/// OpenGL backend context fields shared by OpenGL render targets.
///
/// See `mln_opengl_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class OpenglContextDescriptor {
  const OpenglContextDescriptor({
    this.ownership = const OpenglContextOwnership.fromRawValue(0),
    required this.data,
  });

  /// Whether the session shares its driver thread and graphics objects with the
  /// host. A private EGL owned texture and a transferred WebGL canvas are
  /// dedicated to their core worker.
  final OpenglContextOwnership ownership;
  final OpenglContextDescriptorData data;
}

sealed class OpenglContextDescriptorData {
  const OpenglContextDescriptorData._();
}

final class OpenglContextDescriptorDataWgl extends OpenglContextDescriptorData {
  const OpenglContextDescriptorDataWgl(this.value) : super._();
  final WglContextDescriptor value;
}

final class OpenglContextDescriptorDataEgl extends OpenglContextDescriptorData {
  const OpenglContextDescriptorDataEgl(this.value) : super._();
  final EglContextDescriptor value;
}

final class OpenglContextDescriptorDataWebgl
    extends OpenglContextDescriptorData {
  const OpenglContextDescriptorDataWebgl(this.value) : super._();
  final WebglContextDescriptor value;
}

final class OpenglContextDescriptorDataUnknown
    extends OpenglContextDescriptorData {
  OpenglContextDescriptorDataUnknown(this.tag, Uint8List rawRecord)
    : rawRecord = Uint8List.fromList(rawRecord).asUnmodifiableView(),
      super._();
  final int tag;
  final Uint8List rawRecord;
}

/// One caller-owned OpenGL texture of a borrowed texture ring.
///
/// See `mln_opengl_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class OpenglBorrowedTexture extends _Value {
  const OpenglBorrowedTexture({this.texture = 0});

  /// Borrowed OpenGL texture object name. Required.
  final int texture;

  @override
  List<Object?> get _members => [texture];
}

/// OpenGL attachment options for a borrowed texture target.
///
/// See `mln_opengl_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class OpenglBorrowedTextureDescriptor extends _Value {
  OpenglBorrowedTextureDescriptor({
    LogicalExtent? extent,
    int? physicalWidth,
    int? physicalHeight,
    OpenglContextDescriptor? context,
    List<OpenglBorrowedTexture>? textures,
    int? target,
  }) : extent = extent ?? openglBorrowedTextureDescriptorDefault().extent,
       physicalWidth =
           physicalWidth ??
           openglBorrowedTextureDescriptorDefault().physicalWidth,
       physicalHeight =
           physicalHeight ??
           openglBorrowedTextureDescriptorDefault().physicalHeight,
       context = context ?? openglBorrowedTextureDescriptorDefault().context,
       textures = List.unmodifiable(
         textures ?? openglBorrowedTextureDescriptorDefault().textures,
       ),
       target = target ?? openglBorrowedTextureDescriptorDefault().target;

  /// Logical texture extent. The map viewport uses width and height and the
  /// renderer uses scale_factor; the physical size is stated separately below.
  /// A scale_factor that differs from the map's is accepted and logged as a
  /// warning.
  final LogicalExtent extent;

  /// Physical texture width in device pixels. Must be positive. Defaults to
  /// 256.
  final int physicalWidth;

  /// Physical texture height in device pixels. Must be positive. Defaults to
  /// 256.
  final int physicalHeight;

  /// Borrowed OpenGL context provider data. The textures must belong to this
  /// context or a context in the same share group.
  final OpenglContextDescriptor context;

  /// The ring's textures, one per slot, in slot order. Required.
  final List<OpenglBorrowedTexture> textures;

  /// OpenGL texture target of every texture. Must be GL_TEXTURE_2D.
  final int target;

  @override
  List<Object?> get _members => [
    extent,
    physicalWidth,
    physicalHeight,
    context,
    textures,
    target,
  ];
}

/// OpenGL attachment options for an owned texture target.
///
/// See `mln_opengl_owned_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class OpenglOwnedTextureDescriptor extends _Value {
  OpenglOwnedTextureDescriptor({
    LogicalExtent? extent,
    OpenglContextDescriptor? context,
  }) : extent = extent ?? openglOwnedTextureDescriptorDefault().extent,
       context = context ?? openglOwnedTextureDescriptorDefault().context;

  /// Logical texture extent. A scale_factor that differs from the map's is
  /// accepted and logged as a warning.
  final LogicalExtent extent;

  /// Borrowed OpenGL context provider data. Shared ownership creates a context
  /// whose texture frames the host can acquire. Dedicated EGL or transferred
  /// WebGL ownership creates a private core-worker context for CPU readback.
  final OpenglContextDescriptor context;

  @override
  List<Object?> get _members => [extent, context];
}

/// OpenGL attachment options for a native surface.
///
/// See `mln_opengl_surface_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
final class OpenglSurfaceDescriptor extends _Value {
  OpenglSurfaceDescriptor({
    LogicalExtent? extent,
    OpenglContextDescriptor? context,
    NativePointer? surface,
  }) : extent = extent ?? openglSurfaceDescriptorDefault().extent,
       context = context ?? openglSurfaceDescriptorDefault().context,
       surface = surface ?? openglSurfaceDescriptorDefault().surface;

  /// Logical surface extent. A scale_factor that differs from the map's is
  /// accepted and logged as a warning.
  final LogicalExtent extent;

  /// Borrowed OpenGL context provider data.
  final OpenglContextDescriptor context;

  /// Borrowed platform surface handle: an HDC for WGL and an EGLSurface for
  /// EGL, both required. Null for WebGL, whose context carries its canvas
  /// binding.
  final NativePointer surface;

  @override
  List<Object?> get _members => [extent, context, surface];
}

/// Vulkan backend context fields shared by Vulkan render targets.
///
/// See `mln_vulkan_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class VulkanContextDescriptor extends _Value {
  const VulkanContextDescriptor({
    this.instance = NativePointer.nullPointer,
    this.physicalDevice = NativePointer.nullPointer,
    this.device = NativePointer.nullPointer,
    this.graphicsQueue = NativePointer.nullPointer,
    this.graphicsQueueFamilyIndex = 0,
    this.getInstanceProcAddr = NativePointer.nullPointer,
    this.getDeviceProcAddr = NativePointer.nullPointer,
  });

  /// Borrowed VkInstance. Required.
  final NativePointer instance;

  /// Borrowed VkPhysicalDevice. Required.
  final NativePointer physicalDevice;

  /// Borrowed VkDevice. Required.
  final NativePointer device;

  /// Borrowed graphics VkQueue. Required. The session's driver submits to it
  /// from its own thread, so a host that uses the same queue passes
  /// `mln_render_session_attach_options.queue_lock` at attach.
  final NativePointer graphicsQueue;

  /// Queue family index for graphics_queue. Must support graphics commands.
  final int graphicsQueueFamilyIndex;

  /// PFN_vkGetInstanceProcAddr for the loader that created the instance.
  final NativePointer getInstanceProcAddr;

  /// PFN_vkGetDeviceProcAddr for the loader that created the Vulkan device.
  final NativePointer getDeviceProcAddr;

  @override
  List<Object?> get _members => [
    instance,
    physicalDevice,
    device,
    graphicsQueue,
    graphicsQueueFamilyIndex,
    getInstanceProcAddr,
    getDeviceProcAddr,
  ];
}

/// One caller-owned Vulkan image of a borrowed texture ring.
///
/// See `mln_vulkan_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class VulkanBorrowedTexture extends _Value {
  const VulkanBorrowedTexture({required this.image, required this.imageView});

  /// Borrowed VkImage. Required.
  final BigInt image;

  /// Borrowed VkImageView for image. Required. The view must be a 2D color view
  /// that matches image and the descriptor's format.
  final BigInt imageView;

  @override
  List<Object?> get _members => [image, imageView];
}

/// Vulkan attachment options for a borrowed texture target.
///
/// See `mln_vulkan_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class VulkanBorrowedTextureDescriptor extends _Value {
  VulkanBorrowedTextureDescriptor({
    LogicalExtent? extent,
    int? physicalWidth,
    int? physicalHeight,
    VulkanContextDescriptor? context,
    List<VulkanBorrowedTexture>? textures,
    int? format,
    int? initialLayout,
    int? finalLayout,
  }) : extent = extent ?? vulkanBorrowedTextureDescriptorDefault().extent,
       physicalWidth =
           physicalWidth ??
           vulkanBorrowedTextureDescriptorDefault().physicalWidth,
       physicalHeight =
           physicalHeight ??
           vulkanBorrowedTextureDescriptorDefault().physicalHeight,
       context = context ?? vulkanBorrowedTextureDescriptorDefault().context,
       textures = List.unmodifiable(
         textures ?? vulkanBorrowedTextureDescriptorDefault().textures,
       ),
       format = format ?? vulkanBorrowedTextureDescriptorDefault().format,
       initialLayout =
           initialLayout ??
           vulkanBorrowedTextureDescriptorDefault().initialLayout,
       finalLayout =
           finalLayout ?? vulkanBorrowedTextureDescriptorDefault().finalLayout;

  /// Logical texture extent. The map viewport uses width and height and the
  /// renderer uses scale_factor; the physical size is stated separately below.
  /// A scale_factor that differs from the map's is accepted and logged as a
  /// warning.
  final LogicalExtent extent;

  /// Physical image width in device pixels. Must be positive. Defaults to 256.
  final int physicalWidth;

  /// Physical image height in device pixels. Must be positive. Defaults to 256.
  final int physicalHeight;

  /// Borrowed Vulkan context. All handles are required.
  final VulkanContextDescriptor context;

  /// The ring's images, one per slot, in slot order. Required.
  final List<VulkanBorrowedTexture> textures;

  /// Backend-native VkFormat value of every image. VK_FORMAT_UNDEFINED is
  /// invalid.
  final int format;

  /// Backend-native VkImageLayout value expected at render-pass begin.
  final int initialLayout;

  /// Backend-native VkImageLayout value left after rendering succeeds. Defaults
  /// to 5, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
  final int finalLayout;

  @override
  List<Object?> get _members => [
    extent,
    physicalWidth,
    physicalHeight,
    context,
    textures,
    format,
    initialLayout,
    finalLayout,
  ];
}

/// Vulkan attachment options for an owned texture target.
///
/// See `mln_vulkan_owned_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class VulkanOwnedTextureDescriptor extends _Value {
  VulkanOwnedTextureDescriptor({
    LogicalExtent? extent,
    VulkanContextDescriptor? context,
  }) : extent = extent ?? vulkanOwnedTextureDescriptorDefault().extent,
       context = context ?? vulkanOwnedTextureDescriptorDefault().context;

  /// Logical texture extent. A scale_factor that differs from the map's is
  /// accepted and logged as a warning.
  final LogicalExtent extent;

  /// Borrowed Vulkan context. All handles are required.
  final VulkanContextDescriptor context;

  @override
  List<Object?> get _members => [extent, context];
}

/// Vulkan attachment options for a native surface.
///
/// See `mln_vulkan_surface_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
final class VulkanSurfaceDescriptor extends _Value {
  VulkanSurfaceDescriptor({
    LogicalExtent? extent,
    VulkanContextDescriptor? context,
    BigInt? surface,
  }) : extent = extent ?? vulkanSurfaceDescriptorDefault().extent,
       context = context ?? vulkanSurfaceDescriptorDefault().context,
       surface = surface ?? vulkanSurfaceDescriptorDefault().surface;

  /// Logical surface extent. A scale_factor that differs from the map's is
  /// accepted and logged as a warning.
  final LogicalExtent extent;

  /// Borrowed Vulkan context. All handles are required. The device must support
  /// VK_KHR_swapchain, and the queue family must support graphics and
  /// presentation to this descriptor's surface.
  final VulkanContextDescriptor context;

  /// Borrowed VkSurfaceKHR bit pattern. Required.
  final BigInt surface;

  @override
  List<Object?> get _members => [extent, context, surface];
}

/// WebGPU backend context fields shared by WebGPU render targets.
///
/// See `mln_webgpu_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class WebgpuContextDescriptor extends _Value {
  const WebgpuContextDescriptor({
    this.instance = NativePointer.nullPointer,
    this.device = NativePointer.nullPointer,
    this.queue = NativePointer.nullPointer,
  });

  /// Borrowed WGPUInstance. Optional for texture targets.
  final NativePointer instance;

  /// Borrowed WGPUDevice. Required.
  final NativePointer device;

  /// Borrowed WGPUQueue. Optional; null uses the device default queue. A
  /// non-null queue must belong to device.
  final NativePointer queue;

  @override
  List<Object?> get _members => [instance, device, queue];
}

/// One caller-owned WebGPU texture of a borrowed texture ring.
///
/// See `mln_webgpu_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class WebgpuBorrowedTexture extends _Value {
  const WebgpuBorrowedTexture({
    this.texture = NativePointer.nullPointer,
    this.textureView = NativePointer.nullPointer,
  });

  /// Borrowed WGPUTexture. Required.
  final NativePointer texture;

  /// Borrowed WGPUTextureView for texture. Required. The view must be a 2D
  /// color view compatible with texture and the descriptor's format.
  final NativePointer textureView;

  @override
  List<Object?> get _members => [texture, textureView];
}

/// WebGPU attachment options for a borrowed texture target.
///
/// See `mln_webgpu_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class WebgpuBorrowedTextureDescriptor extends _Value {
  WebgpuBorrowedTextureDescriptor({
    LogicalExtent? extent,
    int? physicalWidth,
    int? physicalHeight,
    WebgpuContextDescriptor? context,
    List<WebgpuBorrowedTexture>? textures,
    int? format,
  }) : extent = extent ?? webgpuBorrowedTextureDescriptorDefault().extent,
       physicalWidth =
           physicalWidth ??
           webgpuBorrowedTextureDescriptorDefault().physicalWidth,
       physicalHeight =
           physicalHeight ??
           webgpuBorrowedTextureDescriptorDefault().physicalHeight,
       context = context ?? webgpuBorrowedTextureDescriptorDefault().context,
       textures = List.unmodifiable(
         textures ?? webgpuBorrowedTextureDescriptorDefault().textures,
       ),
       format = format ?? webgpuBorrowedTextureDescriptorDefault().format;

  /// Logical texture extent. The map viewport uses width and height and the
  /// renderer uses scale_factor; the physical size is stated separately below.
  /// A scale_factor that differs from the map's is accepted and logged as a
  /// warning.
  final LogicalExtent extent;

  /// Physical texture width in device pixels. Defaults to 256.
  final int physicalWidth;

  /// Physical texture height in device pixels. Defaults to 256.
  final int physicalHeight;

  /// Borrowed WebGPU context. device is required. Rendering is submitted
  /// through context.queue or that device's default queue.
  final WebgpuContextDescriptor context;

  /// The ring's textures, one per slot, in slot order. Required.
  final List<WebgpuBorrowedTexture> textures;

  /// Backend-native WGPUTextureFormat value of every texture. Undefined is
  /// invalid.
  final int format;

  @override
  List<Object?> get _members => [
    extent,
    physicalWidth,
    physicalHeight,
    context,
    textures,
    format,
  ];
}

/// WebGPU attachment options for an owned texture target.
///
/// See `mln_webgpu_owned_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class WebgpuOwnedTextureDescriptor extends _Value {
  WebgpuOwnedTextureDescriptor({
    LogicalExtent? extent,
    WebgpuContextDescriptor? context,
  }) : extent = extent ?? webgpuOwnedTextureDescriptorDefault().extent,
       context = context ?? webgpuOwnedTextureDescriptorDefault().context;

  /// Logical texture extent. A scale_factor that differs from the map's is
  /// accepted and logged as a warning.
  final LogicalExtent extent;

  /// Borrowed WebGPU context. device is required.
  final WebgpuContextDescriptor context;

  @override
  List<Object?> get _members => [extent, context];
}

/// WebGPU attachment options for a native surface.
///
/// See `mln_webgpu_surface_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
final class WebgpuSurfaceDescriptor extends _Value {
  WebgpuSurfaceDescriptor({
    LogicalExtent? extent,
    WebgpuContextDescriptor? context,
    NativePointer? surface,
    int? format,
  }) : extent = extent ?? webgpuSurfaceDescriptorDefault().extent,
       context = context ?? webgpuSurfaceDescriptorDefault().context,
       surface = surface ?? webgpuSurfaceDescriptorDefault().surface,
       format = format ?? webgpuSurfaceDescriptorDefault().format;

  /// Logical surface extent. A scale_factor that differs from the map's is
  /// accepted and logged as a warning.
  final LogicalExtent extent;

  /// Borrowed WebGPU context. device is required.
  final WebgpuContextDescriptor context;

  /// Borrowed WGPUSurface. Required, and must stay alive for the session. The
  /// session configures it for this device and extent, and unconfigures it when
  /// the session ends.
  final NativePointer surface;

  /// WGPUTextureFormat to configure the surface with. Required. A browser host
  /// takes it from navigator.gpu.getPreferredCanvasFormat().
  final int format;

  @override
  List<Object?> get _members => [extent, context, surface, format];
}

/// Camera result borrowed for an ordered camera-query completion.
///
/// See `mln_camera_query_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraQueryResult extends _Value {
  const CameraQueryResult({required this.generation, required this.camera});
  final BigInt generation;
  final CameraOptions camera;

  @override
  List<Object?> get _members => [generation, camera];
}

/// Feature-state source, feature, and key selector.
///
/// See `mln_feature_state_selector` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class FeatureStateSelector extends _Value {
  const FeatureStateSelector({
    required this.sourceId,
    this.sourceLayerId,
    this.featureId,
    this.stateKey,
  });

  /// Source ID. Required and borrowed for the duration of the call.
  final String sourceId;

  /// Optional source layer ID. Required for vector-source disambiguation.
  final String? sourceLayerId;

  /// Optional feature ID. Required by set and get, optional for remove.
  final String? featureId;

  /// Optional state key. Used only by remove and requires feature_id.
  final String? stateKey;

  @override
  List<Object?> get _members => [sourceId, sourceLayerId, featureId, stateKey];
}

/// Tile prefetch and LOD tuning controls.
///
/// See `mln_map_tile_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class MapTileOptions extends _Value {
  MapTileOptions({
    this.prefetchZoomDelta,
    this.lodMinRadius,
    this.lodScale,
    this.lodPitchThreshold,
    this.lodZoomShift,
    this.lodMode,
  });

  /// Native uint8_t prefetch zoom delta.
  final int? prefetchZoomDelta;
  final double? lodMinRadius;
  final double? lodScale;
  final double? lodPitchThreshold;
  final double? lodZoomShift;

  /// One of `mln_tile_lod_mode`.
  final TileLodMode? lodMode;

  @override
  List<Object?> get _members => [
    prefetchZoomDelta,
    lodMinRadius,
    lodScale,
    lodPitchThreshold,
    lodZoomShift,
    lodMode,
  ];
}

/// Immutable map state copied from the latest published generation.
///
/// See `mln_map_snapshot` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class MapSnapshot extends _Value {
  const MapSnapshot({
    this.debugOptions = const MapDebugOption.fromRawValue(0),
    required this.generation,
    this.fullyLoaded = false,
    this.renderingStatsViewEnabled = false,
    this.gestureInProgress = false,
    this.eventMask = const RuntimeEventMask.fromRawValue(0),
    required this.latestRenderUpdateGeneration,
    required this.tile,
    required this.bounds,
    required this.freeCamera,
    required this.view,
  });

  /// Debug overlay mask of `mln_map_debug_option` values.
  final MapDebugOption debugOptions;
  final BigInt generation;

  /// True once every requested style and tile resource finished loading.
  final bool fullyLoaded;
  final bool renderingStatsViewEnabled;

  /// True while the map is inside a gesture.
  final bool gestureInProgress;
  final RuntimeEventMask eventMask;

  /// Generation of the latest render update the map published. A rendered frame
  /// at or past it draws map state that includes every command this snapshot
  /// observes, though animations and resource loads finish in later frames.
  final BigInt latestRenderUpdateGeneration;
  final MapTileOptions tile;
  final BoundOptions bounds;
  final FreeCameraOptions freeCamera;

  /// The map's camera, extent, projection mode, and viewport options.
  /// `mln_map_projection_create()` converts coordinates for this view.
  final MapView view;

  @override
  List<Object?> get _members => [
    debugOptions,
    generation,
    fullyLoaded,
    renderingStatsViewEnabled,
    gestureInProgress,
    eventMask,
    latestRenderUpdateGeneration,
    tile,
    bounds,
    freeCamera,
    view,
  ];
}

/// One stretchable interval along an image axis, in image pixels.
///
/// See `mln_image_stretch` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class ImageStretch extends _Value {
  const ImageStretch({this.from = 0, this.to = 0});
  final double from;
  final double to;

  @override
  List<Object?> get _members => [from, to];
}

/// Content-box insets in image pixels, measured from the image's top-left.
///
/// See `mln_image_content` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class ImageContent extends _Value {
  const ImageContent({
    this.left = 0,
    this.top = 0,
    this.right = 0,
    this.bottom = 0,
  });
  final double left;
  final double top;
  final double right;
  final double bottom;

  @override
  List<Object?> get _members => [left, top, right, bottom];
}

/// One complete runtime style image, borrowed for a completion callback.
///
/// See `mln_style_image_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleImageInfo extends _Value {
  StyleImageInfo({
    this.width = 0,
    this.height = 0,
    Uint8List? pixels,
    required List<ImageStretch> stretchX,
    required List<ImageStretch> stretchY,
    this.content,
    this.textFitWidth,
    this.textFitHeight,
    this.pixelRatio = 0,
    this.sdf = false,
  }) : pixels = Uint8List.fromList(
         pixels ?? const <int>[],
       ).asUnmodifiableView(),
       stretchX = List.unmodifiable(stretchX),
       stretchY = List.unmodifiable(stretchY);

  /// Image width in pixels.
  final int width;

  /// Image height in pixels.
  final int height;

  /// Premultiplied RGBA8 pixels in tightly packed rows of width \* 4 bytes, top
  /// row first.
  final Uint8List pixels;

  /// Horizontally stretchable intervals.
  final List<ImageStretch> stretchX;

  /// Vertically stretchable intervals.
  final List<ImageStretch> stretchY;

  /// Content box, when the image sets one.
  final ImageContent? content;

  /// How the image fits text horizontally, when it sets this.
  final StyleImageTextFit? textFitWidth;

  /// How the image fits text vertically, when it sets this.
  final StyleImageTextFit? textFitHeight;

  /// Sprite pixel ratio.
  final double pixelRatio;

  /// Whether the image is a signed distance field icon.
  final bool sdf;

  @override
  List<Object?> get _members => [
    width,
    height,
    pixels,
    stretchX,
    stretchY,
    content,
    textFitWidth,
    textFitHeight,
    pixelRatio,
    sdf,
  ];
}

/// One style layer, borrowed for a completion callback.
///
/// See `mln_style_layer_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleLayerInfo extends _Value {
  const StyleLayerInfo({
    required this.id,
    required this.type,
    this.sourceId,
    this.sourceLayer,
    this.minZoom = 0,
    this.maxZoom = 0,
    this.visibility = const StyleLayerVisibility.fromRawValue(0),
  });

  /// Layer ID.
  final String id;

  /// The style-spec layer type string.
  final String type;

  /// Source ID. Empty for a layer type that takes no source.
  final String? sourceId;

  /// Source-layer ID. Empty when the layer sets none.
  final String? sourceLayer;

  /// Lowest zoom at which the layer draws; -INFINITY with no lower bound.
  final double minZoom;

  /// Highest zoom at which the layer draws; INFINITY with no upper bound.
  final double maxZoom;

  /// One of `mln_style_layer_visibility`.
  final StyleLayerVisibility visibility;

  @override
  List<Object?> get _members => [
    id,
    type,
    sourceId,
    sourceLayer,
    minZoom,
    maxZoom,
    visibility,
  ];
}

/// Inline TileJSON metadata of a tile source.
///
/// See `mln_style_source_tile_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleSourceTileInfo extends _Value {
  StyleSourceTileInfo({
    required List<String> tileUrls,
    this.minZoom = 0,
    this.maxZoom = 0,
    this.scheme = const StyleTileScheme.fromRawValue(0),
  }) : tileUrls = List.unmodifiable(tileUrls);

  /// Tile URL templates in TileJSON order.
  final List<String> tileUrls;

  /// Lowest zoom level the TileJSON provides tiles for.
  final double minZoom;

  /// Highest zoom level the TileJSON provides tiles for.
  final double maxZoom;

  /// One of `mln_style_tile_scheme`.
  final StyleTileScheme scheme;

  @override
  List<Object?> get _members => [tileUrls, minZoom, maxZoom, scheme];
}

/// Complete metadata of one style source, borrowed for a completion callback.
///
/// See `mln_style_source_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleSourceInfo extends _Value {
  const StyleSourceInfo({
    required this.id,
    this.type = const StyleSourceType.fromRawValue(0),
    this.isVolatile = false,
    this.attribution,
    this.url,
    this.tilejson,
    this.bounds,
    this.tileSize,
    this.vectorEncoding,
    this.rasterEncoding,
  });

  /// Source ID.
  final String id;

  /// One of `mln_style_source_type`.
  final StyleSourceType type;

  /// Whether the source is marked volatile.
  final bool isVolatile;

  /// Attribution string, when the source sets one. It may be empty.
  final String? attribution;

  /// URL that the source loads from, when it has one.
  final String? url;

  /// Inline TileJSON metadata, when the source was defined with it.
  final StyleSourceTileInfo? tilejson;

  /// Geographic bounds, when inline TileJSON sets them.
  final LatLngBounds? bounds;

  /// Tile size in pixels, for a tile source.
  final int? tileSize;

  /// Vector tile encoding, for a vector source.
  final StyleVectorTileEncoding? vectorEncoding;

  /// DEM raster encoding, when inline TileJSON sets one.
  final StyleRasterDemEncoding? rasterEncoding;

  @override
  List<Object?> get _members => [
    id,
    type,
    isVolatile,
    attribution,
    url,
    tilejson,
    bounds,
    tileSize,
    vectorEncoding,
    rasterEncoding,
  ];
}

/// Global style transition options.
///
/// See `mln_style_transition_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleTransitionOptions extends _Value {
  StyleTransitionOptions({
    this.durationMs,
    this.delayMs,
    this.enablePlacementTransitions,
  });

  /// Transition duration in milliseconds. Must be finite and non-negative.
  /// Values that would overflow MapLibre Native's internal duration are
  /// invalid.
  final double? durationMs;

  /// Transition delay in milliseconds. Must be finite and non-negative. Values
  /// that would overflow MapLibre Native's internal duration are invalid.
  final double? delayMs;

  /// Whether symbol placement changes cross-fade.
  final bool? enablePlacementTransitions;

  @override
  List<Object?> get _members => [
    durationMs,
    delayMs,
    enablePlacementTransitions,
  ];
}

/// Options used when creating a map.
///
/// See `mln_map_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class MapOptions extends _Value {
  MapOptions({
    LogicalExtent? initialExtent,
    MapMode? mapMode,
    bool? fastPforEnabled,
    RuntimeEventMask? eventMask,
  }) : initialExtent = initialExtent ?? mapOptionsDefault().initialExtent,
       mapMode = mapMode ?? mapOptionsDefault().mapMode,
       fastPforEnabled = fastPforEnabled ?? mapOptionsDefault().fastPforEnabled,
       eventMask = eventMask ?? mapOptionsDefault().eventMask;

  /// Initial logical extent. Width and height must be nonzero, and scale_factor
  /// must be finite and positive. scale_factor is fixed for the map's lifetime.
  final LogicalExtent initialExtent;

  /// One of `mln_map_mode`. Defaults to `MLN_MAP_MODE_CONTINUOUS`.
  final MapMode mapMode;

  /// Decodes MapLibre Tile (MLT) tiles whose integer streams use FastPFOR
  /// encodings. Defaults to false.
  final bool fastPforEnabled;

  /// Map-originated event types this map queues, as a bitwise OR of
  /// `mln_runtime_event_mask` values.
  final RuntimeEventMask eventMask;

  @override
  List<Object?> get _members => [
    initialExtent,
    mapMode,
    fastPforEnabled,
    eventMask,
  ];
}

/// Options for runtime style images.
///
/// See `mln_style_image_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleImageOptions extends _Value {
  StyleImageOptions({
    List<ImageStretch>? stretchX,
    List<ImageStretch>? stretchY,
    this.content,
    this.textFitWidth,
    this.textFitHeight,
    this.pixelRatio,
    this.sdf,
  }) : stretchX = stretchX == null ? null : List.unmodifiable(stretchX),
       stretchY = stretchY == null ? null : List.unmodifiable(stretchY);

  /// Horizontally stretchable intervals. Borrowed for the call and copied
  /// before return. May be null only when stretch_x_count is 0.
  final List<ImageStretch>? stretchX;

  /// Vertically stretchable intervals. Borrowed for the call and copied before
  /// return. May be null only when stretch_y_count is 0.
  final List<ImageStretch>? stretchY;

  /// Content box used when icon-text-fit applies.
  final ImageContent? content;

  /// One of `mln_style_image_text_fit`. Defaults to STRETCH_OR_SHRINK.
  final StyleImageTextFit? textFitWidth;

  /// One of `mln_style_image_text_fit`. Defaults to STRETCH_OR_SHRINK.
  final StyleImageTextFit? textFitHeight;

  /// Sprite pixel ratio. Defaults to 1.
  final double? pixelRatio;

  /// Whether the image is a signed distance field icon. Defaults to false.
  final bool? sdf;

  @override
  List<Object?> get _members => [
    stretchX,
    stretchY,
    content,
    textFitWidth,
    textFitHeight,
    pixelRatio,
    sdf,
  ];
}

/// A borrowed view of one owned frame-result batch.
///
/// See `mln_render_frame_batch_view` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
final class RenderFrameBatchView extends _Value {
  RenderFrameBatchView({required List<RenderFrameResult> results})
    : results = List.unmodifiable(results);

  /// Borrowed array of result_count terminal frame results in completion order.
  final List<RenderFrameResult> results;

  @override
  List<Object?> get _members => [results];
}

final class RenderAbandonResult extends _Value {
  const RenderAbandonResult({
    this.disposition = const RenderAbandonDisposition.fromRawValue(0),
    this.quarantinedResourceCount = 0,
  });

  /// One `mln_render_abandon_disposition` value.
  final RenderAbandonDisposition disposition;

  /// Backend resource groups intentionally retained until process exit.
  final int quarantinedResourceCount;

  @override
  List<Object?> get _members => [disposition, quarantinedResourceCount];
}

/// Driver and target capabilities fixed for one attached render session.
///
/// See `mln_render_session_capabilities` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class RenderSessionCapabilities extends _Value {
  const RenderSessionCapabilities({
    required this.driver,
    this.textureRingDepth = 0,
    this.flags = const RenderSessionCapabilityFlag.fromRawValue(0),
  });

  /// One `mln_render_driver_kind` value.
  final RenderDriverKind driver;

  /// Granted texture ring depth: the slot count of a session-owned ring, or the
  /// texture count of a borrowed one. Zero for a surface.
  final int textureRingDepth;

  /// A bitwise OR of `mln_render_session_capability_flag` values.
  final RenderSessionCapabilityFlag flags;

  @override
  List<Object?> get _members => [driver, textureRingDepth, flags];
}

/// Any-thread render-session snapshot.
///
/// See `mln_render_session_snapshot` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
final class RenderSessionSnapshot extends _Value {
  const RenderSessionSnapshot({
    required this.state,
    required this.extent,
    required this.generation,
    required this.mapUpdateGeneration,
    required this.renderedUpdateGeneration,
    required this.frameGeneration,
    this.acquiredFrameCount = 0,
    this.pendingChanges = false,
  });

  /// One `mln_render_session_state` value.
  final RenderSessionState state;

  /// Logical extent applied by the driver.
  final LogicalExtent extent;

  /// Increases when the session's lifecycle state, target, or applied extent
  /// changes. An acquired texture frame reports the value it rendered under.
  final BigInt generation;

  /// Generation of the latest map render update that the session has seen.
  final BigInt mapUpdateGeneration;

  /// Map render update generation that the latest rendered frame drew. Zero
  /// before the first frame after attachment or a target replacement.
  final BigInt renderedUpdateGeneration;

  /// Frame generation of the latest rendered frame, zero before the first.
  final BigInt frameGeneration;

  /// Acquired frames that the host has not released or disposed.
  final int acquiredFrameCount;

  /// True when a map update or an accepted resize arrived after the latest
  /// rendered frame.
  final bool pendingChanges;

  @override
  List<Object?> get _members => [
    state,
    extent,
    generation,
    mapUpdateGeneration,
    renderedUpdateGeneration,
    frameGeneration,
    acquiredFrameCount,
    pendingChanges,
  ];
}

/// Screen-space box in logical map pixels.
///
/// See `mln_screen_box` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
final class ScreenBox extends _Value {
  const ScreenBox({
    this.min = const ScreenPoint(),
    this.max = const ScreenPoint(),
  });
  final ScreenPoint min;
  final ScreenPoint max;

  @override
  List<Object?> get _members => [min, max];
}

/// Screen-space line string in logical map pixels.
///
/// See `mln_screen_line_string` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
final class ScreenLineString extends _Value {
  ScreenLineString({required List<ScreenPoint> points})
    : points = List.unmodifiable(points);

  /// Points. Null only when point_count is 0.
  final List<ScreenPoint> points;

  @override
  List<Object?> get _members => [points];
}

/// Rendered feature query geometry descriptor.
///
/// See `mln_rendered_query_geometry` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
sealed class RenderedQueryGeometry {
  const RenderedQueryGeometry._();
  const factory RenderedQueryGeometry.point(ScreenPoint value) =
      RenderedQueryGeometryPoint;
  const factory RenderedQueryGeometry.box(ScreenBox value) =
      RenderedQueryGeometryBox;
  const factory RenderedQueryGeometry.lineString(ScreenLineString value) =
      RenderedQueryGeometryLineString;
}

final class RenderedQueryGeometryPoint extends RenderedQueryGeometry {
  const RenderedQueryGeometryPoint(this.value) : super._();
  final ScreenPoint value;
  @override
  bool operator ==(Object other) =>
      other is RenderedQueryGeometryPoint && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class RenderedQueryGeometryBox extends RenderedQueryGeometry {
  const RenderedQueryGeometryBox(this.value) : super._();
  final ScreenBox value;
  @override
  bool operator ==(Object other) =>
      other is RenderedQueryGeometryBox && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class RenderedQueryGeometryLineString extends RenderedQueryGeometry {
  const RenderedQueryGeometryLineString(this.value) : super._();
  final ScreenLineString value;
  @override
  bool operator ==(Object other) =>
      other is RenderedQueryGeometryLineString && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

/// Options for rendered feature queries.
///
/// See `mln_rendered_feature_query_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
final class RenderedFeatureQueryOptions extends _Value {
  RenderedFeatureQueryOptions({List<String>? layerIds, Uint8List? filter})
    : layerIds = layerIds == null ? null : List.unmodifiable(layerIds),
      filter = filter == null
          ? null
          : Uint8List.fromList(filter).asUnmodifiableView();

  /// Optional style layer IDs. When absent, all rendered layers are queried.
  final List<String>? layerIds;

  /// Optional UTF-8 MapLibre style-spec filter JSON. When absent, no filter.
  final Uint8List? filter;

  @override
  List<Object?> get _members => [layerIds, filter];
}

/// One query hit borrowed for a completion callback.
///
/// See `mln_queried_feature` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
final class QueriedFeature extends _Value {
  QueriedFeature({
    Uint8List? feature,
    this.sourceId,
    this.sourceLayerId,
    Uint8List? state,
  }) : feature = Uint8List.fromList(
         feature ?? const <int>[],
       ).asUnmodifiableView(),
       state = state == null
           ? null
           : Uint8List.fromList(state).asUnmodifiableView();
  final Uint8List feature;
  final String? sourceId;
  final String? sourceLayerId;
  final Uint8List? state;

  @override
  List<Object?> get _members => [feature, sourceId, sourceLayerId, state];
}

/// Options for source feature queries.
///
/// See `mln_source_feature_query_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
final class SourceFeatureQueryOptions extends _Value {
  SourceFeatureQueryOptions({List<String>? sourceLayerIds, Uint8List? filter})
    : sourceLayerIds = sourceLayerIds == null
          ? null
          : List.unmodifiable(sourceLayerIds),
      filter = filter == null
          ? null
          : Uint8List.fromList(filter).asUnmodifiableView();

  /// Optional source-layer IDs. Required by vector sources; ignored by GeoJSON.
  final List<String>? sourceLayerIds;

  /// Optional UTF-8 MapLibre style-spec filter JSON. When absent, no filter.
  final Uint8List? filter;

  @override
  List<Object?> get _members => [sourceLayerIds, filter];
}

/// CPU image readback metadata for a texture target frame.
///
/// See `mln_texture_image_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class TextureImageInfo extends _Value {
  const TextureImageInfo({
    this.width = 0,
    this.height = 0,
    this.stride = 0,
    this.byteLength = 0,
  });

  /// Physical image width in device pixels.
  final int width;

  /// Physical image height in device pixels.
  final int height;

  /// Bytes per image row.
  final int stride;

  /// Required output buffer byte length.
  final int byteLength;

  @override
  List<Object?> get _members => [width, height, stride, byteLength];
}

/// Texture readback borrowed for a completion callback.
///
/// See `mln_texture_readback_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class TextureReadbackResult extends _Value {
  TextureReadbackResult({Uint8List? data, this.info = const TextureImageInfo()})
    : data = Uint8List.fromList(data ?? const <int>[]).asUnmodifiableView();

  /// Borrowed pixel bytes, valid only during the callback.
  final Uint8List data;
  final TextureImageInfo info;

  @override
  List<Object?> get _members => [data, info];
}

/// A resource provider's answer to one request.
///
/// See `mln_resource_response` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class ResourceResponse extends _Value {
  ResourceResponse({
    this.status = const ResourceResponseStatus.fromRawValue(0),
    this.errorReason = const ResourceErrorReason.fromRawValue(0),
    Uint8List? bytes,
    this.errorMessage,
    this.mustRevalidate = false,
    this.modifiedUnixMs,
    this.expiresUnixMs,
    this.etag,
    this.retryAfterUnixMs,
  }) : bytes = Uint8List.fromList(bytes ?? const <int>[]).asUnmodifiableView();
  final ResourceResponseStatus status;
  final ResourceErrorReason errorReason;

  /// Response bytes. May be null only when byte_count is 0.
  final Uint8List bytes;
  final String? errorMessage;
  final bool mustRevalidate;
  final int? modifiedUnixMs;
  final int? expiresUnixMs;
  final String? etag;
  final int? retryAfterUnixMs;

  @override
  List<Object?> get _members => [
    status,
    errorReason,
    bytes,
    errorMessage,
    mustRevalidate,
    modifiedUnixMs,
    expiresUnixMs,
    etag,
    retryAfterUnixMs,
  ];
}

/// Reports that MapLibre cancelled a C API resource provider request.
///
/// See `mln_resource_request_cancel_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
typedef ResourceRequestCancelCallback = void Function();

/// Cancel callback or queue state for one handled resource request.
///
/// See `mln_resource_request_cancel_handler` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class ResourceRequestCancelHandler {
  const ResourceRequestCancelHandler({required this.callback});

  /// Required in callback mode; null in queue mode.
  final ResourceRequestCancelCallback callback;
}

/// One request pattern that a resource provider claims.
///
/// See `mln_resource_route` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class ResourceRoute extends _Value {
  ResourceRoute({ResourceKindMask? kinds, UrlMatchFlags? flags, String? url})
    : kinds = kinds ?? resourceRouteDefault().kinds,
      flags = flags ?? resourceRouteDefault().flags,
      url = url ?? resourceRouteDefault().url;

  /// The resource kinds to match.
  final ResourceKindMask kinds;

  /// A bitwise OR of `mln_url_match_flags` values.
  final UrlMatchFlags flags;

  /// The URL or glob pattern to compare.
  final String url;

  @override
  List<Object?> get _members => [kinds, flags, url];
}

/// Options used when creating a runtime.
///
/// See `mln_runtime_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeOptions extends _Value {
  RuntimeOptions({this.assetPath, this.cachePath});

  /// Directory root for asset:// URLs. Copied during runtime creation. Empty
  /// selects `/android_asset` on Android and `.` elsewhere.
  final String? assetPath;

  /// Cache database path. Copied during runtime creation.
  final String? cachePath;

  @override
  List<Object?> get _members => [assetPath, cachePath];
}

/// Tile-pyramid offline region definition.
///
/// See `mln_offline_tile_pyramid_region_definition` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class OfflineTilePyramidRegionDefinition extends _Value {
  const OfflineTilePyramidRegionDefinition({
    required this.styleUrl,
    this.bounds = const LatLngBounds(),
    this.minZoom = 0,
    this.maxZoom = 0,
    this.pixelRatio = 0,
    this.includeIdeographs = false,
  });

  /// Style URL. Copied during region creation.
  final String styleUrl;
  final LatLngBounds bounds;
  final double minZoom;

  /// Maximum zoom. Positive infinity follows MapLibre Native behavior and lets
  /// each tile source use its own maximum zoom.
  final double maxZoom;
  final double pixelRatio;
  final bool includeIdeographs;

  @override
  List<Object?> get _members => [
    styleUrl,
    bounds,
    minZoom,
    maxZoom,
    pixelRatio,
    includeIdeographs,
  ];
}

/// Geometry offline region definition.
///
/// See `mln_offline_geometry_region_definition` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class OfflineGeometryRegionDefinition extends _Value {
  OfflineGeometryRegionDefinition({
    required this.styleUrl,
    Uint8List? geometry,
    this.minZoom = 0,
    this.maxZoom = 0,
    this.pixelRatio = 0,
    this.includeIdeographs = false,
  }) : geometry = Uint8List.fromList(
         geometry ?? const <int>[],
       ).asUnmodifiableView();

  /// Style URL. Copied during region creation.
  final String styleUrl;

  /// UTF-8 GeoJSON Geometry bytes. Borrowed during region creation.
  final Uint8List geometry;
  final double minZoom;

  /// Maximum zoom. Positive infinity follows MapLibre Native behavior and lets
  /// each tile source use its own maximum zoom.
  final double maxZoom;
  final double pixelRatio;
  final bool includeIdeographs;

  @override
  List<Object?> get _members => [
    styleUrl,
    geometry,
    minZoom,
    maxZoom,
    pixelRatio,
    includeIdeographs,
  ];
}

/// Tagged offline region definition.
///
/// See `mln_offline_region_definition` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
sealed class OfflineRegionDefinition {
  const OfflineRegionDefinition._();
  const factory OfflineRegionDefinition.tilePyramid(
    OfflineTilePyramidRegionDefinition value,
  ) = OfflineRegionDefinitionTilePyramid;
  const factory OfflineRegionDefinition.geometry(
    OfflineGeometryRegionDefinition value,
  ) = OfflineRegionDefinitionGeometry;
}

final class OfflineRegionDefinitionTilePyramid extends OfflineRegionDefinition {
  const OfflineRegionDefinitionTilePyramid(this.value) : super._();
  final OfflineTilePyramidRegionDefinition value;
  @override
  bool operator ==(Object other) =>
      other is OfflineRegionDefinitionTilePyramid && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class OfflineRegionDefinitionGeometry extends OfflineRegionDefinition {
  const OfflineRegionDefinitionGeometry(this.value) : super._();
  final OfflineGeometryRegionDefinition value;
  @override
  bool operator ==(Object other) =>
      other is OfflineRegionDefinitionGeometry && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

/// Region data delivered by an offline completion.
///
/// See `mln_offline_region_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class OfflineRegionInfo extends _Value {
  OfflineRegionInfo({
    this.id = 0,
    required this.definition,
    Uint8List? metadata,
  }) : metadata = Uint8List.fromList(
         metadata ?? const <int>[],
       ).asUnmodifiableView();
  final int id;
  final OfflineRegionDefinition definition;

  /// Metadata bytes.
  final Uint8List metadata;

  @override
  List<Object?> get _members => [id, definition, metadata];
}

/// Offline region status snapshot.
///
/// See `mln_offline_region_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class OfflineRegionStatus extends _Value {
  const OfflineRegionStatus({
    this.downloadState = const OfflineRegionDownloadState.fromRawValue(0),
    required this.completedResourceCount,
    required this.completedResourceSize,
    required this.completedTileCount,
    required this.requiredTileCount,
    required this.completedTileSize,
    required this.requiredResourceCount,
    this.requiredResourceCountIsPrecise = false,
    this.complete = false,
  });

  /// One of `mln_offline_region_download_state`.
  final OfflineRegionDownloadState downloadState;
  final BigInt completedResourceCount;
  final BigInt completedResourceSize;
  final BigInt completedTileCount;
  final BigInt requiredTileCount;
  final BigInt completedTileSize;
  final BigInt requiredResourceCount;
  final bool requiredResourceCountIsPrecise;
  final bool complete;

  @override
  List<Object?> get _members => [
    downloadState,
    completedResourceCount,
    completedResourceSize,
    completedTileCount,
    requiredTileCount,
    completedTileSize,
    requiredResourceCount,
    requiredResourceCountIsPrecise,
    complete,
  ];
}

/// Rendering statistics reported in `MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME`.
///
/// See `mln_rendering_stats` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RenderingStats extends _Value {
  const RenderingStats({
    this.encodingTime = 0,
    this.renderingTime = 0,
    this.frameCount = 0,
    this.drawCallCount = 0,
    this.totalDrawCallCount = 0,
  });

  /// Frame CPU encoding time in seconds.
  final double encodingTime;

  /// Frame CPU rendering time in seconds.
  final double renderingTime;

  /// Number of frames rendered by the native renderer.
  final int frameCount;

  /// Draw calls executed during the most recent frame.
  final int drawCallCount;

  /// Total draw calls executed by the native renderer.
  final int totalDrawCallCount;

  @override
  List<Object?> get _members => [
    encodingTime,
    renderingTime,
    frameCount,
    drawCallCount,
    totalDrawCallCount,
  ];
}

/// Payload for `MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED`.
///
/// See `mln_runtime_event_render_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventRenderFrame extends _Value {
  const RuntimeEventRenderFrame({
    this.mode = const RenderMode.fromRawValue(0),
    this.needsRepaint = false,
    this.placementChanged = false,
    this.stats = const RenderingStats(),
  });

  /// One of `mln_render_mode`.
  final RenderMode mode;

  /// Whether MapLibre needs another frame after this one.
  final bool needsRepaint;

  /// Whether symbol placement changed during this frame.
  final bool placementChanged;
  final RenderingStats stats;

  @override
  List<Object?> get _members => [mode, needsRepaint, placementChanged, stats];
}

/// Payload for `MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED`.
///
/// See `mln_runtime_event_render_map` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventRenderMap extends _Value {
  const RuntimeEventRenderMap({this.mode = const RenderMode.fromRawValue(0)});

  /// One of `mln_render_mode`.
  final RenderMode mode;

  @override
  List<Object?> get _members => [mode];
}

/// Overscaled tile identity reported in tile observer events.
///
/// See `mln_tile_id` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class TileId extends _Value {
  const TileId({
    this.overscaledZ = 0,
    this.wrap = 0,
    this.canonicalZ = 0,
    this.canonicalX = 0,
    this.canonicalY = 0,
  });
  final int overscaledZ;
  final int wrap;
  final int canonicalZ;
  final int canonicalX;
  final int canonicalY;

  @override
  List<Object?> get _members => [
    overscaledZ,
    wrap,
    canonicalZ,
    canonicalX,
    canonicalY,
  ];
}

/// Payload for `MLN_RUNTIME_EVENT_MAP_TILE_ACTION`.
///
/// See `mln_runtime_event_tile_action` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventTileAction extends _Value {
  const RuntimeEventTileAction({
    this.operation = const TileOperation.fromRawValue(0),
    this.tileId = const TileId(),
  });

  /// One of `mln_tile_operation`.
  final TileOperation operation;
  final TileId tileId;

  @override
  List<Object?> get _members => [operation, tileId];
}

/// Payload for `MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED`.
///
/// See `mln_runtime_event_offline_region_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventOfflineRegionStatus extends _Value {
  const RuntimeEventOfflineRegionStatus({
    this.regionId = 0,
    required this.status,
  });
  final int regionId;
  final OfflineRegionStatus status;

  @override
  List<Object?> get _members => [regionId, status];
}

/// Payload for `MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR`.
///
/// See `mln_runtime_event_offline_region_response_error` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventOfflineRegionResponseError extends _Value {
  const RuntimeEventOfflineRegionResponseError({
    this.regionId = 0,
    this.reason = const ResourceErrorReason.fromRawValue(0),
  });
  final int regionId;

  /// One of `mln_resource_error_reason`.
  final ResourceErrorReason reason;

  @override
  List<Object?> get _members => [regionId, reason];
}

/// Payload for `MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED`.
///
/// See `mln_runtime_event_offline_region_tile_count_limit` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventOfflineRegionTileCountLimit extends _Value {
  const RuntimeEventOfflineRegionTileCountLimit({
    this.regionId = 0,
    required this.limit,
  });
  final int regionId;
  final BigInt limit;

  @override
  List<Object?> get _members => [regionId, limit];
}

/// One delivered runtime event.
///
/// See `mln_runtime_event` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEvent {
  const RuntimeEvent({
    required this.type,
    this.sourceType = const RuntimeEventSourceType.fromRawValue(0),
    required this.source,
    required this.generation,
    this.code = 0,
    required this.message,
    required this.payload,
  });

  /// One of `mln_runtime_event_type`.
  final RuntimeEventType type;

  /// One of `mln_runtime_event_source_type`.
  final RuntimeEventSourceType sourceType;

  /// Source handle selected by source_type: an `mln_runtime` or an `mln_map`.
  final BigInt source;

  /// Map snapshot generation that the event reports, or zero when source_type
  /// is `MLN_RUNTIME_EVENT_SOURCE_RUNTIME`.
  final BigInt generation;

  /// Secondary event detail whose meaning type selects. Depending on type it
  /// carries an `mln_camera_change_mode`, a MapLibre Native error ordinal, or
  /// 0. See `mln_runtime_event_type` for the per-type meaning.
  final int code;

  /// Borrowed UTF-8 message bytes, empty when this event carries no message.
  final String message;
  final RuntimeEventPayload payload;
}

sealed class RuntimeEventPayload {
  const RuntimeEventPayload._();
}

final class RuntimeEventPayloadRenderFrame extends RuntimeEventPayload {
  const RuntimeEventPayloadRenderFrame(this.value) : super._();
  final RuntimeEventRenderFrame value;
}

final class RuntimeEventPayloadRenderMap extends RuntimeEventPayload {
  const RuntimeEventPayloadRenderMap(this.value) : super._();
  final RuntimeEventRenderMap value;
}

final class RuntimeEventPayloadTileAction extends RuntimeEventPayload {
  const RuntimeEventPayloadTileAction(this.value) : super._();
  final RuntimeEventTileAction value;
}

final class RuntimeEventPayloadOfflineRegionStatus extends RuntimeEventPayload {
  const RuntimeEventPayloadOfflineRegionStatus(this.value) : super._();
  final RuntimeEventOfflineRegionStatus value;
}

final class RuntimeEventPayloadOfflineRegionResponseError
    extends RuntimeEventPayload {
  const RuntimeEventPayloadOfflineRegionResponseError(this.value) : super._();
  final RuntimeEventOfflineRegionResponseError value;
}

final class RuntimeEventPayloadOfflineRegionTileCountLimit
    extends RuntimeEventPayload {
  const RuntimeEventPayloadOfflineRegionTileCountLimit(this.value) : super._();
  final RuntimeEventOfflineRegionTileCountLimit value;
}

final class RuntimeEventPayloadNone extends RuntimeEventPayload {
  const RuntimeEventPayloadNone() : super._();
}

final class RuntimeEventPayloadUnknown extends RuntimeEventPayload {
  RuntimeEventPayloadUnknown(this.tag, Uint8List rawRecord)
    : rawRecord = Uint8List.fromList(rawRecord).asUnmodifiableView(),
      super._();
  final int tag;
  final Uint8List rawRecord;
}

/// A borrowed span of runtime events in one delivery.
///
/// See `mln_event_batch_view` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class EventBatchView extends _Value {
  EventBatchView({required List<RuntimeEvent> events})
    : events = List.unmodifiable(events);

  /// Borrowed array of event_count events in queue order.
  final List<RuntimeEvent> events;

  @override
  List<Object?> get _members => [events];
}

/// Receives selected events after their snapshots have been published.
///
/// See `mln_runtime_event_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
typedef RuntimeEventCallback = void Function(EventBatchView);

/// Runtime and map event delivery. Set either callback or queue.
///
/// See `mln_runtime_event_handler` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventHandler {
  const RuntimeEventHandler({this.callback});
  final RuntimeEventCallback? callback;
}

/// HTTP header transform state: a callback, or native header rules.
///
/// See `mln_http_header_transform` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class HttpHeaderTransform {
  HttpHeaderTransform({List<HttpHeaderRule> rules = const []})
    : rules = List.unmodifiable(rules);

  /// Header rules for a transform without a callback; empty with one.
  final List<HttpHeaderRule> rules;
}

/// Inclusive byte range of a resource request.
///
/// See `mln_resource_range` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class ResourceRange extends _Value {
  const ResourceRange({required this.start, required this.end});

  /// First byte offset of the requested range.
  final BigInt start;

  /// Last byte offset of the requested range, inclusive.
  final BigInt end;

  @override
  List<Object?> get _members => [start, end];
}

final class ResourceRequestInfo extends _Value {
  ResourceRequestInfo({
    this.requestedUrl,
    this.resolvedUrl,
    this.kind = const ResourceKind.fromRawValue(0),
    this.loadingMethod = const ResourceLoadingMethod.fromRawValue(0),
    this.priority = const ResourcePriority.fromRawValue(0),
    this.usage = const ResourceUsage.fromRawValue(0),
    this.storagePolicy = const ResourceStoragePolicy.fromRawValue(0),
    this.range,
    this.priorModifiedUnixMs,
    this.priorExpiresUnixMs,
    this.priorEtag,
    Uint8List? priorData,
  }) : priorData = Uint8List.fromList(
         priorData ?? const <int>[],
       ).asUnmodifiableView();

  /// URL entering the network layer, before tile server normalization.
  final String? requestedUrl;

  /// URL to fetch, after resource-kind normalization against the runtime's tile
  /// server options and API key.
  final String? resolvedUrl;
  final ResourceKind kind;
  final ResourceLoadingMethod loadingMethod;
  final ResourcePriority priority;
  final ResourceUsage usage;
  final ResourceStoragePolicy storagePolicy;
  final ResourceRange? range;
  final int? priorModifiedUnixMs;
  final int? priorExpiresUnixMs;
  final String? priorEtag;
  final Uint8List priorData;

  @override
  List<Object?> get _members => [
    requestedUrl,
    resolvedUrl,
    kind,
    loadingMethod,
    priority,
    usage,
    storagePolicy,
    range,
    priorModifiedUnixMs,
    priorExpiresUnixMs,
    priorEtag,
    priorData,
  ];
}

/// Intercepts a network resource request.
///
/// See `mln_resource_provider_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
typedef ResourceProviderCallback =
    void Function(ResourceRequestInfo, ResourceRequestHandle);

/// Resource provider state.
///
/// See `mln_resource_provider` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class ResourceProvider {
  ResourceProvider({
    required this.callback,
    List<ResourceRoute> routes = const [],
  }) : routes = List.unmodifiable(routes);

  /// Decides each request that the routes leave to the provider.
  final ResourceProviderCallback callback;

  /// The requests that reach the provider; empty to claim every request.
  final List<ResourceRoute> routes;
}

/// Asks the host for the URL of one network resource request.
///
/// See `mln_resource_transform_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
typedef ResourceTransformCallback =
    void Function(ResourceKind, String, ResourceTransformRequestHandle);

/// Resource transform state.
///
/// See `mln_resource_transform` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class ResourceTransform {
  const ResourceTransform({required this.callback});

  /// Required in callback mode; null in queue mode.
  final ResourceTransformCallback callback;
}
