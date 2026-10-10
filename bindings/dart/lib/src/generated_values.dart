// Generated from the C headers by tools/bindgen. Do not edit.
part of 'values.dart';

final class AmbientCacheOperation extends _Enum {
  const AmbientCacheOperation.fromRawValue(super.rawValue);
  static const resetDatabase = AmbientCacheOperation.fromRawValue(1);
  static const packDatabase = AmbientCacheOperation.fromRawValue(2);
  static const invalidate = AmbientCacheOperation.fromRawValue(3);
  static const clear = AmbientCacheOperation.fromRawValue(4);
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
}

/// Relative camera operation carried by `mln_camera_delta`.
///
/// See `mln_camera_delta_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraDeltaKind extends _Enum {
  const CameraDeltaKind.fromRawValue(super.rawValue);
  static const move = CameraDeltaKind.fromRawValue(0);
  static const scale = CameraDeltaKind.fromRawValue(1);
  static const bearing = CameraDeltaKind.fromRawValue(2);
  static const pitch = CameraDeltaKind.fromRawValue(3);
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

/// Camera transition behavior for `mln_camera_update`.
///
/// See `mln_camera_update_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraUpdateMode extends _Enum {
  const CameraUpdateMode.fromRawValue(super.rawValue);
  static const jump = CameraUpdateMode.fromRawValue(0);
  static const ease = CameraUpdateMode.fromRawValue(1);
  static const fly = CameraUpdateMode.fromRawValue(2);
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
  static const tileSize = GeojsonSourceOptionField.fromRawValue(32);
  static const buffer = GeojsonSourceOptionField.fromRawValue(64);
  static const clusterRadius = GeojsonSourceOptionField.fromRawValue(128);
  static const clusterMinPoints = GeojsonSourceOptionField.fromRawValue(256);
  static const lineMetrics = GeojsonSourceOptionField.fromRawValue(512);
  static const cluster = GeojsonSourceOptionField.fromRawValue(1024);
  static const synchronousTiling = GeojsonSourceOptionField.fromRawValue(2048);
  @override
  GeojsonSourceOptionField _of(int rawValue) =>
      GeojsonSourceOptionField.fromRawValue(rawValue);
}

/// Gesture boundary carried atomically with a camera update.
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
}

/// Synchronization payload kind for acquired texture frames.
///
/// See `mln_gpu_sync_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class GpuSyncKind extends _Enum {
  const GpuSyncKind.fromRawValue(super.rawValue);

  /// The producer or consumer has completed before the API call returns.
  static const cpuComplete = GpuSyncKind.fromRawValue(0);

  /// `id<MTLSharedEvent>` plus a monotonically increasing signal value.
  static const metalSharedEvent = GpuSyncKind.fromRawValue(1);

  /// VkSemaphore plus a timeline value.
  static const vulkanTimelineSemaphore = GpuSyncKind.fromRawValue(2);

  /// GLsync, used only by a caller-graphics-thread driver.
  static const openglFence = GpuSyncKind.fromRawValue(3);

  /// A backend-defined WebGPU completion token.
  static const webgpuToken = GpuSyncKind.fromRawValue(4);
}

/// Location indicator image-name properties.
///
/// See `mln_location_indicator_image_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class LocationIndicatorImageKind extends _Enum {
  const LocationIndicatorImageKind.fromRawValue(super.rawValue);
  static const top = LocationIndicatorImageKind.fromRawValue(0);
  static const bearing = LocationIndicatorImageKind.fromRawValue(1);
  static const shadow = LocationIndicatorImageKind.fromRawValue(2);
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
}

final class OfflineRegionDefinitionType extends _Enum {
  const OfflineRegionDefinitionType.fromRawValue(super.rawValue);
  static const tilePyramid = OfflineRegionDefinitionType.fromRawValue(1);
  static const geometry = OfflineRegionDefinitionType.fromRawValue(2);
}

final class OfflineRegionDownloadState extends _Enum {
  const OfflineRegionDownloadState.fromRawValue(super.rawValue);
  static const inactive = OfflineRegionDownloadState.fromRawValue(0);
  static const active = OfflineRegionDownloadState.fromRawValue(1);
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

/// Result of irreversible CPU-side target abandonment.
///
/// See `mln_render_abandon_disposition` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
final class RenderAbandonDisposition extends _Enum {
  const RenderAbandonDisposition.fromRawValue(super.rawValue);

  /// No graphics resources remained when control was abandoned.
  static const clean = RenderAbandonDisposition.fromRawValue(0);

  /// Graphics resources could not be destroyed and were quarantined.
  static const quarantined = RenderAbandonDisposition.fromRawValue(1);
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

  /// Native code owns a serial worker that initializes, drives, and tears down
  /// transferable graphics state.
  static const coreWorker = RenderDriverKind.fromRawValue(1);

  /// The host explicitly calls the narrow driver API from the thread or realm
  /// where its graphics context is current.
  static const callerGraphicsThread = RenderDriverKind.fromRawValue(2);
}

/// Render modes reported by render observer events.
///
/// See `mln_render_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RenderMode extends _Enum {
  const RenderMode.fromRawValue(super.rawValue);
  static const partial = RenderMode.fromRawValue(0);
  static const full = RenderMode.fromRawValue(1);
}

/// Terminal disposition of one accepted frame demand.
///
/// See `mln_render_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
final class RenderResult extends _Enum {
  const RenderResult.fromRawValue(super.rawValue);

  /// A frame was rendered for acquisition, presentation, or ordered readback.
  static const rendered = RenderResult.fromRawValue(0);

  /// No newer map update was available, or the map had no complete frame to
  /// draw yet. The map publishes another update when it has one.
  static const noUpdate = RenderResult.fromRawValue(1);

  /// An ordered extent change had not reached the map. The map publishes an
  /// update at the new extent.
  static const sizePending = RenderResult.fromRawValue(2);

  /// The target could not produce a frame. The attempt consumes nothing, so a
  /// later demand with the same flags renders what this one would have. This
  /// result does not cause a map update, so the host demands again when the
  /// target can be ready, such as after a paced delay.
  static const targetNotReady = RenderResult.fromRawValue(3);

  /// A newer demand in the same coalescing boundary replaced this demand.
  static const superseded = RenderResult.fromRawValue(4);

  /// The demand's timeout elapsed before driver work began.
  static const deadlineMissed = RenderResult.fromRawValue(5);
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
}

/// Optional fields for `mln_rendered_feature_query_options`.
///
/// See `mln_rendered_feature_query_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
final class RenderedFeatureQueryOptionField
    extends _Flags<RenderedFeatureQueryOptionField> {
  const RenderedFeatureQueryOptionField.fromRawValue(super.rawValue);
  static const ids = RenderedFeatureQueryOptionField.fromRawValue(1);
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
}

final class ResourceErrorReason extends _Enum {
  const ResourceErrorReason.fromRawValue(super.rawValue);
  static const none = ResourceErrorReason.fromRawValue(0);
  static const notFound = ResourceErrorReason.fromRawValue(1);
  static const server = ResourceErrorReason.fromRawValue(2);
  static const connection = ResourceErrorReason.fromRawValue(3);
  static const rateLimit = ResourceErrorReason.fromRawValue(4);
  static const other = ResourceErrorReason.fromRawValue(5);
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
}

final class ResourceLoadingMethod extends _Enum {
  const ResourceLoadingMethod.fromRawValue(super.rawValue);
  static const all = ResourceLoadingMethod.fromRawValue(0);
  static const cacheOnly = ResourceLoadingMethod.fromRawValue(1);
  static const networkOnly = ResourceLoadingMethod.fromRawValue(2);
}

final class ResourcePriority extends _Enum {
  const ResourcePriority.fromRawValue(super.rawValue);
  static const regular = ResourcePriority.fromRawValue(0);
  static const low = ResourcePriority.fromRawValue(1);
}

final class ResourceProviderDecision extends _Enum {
  const ResourceProviderDecision.fromRawValue(super.rawValue);
  static const passThrough = ResourceProviderDecision.fromRawValue(0);
  static const handle = ResourceProviderDecision.fromRawValue(1);
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
}

final class ResourceStoragePolicy extends _Enum {
  const ResourceStoragePolicy.fromRawValue(super.rawValue);
  static const permanent = ResourceStoragePolicy.fromRawValue(0);
  static const volatile = ResourceStoragePolicy.fromRawValue(1);
}

final class ResourceUsage extends _Enum {
  const ResourceUsage.fromRawValue(super.rawValue);
  static const online = ResourceUsage.fromRawValue(0);
  static const offline = ResourceUsage.fromRawValue(1);
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
  static const mapRenderUpdateAvailable = RuntimeEventMask.fromRawValue(512);
  static const mapRenderError = RuntimeEventMask.fromRawValue(1024);
  static const mapStillImageFinished = RuntimeEventMask.fromRawValue(2048);
  static const mapStillImageFailed = RuntimeEventMask.fromRawValue(4096);
  static const mapRenderFrameStarted = RuntimeEventMask.fromRawValue(8192);
  static const mapRenderFrameFinished = RuntimeEventMask.fromRawValue(16384);
  static const mapRenderMapStarted = RuntimeEventMask.fromRawValue(32768);
  static const mapRenderMapFinished = RuntimeEventMask.fromRawValue(65536);
  static const mapStyleImageMissing = RuntimeEventMask.fromRawValue(131072);
  static const mapTileAction = RuntimeEventMask.fromRawValue(262144);
  static const mapCameraTransitionFinished = RuntimeEventMask.fromRawValue(
    4194304,
  );
  static const offlineRegionStatusChanged = RuntimeEventMask.fromRawValue(
    524288,
  );
  static const offlineRegionResponseError = RuntimeEventMask.fromRawValue(
    1048576,
  );
  static const offlineRegionTileCountLimitExceeded =
      RuntimeEventMask.fromRawValue(2097152);

  /// Selects every map-originated event type this version defines.
  static const allMapEvents = RuntimeEventMask.fromRawValue(4718590);

  /// Selects every runtime-originated event type this version defines.
  static const allRuntimeEvents = RuntimeEventMask.fromRawValue(3670016);

  /// Selects every event type this version defines.
  static const all = RuntimeEventMask.fromRawValue(8388606);
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
  static const cameraTransitionFinished = RuntimeEventPayloadType.fromRawValue(
    9,
  );
}

/// Source kinds used by `mln_runtime_event.source_type`.
///
/// See `mln_runtime_event_source_type` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventSourceType extends _Enum {
  const RuntimeEventSourceType.fromRawValue(super.rawValue);
  static const runtime = RuntimeEventSourceType.fromRawValue(0);
  static const map = RuntimeEventSourceType.fromRawValue(1);
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
  static const mapRenderUpdateAvailable = RuntimeEventType.fromRawValue(9);
  static const mapRenderError = RuntimeEventType.fromRawValue(10);
  static const mapStillImageFinished = RuntimeEventType.fromRawValue(11);
  static const mapStillImageFailed = RuntimeEventType.fromRawValue(12);
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
  static const mapCameraTransitionFinished = RuntimeEventType.fromRawValue(22);
}

/// Optional fields for `mln_source_feature_query_options`.
///
/// See `mln_source_feature_query_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
final class SourceFeatureQueryOptionField
    extends _Flags<SourceFeatureQueryOptionField> {
  const SourceFeatureQueryOptionField.fromRawValue(super.rawValue);
  static const ids = SourceFeatureQueryOptionField.fromRawValue(1);
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

  /// The handle is thread-affine and the call was made from the wrong thread.
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

  /// A nonblocking acquisition or service call has no result yet.
  static const notReady = Status.fromRawValue(-9);

  /// A command or operation named an ID with no live object behind it.
  static const notFound = Status.fromRawValue(-10);
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
}

/// Layer visibility values used by the visibility setter and layer info.
///
/// See `mln_style_layer_visibility` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleLayerVisibility extends _Enum {
  const StyleLayerVisibility.fromRawValue(super.rawValue);
  static const visible = StyleLayerVisibility.fromRawValue(0);
  static const none = StyleLayerVisibility.fromRawValue(1);
}

/// DEM raster encoding values used by `mln_style_tile_source_options`.
///
/// See `mln_style_raster_dem_encoding` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleRasterDemEncoding extends _Enum {
  const StyleRasterDemEncoding.fromRawValue(super.rawValue);
  static const mapbox = StyleRasterDemEncoding.fromRawValue(0);
  static const terrarium = StyleRasterDemEncoding.fromRawValue(1);
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
}

/// Tile URL coordinate scheme values used by `mln_style_tile_source_options`.
///
/// See `mln_style_tile_scheme` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleTileScheme extends _Enum {
  const StyleTileScheme.fromRawValue(super.rawValue);
  static const xyz = StyleTileScheme.fromRawValue(0);
  static const tms = StyleTileScheme.fromRawValue(1);
}

/// Field mask values for `mln_style_tile_source_options`.
///
/// See `mln_style_tile_source_option_field` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleTileSourceOptionField
    extends _Flags<StyleTileSourceOptionField> {
  const StyleTileSourceOptionField.fromRawValue(super.rawValue);
  static const minZoom = StyleTileSourceOptionField.fromRawValue(1);
  static const maxZoom = StyleTileSourceOptionField.fromRawValue(2);
  static const attribution = StyleTileSourceOptionField.fromRawValue(4);
  static const scheme = StyleTileSourceOptionField.fromRawValue(8);
  static const bounds = StyleTileSourceOptionField.fromRawValue(16);
  static const tileSize = StyleTileSourceOptionField.fromRawValue(32);
  static const vectorEncoding = StyleTileSourceOptionField.fromRawValue(64);
  static const rasterEncoding = StyleTileSourceOptionField.fromRawValue(128);
  @override
  StyleTileSourceOptionField _of(int rawValue) =>
      StyleTileSourceOptionField.fromRawValue(rawValue);
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

/// Vector tile encoding values used by `mln_style_tile_source_options`.
///
/// See `mln_style_vector_tile_encoding` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleVectorTileEncoding extends _Enum {
  const StyleVectorTileEncoding.fromRawValue(super.rawValue);
  static const mvt = StyleVectorTileEncoding.fromRawValue(0);
  static const mlt = StyleVectorTileEncoding.fromRawValue(1);
}

/// Tile LOD algorithms used by `mln_map_tile_options`.
///
/// See `mln_tile_lod_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class TileLodMode extends _Enum {
  const TileLodMode.fromRawValue(super.rawValue);
  static const defaultValue = TileLodMode.fromRawValue(0);
  static const distance = TileLodMode.fromRawValue(1);
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
}

/// Viewport orientation modes used by `mln_map_viewport_options`.
///
/// See `mln_viewport_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class ViewportMode extends _Enum {
  const ViewportMode.fromRawValue(super.rawValue);
  static const defaultValue = ViewportMode.fromRawValue(0);
  static const flippedY = ViewportMode.fromRawValue(1);
}

/// WebGL context placement.
///
/// See `mln_webgl_context_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class WebglContextKind extends _Enum {
  const WebglContextKind.fromRawValue(super.rawValue);

  /// Use a host-created context on its current browser agent.
  static const existing = WebglContextKind.fromRawValue(0);

  /// Create a WebGL 2 context on a native worker whose pthread creation claims
  /// canvas_selector through Emscripten's transferred-canvases attribute.
  static const transferredCanvas = WebglContextKind.fromRawValue(1);
}

/// Metal frame acquired from a session-owned texture target.
///
/// See `mln_metal_owned_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class MetalOwnedTextureFrame extends _Value {
  const MetalOwnedTextureFrame({
    required this.generation,
    this.width = 0,
    this.height = 0,
    this.scaleFactor = 0,
    required this.frameId,
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
    texture,
    device,
    pixelFormat,
  ];
}

/// OpenGL frame acquired from a session-owned texture target.
///
/// See `mln_opengl_owned_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class OpenglOwnedTextureFrame extends _Value {
  const OpenglOwnedTextureFrame({
    required this.generation,
    this.width = 0,
    this.height = 0,
    this.scaleFactor = 0,
    required this.frameId,
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

  /// Borrowed OpenGL texture object name. Valid until frame release.
  final int texture;

  /// OpenGL texture target. GL_TEXTURE_2D is the expected target.
  final int target;

  /// OpenGL internal format, such as GL_RGBA8.
  final int internalFormat;

  /// OpenGL pixel format, such as GL_RGBA.
  final int format;

  /// OpenGL pixel type, such as GL_UNSIGNED_BYTE.
  final int type;

  @override
  List<Object?> get _members => [
    generation,
    width,
    height,
    scaleFactor,
    frameId,
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
  const GpuSync({
    this.kind = const GpuSyncKind.fromRawValue(0),
    required this.object,
    required this.value,
  });

  /// One `mln_gpu_sync_kind` value.
  final GpuSyncKind kind;

  /// Bit pattern of the backend object that kind names: the
  /// `id<MTLSharedEvent>` pointer, the VkSemaphore handle, the GLsync pointer,
  /// or the WebGPU token.
  final BigInt object;
  final BigInt value;

  @override
  List<Object?> get _members => [kind, object, value];
}

/// Immutable result record copied into an owned frame-result batch.
///
/// See `mln_render_frame_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
final class RenderFrameResult extends _Value {
  const RenderFrameResult({
    this.disposition = const RenderResult.fromRawValue(0),
    required this.token,
    required this.mapUpdateGeneration,
    required this.extentGeneration,
    required this.frameGeneration,
    this.needsRepaint = false,
  });

  /// One `mln_render_result` value.
  final RenderResult disposition;
  final BigInt token;
  final BigInt mapUpdateGeneration;
  final BigInt extentGeneration;

  /// Zero unless disposition is `MLN_RENDER_RESULT_RENDERED`.
  final BigInt frameGeneration;

  /// Whether the map asked for another frame while it rendered this one, as
  /// during an ongoing paint transition. Set only when disposition is
  /// `MLN_RENDER_RESULT_RENDERED`, and false for every other outcome. This is
  /// the same signal that `MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED` carries
  /// in its needs_repaint field, delivered with the frame result so a host can
  /// re-arm its frame loop without the runtime event round trip. A camera
  /// transition does not set it by itself: the map publishes a new update after
  /// each of the transition's frames instead, which a render-if-needed demand
  /// renders.
  final bool needsRepaint;

  @override
  List<Object?> get _members => [
    disposition,
    token,
    mapUpdateGeneration,
    extentGeneration,
    frameGeneration,
    needsRepaint,
  ];
}

/// Vulkan frame acquired from a session-owned texture target.
///
/// See `mln_vulkan_owned_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class VulkanOwnedTextureFrame extends _Value {
  const VulkanOwnedTextureFrame({
    required this.generation,
    this.width = 0,
    this.height = 0,
    this.scaleFactor = 0,
    required this.frameId,
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

  /// Borrowed VkImage bit pattern. Valid until frame release.
  final BigInt image;

  /// Borrowed VkImageView bit pattern. Valid until frame release.
  final BigInt imageView;

  /// Borrowed VkDevice. Valid until frame release.
  final NativePointer device;

  /// Backend-native VkFormat value.
  final int format;

  /// Backend-native VkImageLayout value; Vulkan frames are host-sampleable.
  final int layout;

  @override
  List<Object?> get _members => [
    generation,
    width,
    height,
    scaleFactor,
    frameId,
    image,
    imageView,
    device,
    format,
    layout,
  ];
}

/// WebGPU frame acquired from a session-owned texture target.
///
/// See `mln_webgpu_owned_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class WebgpuOwnedTextureFrame extends _Value {
  const WebgpuOwnedTextureFrame({
    required this.generation,
    this.width = 0,
    this.height = 0,
    this.scaleFactor = 0,
    required this.frameId,
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
  const UnitBezier(this.x1, this.y1, this.x2, this.y2);
  final double x1;
  final double y1;
  final double x2;
  final double y2;

  @override
  List<Object?> get _members => [x1, y1, x2, y2];
}

/// Optional animation controls for camera transitions.
///
/// See `mln_animation_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class AnimationOptions extends _Value {
  const AnimationOptions({
    this.durationMs,
    this.velocity,
    this.minZoom,
    this.easing,
    this.transitionId,
  });

  /// Duration in milliseconds. Must be finite and non-negative. Values that
  /// would overflow MapLibre Native's internal duration are invalid.
  final double? durationMs;

  /// Average fly velocity in screenfuls per second. Must be positive and
  /// defaults to 1.2 when omitted.
  final double? velocity;

  /// Peak zoom for flyTo transitions.
  final double? minZoom;
  final UnitBezier? easing;

  /// Caller-chosen identity for the transition this options struct starts.
  final BigInt? transitionId;

  @override
  List<Object?> get _members => [
    durationMs,
    velocity,
    minZoom,
    easing,
    transitionId,
  ];
}

/// Geographic coordinate in degrees used by map and projection APIs.
///
/// See `mln_lat_lng` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class LatLng extends _Value {
  const LatLng(this.latitude, this.longitude);

  /// Latitude in degrees. Input latitude must be finite and within \[-90, 90\].
  final double latitude;

  /// Longitude in degrees. Input longitude must be finite.
  final double longitude;

  @override
  List<Object?> get _members => [latitude, longitude];
}

/// Geographic bounds in degrees.
///
/// See `mln_lat_lng_bounds` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class LatLngBounds extends _Value {
  const LatLngBounds({
    this.southwest = const LatLng(0, 0),
    this.northeast = const LatLng(0, 0),
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
  const BoundOptions({
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

/// Screen-space point in logical map pixels.
///
/// See `mln_screen_point` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class ScreenPoint extends _Value {
  const ScreenPoint(this.x, this.y);
  final double x;
  final double y;

  @override
  List<Object?> get _members => [x, y];
}

/// One relative camera operation.
///
/// See `mln_camera_delta` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraDelta extends _Value {
  const CameraDelta({
    this.kind = const CameraDeltaKind.fromRawValue(0),
    this.offset = const ScreenPoint(0, 0),
    this.amount = 0,
    this.anchor,
    this.animation = const AnimationOptions(),
  });
  final CameraDeltaKind kind;
  final ScreenPoint offset;
  final double amount;
  final ScreenPoint? anchor;
  final AnimationOptions animation;

  @override
  List<Object?> get _members => [kind, offset, amount, anchor, animation];
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

/// Optional fitting controls for camera-for-viewport queries.
///
/// See `mln_camera_fit_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraFitOptions extends _Value {
  const CameraFitOptions({this.padding, this.bearing, this.pitch});
  final EdgeInsets? padding;
  final double? bearing;
  final double? pitch;

  @override
  List<Object?> get _members => [padding, bearing, pitch];
}

/// Camera fields used by snapshots and camera updates.
///
/// See `mln_camera_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraOptions extends _Value {
  const CameraOptions({
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

/// One atomic absolute camera update.
///
/// See `mln_camera_update` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraUpdate extends _Value {
  const CameraUpdate({
    this.mode = const CameraUpdateMode.fromRawValue(0),
    this.camera = const CameraOptions(),
    this.animation = const AnimationOptions(),
    this.gesturePhase = const GesturePhase.fromRawValue(0),
  });
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

/// Callback invoked for custom geometry source tile requests and cancels.
///
/// See `mln_custom_geometry_source_tile_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
typedef CustomGeometrySourceTileCallback = void Function(CanonicalTileId);

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
  final CustomGeometrySourceTileCallback? fetchTile;
  final CustomGeometrySourceTileCallback? cancelTile;
  final double? minZoom;
  final double? maxZoom;
  final double? tolerance;
  final int? tileSize;
  final int? buffer;
  final bool? clip;
  final bool? wrap;
}

/// Callback invoked for custom MVT vector source tile requests and cancels.
///
/// See `mln_custom_mvt_vector_source_tile_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
typedef CustomMvtVectorSourceTileCallback = void Function(CanonicalTileId);

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
  final CustomMvtVectorSourceTileCallback? fetchTile;
  final CustomMvtVectorSourceTileCallback? cancelTile;
  final double? minZoom;
  final double? maxZoom;
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

  /// Region status. This member keeps its own size field because the same
  /// struct is also returned by `mln_runtime_offline_region_get_status()`.
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

/// Payload for `MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED`.
///
/// See `mln_runtime_event_camera_transition_finished` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventCameraTransitionFinished extends _Value {
  const RuntimeEventCameraTransitionFinished({required this.transitionId});

  /// The transition_id the caller set on the `mln_animation_options` that
  /// started this transition.
  final BigInt transitionId;

  @override
  List<Object?> get _members => [transitionId];
}

/// One drained runtime event.
///
/// See `mln_runtime_event` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEvent {
  const RuntimeEvent({
    required this.type,
    this.sourceType = const RuntimeEventSourceType.fromRawValue(0),
    required this.source,
    this.code = 0,
    required this.messageOffset,
    this.messageSize = 0,
    this.message = '',
    required this.payload,
  });
  final RuntimeEventType type;
  final RuntimeEventSourceType sourceType;
  final BigInt source;
  final int code;
  final BigInt messageOffset;
  final int messageSize;
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

final class RuntimeEventPayloadCameraTransitionFinished
    extends RuntimeEventPayload {
  const RuntimeEventPayloadCameraTransitionFinished(this.value) : super._();
  final RuntimeEventCameraTransitionFinished value;
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

/// A borrowed view of one owned runtime-event batch.
///
/// See `mln_runtime_event_batch_view` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventBatchView extends _Value {
  RuntimeEventBatchView({required List<RuntimeEvent> events})
    : events = List.unmodifiable(events);

  /// Borrowed array of event_count events in queue order.
  final List<RuntimeEvent> events;

  @override
  List<Object?> get _members => [events];
}

/// One nonblocking request for a frame.
///
/// See `mln_frame_demand` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
final class FrameDemand extends _Value {
  const FrameDemand({
    this.flags = FrameDemandFlag.ifNeeded,
    required this.token,
    required this.coalescingBoundary,
    required this.timeoutNs,
  });

  /// A bitwise OR of `mln_frame_demand_flag` values. Defaults to
  /// `MLN_FRAME_DEMAND_IF_NEEDED`.
  final FrameDemandFlag flags;

  /// Host identity returned with the terminal frame result.
  final BigInt token;

  /// Demands coalesce only when this value and their flags match.
  final BigInt coalescingBoundary;

  /// Positive time allowed before driver work begins, in nanoseconds; zero has
  /// no limit.
  final BigInt timeoutNs;

  @override
  List<Object?> get _members => [flags, token, coalescingBoundary, timeoutNs];
}

/// Three-component vector used by free camera options.
///
/// See `mln_vec3` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class Vec3 extends _Value {
  const Vec3(this.x, this.y, this.z);
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
  const Quaternion(this.x, this.y, this.z, this.w);
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
  const FreeCameraOptions({this.position, this.orientation});
  final Vec3? position;
  final Quaternion? orientation;

  @override
  List<Object?> get _members => [position, orientation];
}

/// Options for GeoJSON sources.
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
    this.tileSize,
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

  /// Maximum tiling zoom. Defaults to 18.
  final double? maxZoom;

  /// Douglas-Peucker simplification tolerance. Defaults to 0.375.
  final double? tolerance;

  /// Highest zoom that clusters points. Defaults to 17.
  final double? clusterMaxZoom;

  /// Cluster aggregation expressions keyed by property name, as a JSON object
  /// whose members follow the MapLibre Style Spec clusterProperties form. The
  /// UTF-8 bytes are borrowed for the call.
  final Uint8List? clusterProperties;

  /// Tile extent in pixels. Defaults to 512.
  final int? tileSize;

  /// Tile buffer in pixels. Defaults to 128.
  final int? buffer;

  /// Cluster radius in pixels. Defaults to 50.
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
    tileSize,
    buffer,
    clusterRadius,
    clusterMinPoints,
    lineMetrics,
    cluster,
    synchronousTiling,
  ];
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

/// Caller-owned premultiplied RGBA8 image pixels.
///
/// See `mln_premultiplied_rgba8_image` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class PremultipliedRgba8Image extends _Value {
  PremultipliedRgba8Image({
    this.width = 0,
    this.height = 0,
    this.stride = 0,
    Uint8List? pixels,
  }) : pixels = Uint8List.fromList(
         pixels ?? const <int>[],
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

/// Options for vector and raster tile sources.
///
/// See `mln_style_tile_source_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleTileSourceOptions extends _Value {
  const StyleTileSourceOptions({
    this.minZoom,
    this.maxZoom,
    this.attribution,
    this.scheme,
    this.bounds,
    this.tileSize,
    this.vectorEncoding,
    this.rasterEncoding,
  });
  final double? minZoom;
  final double? maxZoom;
  final String? attribution;

  /// One of `mln_style_tile_scheme`. Defaults to `MLN_STYLE_TILE_SCHEME_XYZ`.
  final StyleTileScheme? scheme;
  final LatLngBounds? bounds;

  /// Raster tile size in pixels. Defaults to 512.
  final int? tileSize;

  /// One of `mln_style_vector_tile_encoding`. Defaults to MVT.
  final StyleVectorTileEncoding? vectorEncoding;

  /// One of `mln_style_raster_dem_encoding`. Defaults to Mapbox.
  final StyleRasterDemEncoding? rasterEncoding;

  @override
  List<Object?> get _members => [
    minZoom,
    maxZoom,
    attribution,
    scheme,
    bounds,
    tileSize,
    vectorEncoding,
    rasterEncoding,
  ];
}

/// Camera result borrowed for an ordered camera-query completion.
///
/// See `mln_camera_query_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraQueryResult extends _Value {
  const CameraQueryResult({
    required this.generation,
    this.camera = const CameraOptions(),
  });
  final BigInt generation;
  final CameraOptions camera;

  @override
  List<Object?> get _members => [generation, camera];
}

/// One stretchable interval along an image axis, in image pixels.
///
/// See `mln_image_stretch` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class ImageStretch extends _Value {
  const ImageStretch(this.from, this.to);
  final double from;
  final double to;

  @override
  List<Object?> get _members => [from, to];
}

/// Borrowed image-stretch arrays available during a completion callback.
///
/// See `mln_style_image_stretches_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleImageStretchesResult extends _Value {
  StyleImageStretchesResult({
    required List<ImageStretch> stretchX,
    required List<ImageStretch> stretchY,
  }) : stretchX = List.unmodifiable(stretchX),
       stretchY = List.unmodifiable(stretchY);
  final List<ImageStretch> stretchX;
  final List<ImageStretch> stretchY;

  @override
  List<Object?> get _members => [stretchX, stretchY];
}

/// Logical map extent in UI pixels and device-pixel scale.
///
/// See `mln_logical_extent` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class LogicalExtent extends _Value {
  const LogicalExtent({
    this.width = 256,
    this.height = 256,
    this.scaleFactor = 1.0,
  });

  /// Width in UI pixels. Defaults to 256.
  final int width;

  /// Height in UI pixels. Defaults to 256.
  final int height;

  /// Device pixels per UI pixel. Defaults to 1.0. The renderer takes it at map
  /// creation, so `mln_map_resize()` accepts only the value the map was created
  /// with.
  final double scaleFactor;

  @override
  List<Object?> get _members => [width, height, scaleFactor];
}

/// Options used when creating a map.
///
/// See `mln_map_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class MapOptions extends _Value {
  const MapOptions({
    this.initialExtent = const LogicalExtent(),
    this.mapMode = const MapMode.fromRawValue(0),
    this.fastPforEnabled = false,
    this.eventMask = RuntimeEventMask.all,
  });

  /// Initial logical extent. Width and height must be positive. The scale
  /// factor must be positive and finite, and fixes the map's scale factor for
  /// its lifetime.
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

  /// Optional feature ID string. Required by set/get and optional for remove.
  final String? featureId;

  /// Optional state key. Used only by remove and requires feature_id.
  final String? stateKey;

  @override
  List<Object?> get _members => [sourceId, sourceLayerId, featureId, stateKey];
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

/// Fixed metadata for one runtime style image.
///
/// See `mln_style_image_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleImageInfo extends _Value {
  const StyleImageInfo({
    this.width = 0,
    this.height = 0,
    this.stride = 0,
    this.byteLength = 0,
    this.stretchXCount = 0,
    this.stretchYCount = 0,
    this.content,
    this.textFitWidth,
    this.textFitHeight,
    this.pixelRatio = 1.0,
    this.sdf = false,
  });
  final int width;
  final int height;

  /// Native copied images are exposed as tightly packed premultiplied RGBA8.
  final int stride;
  final int byteLength;

  /// Interval counts for the stretchable axes.
  final int stretchXCount;
  final int stretchYCount;

  /// Content box, meaningful only when has_content is true.
  final ImageContent? content;

  /// One of `mln_style_image_text_fit`, meaningful only when its flag is true.
  final StyleImageTextFit? textFitWidth;

  /// One of `mln_style_image_text_fit`, meaningful only when its flag is true.
  final StyleImageTextFit? textFitHeight;

  /// Sprite pixel ratio. Defaults to 1.0.
  final double pixelRatio;
  final bool sdf;

  @override
  List<Object?> get _members => [
    width,
    height,
    stride,
    byteLength,
    stretchXCount,
    stretchYCount,
    content,
    textFitWidth,
    textFitHeight,
    pixelRatio,
    sdf,
  ];
}

/// Complete style image borrowed for a completion callback.
///
/// See `mln_style_image_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleImageResult extends _Value {
  StyleImageResult({
    this.info = const StyleImageInfo(),
    Uint8List? pixels,
    required List<ImageStretch> stretchX,
    required List<ImageStretch> stretchY,
  }) : pixels = Uint8List.fromList(
         pixels ?? const <int>[],
       ).asUnmodifiableView(),
       stretchX = List.unmodifiable(stretchX),
       stretchY = List.unmodifiable(stretchY);
  final StyleImageInfo info;
  final Uint8List pixels;
  final List<ImageStretch> stretchX;
  final List<ImageStretch> stretchY;

  @override
  List<Object?> get _members => [info, pixels, stretchX, stretchY];
}

/// Fixed layer metadata included in `mln_style_layer_result`.
///
/// See `mln_style_layer_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleLayerInfo extends _Value {
  const StyleLayerInfo({
    required this.type,
    this.minZoom = 0,
    this.maxZoom = 0,
    this.visibility = const StyleLayerVisibility.fromRawValue(0),
  });

  /// View of a static style-spec layer type string. It stays valid for the life
  /// of the process.
  final String type;

  /// Lowest zoom at which the layer draws; -INFINITY with no lower bound.
  final double minZoom;

  /// Highest zoom at which the layer draws; INFINITY with no upper bound.
  final double maxZoom;

  /// One of `mln_style_layer_visibility`.
  final StyleLayerVisibility visibility;

  @override
  List<Object?> get _members => [type, minZoom, maxZoom, visibility];
}

/// Complete layer metadata borrowed for a completion callback.
///
/// See `mln_style_layer_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleLayerResult extends _Value {
  const StyleLayerResult({required this.info, this.sourceId, this.sourceLayer});
  final StyleLayerInfo info;

  /// Source ID. Empty for a layer type that takes no source.
  final String? sourceId;

  /// Source-layer ID. Empty when the layer sets none.
  final String? sourceLayer;

  @override
  List<Object?> get _members => [info, sourceId, sourceLayer];
}

/// Inline tile metadata selected as one value by the source-info field mask.
///
/// See `mln_style_source_tile_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleSourceTileInfo extends _Value {
  const StyleSourceTileInfo({
    this.tileCount = 0,
    this.minZoom = 0,
    this.maxZoom = 0,
    this.scheme = const StyleTileScheme.fromRawValue(0),
  });
  final int tileCount;
  final double minZoom;
  final double maxZoom;
  final StyleTileScheme scheme;

  @override
  List<Object?> get _members => [tileCount, minZoom, maxZoom, scheme];
}

/// Fixed source metadata included in `mln_style_source_result`.
///
/// See `mln_style_source_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleSourceInfo extends _Value {
  const StyleSourceInfo({
    this.type = const StyleSourceType.fromRawValue(0),
    this.idSize = 0,
    this.isVolatile = false,
    this.attributionSize,
    this.urlSize,
    this.tilejson,
    this.bounds,
    this.tileSize,
    this.vectorEncoding,
    this.rasterEncoding,
  });

  /// One of `mln_style_source_type`.
  final StyleSourceType type;

  /// Source ID byte length, excluding any null terminator.
  final int idSize;

  /// Whether the source is marked volatile.
  final bool isVolatile;

  /// Attribution byte length, excluding any null terminator.
  final int? attributionSize;

  /// URL byte length, meaningful when fields contains URL.
  final int? urlSize;
  final StyleSourceTileInfo? tilejson;

  /// Geographic bounds, meaningful when fields contains BOUNDS.
  final LatLngBounds? bounds;

  /// Tile size in pixels, meaningful when fields contains TILE_SIZE.
  final int? tileSize;

  /// Vector encoding, meaningful when fields contains VECTOR_ENCODING.
  final StyleVectorTileEncoding? vectorEncoding;

  /// DEM encoding, meaningful when fields contains RASTER_ENCODING.
  final StyleRasterDemEncoding? rasterEncoding;

  @override
  List<Object?> get _members => [
    type,
    idSize,
    isVolatile,
    attributionSize,
    urlSize,
    tilejson,
    bounds,
    tileSize,
    vectorEncoding,
    rasterEncoding,
  ];
}

/// Complete source metadata borrowed for a completion callback.
///
/// See `mln_style_source_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleSourceResult extends _Value {
  StyleSourceResult({
    this.info = const StyleSourceInfo(),
    this.attribution,
    this.url,
    List<String>? tileUrls,
  }) : tileUrls = tileUrls == null ? null : List.unmodifiable(tileUrls);
  final StyleSourceInfo info;
  final String? attribution;
  final String? url;
  final List<String>? tileUrls;

  @override
  List<Object?> get _members => [info, attribution, url, tileUrls];
}

/// Borrowed inline TileJSON tile URLs available during a completion callback.
///
/// See `mln_style_source_tile_urls_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleSourceTileUrlsResult extends _Value {
  StyleSourceTileUrlsResult({required List<String> tileUrls})
    : tileUrls = List.unmodifiable(tileUrls);
  final List<String> tileUrls;

  @override
  List<Object?> get _members => [tileUrls];
}

/// Global style transition options.
///
/// See `mln_style_transition_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleTransitionOptions extends _Value {
  const StyleTransitionOptions({
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

/// One style layer borrowed for a list completion callback.
///
/// See `mln_style_layer_entry` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleLayerEntry extends _Value {
  const StyleLayerEntry({
    required this.id,
    required this.type,
    this.sourceId,
    this.sourceLayer,
  });
  final String id;
  final String type;
  final String? sourceId;
  final String? sourceLayer;

  @override
  List<Object?> get _members => [id, type, sourceId, sourceLayer];
}

/// MapLibre axonometric rendering options used for snapshots and commands.
///
/// See `mln_projection_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class ProjectionMode extends _Value {
  const ProjectionMode({this.axonometric, this.xSkew, this.ySkew});

  /// Enables a non-perspective axonometric render transform.
  final bool? axonometric;

  /// Native x-skew factor used by the axonometric transform.
  final double? xSkew;

  /// Native y-skew factor used by the axonometric transform.
  final double? ySkew;

  @override
  List<Object?> get _members => [axonometric, xSkew, ySkew];
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

/// Tile prefetch and LOD tuning controls.
///
/// See `mln_map_tile_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class MapTileOptions extends _Value {
  const MapTileOptions({
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

/// Live map viewport and render-transform controls.
///
/// See `mln_map_viewport_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class MapViewportOptions extends _Value {
  const MapViewportOptions({
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

/// Immutable map state copied from the latest published generation.
///
/// See `mln_map_snapshot` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class MapSnapshot extends _Value {
  const MapSnapshot({
    this.debugOptions = const MapDebugOption.fromRawValue(0),
    required this.generation,
    this.camera = const CameraOptions(),
    this.logicalExtent = const LogicalExtent(),
    this.projectionMode = const ProjectionMode(),
    this.viewport = const MapViewportOptions(),
    this.fullyLoaded = false,
    this.renderingStatsViewEnabled = false,
    this.repaintDemand = false,
    this.gestureInProgress = false,
    this.eventMask = const RuntimeEventMask.fromRawValue(0),
    required this.latestRenderUpdateGeneration,
    this.tile = const MapTileOptions(),
    this.bounds = const BoundOptions(),
    this.freeCamera = const FreeCameraOptions(),
  });

  /// Debug overlay mask of `mln_map_debug_option` values.
  final MapDebugOption debugOptions;
  final BigInt generation;
  final CameraOptions camera;
  final LogicalExtent logicalExtent;
  final ProjectionMode projectionMode;
  final MapViewportOptions viewport;

  /// True once every requested style and tile resource finished loading.
  final bool fullyLoaded;
  final bool renderingStatsViewEnabled;
  final bool repaintDemand;

  /// True while the map is inside a gesture.
  final bool gestureInProgress;
  final RuntimeEventMask eventMask;
  final BigInt latestRenderUpdateGeneration;
  final MapTileOptions tile;
  final BoundOptions bounds;
  final FreeCameraOptions freeCamera;

  @override
  List<Object?> get _members => [
    debugOptions,
    generation,
    camera,
    logicalExtent,
    projectionMode,
    viewport,
    fullyLoaded,
    renderingStatsViewEnabled,
    repaintDemand,
    gestureInProgress,
    eventMask,
    latestRenderUpdateGeneration,
    tile,
    bounds,
    freeCamera,
  ];
}

/// Logical render target extent in UI pixels.
///
/// See `mln_render_target_extent` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class RenderTargetExtent extends _Value {
  const RenderTargetExtent({
    this.width = 256,
    this.height = 256,
    this.scaleFactor = 1.0,
  });

  /// Logical map width in UI pixels. Defaults to 256.
  final int width;

  /// Logical map height in UI pixels. Defaults to 256.
  final int height;

  /// UI-to-device pixel scale. Must be positive and finite. Defaults to 1.0.
  final double scaleFactor;

  @override
  List<Object?> get _members => [width, height, scaleFactor];
}

/// Metal attachment options for a borrowed texture target.
///
/// See `mln_metal_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class MetalBorrowedTextureDescriptor extends _Value {
  const MetalBorrowedTextureDescriptor({
    this.extent = const RenderTargetExtent(),
    this.physicalWidth = 256,
    this.physicalHeight = 256,
    this.texture = NativePointer.nullPointer,
  });

  /// Logical texture extent. The map viewport uses width and height and the
  /// renderer uses scale_factor; the physical size is stated separately below.
  final RenderTargetExtent extent;

  /// Physical texture width in device pixels. Must be positive. Defaults to
  /// 256.
  final int physicalWidth;

  /// Physical texture height in device pixels. Must be positive. Defaults to
  /// 256.
  final int physicalHeight;

  /// Borrowed `id<MTLTexture>` / `MTL::Texture*`. Required.
  final NativePointer texture;

  @override
  List<Object?> get _members => [
    extent,
    physicalWidth,
    physicalHeight,
    texture,
  ];
}

/// Schedules service by the receiver that owns a queue or driver.
///
/// See `mln_wake_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wake_8h.html).
typedef WakeCallback = void Function();

/// Receiver wake callback copied by a successful owning call.
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
  const RenderSessionAttachOptions({
    this.driver = RenderDriverKind.callerGraphicsThread,
    this.requestedTextureRingDepth = 1,
    this.frameWake = const Wake(),
    this.driverWorkWake = const Wake(),
  });

  /// One `mln_render_driver_kind` value. Defaults to
  /// `MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD`.
  final RenderDriverKind driver;

  /// Requested host-acquirable owned-texture slot count. Private targets grant
  /// one slot regardless of this value. Ignored by other targets. Defaults to
  /// 1.
  final int requestedTextureRingDepth;

  /// Wakes the receiver when the frame-result queue becomes nonempty.
  final Wake frameWake;

  /// Wakes the graphics receiver when caller-driver work is available.
  final Wake driverWorkWake;

  @override
  List<Object?> get _members => [
    driver,
    requestedTextureRingDepth,
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
  const MetalOwnedTextureDescriptor({
    this.extent = const RenderTargetExtent(),
    this.context = const MetalContextDescriptor(),
  });

  /// Logical texture extent.
  final RenderTargetExtent extent;

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
  const MetalSurfaceDescriptor({
    this.extent = const RenderTargetExtent(),
    this.context = const MetalContextDescriptor(),
    this.layer = NativePointer.nullPointer,
  });

  /// Logical surface extent.
  final RenderTargetExtent extent;

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

/// OpenGL attachment options for a borrowed texture target.
///
/// See `mln_opengl_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class OpenglBorrowedTextureDescriptor extends _Value {
  const OpenglBorrowedTextureDescriptor({
    this.extent = const RenderTargetExtent(),
    this.physicalWidth = 256,
    this.physicalHeight = 256,
    required this.context,
    this.texture = 0,
    this.target = 0,
  });

  /// Logical texture extent. The map viewport uses width and height and the
  /// renderer uses scale_factor; the physical size is stated separately below.
  final RenderTargetExtent extent;

  /// Physical texture width in device pixels. Must be positive. Defaults to
  /// 256.
  final int physicalWidth;

  /// Physical texture height in device pixels. Must be positive. Defaults to
  /// 256.
  final int physicalHeight;

  /// Borrowed OpenGL context provider data. The texture must belong to this
  /// context or a context in the same share group.
  final OpenglContextDescriptor context;

  /// Borrowed OpenGL texture object name. Required.
  final int texture;

  /// OpenGL texture target. GL_TEXTURE_2D is the expected target.
  final int target;

  @override
  List<Object?> get _members => [
    extent,
    physicalWidth,
    physicalHeight,
    context,
    texture,
    target,
  ];
}

/// OpenGL attachment options for an owned texture target.
///
/// See `mln_opengl_owned_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class OpenglOwnedTextureDescriptor extends _Value {
  const OpenglOwnedTextureDescriptor({
    this.extent = const RenderTargetExtent(),
    required this.context,
  });

  /// Logical texture extent.
  final RenderTargetExtent extent;

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
  const OpenglSurfaceDescriptor({
    this.extent = const RenderTargetExtent(),
    required this.context,
    this.surface = NativePointer.nullPointer,
  });

  /// Logical surface extent.
  final RenderTargetExtent extent;

  /// Borrowed OpenGL context provider data.
  final OpenglContextDescriptor context;

  /// Borrowed platform surface handle: an HDC for WGL and an EGLSurface for
  /// EGL, both required. Null for WebGL, whose context carries its canvas
  /// binding.
  final NativePointer surface;

  @override
  List<Object?> get _members => [extent, context, surface];
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

  /// Granted owned-texture slot count, or zero for a target without a ring.
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
    required this.driver,
    this.latestResult = const RenderResult.fromRawValue(0),
    this.extent = const RenderTargetExtent(),
    required this.generation,
    required this.mapUpdateGeneration,
    required this.renderedUpdateGeneration,
    required this.extentGeneration,
    required this.frameGeneration,
    required this.latestDemandToken,
    this.pendingDemandCount = 0,
    this.acquiredFrameCount = 0,
    this.targetReady = false,
    this.pendingChanges = false,
  });

  /// One `mln_render_session_state` value.
  final RenderSessionState state;

  /// One `mln_render_driver_kind` value.
  final RenderDriverKind driver;

  /// Most recent terminal `mln_render_result` value.
  final RenderResult latestResult;
  final RenderTargetExtent extent;
  final BigInt generation;
  final BigInt mapUpdateGeneration;
  final BigInt renderedUpdateGeneration;
  final BigInt extentGeneration;
  final BigInt frameGeneration;
  final BigInt latestDemandToken;
  final int pendingDemandCount;
  final int acquiredFrameCount;
  final bool targetReady;
  final bool pendingChanges;

  @override
  List<Object?> get _members => [
    state,
    driver,
    latestResult,
    extent,
    generation,
    mapUpdateGeneration,
    renderedUpdateGeneration,
    extentGeneration,
    frameGeneration,
    latestDemandToken,
    pendingDemandCount,
    acquiredFrameCount,
    targetReady,
    pendingChanges,
  ];
}

/// Screen-space box in logical map pixels.
///
/// See `mln_screen_box` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
final class ScreenBox extends _Value {
  const ScreenBox({
    this.min = const ScreenPoint(0, 0),
    this.max = const ScreenPoint(0, 0),
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

  /// Optional UTF-8 MapLibre style-spec filter JSON. Null means no filter.
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

  /// Optional UTF-8 MapLibre style-spec filter JSON. Null means no filter.
  final Uint8List? filter;

  @override
  List<Object?> get _members => [sourceLayerIds, filter];
}

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

/// Options used when creating a runtime.
///
/// See `mln_runtime_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeOptions extends _Value {
  const RuntimeOptions({
    this.flags = 0,
    this.assetPath,
    this.cachePath,
    this.eventMask = RuntimeEventMask.all,
    this.eventWake = const Wake(),
  });

  /// No flags are currently defined. Must be zero.
  final int flags;

  /// Directory root for asset:// URLs. Copied during runtime creation. Null or
  /// empty selects `/android_asset` on Android and `.` elsewhere.
  final String? assetPath;

  /// Cache database path. Copied during runtime creation.
  final String? cachePath;

  /// Runtime-scoped event types this runtime queues, as a bitwise OR of
  /// `mln_runtime_event_mask` values.
  final RuntimeEventMask eventMask;

  /// Wakes the receiver when the runtime event queue becomes nonempty.
  final Wake eventWake;

  @override
  List<Object?> get _members => [
    flags,
    assetPath,
    cachePath,
    eventMask,
    eventWake,
  ];
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

final class AdapterUrlMatchFlags extends _Flags<AdapterUrlMatchFlags> {
  const AdapterUrlMatchFlags.fromRawValue(super.rawValue);
  static const flagsNone = AdapterUrlMatchFlags.fromRawValue(0);
  static const glob = AdapterUrlMatchFlags.fromRawValue(1);
  @override
  AdapterUrlMatchFlags _of(int rawValue) =>
      AdapterUrlMatchFlags.fromRawValue(rawValue);
}

final class AdapterHttpHeader extends _Value {
  const AdapterHttpHeader({this.name, this.value});
  final String? name;
  final String? value;

  @override
  List<Object?> get _members => [name, value];
}

final class AdapterHttpHeaderTransformRule extends _Value {
  AdapterHttpHeaderTransformRule({
    this.kind = 0,
    this.flags = const AdapterUrlMatchFlags.fromRawValue(0),
    this.url,
    required List<AdapterHttpHeader> headers,
  }) : headers = List.unmodifiable(headers);
  final int kind;
  final AdapterUrlMatchFlags flags;
  final String? url;
  final List<AdapterHttpHeader> headers;

  @override
  List<Object?> get _members => [kind, flags, url, headers];
}

final class AdapterHttpHeaderTransformRules extends _Value {
  AdapterHttpHeaderTransformRules({
    required List<AdapterHttpHeaderTransformRule> rules,
  }) : rules = List.unmodifiable(rules);
  final List<AdapterHttpHeaderTransformRule> rules;

  @override
  List<Object?> get _members => [rules];
}

sealed class HttpHeaderTransform {
  const HttpHeaderTransform._();
  const factory HttpHeaderTransform.empty() = HttpHeaderTransformEmpty;
  const factory HttpHeaderTransform.httpHeaderTransformRules(
    AdapterHttpHeaderTransformRules value,
  ) = HttpHeaderTransformHttpHeaderTransformRules;
}

final class HttpHeaderTransformEmpty extends HttpHeaderTransform {
  const HttpHeaderTransformEmpty() : super._();
}

final class HttpHeaderTransformHttpHeaderTransformRules
    extends HttpHeaderTransform {
  const HttpHeaderTransformHttpHeaderTransformRules(this.value) : super._();
  final AdapterHttpHeaderTransformRules value;
}

final class AdapterResourceProviderRule extends _Value {
  const AdapterResourceProviderRule({
    this.kind = 0,
    this.flags = const AdapterUrlMatchFlags.fromRawValue(0),
    this.requestedUrl,
    required this.response,
  });
  final int kind;
  final AdapterUrlMatchFlags flags;
  final String? requestedUrl;
  final ResourceResponse response;

  @override
  List<Object?> get _members => [kind, flags, requestedUrl, response];
}

final class AdapterResourceProviderRules extends _Value {
  AdapterResourceProviderRules({
    required List<AdapterResourceProviderRule> rules,
  }) : rules = List.unmodifiable(rules);
  final List<AdapterResourceProviderRule> rules;

  @override
  List<Object?> get _members => [rules];
}

final class AdapterResourceRouteFlags
    extends _Flags<AdapterResourceRouteFlags> {
  const AdapterResourceRouteFlags.fromRawValue(super.rawValue);
  static const flagsNone = AdapterResourceRouteFlags.fromRawValue(0);
  static const matchGlob = AdapterResourceRouteFlags.fromRawValue(1);
  static const useRequestedUrl = AdapterResourceRouteFlags.fromRawValue(2);
  @override
  AdapterResourceRouteFlags _of(int rawValue) =>
      AdapterResourceRouteFlags.fromRawValue(rawValue);
}

final class AdapterResourceRoute extends _Value {
  const AdapterResourceRoute({
    this.kind = 0,
    this.flags = const AdapterResourceRouteFlags.fromRawValue(0),
    this.url,
  });
  final int kind;
  final AdapterResourceRouteFlags flags;
  final String? url;

  @override
  List<Object?> get _members => [kind, flags, url];
}

final class ResourceRequest extends _Value {
  ResourceRequest({
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
  final ({BigInt rangeStart, BigInt rangeEnd})? range;
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
    void Function(ResourceRequest, ResourceRequestHandle);

final class AdapterRoutedResourceProvider extends _Value {
  AdapterRoutedResourceProvider({
    required List<AdapterResourceRoute> routes,
    required this.callback,
  }) : routes = List.unmodifiable(routes);
  final List<AdapterResourceRoute> routes;
  final ResourceProviderCallback callback;

  @override
  List<Object?> get _members => [routes, callback];
}

sealed class ResourceProvider {
  const ResourceProvider._();
  const factory ResourceProvider.empty() = ResourceProviderEmpty;
  const factory ResourceProvider.resourceProviderRules(
    AdapterResourceProviderRules value,
  ) = ResourceProviderResourceProviderRules;
  const factory ResourceProvider.routedResourceProvider(
    AdapterRoutedResourceProvider value,
  ) = ResourceProviderRoutedResourceProvider;
}

final class ResourceProviderEmpty extends ResourceProvider {
  const ResourceProviderEmpty() : super._();
}

final class ResourceProviderResourceProviderRules extends ResourceProvider {
  const ResourceProviderResourceProviderRules(this.value) : super._();
  final AdapterResourceProviderRules value;
}

final class ResourceProviderRoutedResourceProvider extends ResourceProvider {
  const ResourceProviderRoutedResourceProvider(this.value) : super._();
  final AdapterRoutedResourceProvider value;
}

final class AdapterResourceRewriteRule extends _Value {
  const AdapterResourceRewriteRule({
    this.kind = 0,
    this.flags = const AdapterUrlMatchFlags.fromRawValue(0),
    this.url,
    this.replacementUrl,
  });
  final int kind;
  final AdapterUrlMatchFlags flags;
  final String? url;
  final String? replacementUrl;

  @override
  List<Object?> get _members => [kind, flags, url, replacementUrl];
}

final class AdapterResourceRewriteRules extends _Value {
  AdapterResourceRewriteRules({required List<AdapterResourceRewriteRule> rules})
    : rules = List.unmodifiable(rules);
  final List<AdapterResourceRewriteRule> rules;

  @override
  List<Object?> get _members => [rules];
}

sealed class ResourceTransform {
  const ResourceTransform._();
  const factory ResourceTransform.empty() = ResourceTransformEmpty;
  const factory ResourceTransform.resourceRewriteRules(
    AdapterResourceRewriteRules value,
  ) = ResourceTransformResourceRewriteRules;
}

final class ResourceTransformEmpty extends ResourceTransform {
  const ResourceTransformEmpty() : super._();
}

final class ResourceTransformResourceRewriteRules extends ResourceTransform {
  const ResourceTransformResourceRewriteRules(this.value) : super._();
  final AdapterResourceRewriteRules value;
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

  /// PFN_vkGetInstanceProcAddr for the loader that created the Vulkan handles.
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

/// Vulkan attachment options for a borrowed texture target.
///
/// See `mln_vulkan_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class VulkanBorrowedTextureDescriptor extends _Value {
  const VulkanBorrowedTextureDescriptor({
    this.extent = const RenderTargetExtent(),
    this.physicalWidth = 256,
    this.physicalHeight = 256,
    this.context = const VulkanContextDescriptor(),
    required this.image,
    required this.imageView,
    this.format = 0,
    this.initialLayout = 0,
    this.finalLayout = 5,
  });

  /// Logical texture extent. The map viewport uses width and height and the
  /// renderer uses scale_factor; the physical size is stated separately below.
  final RenderTargetExtent extent;

  /// Physical image width in device pixels. Must be positive. Defaults to 256.
  final int physicalWidth;

  /// Physical image height in device pixels. Must be positive. Defaults to 256.
  final int physicalHeight;

  /// Borrowed Vulkan context. All handles are required.
  final VulkanContextDescriptor context;

  /// Borrowed VkImage. Required.
  final BigInt image;

  /// Borrowed VkImageView for image. Required.
  final BigInt imageView;

  /// Backend-native VkFormat value for image. VK_FORMAT_UNDEFINED is invalid.
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
    image,
    imageView,
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
  const VulkanOwnedTextureDescriptor({
    this.extent = const RenderTargetExtent(),
    this.context = const VulkanContextDescriptor(),
  });

  /// Logical texture extent.
  final RenderTargetExtent extent;

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
  const VulkanSurfaceDescriptor({
    this.extent = const RenderTargetExtent(),
    this.context = const VulkanContextDescriptor(),
    required this.surface,
  });

  /// Logical surface extent.
  final RenderTargetExtent extent;

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

/// WebGPU attachment options for a borrowed texture target.
///
/// See `mln_webgpu_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class WebgpuBorrowedTextureDescriptor extends _Value {
  const WebgpuBorrowedTextureDescriptor({
    this.extent = const RenderTargetExtent(),
    this.physicalWidth = 256,
    this.physicalHeight = 256,
    this.context = const WebgpuContextDescriptor(),
    this.texture = NativePointer.nullPointer,
    this.textureView = NativePointer.nullPointer,
    this.format = 0,
  });

  /// Logical texture extent.
  final RenderTargetExtent extent;

  /// Physical texture width in device pixels. Defaults to 256.
  final int physicalWidth;

  /// Physical texture height in device pixels. Defaults to 256.
  final int physicalHeight;

  /// Borrowed WebGPU context. device is required.
  final WebgpuContextDescriptor context;

  /// Borrowed WGPUTexture. Required.
  final NativePointer texture;

  /// Borrowed WGPUTextureView for texture. Required.
  final NativePointer textureView;

  /// Backend-native WGPUTextureFormat value. Undefined is invalid.
  final int format;

  @override
  List<Object?> get _members => [
    extent,
    physicalWidth,
    physicalHeight,
    context,
    texture,
    textureView,
    format,
  ];
}

/// WebGPU attachment options for an owned texture target.
///
/// See `mln_webgpu_owned_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class WebgpuOwnedTextureDescriptor extends _Value {
  const WebgpuOwnedTextureDescriptor({
    this.extent = const RenderTargetExtent(),
    this.context = const WebgpuContextDescriptor(),
  });

  /// Logical texture extent.
  final RenderTargetExtent extent;

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
  const WebgpuSurfaceDescriptor({
    this.extent = const RenderTargetExtent(),
    this.context = const WebgpuContextDescriptor(),
    this.surface = NativePointer.nullPointer,
    this.format = 0,
  });

  /// Logical surface extent.
  final RenderTargetExtent extent;

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
