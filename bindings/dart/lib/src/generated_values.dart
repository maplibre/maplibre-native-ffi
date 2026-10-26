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

/// How the transitions of one camera command ended.
///
/// See `mln_camera_transition_outcome` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraTransitionOutcome extends _Enum {
  const CameraTransitionOutcome.fromRawValue(super.rawValue);

  /// Every property of the command reached its target, or a later camera write
  /// replaced it.
  static const completed = CameraTransitionOutcome.fromRawValue(0);

  /// `MapHandle.cancelTransitions()`, `MapHandle.cancelCameraTransition()`, or
  /// `GesturePhase.cancel` ended the transitions, or the command failed before
  /// it started them.
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

/// Camera transition behavior for `CameraUpdate`.
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

/// Map constraint modes used by `MapViewportOptions`.
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

/// Frame-demand policy bits.
///
/// See `mln_frame_demand_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
final class FrameDemandFlag extends _Flags<FrameDemandFlag> {
  const FrameDemandFlag.fromRawValue(super.rawValue);

  /// Render even when no map update is newer than the last rendered one.
  static const force = FrameDemandFlag.fromRawValue(1);

  /// Present the rendered frame on a target that supports presentation. A
  /// presenting target whose demand clears this bit still renders and keeps
  /// whatever it presented last. Ignored by targets without presentation.
  static const present = FrameDemandFlag.fromRawValue(2);

  /// Without `FrameDemandFlag.force`, a demand that would finish with
  /// `RenderResult.noUpdate` waits instead, and runs again after the map's next
  /// update, a target replacement, or an applied resize. A waiting demand holds
  /// no ring slot. A later demand with the same flags supersedes it, a barrier
  /// ends its wait with `RenderResult.noUpdate`, and detach, abandon, or the
  /// quarantine of the ring's last usable slot end it with
  /// `RenderResult.targetNotReady`. A waiting demand's result can follow the
  /// results of demands accepted after it. The flag does not pace: a host that
  /// re-arms a waiting demand as each result arrives renders every update the
  /// map publishes, and a command that arrives while a frame renders appears in
  /// a later frame. To show several commands in one frame, submit them in a
  /// command group, or arm demands once per display frame.
  static const waitForUpdate = FrameDemandFlag.fromRawValue(4);

  /// No flags.
  static const none = FrameDemandFlag.fromRawValue(0);
  @override
  FrameDemandFlag _of(int rawValue) => FrameDemandFlag.fromRawValue(rawValue);
  @override
  String toString() => _describe('FrameDemandFlag', const {
    0: 'none',
    1: 'force',
    2: 'present',
    4: 'waitForUpdate',
  });
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
  /// running transitions; use `MapHandle.cancelTransitions()` for that.
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
  /// caller-graphics-thread driver. handle is nonzero.
  static const openGlFence = GpuSyncKind.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'GpuSyncKind.cpuComplete',
    1 => 'GpuSyncKind.openGlFence',
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
/// See `mln_log_severity_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
final class LogSeverityFlag extends _Flags<LogSeverityFlag> {
  const LogSeverityFlag.fromRawValue(super.rawValue);
  static const info = LogSeverityFlag.fromRawValue(2);
  static const warning = LogSeverityFlag.fromRawValue(4);
  static const error = LogSeverityFlag.fromRawValue(8);
  static const defaultValue = LogSeverityFlag.fromRawValue(6);
  static const all = LogSeverityFlag.fromRawValue(14);

  /// No flags.
  static const none = LogSeverityFlag.fromRawValue(0);
  @override
  LogSeverityFlag _of(int rawValue) => LogSeverityFlag.fromRawValue(rawValue);
  @override
  String toString() => _describe('LogSeverityFlag', const {
    0: 'none',
    2: 'info',
    4: 'warning',
    8: 'error',
    6: 'defaultValue',
    14: 'all',
  });
}

/// Debug overlay mask values for `MapHandle.setDebugOptions()`.
///
/// See `mln_map_debug_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class MapDebugFlag extends _Flags<MapDebugFlag> {
  const MapDebugFlag.fromRawValue(super.rawValue);
  static const tileBorders = MapDebugFlag.fromRawValue(2);
  static const parseStatus = MapDebugFlag.fromRawValue(4);
  static const timestamps = MapDebugFlag.fromRawValue(8);
  static const collision = MapDebugFlag.fromRawValue(16);
  static const overdraw = MapDebugFlag.fromRawValue(32);
  static const stencilClip = MapDebugFlag.fromRawValue(64);
  static const depthBuffer = MapDebugFlag.fromRawValue(128);

  /// No flags.
  static const none = MapDebugFlag.fromRawValue(0);
  @override
  MapDebugFlag _of(int rawValue) => MapDebugFlag.fromRawValue(rawValue);
  @override
  String toString() => _describe('MapDebugFlag', const {
    0: 'none',
    2: 'tileBorders',
    4: 'parseStatus',
    8: 'timestamps',
    16: 'collision',
    32: 'overdraw',
    64: 'stencilClip',
    128: 'depthBuffer',
  });
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

/// Map north orientation values used by `MapViewportOptions`.
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
/// See `mln_open_gl_client_api` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class OpenGlClientApi extends _Enum {
  const OpenGlClientApi.fromRawValue(super.rawValue);

  /// No client API is named.
  static const unspecified = OpenGlClientApi.fromRawValue(0);

  /// Desktop OpenGL, as EGL_OPENGL_API names it.
  static const gl = OpenGlClientApi.fromRawValue(1);

  /// OpenGL ES, as EGL_OPENGL_ES_API names it.
  static const gles = OpenGlClientApi.fromRawValue(2);
  @override
  String toString() => switch (rawValue) {
    0 => 'OpenGlClientApi.unspecified',
    1 => 'OpenGlClientApi.gl',
    2 => 'OpenGlClientApi.gles',
    _ => 'OpenGlClientApi($rawValue)',
  };
}

/// How a session's OpenGL context relates to its driver thread and host
/// graphics state.
///
/// See `mln_open_gl_context_ownership` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class OpenGlContextOwnership extends _Enum {
  const OpenGlContextOwnership.fromRawValue(super.rawValue);

  /// The session shares its thread with host graphics work.
  static const shared = OpenGlContextOwnership.fromRawValue(0);

  /// The session keeps its own OpenGL context current on its thread.
  static const dedicated = OpenGlContextOwnership.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'OpenGlContextOwnership.shared',
    1 => 'OpenGlContextOwnership.dedicated',
    _ => 'OpenGlContextOwnership($rawValue)',
  };
}

/// OpenGL context providers supported by this build.
///
/// See `mln_open_gl_context_provider_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class OpenGlContextProviderFlag
    extends _Flags<OpenGlContextProviderFlag> {
  const OpenGlContextProviderFlag.fromRawValue(super.rawValue);
  static const wgl = OpenGlContextProviderFlag.fromRawValue(1);
  static const egl = OpenGlContextProviderFlag.fromRawValue(2);

  /// Browser WebGL context imported into an Emscripten module.
  static const webGl = OpenGlContextProviderFlag.fromRawValue(4);

  /// No flags.
  static const none = OpenGlContextProviderFlag.fromRawValue(0);
  @override
  OpenGlContextProviderFlag _of(int rawValue) =>
      OpenGlContextProviderFlag.fromRawValue(rawValue);
  @override
  String toString() => _describe('OpenGlContextProviderFlag', const {
    0: 'none',
    1: 'wgl',
    2: 'egl',
    4: 'webGl',
  });
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
  static const openGl = RenderBackendFlag.fromRawValue(4);
  static const webGpu = RenderBackendFlag.fromRawValue(8);

  /// No flags.
  static const none = RenderBackendFlag.fromRawValue(0);
  @override
  RenderBackendFlag _of(int rawValue) =>
      RenderBackendFlag.fromRawValue(rawValue);
  @override
  String toString() => _describe('RenderBackendFlag', const {
    0: 'none',
    1: 'metal',
    2: 'vulkan',
    4: 'openGl',
    8: 'webGpu',
  });
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
  /// update when it has one. A demand with `FrameDemandFlag.waitForUpdate`
  /// waits for that update instead, and finishes with this result only when a
  /// barrier ends its wait.
  static const noUpdate = RenderResult.fromRawValue(1);

  /// The target could not produce a frame. The attempt consumes nothing, so a
  /// later demand with the same flags renders what this one would have. This
  /// result does not cause a map update, so the host demands again when the
  /// target can be ready, such as after a paced delay.
  static const targetNotReady = RenderResult.fromRawValue(2);

  /// A newer demand with the same flags, neither of them forced, replaced this
  /// demand. A still image's demand is never replaced, and never replaces one.
  static const superseded = RenderResult.fromRawValue(3);
  @override
  String toString() => switch (rawValue) {
    0 => 'RenderResult.rendered',
    1 => 'RenderResult.noUpdate',
    2 => 'RenderResult.targetNotReady',
    3 => 'RenderResult.superseded',
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

  /// No flags.
  static const none = RenderSessionCapabilityFlag.fromRawValue(0);
  @override
  RenderSessionCapabilityFlag _of(int rawValue) =>
      RenderSessionCapabilityFlag.fromRawValue(rawValue);
  @override
  String toString() => _describe('RenderSessionCapabilityFlag', const {
    0: 'none',
    1: 'frameAcquisition',
    2: 'readback',
    4: 'consumerSync',
    8: 'presentation',
  });
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

  /// A frame or driver task failed and the session can no longer render. The
  /// map reports the failure as `RuntimeEventType.mapRenderError`. A failed
  /// attach reports it through its completion instead.
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

/// The resource kinds that a route or rule matches, one bit per `ResourceKind`
/// value. Registration rejects unknown bits. `ResourceKindFlag.all`, like zero,
/// selects every resource kind.
///
/// See `mln_resource_kind_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class ResourceKindFlag extends _Flags<ResourceKindFlag> {
  const ResourceKindFlag.fromRawValue(super.rawValue);
  static const unknown = ResourceKindFlag.fromRawValue(1);
  static const style = ResourceKindFlag.fromRawValue(2);
  static const source = ResourceKindFlag.fromRawValue(4);
  static const tile = ResourceKindFlag.fromRawValue(8);
  static const glyphs = ResourceKindFlag.fromRawValue(16);
  static const spriteImage = ResourceKindFlag.fromRawValue(32);
  static const spriteJson = ResourceKindFlag.fromRawValue(64);
  static const image = ResourceKindFlag.fromRawValue(128);
  static const all = ResourceKindFlag.fromRawValue(255);

  /// No flags.
  static const none = ResourceKindFlag.fromRawValue(0);
  @override
  ResourceKindFlag _of(int rawValue) => ResourceKindFlag.fromRawValue(rawValue);
  @override
  String toString() => _describe('ResourceKindFlag', const {
    0: 'none',
    1: 'unknown',
    2: 'style',
    4: 'source',
    8: 'tile',
    16: 'glyphs',
    32: 'spriteImage',
    64: 'spriteJson',
    128: 'image',
    255: 'all',
  });
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
/// See `mln_runtime_event_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventFlag extends _Flags<RuntimeEventFlag> {
  const RuntimeEventFlag.fromRawValue(super.rawValue);

  /// Selects no event type.
  static const none = RuntimeEventFlag.fromRawValue(0);
  static const mapCameraWillChange = RuntimeEventFlag.fromRawValue(2);
  static const mapCameraIsChanging = RuntimeEventFlag.fromRawValue(4);
  static const mapCameraDidChange = RuntimeEventFlag.fromRawValue(8);
  static const mapStyleLoaded = RuntimeEventFlag.fromRawValue(16);
  static const mapLoadingStarted = RuntimeEventFlag.fromRawValue(32);
  static const mapLoadingFinished = RuntimeEventFlag.fromRawValue(64);
  static const mapLoadingFailed = RuntimeEventFlag.fromRawValue(128);
  static const mapIdle = RuntimeEventFlag.fromRawValue(256);
  static const mapRenderError = RuntimeEventFlag.fromRawValue(512);
  static const mapRenderFrameStarted = RuntimeEventFlag.fromRawValue(1024);
  static const mapRenderFrameFinished = RuntimeEventFlag.fromRawValue(2048);
  static const mapRenderMapStarted = RuntimeEventFlag.fromRawValue(4096);
  static const mapRenderMapFinished = RuntimeEventFlag.fromRawValue(8192);
  static const mapStyleImageMissing = RuntimeEventFlag.fromRawValue(16384);
  static const mapTileAction = RuntimeEventFlag.fromRawValue(32768);
  static const mapSourceChanged = RuntimeEventFlag.fromRawValue(65536);
  static const offlineRegionStatusChanged = RuntimeEventFlag.fromRawValue(
    131072,
  );
  static const offlineRegionResponseError = RuntimeEventFlag.fromRawValue(
    262144,
  );
  static const offlineRegionTileCountLimitExceeded =
      RuntimeEventFlag.fromRawValue(524288);

  /// Selects every map-originated event type this version defines.
  static const allMapEvents = RuntimeEventFlag.fromRawValue(131070);

  /// Selects every runtime-originated event type this version defines.
  static const allRuntimeEvents = RuntimeEventFlag.fromRawValue(917504);

  /// Selects every event type this version defines.
  static const all = RuntimeEventFlag.fromRawValue(1048574);
  @override
  RuntimeEventFlag _of(int rawValue) => RuntimeEventFlag.fromRawValue(rawValue);
  @override
  String toString() => _describe('RuntimeEventFlag', const {
    0: 'none',
    2: 'mapCameraWillChange',
    4: 'mapCameraIsChanging',
    8: 'mapCameraDidChange',
    16: 'mapStyleLoaded',
    32: 'mapLoadingStarted',
    64: 'mapLoadingFinished',
    128: 'mapLoadingFailed',
    256: 'mapIdle',
    512: 'mapRenderError',
    1024: 'mapRenderFrameStarted',
    2048: 'mapRenderFrameFinished',
    4096: 'mapRenderMapStarted',
    8192: 'mapRenderMapFinished',
    16384: 'mapStyleImageMissing',
    32768: 'mapTileAction',
    65536: 'mapSourceChanged',
    131072: 'offlineRegionStatusChanged',
    262144: 'offlineRegionResponseError',
    524288: 'offlineRegionTileCountLimitExceeded',
    131070: 'allMapEvents',
    917504: 'allRuntimeEvents',
    1048574: 'all',
  });
}

/// Source kinds used by `RuntimeEvent.sourceType`.
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

/// Runtime event types carried by `RuntimeEvent.type`.
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
  static const mapRenderError = RuntimeEventType.fromRawValue(9);
  static const mapRenderFrameStarted = RuntimeEventType.fromRawValue(10);
  static const mapRenderFrameFinished = RuntimeEventType.fromRawValue(11);
  static const mapRenderMapStarted = RuntimeEventType.fromRawValue(12);
  static const mapRenderMapFinished = RuntimeEventType.fromRawValue(13);
  static const mapStyleImageMissing = RuntimeEventType.fromRawValue(14);
  static const mapTileAction = RuntimeEventType.fromRawValue(15);
  static const mapSourceChanged = RuntimeEventType.fromRawValue(16);
  static const offlineRegionStatusChanged = RuntimeEventType.fromRawValue(17);
  static const offlineRegionResponseError = RuntimeEventType.fromRawValue(18);
  static const offlineRegionTileCountLimitExceeded =
      RuntimeEventType.fromRawValue(19);
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
    9 => 'RuntimeEventType.mapRenderError',
    10 => 'RuntimeEventType.mapRenderFrameStarted',
    11 => 'RuntimeEventType.mapRenderFrameFinished',
    12 => 'RuntimeEventType.mapRenderMapStarted',
    13 => 'RuntimeEventType.mapRenderMapFinished',
    14 => 'RuntimeEventType.mapStyleImageMissing',
    15 => 'RuntimeEventType.mapTileAction',
    16 => 'RuntimeEventType.mapSourceChanged',
    17 => 'RuntimeEventType.offlineRegionStatusChanged',
    18 => 'RuntimeEventType.offlineRegionResponseError',
    19 => 'RuntimeEventType.offlineRegionTileCountLimitExceeded',
    _ => 'RuntimeEventType($rawValue)',
  };
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

  /// Native code failed; the diagnostic describes the error.
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

/// Style expression types that a parsed expression may be required to have.
///
/// See `mln_style_expression_type_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
final class StyleExpressionTypeKind extends _Enum {
  const StyleExpressionTypeKind.fromRawValue(super.rawValue);

  /// Any value.
  static const value = StyleExpressionTypeKind.fromRawValue(0);
  static const number = StyleExpressionTypeKind.fromRawValue(1);
  static const string = StyleExpressionTypeKind.fromRawValue(2);
  static const boolean = StyleExpressionTypeKind.fromRawValue(3);
  static const color = StyleExpressionTypeKind.fromRawValue(4);
  static const object = StyleExpressionTypeKind.fromRawValue(5);
  static const formatted = StyleExpressionTypeKind.fromRawValue(6);

  /// A resolved image, as the image expression returns.
  static const image = StyleExpressionTypeKind.fromRawValue(7);
  static const padding = StyleExpressionTypeKind.fromRawValue(8);
  static const variableAnchorOffsetCollection =
      StyleExpressionTypeKind.fromRawValue(9);

  /// An array whose items have item_kind, which is not itself an array.
  static const array = StyleExpressionTypeKind.fromRawValue(10);
  @override
  String toString() => switch (rawValue) {
    0 => 'StyleExpressionTypeKind.value',
    1 => 'StyleExpressionTypeKind.number',
    2 => 'StyleExpressionTypeKind.string',
    3 => 'StyleExpressionTypeKind.boolean',
    4 => 'StyleExpressionTypeKind.color',
    5 => 'StyleExpressionTypeKind.object',
    6 => 'StyleExpressionTypeKind.formatted',
    7 => 'StyleExpressionTypeKind.image',
    8 => 'StyleExpressionTypeKind.padding',
    9 => 'StyleExpressionTypeKind.variableAnchorOffsetCollection',
    10 => 'StyleExpressionTypeKind.array',
    _ => 'StyleExpressionTypeKind($rawValue)',
  };
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
  static const geoJson = StyleSourceType.fromRawValue(4);
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
    4 => 'StyleSourceType.geoJson',
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

/// Tile LOD algorithms used by `MapTileOptions`.
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
/// See `mln_url_match_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class UrlMatchFlag extends _Flags<UrlMatchFlag> {
  const UrlMatchFlag.fromRawValue(super.rawValue);
  static const exact = UrlMatchFlag.fromRawValue(0);

  /// Reads url as a glob pattern.
  static const glob = UrlMatchFlag.fromRawValue(1);
  @override
  UrlMatchFlag _of(int rawValue) => UrlMatchFlag.fromRawValue(rawValue);
  @override
  String toString() => _describe('UrlMatchFlag', const {0: 'exact', 1: 'glob'});
}

/// Viewport orientation modes used by `MapViewportOptions`.
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
/// See `mln_web_gl_context_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class WebGlContextKind extends _Enum {
  const WebGlContextKind.fromRawValue(super.rawValue);

  /// Use a host-created context on its current browser agent.
  static const existing = WebGlContextKind.fromRawValue(0);

  /// Transfer the canvas named by canvas_selector to a native worker and create
  /// a WebGL 2 context there.
  static const transferredCanvas = WebGlContextKind.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'WebGlContextKind.existing',
    1 => 'WebGlContextKind.transferredCanvas',
    _ => 'WebGlContextKind($rawValue)',
  };
}

/// Metal frame acquired from a texture ring.
///
/// See `mln_metal_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class MetalTextureFrame extends _Value {
  const MetalTextureFrame({
    this.generation = 0,
    this.width = 0,
    this.height = 0,
    this.scaleFactor = 0,
    this.frameId = 0,
    this.slot = 0,
    this.texture = NativePointer.nullPointer,
    this.device = NativePointer.nullPointer,
    this.pixelFormat = 0,
  });

  /// Session generation that produced this frame.
  final int generation;

  /// Physical Metal texture width in device pixels.
  final int width;

  /// Physical Metal texture height in device pixels.
  final int height;

  /// UI-to-device pixel scale used for this frame.
  final double scaleFactor;

  /// Opaque identity of this frame.
  final int frameId;

  /// Ring slot that holds this frame. For a borrowed target, the index of its
  /// texture in the descriptor's textures array.
  final int slot;

  /// Borrowed `id<MTLTexture>` / `MTL::Texture*`. Valid until frame release.
  final NativePointer texture;

  /// Borrowed `id<MTLDevice>` / `MTL::Device*`. Valid until frame release.
  final NativePointer device;

  /// Backend-native pixel format value. Metal uses MTLPixelFormat.
  final int pixelFormat;

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
  @override
  String toString() => _describe('MetalTextureFrame', const [
    'generation',
    'width',
    'height',
    'scaleFactor',
    'frameId',
    'slot',
    'texture',
    'device',
    'pixelFormat',
  ]);
}

/// OpenGL frame acquired from a texture ring.
///
/// See `mln_open_gl_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class OpenGlTextureFrame extends _Value {
  const OpenGlTextureFrame({
    this.generation = 0,
    this.width = 0,
    this.height = 0,
    this.scaleFactor = 0,
    this.frameId = 0,
    this.slot = 0,
    this.texture = 0,
    this.target = 0,
    this.internalFormat = 0,
    this.format = 0,
    this.type = 0,
  });

  /// Session generation that produced this frame.
  final int generation;

  /// Physical OpenGL texture width in device pixels.
  final int width;

  /// Physical OpenGL texture height in device pixels.
  final int height;

  /// UI-to-device pixel scale used for this frame.
  final double scaleFactor;

  /// Opaque identity of this frame.
  final int frameId;

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
  @override
  String toString() => _describe('OpenGlTextureFrame', const [
    'generation',
    'width',
    'height',
    'scaleFactor',
    'frameId',
    'slot',
    'texture',
    'target',
    'internalFormat',
    'format',
    'type',
  ]);
}

/// Backend synchronization copied by frame access and release calls.
///
/// See `mln_gpu_sync` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class GpuSync extends _Value {
  const GpuSync({
    this.kind = const GpuSyncKind.fromRawValue(0),
    this.handle = 0,
  });
  final GpuSyncKind kind;

  /// Bit pattern of the GLsync pointer.
  final int handle;

  /// A copy of this value with the given members replaced.
  GpuSync copyWith({GpuSyncKind? kind, int? handle}) =>
      GpuSync(kind: kind ?? this.kind, handle: handle ?? this.handle);

  @override
  List<Object?> get _members => [kind, handle];
  @override
  String toString() => _describe('GpuSync', const ['kind', 'handle']);
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

  /// A copy of this value with the given members replaced.
  LatLng copyWith({double? latitude, double? longitude}) => LatLng(
    latitude: latitude ?? this.latitude,
    longitude: longitude ?? this.longitude,
  );

  @override
  List<Object?> get _members => [latitude, longitude];
  @override
  String toString() => _describe('LatLng', const ['latitude', 'longitude']);
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

  /// A copy of this value with the given members replaced.
  EdgeInsets copyWith({
    double? top,
    double? left,
    double? bottom,
    double? right,
  }) => EdgeInsets(
    top: top ?? this.top,
    left: left ?? this.left,
    bottom: bottom ?? this.bottom,
    right: right ?? this.right,
  );

  @override
  List<Object?> get _members => [top, left, bottom, right];
  @override
  String toString() =>
      _describe('EdgeInsets', const ['top', 'left', 'bottom', 'right']);
}

/// A complete camera, as native reports it.
///
/// See `mln_camera` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class Camera extends _Value {
  const Camera({
    this.center = const LatLng(),
    this.centerAltitude = 0,
    this.padding = const EdgeInsets(),
    this.zoom = 0,
    this.bearing = 0,
    this.pitch = 0,
    this.roll = 0,
    this.fieldOfView = 0,
  });
  final LatLng center;
  final double centerAltitude;
  final EdgeInsets padding;
  final double zoom;
  final double bearing;
  final double pitch;
  final double roll;
  final double fieldOfView;

  /// A copy of this value with the given members replaced.
  Camera copyWith({
    LatLng? center,
    double? centerAltitude,
    EdgeInsets? padding,
    double? zoom,
    double? bearing,
    double? pitch,
    double? roll,
    double? fieldOfView,
  }) => Camera(
    center: center ?? this.center,
    centerAltitude: centerAltitude ?? this.centerAltitude,
    padding: padding ?? this.padding,
    zoom: zoom ?? this.zoom,
    bearing: bearing ?? this.bearing,
    pitch: pitch ?? this.pitch,
    roll: roll ?? this.roll,
    fieldOfView: fieldOfView ?? this.fieldOfView,
  );

  @override
  List<Object?> get _members => [
    center,
    centerAltitude,
    padding,
    zoom,
    bearing,
    pitch,
    roll,
    fieldOfView,
  ];
  @override
  String toString() => _describe('Camera', const [
    'center',
    'centerAltitude',
    'padding',
    'zoom',
    'bearing',
    'pitch',
    'roll',
    'fieldOfView',
  ]);
}

/// Logical extent in UI pixels and the device-pixel scale.
///
/// See `mln_logical_extent` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class LogicalExtent extends _Value {
  const LogicalExtent({this.width = 0, this.height = 0, this.scaleFactor = 0});

  /// Width in UI pixels.
  final int width;

  /// Height in UI pixels.
  final int height;

  /// Device pixels per UI pixel.
  final double scaleFactor;

  /// A copy of this value with the given members replaced.
  LogicalExtent copyWith({int? width, int? height, double? scaleFactor}) =>
      LogicalExtent(
        width: width ?? this.width,
        height: height ?? this.height,
        scaleFactor: scaleFactor ?? this.scaleFactor,
      );

  @override
  List<Object?> get _members => [width, height, scaleFactor];
  @override
  String toString() =>
      _describe('LogicalExtent', const ['width', 'height', 'scaleFactor']);
}

/// MapLibre axonometric rendering options used for snapshots and commands.
///
/// See `mln_projection_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class ProjectionMode extends _Value {
  const ProjectionMode({this.axonometric, this.xSkew, this.ySkew});

  /// Enables a non-perspective axonometric render transform.
  final bool? axonometric;

  /// X-skew factor of the axonometric transform.
  final double? xSkew;

  /// Y-skew factor of the axonometric transform.
  final double? ySkew;

  /// A copy of this value with the given members replaced.
  ProjectionMode copyWith({bool? axonometric, double? xSkew, double? ySkew}) =>
      ProjectionMode(
        axonometric: axonometric ?? this.axonometric,
        xSkew: xSkew ?? this.xSkew,
        ySkew: ySkew ?? this.ySkew,
      );

  @override
  List<Object?> get _members => [axonometric, xSkew, ySkew];
  @override
  String toString() =>
      _describe('ProjectionMode', const ['axonometric', 'xSkew', 'ySkew']);
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
  final NorthOrientation? northOrientation;
  final ConstrainMode? constrainMode;
  final ViewportMode? viewportMode;
  final EdgeInsets? frustumOffset;

  /// A copy of this value with the given members replaced.
  MapViewportOptions copyWith({
    NorthOrientation? northOrientation,
    ConstrainMode? constrainMode,
    ViewportMode? viewportMode,
    EdgeInsets? frustumOffset,
  }) => MapViewportOptions(
    northOrientation: northOrientation ?? this.northOrientation,
    constrainMode: constrainMode ?? this.constrainMode,
    viewportMode: viewportMode ?? this.viewportMode,
    frustumOffset: frustumOffset ?? this.frustumOffset,
  );

  @override
  List<Object?> get _members => [
    northOrientation,
    constrainMode,
    viewportMode,
    frustumOffset,
  ];
  @override
  String toString() => _describe('MapViewportOptions', const [
    'northOrientation',
    'constrainMode',
    'viewportMode',
    'frustumOffset',
  ]);
}

/// Everything that a projection depends on: the transform that a published
/// snapshot or a rendered frame describes.
///
/// See `mln_map_view` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class MapView extends _Value {
  const MapView({
    this.camera = const Camera(),
    this.extent = const LogicalExtent(),
    this.projectionMode = const ProjectionMode(),
    this.viewport = const MapViewportOptions(),
  });

  /// The camera. Its center is unwrapped: the longitude keeps the visible world
  /// copy and may fall outside -180 to 180.
  final Camera camera;

  /// Logical size and device-pixel scale. A snapshot carries the map's scale
  /// factor, and a frame carries the scale factor of the target that drew it.
  final LogicalExtent extent;

  /// Axonometric mode and skew, with every field set.
  final ProjectionMode projectionMode;

  /// North orientation, constrain mode, viewport mode, and frustum offset, with
  /// every field set.
  final MapViewportOptions viewport;

  /// A copy of this value with the given members replaced.
  MapView copyWith({
    Camera? camera,
    LogicalExtent? extent,
    ProjectionMode? projectionMode,
    MapViewportOptions? viewport,
  }) => MapView(
    camera: camera ?? this.camera,
    extent: extent ?? this.extent,
    projectionMode: projectionMode ?? this.projectionMode,
    viewport: viewport ?? this.viewport,
  );

  @override
  List<Object?> get _members => [camera, extent, projectionMode, viewport];
  @override
  String toString() => _describe('MapView', const [
    'camera',
    'extent',
    'projectionMode',
    'viewport',
  ]);
}

/// Terminal result of one frame demand, held by an owned frame-result batch and
/// copied by `AcquiredFrameHandle.getResult()`.
///
/// See `mln_render_frame_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
final class RenderFrameResult extends _Value {
  const RenderFrameResult({
    this.disposition = const RenderResult.fromRawValue(0),
    this.token = 0,
    this.mapUpdateGeneration = 0,
    this.generation = 0,
    this.frameGeneration = 0,
    this.needsRepaint = false,
    this.view = const MapView(),
  });
  final RenderResult disposition;
  final int token;

  /// Generation of the map render update the demand evaluated. When disposition
  /// is `RenderResult.rendered`, the frame drew that update; compare it with
  /// `CommandCompletion.renderUpdateGeneration` to find the first frame that
  /// includes a command.
  final int mapUpdateGeneration;

  /// The session's target generation when the demand reached its result. See
  /// `RenderSessionSnapshot.generation`.
  final int generation;

  /// Position of the frame among the frames that the session rendered, counting
  /// from one. Zero unless disposition is `RenderResult.rendered`.
  final int frameGeneration;

  /// Whether the map asked for another frame while it rendered this one, as
  /// during an ongoing paint transition. Set only when disposition is
  /// `RenderResult.rendered`, and false for every other outcome. It matches the
  /// needs_repaint field of `RuntimeEventType.mapRenderFrameFinished`. A camera
  /// transition does not set it: the map publishes an update for each of the
  /// transition's frames instead, which a render-if-needed demand renders.
  final bool needsRepaint;

  /// The view that this frame drew, from the transform of the map update that
  /// it rendered. A host that displays the frame builds its projection with
  /// `MapProjectionHandle.create()` to place overlays where the frame drew
  /// their coordinates. Zero, with a size of zero, unless disposition is
  /// `RenderResult.rendered`.
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
  @override
  String toString() => _describe('RenderFrameResult', const [
    'disposition',
    'token',
    'mapUpdateGeneration',
    'generation',
    'frameGeneration',
    'needsRepaint',
    'view',
  ]);
}

/// Vulkan frame acquired from a texture ring.
///
/// See `mln_vulkan_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class VulkanTextureFrame extends _Value {
  const VulkanTextureFrame({
    this.generation = 0,
    this.width = 0,
    this.height = 0,
    this.scaleFactor = 0,
    this.frameId = 0,
    this.slot = 0,
    this.image = 0,
    this.imageView = 0,
    this.device = NativePointer.nullPointer,
    this.format = 0,
    this.layout = 0,
  });

  /// Session generation that produced this frame.
  final int generation;

  /// Physical Vulkan image width in device pixels.
  final int width;

  /// Physical Vulkan image height in device pixels.
  final int height;

  /// UI-to-device pixel scale used for this frame.
  final double scaleFactor;

  /// Opaque identity of this frame.
  final int frameId;

  /// Ring slot that holds this frame. For a borrowed target, the index of its
  /// image in the descriptor's textures array.
  final int slot;

  /// Borrowed VkImage bit pattern. Valid until frame release.
  final int image;

  /// Borrowed VkImageView bit pattern. Valid until frame release.
  final int imageView;

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
  @override
  String toString() => _describe('VulkanTextureFrame', const [
    'generation',
    'width',
    'height',
    'scaleFactor',
    'frameId',
    'slot',
    'image',
    'imageView',
    'device',
    'format',
    'layout',
  ]);
}

/// Options for GeoJSON sources, indexed and rendered at a fixed 512-pixel tile
/// size.
///
/// See `mln_geo_json_source_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class GeoJsonSourceOptions extends _Value {
  const GeoJsonSourceOptions({
    this.minZoom,
    this.maxZoom,
    this.tolerance,
    this.clusterMaxZoom,
    this.clusterProperties,
    this.buffer,
    this.clusterRadius,
    this.clusterMinPoints,
    this.lineMetrics,
    this.cluster,
    this.synchronousTiling,
  });

  /// Minimum tiling zoom. Absent selects 0.
  final double? minZoom;

  /// Maximum tiling zoom, an integer within \[0, 24\]. Absent selects 18.
  final double? maxZoom;

  /// Douglas-Peucker simplification tolerance. Absent selects 0.375.
  final double? tolerance;

  /// Highest zoom that clusters points. Absent selects 17.
  final double? clusterMaxZoom;

  /// Cluster aggregation expressions keyed by property name, as a JSON object
  /// whose members follow the MapLibre Style Spec clusterProperties form, or
  /// empty for none.
  final Object? clusterProperties;

  /// Tile buffer in pixels within \[0, 512\]. Absent selects 128.
  final int? buffer;

  /// Cluster radius in pixels within \[0, 4095\]. Absent selects 50.
  final int? clusterRadius;

  /// Points required to form a cluster. Absent selects 2.
  final int? clusterMinPoints;

  /// Adds line distance metrics to line features. Absent selects false.
  final bool? lineMetrics;

  /// Clusters point features. Absent selects false.
  final bool? cluster;

  /// Tiles the data for the next render update instead of in the background.
  /// Absent selects false.
  final bool? synchronousTiling;

  /// A copy of this value with the given members replaced.
  GeoJsonSourceOptions copyWith({
    double? minZoom,
    double? maxZoom,
    double? tolerance,
    double? clusterMaxZoom,
    Object? clusterProperties = _unchanged,
    int? buffer,
    int? clusterRadius,
    int? clusterMinPoints,
    bool? lineMetrics,
    bool? cluster,
    bool? synchronousTiling,
  }) => GeoJsonSourceOptions(
    minZoom: minZoom ?? this.minZoom,
    maxZoom: maxZoom ?? this.maxZoom,
    tolerance: tolerance ?? this.tolerance,
    clusterMaxZoom: clusterMaxZoom ?? this.clusterMaxZoom,
    clusterProperties: identical(clusterProperties, _unchanged)
        ? this.clusterProperties
        : clusterProperties,
    buffer: buffer ?? this.buffer,
    clusterRadius: clusterRadius ?? this.clusterRadius,
    clusterMinPoints: clusterMinPoints ?? this.clusterMinPoints,
    lineMetrics: lineMetrics ?? this.lineMetrics,
    cluster: cluster ?? this.cluster,
    synchronousTiling: synchronousTiling ?? this.synchronousTiling,
  );

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
  @override
  String toString() => _describe('GeoJsonSourceOptions', const [
    'minZoom',
    'maxZoom',
    'tolerance',
    'clusterMaxZoom',
    'clusterProperties',
    'buffer',
    'clusterRadius',
    'clusterMinPoints',
    'lineMetrics',
    'cluster',
    'synchronousTiling',
  ]);
}

/// Options for creating a graphics device.
///
/// See `mln_graphics_device_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/graphics__device_8h.html).
final class GraphicsDeviceOptions extends _Value {
  const GraphicsDeviceOptions({this.adapterIndex});

  /// The adapter to create the device on, as a position in the graphics API's
  /// own list of adapters, from zero. Without it, the backend picks its default
  /// adapter.
  final int? adapterIndex;

  /// A copy of this value with the given members replaced.
  GraphicsDeviceOptions copyWith({int? adapterIndex}) =>
      GraphicsDeviceOptions(adapterIndex: adapterIndex ?? this.adapterIndex);

  @override
  List<Object?> get _members => [adapterIndex];
  @override
  String toString() =>
      _describe('GraphicsDeviceOptions', const ['adapterIndex']);
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

  /// A copy of this value with the given members replaced.
  ProjectedMeters copyWith({double? northing, double? easting}) =>
      ProjectedMeters(
        northing: northing ?? this.northing,
        easting: easting ?? this.easting,
      );

  @override
  List<Object?> get _members => [northing, easting];
  @override
  String toString() =>
      _describe('ProjectedMeters', const ['northing', 'easting']);
}

/// Receives a MapLibre Native log record. The callback runs asynchronously in
/// the isolate and zone that registered it, after native queues the call.
///
/// See `mln_log_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
typedef LogCallback =
    void Function(
      LogSeverity severity,
      LogEvent event,
      int code,
      String message,
    );

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

  /// A copy of this value with the given members replaced.
  CanonicalTileId copyWith({int? z, int? x, int? y}) =>
      CanonicalTileId(z: z ?? this.z, x: x ?? this.x, y: y ?? this.y);

  @override
  List<Object?> get _members => [z, x, y];
  @override
  String toString() => _describe('CanonicalTileId', const ['z', 'x', 'y']);
}

/// Requests one tile of a custom geometry or custom MVT vector source. The
/// callback runs asynchronously in the isolate and zone that registered it,
/// after native queues the call.
///
/// See `mln_custom_source_fetch_tile_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
typedef CustomSourceFetchTileCallback = void Function(CanonicalTileId tileId);

/// Cancels a request for one tile of a custom geometry or custom MVT vector
/// source, best effort. The callback runs asynchronously in the isolate and
/// zone that registered it, after native queues the call.
///
/// See `mln_custom_source_cancel_tile_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
typedef CustomSourceCancelTileCallback = void Function(CanonicalTileId tileId);

/// Options for custom geometry sources. The callback runs asynchronously in the
/// isolate and zone that registered it, after native queues the call.
///
/// See `mln_custom_geometry_source_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class CustomGeometrySourceOptions {
  const CustomGeometrySourceOptions({
    required this.fetchTile,
    this.cancelTile,
    this.minZoom,
    this.maxZoom,
    this.tolerance,
    this.tileSize,
    this.buffer,
    this.clip,
    this.wrap,
  });

  /// Tile fetch callback.
  final CustomSourceFetchTileCallback fetchTile;

  /// Optional best-effort tile cancel callback.
  final CustomSourceCancelTileCallback? cancelTile;
  final double? minZoom;
  final double? maxZoom;
  final double? tolerance;
  final int? tileSize;
  final int? buffer;
  final bool? clip;
  final bool? wrap;
}

/// Options for custom MVT vector sources. The callback runs asynchronously in
/// the isolate and zone that registered it, after native queues the call.
///
/// See `mln_custom_mvt_vector_source_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class CustomMvtVectorSourceOptions {
  const CustomMvtVectorSourceOptions({
    required this.fetchTile,
    this.cancelTile,
    this.minZoom,
    this.maxZoom,
  });

  /// Tile fetch callback.
  final CustomSourceFetchTileCallback fetchTile;

  /// Optional best-effort tile cancel callback.
  final CustomSourceCancelTileCallback? cancelTile;
  final double? minZoom;
  final double? maxZoom;
}

/// Caller-owned premultiplied RGBA8 image pixels.
///
/// See `mln_premultiplied_rgba8_image` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class PremultipliedRgba8Image extends _Value {
  PremultipliedRgba8Image({
    required this.width,
    required this.height,
    required this.stride,
    required Uint8List pixels,
  }) : pixels = Uint8List.fromList(pixels).asUnmodifiableView();

  /// Image width in pixels. Required.
  final int width;

  /// Image height in pixels. Required.
  final int height;

  /// Bytes per image row. Required, and at least width \* 4.
  final int stride;

  /// Premultiplied RGBA8 pixels. Required.
  final Uint8List pixels;

  /// A copy of this value with the given members replaced.
  PremultipliedRgba8Image copyWith({
    int? width,
    int? height,
    int? stride,
    Uint8List? pixels,
  }) => PremultipliedRgba8Image(
    width: width ?? this.width,
    height: height ?? this.height,
    stride: stride ?? this.stride,
    pixels: pixels ?? this.pixels,
  );

  @override
  List<Object?> get _members => [width, height, stride, pixels];
  @override
  String toString() => _describe('PremultipliedRgba8Image', const [
    'width',
    'height',
    'stride',
    'pixels',
  ]);
}

/// Screen-space point in logical map pixels, from the top-left of the viewport
/// with y growing downward.
///
/// See `mln_screen_point` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class ScreenPoint extends _Value {
  const ScreenPoint({this.x = 0, this.y = 0});
  final double x;
  final double y;

  /// A copy of this value with the given members replaced.
  ScreenPoint copyWith({double? x, double? y}) =>
      ScreenPoint(x: x ?? this.x, y: y ?? this.y);

  @override
  List<Object?> get _members => [x, y];
  @override
  String toString() => _describe('ScreenPoint', const ['x', 'y']);
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

  /// A copy of this value with the given members replaced.
  UnitBezier copyWith({double? x1, double? y1, double? x2, double? y2}) =>
      UnitBezier(
        x1: x1 ?? this.x1,
        y1: y1 ?? this.y1,
        x2: x2 ?? this.x2,
        y2: y2 ?? this.y2,
      );

  @override
  List<Object?> get _members => [x1, y1, x2, y2];
  @override
  String toString() => _describe('UnitBezier', const ['x1', 'y1', 'x2', 'y2']);
}

/// The end of one camera command's transitions.
///
/// See `mln_camera_transition_end` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraTransitionEnd extends _Value {
  const CameraTransitionEnd({
    this.outcome = const CameraTransitionOutcome.fromRawValue(0),
    this.generation = 0,
  });
  final CameraTransitionOutcome outcome;

  /// Generation of the published snapshot that shows the camera where the
  /// transitions left it. The map events of the change that ended them carry
  /// this generation and are queued before the callback runs. Zero for
  /// `CameraTransitionOutcome.closed`.
  final int generation;

  @override
  List<Object?> get _members => [outcome, generation];
  @override
  String toString() =>
      _describe('CameraTransitionEnd', const ['outcome', 'generation']);
}

/// Receives the end of one camera command's transitions. The callback runs
/// asynchronously in the isolate and zone that registered it, after native
/// queues the call.
///
/// See `mln_camera_transition_end_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
typedef CameraTransitionEndCallback = void Function(CameraTransitionEnd end);

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
    this.endHandler,
  });

  /// Duration in milliseconds. Must be finite, non-negative, and short enough
  /// to represent as a duration.
  final double? durationMs;

  /// Average fly velocity in screenfuls per second. Must be positive and
  /// finite; absent selects 1.2. A flight whose derived duration is too long to
  /// represent fails its completion with `Status.nativeError`. An explicit
  /// duration overrides velocity.
  final double? velocity;

  /// Peak zoom for flyTo transitions.
  final double? minZoom;
  final UnitBezier? easing;

  /// Caller-chosen identity of the command that these options animate, which
  /// `MapHandle.cancelCameraTransition()` matches.
  final int? transitionId;

  /// Reports the end of the command's transitions. Disabled by default.
  final CameraTransitionEndCallback? endHandler;

  /// A copy of this value with the given members replaced.
  AnimationOptions copyWith({
    double? durationMs,
    double? velocity,
    double? minZoom,
    UnitBezier? easing,
    int? transitionId,
    CameraTransitionEndCallback? endHandler =
        _unchangedCameraTransitionEndCallback,
  }) => AnimationOptions(
    durationMs: durationMs ?? this.durationMs,
    velocity: velocity ?? this.velocity,
    minZoom: minZoom ?? this.minZoom,
    easing: easing ?? this.easing,
    transitionId: transitionId ?? this.transitionId,
    endHandler: identical(endHandler, _unchangedCameraTransitionEndCallback)
        ? this.endHandler
        : endHandler,
  );

  @override
  List<Object?> get _members => [
    durationMs,
    velocity,
    minZoom,
    easing,
    transitionId,
    endHandler,
  ];
  @override
  String toString() => _describe('AnimationOptions', const [
    'durationMs',
    'velocity',
    'minZoom',
    'easing',
    'transitionId',
    'endHandler',
  ]);
}

void _unchangedCameraTransitionEndCallback(CameraTransitionEnd end) {}

/// One atomic relative camera update.
///
/// See `mln_camera_delta` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraDelta extends _Value {
  const CameraDelta({
    this.offset,
    this.scale,
    this.bearing,
    this.pitch,
    this.anchor,
    this.animation = const AnimationOptions(),
    this.gesturePhase = const GesturePhase.fromRawValue(0),
  });

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

  /// A copy of this value with the given members replaced.
  CameraDelta copyWith({
    ScreenPoint? offset,
    double? scale,
    double? bearing,
    double? pitch,
    ScreenPoint? anchor,
    AnimationOptions? animation,
    GesturePhase? gesturePhase,
  }) => CameraDelta(
    offset: offset ?? this.offset,
    scale: scale ?? this.scale,
    bearing: bearing ?? this.bearing,
    pitch: pitch ?? this.pitch,
    anchor: anchor ?? this.anchor,
    animation: animation ?? this.animation,
    gesturePhase: gesturePhase ?? this.gesturePhase,
  );

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
  @override
  String toString() => _describe('CameraDelta', const [
    'offset',
    'scale',
    'bearing',
    'pitch',
    'anchor',
    'animation',
    'gesturePhase',
  ]);
}

/// Attachment options for a graphics device texture target.
///
/// See `mln_graphics_device_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/graphics__device_8h.html).
final class GraphicsDeviceTextureDescriptor extends _Value {
  const GraphicsDeviceTextureDescriptor({required this.extent});

  /// Logical texture extent. Required. A scale_factor that differs from the
  /// map's is accepted and logged as a warning.
  final LogicalExtent extent;

  /// A copy of this value with the given members replaced.
  GraphicsDeviceTextureDescriptor copyWith({LogicalExtent? extent}) =>
      GraphicsDeviceTextureDescriptor(extent: extent ?? this.extent);

  @override
  List<Object?> get _members => [extent];
  @override
  String toString() =>
      _describe('GraphicsDeviceTextureDescriptor', const ['extent']);
}

/// Schedules service by the receiver that owns a queue or driver. The callback
/// runs asynchronously in the isolate and zone that registered it, after native
/// queues the call.
///
/// See `mln_wake_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wake_8h.html).
typedef WakeCallback = void Function();

/// Receiver wake callback copied by a successful owning call. The callback
/// schedules service by the receiver and returns without waiting for another
/// thread. The callback runs asynchronously in the isolate and zone that
/// registered it, after native queues the call.
///
/// See `mln_wake` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wake_8h.html).
final class Wake {
  const Wake({this.callback});

  /// Creates a native registration from [callback], [userData], and
  /// [releaseUserData], which native code in the host supplies. The binding
  /// copies them unchanged.
  ///
  /// The first call that carries the registration consumes it, and a later
  /// call throws an invalid-state error. Native code releases the context
  /// of a call that it accepts, and the binding calls [releaseUserData] for
  /// a call that native code does not accept.
  Wake.native({
    required NativePointer callback,
    NativePointer userData = NativePointer.nullPointer,
    NativePointer releaseUserData = NativePointer.nullPointer,
  }) : callback = null {
    nativeRegistrations[this] = NativeRegistration(
      [callback],
      userData,
      releaseUserData,
    );
  }
  final WakeCallback? callback;
}

/// Host lock on the graphics queue that a session shares with its host, copied
/// by a successful attach. The callback runs asynchronously in the isolate and
/// zone that registered it, after native queues the call.
///
/// See `mln_queue_lock` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class QueueLock {
  /// Creates a native registration from [lock], [unlock], [userData], and
  /// [releaseUserData], which native code in the host supplies. The binding
  /// copies them unchanged.
  ///
  /// The first call that carries the registration consumes it, and a later
  /// call throws an invalid-state error. Native code releases the context
  /// of a call that it accepts, and the binding calls [releaseUserData] for
  /// a call that native code does not accept.
  QueueLock.native({
    required NativePointer lock,
    required NativePointer unlock,
    NativePointer userData = NativePointer.nullPointer,
    NativePointer releaseUserData = NativePointer.nullPointer,
  }) {
    nativeRegistrations[this] = NativeRegistration(
      [lock, unlock],
      userData,
      releaseUserData,
    );
  }
}

/// Common attachment policy copied before an attach call returns.
///
/// See `mln_render_session_attach_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class RenderSessionAttachOptions extends _Value {
  const RenderSessionAttachOptions({
    required this.driver,
    this.requestedTextureRingDepth = 0,
    this.producerSyncKind = const GpuSyncKind.fromRawValue(0),
    this.frameWake = const Wake(),
    this.driverWorkWake = const Wake(),
    this.queueLock,
  });

  /// Where the session's driver runs. Required.
  ///
  /// An isolate cannot service a caller-graphics-thread driver, because Dart
  /// can resume it on another native thread after any `await`. Host native code
  /// services such a session on the graphics thread by passing its `id` to the
  /// C function that `RenderSessionHandle.serviceDriverWork()` calls.
  final RenderDriverKind driver;

  /// Requested slot count of a session-owned texture ring, up to 8. Zero
  /// selects 2, so the host can retain one frame while the session renders its
  /// replacement. Private targets grant one slot regardless of this value. A
  /// borrowed texture ring's depth is its texture count, so borrowed and other
  /// targets ignore this value.
  final int requestedTextureRingDepth;

  /// Producer synchronization for acquired texture frames.
  /// `GpuSyncKind.openGlFence` requires a shared EGL or WGL texture target and
  /// a caller-graphics-thread driver. Other targets support only CPU completion
  /// and fail an unsupported request.
  final GpuSyncKind producerSyncKind;

  /// Wakes the receiver when the frame-result queue becomes nonempty.
  final Wake frameWake;

  /// Wakes the graphics receiver when caller-driver work is available,
  /// including work that a finished driver result releases.
  final Wake driverWorkWake;

  /// Host lock on the graphics queue, disabled by default. Only Vulkan targets
  /// accept an enabled lock; other backends fail the attach with
  /// `Status.unsupported`.
  final QueueLock? queueLock;

  /// A copy of this value with the given members replaced.
  RenderSessionAttachOptions copyWith({
    RenderDriverKind? driver,
    int? requestedTextureRingDepth,
    GpuSyncKind? producerSyncKind,
    Wake? frameWake,
    Wake? driverWorkWake,
    QueueLock? queueLock,
  }) => RenderSessionAttachOptions(
    driver: driver ?? this.driver,
    requestedTextureRingDepth:
        requestedTextureRingDepth ?? this.requestedTextureRingDepth,
    producerSyncKind: producerSyncKind ?? this.producerSyncKind,
    frameWake: frameWake ?? this.frameWake,
    driverWorkWake: driverWorkWake ?? this.driverWorkWake,
    queueLock: queueLock ?? this.queueLock,
  );

  @override
  List<Object?> get _members => [
    driver,
    requestedTextureRingDepth,
    producerSyncKind,
    frameWake,
    driverWorkWake,
    queueLock,
  ];
  @override
  String toString() => _describe('RenderSessionAttachOptions', const [
    'driver',
    'requestedTextureRingDepth',
    'producerSyncKind',
    'frameWake',
    'driverWorkWake',
    'queueLock',
  ]);
}

/// One caller-owned Metal texture of a borrowed texture ring.
///
/// See `mln_metal_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class MetalBorrowedTexture extends _Value {
  const MetalBorrowedTexture({required this.texture});

  /// Borrowed `id<MTLTexture>` / `MTL::Texture*`. Required.
  final NativePointer texture;

  /// A copy of this value with the given members replaced.
  MetalBorrowedTexture copyWith({NativePointer? texture}) =>
      MetalBorrowedTexture(texture: texture ?? this.texture);

  @override
  List<Object?> get _members => [texture];
  @override
  String toString() => _describe('MetalBorrowedTexture', const ['texture']);
}

/// Metal attachment options for a borrowed texture target.
///
/// See `mln_metal_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class MetalBorrowedTextureDescriptor extends _Value {
  MetalBorrowedTextureDescriptor({
    required this.extent,
    required this.physicalWidth,
    required this.physicalHeight,
    required List<MetalBorrowedTexture> textures,
  }) : textures = List.unmodifiable(textures);

  /// Logical texture extent. Required. physical_width and physical_height give
  /// the texture's size. A scale_factor that differs from the map's is accepted
  /// and logged as a warning.
  final LogicalExtent extent;

  /// Physical texture width in device pixels. Required.
  final int physicalWidth;

  /// Physical texture height in device pixels. Required.
  final int physicalHeight;

  /// The ring's textures, one per slot, in slot order. Required.
  final List<MetalBorrowedTexture> textures;

  /// A copy of this value with the given members replaced.
  MetalBorrowedTextureDescriptor copyWith({
    LogicalExtent? extent,
    int? physicalWidth,
    int? physicalHeight,
    List<MetalBorrowedTexture>? textures,
  }) => MetalBorrowedTextureDescriptor(
    extent: extent ?? this.extent,
    physicalWidth: physicalWidth ?? this.physicalWidth,
    physicalHeight: physicalHeight ?? this.physicalHeight,
    textures: textures ?? this.textures,
  );

  @override
  List<Object?> get _members => [
    extent,
    physicalWidth,
    physicalHeight,
    textures,
  ];
  @override
  String toString() => _describe('MetalBorrowedTextureDescriptor', const [
    'extent',
    'physicalWidth',
    'physicalHeight',
    'textures',
  ]);
}

/// Metal backend context fields shared by Metal render targets.
///
/// See `mln_metal_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class MetalContextDescriptor extends _Value {
  const MetalContextDescriptor({this.device = NativePointer.nullPointer});

  /// `id<MTLDevice>` / `MTL::Device*`. Retained when the target requires it.
  final NativePointer device;

  /// A copy of this value with the given members replaced.
  MetalContextDescriptor copyWith({NativePointer? device}) =>
      MetalContextDescriptor(device: device ?? this.device);

  @override
  List<Object?> get _members => [device];
  @override
  String toString() => _describe('MetalContextDescriptor', const ['device']);
}

/// Metal attachment options for an owned texture target.
///
/// See `mln_metal_owned_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class MetalOwnedTextureDescriptor extends _Value {
  const MetalOwnedTextureDescriptor({
    required this.extent,
    this.context = const MetalContextDescriptor(),
    required this.pixelFormat,
  });

  /// Logical texture extent. Required. A scale_factor that differs from the
  /// map's is accepted and logged as a warning.
  final LogicalExtent extent;

  /// Metal backend context. device is required.
  final MetalContextDescriptor context;

  /// Backend-native MTLPixelFormat value of the ring's textures: either
  /// MTLPixelFormatRGBA8Unorm or MTLPixelFormatBGRA8Unorm. Required. Readback
  /// returns RGBA8 for either format.
  final int pixelFormat;

  /// A copy of this value with the given members replaced.
  MetalOwnedTextureDescriptor copyWith({
    LogicalExtent? extent,
    MetalContextDescriptor? context,
    int? pixelFormat,
  }) => MetalOwnedTextureDescriptor(
    extent: extent ?? this.extent,
    context: context ?? this.context,
    pixelFormat: pixelFormat ?? this.pixelFormat,
  );

  @override
  List<Object?> get _members => [extent, context, pixelFormat];
  @override
  String toString() => _describe('MetalOwnedTextureDescriptor', const [
    'extent',
    'context',
    'pixelFormat',
  ]);
}

/// Metal attachment options for a native surface.
///
/// See `mln_metal_surface_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
final class MetalSurfaceDescriptor extends _Value {
  const MetalSurfaceDescriptor({
    required this.extent,
    this.context = const MetalContextDescriptor(),
    required this.layer,
  });

  /// Logical surface extent. Required. A scale_factor that differs from the
  /// map's is accepted and logged as a warning.
  final LogicalExtent extent;

  /// Metal backend context. device is optional for Metal surfaces.
  final MetalContextDescriptor context;

  /// `CAMetalLayer*` / `CA::MetalLayer*` retained by the session. Required.
  final NativePointer layer;

  /// A copy of this value with the given members replaced.
  MetalSurfaceDescriptor copyWith({
    LogicalExtent? extent,
    MetalContextDescriptor? context,
    NativePointer? layer,
  }) => MetalSurfaceDescriptor(
    extent: extent ?? this.extent,
    context: context ?? this.context,
    layer: layer ?? this.layer,
  );

  @override
  List<Object?> get _members => [extent, context, layer];
  @override
  String toString() =>
      _describe('MetalSurfaceDescriptor', const ['extent', 'context', 'layer']);
}

/// WGL context fields shared by OpenGL render targets on Windows.
///
/// See `mln_wgl_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class WglContextDescriptor extends _Value {
  const WglContextDescriptor({
    required this.deviceContext,
    this.shareContext = NativePointer.nullPointer,
    this.getProcAddress = NativePointer.nullPointer,
  });

  /// Borrowed HDC used to create the session context. Required. A dedicated
  /// owned texture makes its context current on this HDC from its core worker.
  final NativePointer deviceContext;

  /// Borrowed HGLRC whose share group the session context joins. Required under
  /// shared ownership. A dedicated session joins no share group, so it must be
  /// null there.
  final NativePointer shareContext;

  /// Optional wglGetProcAddress-compatible function for the host loader.
  final NativePointer getProcAddress;

  /// A copy of this value with the given members replaced.
  WglContextDescriptor copyWith({
    NativePointer? deviceContext,
    NativePointer? shareContext,
    NativePointer? getProcAddress,
  }) => WglContextDescriptor(
    deviceContext: deviceContext ?? this.deviceContext,
    shareContext: shareContext ?? this.shareContext,
    getProcAddress: getProcAddress ?? this.getProcAddress,
  );

  @override
  List<Object?> get _members => [deviceContext, shareContext, getProcAddress];
  @override
  String toString() => _describe('WglContextDescriptor', const [
    'deviceContext',
    'shareContext',
    'getProcAddress',
  ]);
}

/// EGL context fields shared by OpenGL render targets.
///
/// See `mln_egl_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class EglContextDescriptor extends _Value {
  const EglContextDescriptor({
    required this.display,
    required this.config,
    this.shareContext = NativePointer.nullPointer,
    this.clientApi = const OpenGlClientApi.fromRawValue(0),
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
  final OpenGlClientApi clientApi;

  /// Optional eglGetProcAddress-compatible function for the host loader.
  final NativePointer getProcAddress;

  /// A copy of this value with the given members replaced.
  EglContextDescriptor copyWith({
    NativePointer? display,
    NativePointer? config,
    NativePointer? shareContext,
    OpenGlClientApi? clientApi,
    NativePointer? getProcAddress,
  }) => EglContextDescriptor(
    display: display ?? this.display,
    config: config ?? this.config,
    shareContext: shareContext ?? this.shareContext,
    clientApi: clientApi ?? this.clientApi,
    getProcAddress: getProcAddress ?? this.getProcAddress,
  );

  @override
  List<Object?> get _members => [
    display,
    config,
    shareContext,
    clientApi,
    getProcAddress,
  ];
  @override
  String toString() => _describe('EglContextDescriptor', const [
    'display',
    'config',
    'shareContext',
    'clientApi',
    'getProcAddress',
  ]);
}

/// WebGL context fields shared by OpenGL render targets in the browser.
///
/// See `mln_web_gl_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class WebGlContextDescriptor extends _Value {
  const WebGlContextDescriptor({
    this.kind = const WebGlContextKind.fromRawValue(0),
    this.context = 0,
    this.canvasSelector = '',
  });
  final WebGlContextKind kind;

  /// Borrowed EMSCRIPTEN_WEBGL_CONTEXT_HANDLE for EXISTING. Must be positive.
  final int context;

  /// Copied UTF-8 Emscripten target selector for TRANSFERRED_CANVAS. The HTML
  /// canvas must still be transferable when attachment starts.
  final String canvasSelector;

  /// A copy of this value with the given members replaced.
  WebGlContextDescriptor copyWith({
    WebGlContextKind? kind,
    int? context,
    String? canvasSelector,
  }) => WebGlContextDescriptor(
    kind: kind ?? this.kind,
    context: context ?? this.context,
    canvasSelector: canvasSelector ?? this.canvasSelector,
  );

  @override
  List<Object?> get _members => [kind, context, canvasSelector];
  @override
  String toString() => _describe('WebGlContextDescriptor', const [
    'kind',
    'context',
    'canvasSelector',
  ]);
}

/// OpenGL backend context fields shared by OpenGL render targets.
///
/// See `mln_open_gl_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class OpenGlContextDescriptor {
  const OpenGlContextDescriptor({
    this.ownership = const OpenGlContextOwnership.fromRawValue(0),
    required this.data,
  });

  /// Whether the session shares its driver thread and graphics objects with the
  /// host. A private EGL or WGL owned texture and a transferred WebGL canvas
  /// are dedicated to their core worker.
  final OpenGlContextOwnership ownership;
  final OpenGlContextDescriptorData data;
}

sealed class OpenGlContextDescriptorData {
  const OpenGlContextDescriptorData._();
}

final class OpenGlContextDescriptorDataWgl extends OpenGlContextDescriptorData {
  const OpenGlContextDescriptorDataWgl(this.value) : super._();
  final WglContextDescriptor value;
}

final class OpenGlContextDescriptorDataEgl extends OpenGlContextDescriptorData {
  const OpenGlContextDescriptorDataEgl(this.value) : super._();
  final EglContextDescriptor value;
}

final class OpenGlContextDescriptorDataWebGl
    extends OpenGlContextDescriptorData {
  const OpenGlContextDescriptorDataWebGl(this.value) : super._();
  final WebGlContextDescriptor value;
}

/// One caller-owned OpenGL texture of a borrowed texture ring.
///
/// See `mln_open_gl_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class OpenGlBorrowedTexture extends _Value {
  const OpenGlBorrowedTexture({required this.texture});

  /// Borrowed OpenGL texture object name. Required.
  final int texture;

  /// A copy of this value with the given members replaced.
  OpenGlBorrowedTexture copyWith({int? texture}) =>
      OpenGlBorrowedTexture(texture: texture ?? this.texture);

  @override
  List<Object?> get _members => [texture];
  @override
  String toString() => _describe('OpenGlBorrowedTexture', const ['texture']);
}

/// OpenGL attachment options for a borrowed texture target.
///
/// See `mln_open_gl_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class OpenGlBorrowedTextureDescriptor extends _Value {
  OpenGlBorrowedTextureDescriptor({
    required this.extent,
    required this.physicalWidth,
    required this.physicalHeight,
    required this.context,
    required List<OpenGlBorrowedTexture> textures,
    required this.target,
  }) : textures = List.unmodifiable(textures);

  /// Logical texture extent. Required. physical_width and physical_height give
  /// the texture's size. A scale_factor that differs from the map's is accepted
  /// and logged as a warning.
  final LogicalExtent extent;

  /// Physical texture width in device pixels. Required.
  final int physicalWidth;

  /// Physical texture height in device pixels. Required.
  final int physicalHeight;

  /// Borrowed OpenGL context provider data. The textures must belong to this
  /// context or a context in the same share group.
  final OpenGlContextDescriptor context;

  /// The ring's textures, one per slot, in slot order. Required.
  final List<OpenGlBorrowedTexture> textures;

  /// OpenGL texture target of every texture. Required: GL_TEXTURE_2D.
  final int target;

  /// A copy of this value with the given members replaced.
  OpenGlBorrowedTextureDescriptor copyWith({
    LogicalExtent? extent,
    int? physicalWidth,
    int? physicalHeight,
    OpenGlContextDescriptor? context,
    List<OpenGlBorrowedTexture>? textures,
    int? target,
  }) => OpenGlBorrowedTextureDescriptor(
    extent: extent ?? this.extent,
    physicalWidth: physicalWidth ?? this.physicalWidth,
    physicalHeight: physicalHeight ?? this.physicalHeight,
    context: context ?? this.context,
    textures: textures ?? this.textures,
    target: target ?? this.target,
  );

  @override
  List<Object?> get _members => [
    extent,
    physicalWidth,
    physicalHeight,
    context,
    textures,
    target,
  ];
  @override
  String toString() => _describe('OpenGlBorrowedTextureDescriptor', const [
    'extent',
    'physicalWidth',
    'physicalHeight',
    'context',
    'textures',
    'target',
  ]);
}

/// OpenGL attachment options for an owned texture target.
///
/// See `mln_open_gl_owned_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class OpenGlOwnedTextureDescriptor extends _Value {
  const OpenGlOwnedTextureDescriptor({
    required this.extent,
    required this.context,
  });

  /// Logical texture extent. Required. A scale_factor that differs from the
  /// map's is accepted and logged as a warning.
  final LogicalExtent extent;

  /// Borrowed OpenGL context provider data. Shared ownership creates a context
  /// whose texture frames the host can acquire. Dedicated EGL, WGL, or
  /// transferred WebGL ownership creates a private core-worker context for CPU
  /// readback.
  final OpenGlContextDescriptor context;

  /// A copy of this value with the given members replaced.
  OpenGlOwnedTextureDescriptor copyWith({
    LogicalExtent? extent,
    OpenGlContextDescriptor? context,
  }) => OpenGlOwnedTextureDescriptor(
    extent: extent ?? this.extent,
    context: context ?? this.context,
  );

  @override
  List<Object?> get _members => [extent, context];
  @override
  String toString() =>
      _describe('OpenGlOwnedTextureDescriptor', const ['extent', 'context']);
}

/// OpenGL attachment options for a native surface.
///
/// See `mln_open_gl_surface_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
final class OpenGlSurfaceDescriptor extends _Value {
  const OpenGlSurfaceDescriptor({
    required this.extent,
    required this.context,
    this.surface = NativePointer.nullPointer,
  });

  /// Logical surface extent. Required. A scale_factor that differs from the
  /// map's is accepted and logged as a warning.
  final LogicalExtent extent;

  /// Borrowed OpenGL context provider data.
  final OpenGlContextDescriptor context;

  /// Borrowed platform surface handle: an HDC for WGL and an EGLSurface for
  /// EGL, both required. Null for WebGL, whose context carries its canvas
  /// binding.
  final NativePointer surface;

  /// A copy of this value with the given members replaced.
  OpenGlSurfaceDescriptor copyWith({
    LogicalExtent? extent,
    OpenGlContextDescriptor? context,
    NativePointer? surface,
  }) => OpenGlSurfaceDescriptor(
    extent: extent ?? this.extent,
    context: context ?? this.context,
    surface: surface ?? this.surface,
  );

  @override
  List<Object?> get _members => [extent, context, surface];
  @override
  String toString() => _describe('OpenGlSurfaceDescriptor', const [
    'extent',
    'context',
    'surface',
  ]);
}

/// Vulkan backend context fields shared by Vulkan render targets.
///
/// See `mln_vulkan_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class VulkanContextDescriptor extends _Value {
  const VulkanContextDescriptor({
    required this.instance,
    required this.physicalDevice,
    required this.device,
    required this.graphicsQueue,
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
  /// `RenderSessionAttachOptions.queueLock` at attach.
  final NativePointer graphicsQueue;

  /// Queue family index for graphics_queue. Must support graphics commands.
  final int graphicsQueueFamilyIndex;

  /// PFN_vkGetInstanceProcAddr for the loader that created the instance.
  final NativePointer getInstanceProcAddr;

  /// PFN_vkGetDeviceProcAddr for the loader that created the Vulkan device.
  final NativePointer getDeviceProcAddr;

  /// A copy of this value with the given members replaced.
  VulkanContextDescriptor copyWith({
    NativePointer? instance,
    NativePointer? physicalDevice,
    NativePointer? device,
    NativePointer? graphicsQueue,
    int? graphicsQueueFamilyIndex,
    NativePointer? getInstanceProcAddr,
    NativePointer? getDeviceProcAddr,
  }) => VulkanContextDescriptor(
    instance: instance ?? this.instance,
    physicalDevice: physicalDevice ?? this.physicalDevice,
    device: device ?? this.device,
    graphicsQueue: graphicsQueue ?? this.graphicsQueue,
    graphicsQueueFamilyIndex:
        graphicsQueueFamilyIndex ?? this.graphicsQueueFamilyIndex,
    getInstanceProcAddr: getInstanceProcAddr ?? this.getInstanceProcAddr,
    getDeviceProcAddr: getDeviceProcAddr ?? this.getDeviceProcAddr,
  );

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
  @override
  String toString() => _describe('VulkanContextDescriptor', const [
    'instance',
    'physicalDevice',
    'device',
    'graphicsQueue',
    'graphicsQueueFamilyIndex',
    'getInstanceProcAddr',
    'getDeviceProcAddr',
  ]);
}

/// One caller-owned Vulkan image of a borrowed texture ring.
///
/// See `mln_vulkan_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class VulkanBorrowedTexture extends _Value {
  const VulkanBorrowedTexture({required this.image, required this.imageView});

  /// Borrowed VkImage. Required.
  final int image;

  /// Borrowed VkImageView for image. Required. The view must be a 2D color view
  /// that matches image and the descriptor's format.
  final int imageView;

  /// A copy of this value with the given members replaced.
  VulkanBorrowedTexture copyWith({int? image, int? imageView}) =>
      VulkanBorrowedTexture(
        image: image ?? this.image,
        imageView: imageView ?? this.imageView,
      );

  @override
  List<Object?> get _members => [image, imageView];
  @override
  String toString() =>
      _describe('VulkanBorrowedTexture', const ['image', 'imageView']);
}

/// Vulkan attachment options for a borrowed texture target.
///
/// See `mln_vulkan_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class VulkanBorrowedTextureDescriptor extends _Value {
  VulkanBorrowedTextureDescriptor({
    required this.extent,
    required this.physicalWidth,
    required this.physicalHeight,
    required this.context,
    required List<VulkanBorrowedTexture> textures,
    required this.format,
    this.initialLayout = 0,
    required this.finalLayout,
  }) : textures = List.unmodifiable(textures);

  /// Logical texture extent. Required. physical_width and physical_height give
  /// the texture's size. A scale_factor that differs from the map's is accepted
  /// and logged as a warning.
  final LogicalExtent extent;

  /// Physical image width in device pixels. Required.
  final int physicalWidth;

  /// Physical image height in device pixels. Required.
  final int physicalHeight;

  /// Borrowed Vulkan context. All handles are required.
  final VulkanContextDescriptor context;

  /// The ring's images, one per slot, in slot order. Required.
  final List<VulkanBorrowedTexture> textures;

  /// Backend-native VkFormat value of every image. Required, so
  /// VK_FORMAT_UNDEFINED is invalid.
  final int format;

  /// Backend-native VkImageLayout value expected at render-pass begin.
  final int initialLayout;

  /// Backend-native VkImageLayout value left after rendering succeeds, such as
  /// VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL. Required, so
  /// VK_IMAGE_LAYOUT_UNDEFINED is invalid.
  final int finalLayout;

  /// A copy of this value with the given members replaced.
  VulkanBorrowedTextureDescriptor copyWith({
    LogicalExtent? extent,
    int? physicalWidth,
    int? physicalHeight,
    VulkanContextDescriptor? context,
    List<VulkanBorrowedTexture>? textures,
    int? format,
    int? initialLayout,
    int? finalLayout,
  }) => VulkanBorrowedTextureDescriptor(
    extent: extent ?? this.extent,
    physicalWidth: physicalWidth ?? this.physicalWidth,
    physicalHeight: physicalHeight ?? this.physicalHeight,
    context: context ?? this.context,
    textures: textures ?? this.textures,
    format: format ?? this.format,
    initialLayout: initialLayout ?? this.initialLayout,
    finalLayout: finalLayout ?? this.finalLayout,
  );

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
  @override
  String toString() => _describe('VulkanBorrowedTextureDescriptor', const [
    'extent',
    'physicalWidth',
    'physicalHeight',
    'context',
    'textures',
    'format',
    'initialLayout',
    'finalLayout',
  ]);
}

/// Vulkan attachment options for an owned texture target.
///
/// See `mln_vulkan_owned_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class VulkanOwnedTextureDescriptor extends _Value {
  const VulkanOwnedTextureDescriptor({
    required this.extent,
    required this.context,
  });

  /// Logical texture extent. Required. A scale_factor that differs from the
  /// map's is accepted and logged as a warning.
  final LogicalExtent extent;

  /// Borrowed Vulkan context. All handles are required.
  final VulkanContextDescriptor context;

  /// A copy of this value with the given members replaced.
  VulkanOwnedTextureDescriptor copyWith({
    LogicalExtent? extent,
    VulkanContextDescriptor? context,
  }) => VulkanOwnedTextureDescriptor(
    extent: extent ?? this.extent,
    context: context ?? this.context,
  );

  @override
  List<Object?> get _members => [extent, context];
  @override
  String toString() =>
      _describe('VulkanOwnedTextureDescriptor', const ['extent', 'context']);
}

/// Vulkan attachment options for a native surface.
///
/// See `mln_vulkan_surface_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
final class VulkanSurfaceDescriptor extends _Value {
  const VulkanSurfaceDescriptor({
    required this.extent,
    required this.context,
    required this.surface,
  });

  /// Logical surface extent. Required. A scale_factor that differs from the
  /// map's is accepted and logged as a warning.
  final LogicalExtent extent;

  /// Borrowed Vulkan context. All handles are required. The device must support
  /// VK_KHR_swapchain, and the queue family must support graphics and
  /// presentation to this descriptor's surface.
  final VulkanContextDescriptor context;

  /// Borrowed VkSurfaceKHR bit pattern. Required.
  final int surface;

  /// A copy of this value with the given members replaced.
  VulkanSurfaceDescriptor copyWith({
    LogicalExtent? extent,
    VulkanContextDescriptor? context,
    int? surface,
  }) => VulkanSurfaceDescriptor(
    extent: extent ?? this.extent,
    context: context ?? this.context,
    surface: surface ?? this.surface,
  );

  @override
  List<Object?> get _members => [extent, context, surface];
  @override
  String toString() => _describe('VulkanSurfaceDescriptor', const [
    'extent',
    'context',
    'surface',
  ]);
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

  /// A copy of this value with the given members replaced.
  CameraFitOptions copyWith({
    EdgeInsets? padding,
    double? bearing,
    double? pitch,
  }) => CameraFitOptions(
    padding: padding ?? this.padding,
    bearing: bearing ?? this.bearing,
    pitch: pitch ?? this.pitch,
  );

  @override
  List<Object?> get _members => [padding, bearing, pitch];
  @override
  String toString() =>
      _describe('CameraFitOptions', const ['padding', 'bearing', 'pitch']);
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

  /// A copy of this value with the given members replaced.
  CameraOptions copyWith({
    LatLng? center,
    double? centerAltitude,
    EdgeInsets? padding,
    ScreenPoint? anchor,
    double? zoom,
    double? bearing,
    double? pitch,
    double? roll,
    double? fieldOfView,
  }) => CameraOptions(
    center: center ?? this.center,
    centerAltitude: centerAltitude ?? this.centerAltitude,
    padding: padding ?? this.padding,
    anchor: anchor ?? this.anchor,
    zoom: zoom ?? this.zoom,
    bearing: bearing ?? this.bearing,
    pitch: pitch ?? this.pitch,
    roll: roll ?? this.roll,
    fieldOfView: fieldOfView ?? this.fieldOfView,
  );

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
  @override
  String toString() => _describe('CameraOptions', const [
    'center',
    'centerAltitude',
    'padding',
    'anchor',
    'zoom',
    'bearing',
    'pitch',
    'roll',
    'fieldOfView',
  ]);
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

  /// A copy of this value with the given members replaced.
  LatLngBounds copyWith({LatLng? southwest, LatLng? northeast}) => LatLngBounds(
    southwest: southwest ?? this.southwest,
    northeast: northeast ?? this.northeast,
  );

  @override
  List<Object?> get _members => [southwest, northeast];
  @override
  String toString() =>
      _describe('LatLngBounds', const ['southwest', 'northeast']);
}

/// Camera result of an ordered camera query.
///
/// See `mln_camera_query_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraQueryResult extends _Value {
  const CameraQueryResult({this.generation = 0, this.camera = const Camera()});
  final int generation;
  final Camera camera;

  @override
  List<Object?> get _members => [generation, camera];
  @override
  String toString() =>
      _describe('CameraQueryResult', const ['generation', 'camera']);
}

/// Feature-state source, feature, and key selector.
///
/// See `mln_feature_state_selector` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class FeatureStateSelector extends _Value {
  const FeatureStateSelector({
    required this.sourceId,
    this.sourceLayerId = '',
    this.featureId,
    this.stateKey = '',
  });

  /// Source ID. Required.
  final String sourceId;

  /// Source layer ID, or empty for none. Required for vector-source
  /// disambiguation.
  final String sourceLayerId;

  /// Optional feature ID. Required by set and get, optional for remove. A
  /// numeric feature ID matches its decimal text, such as "7" for the GeoJSON
  /// id 7. Its field bit tells an absent ID from the empty ID "".
  final String? featureId;

  /// State key, or empty to select every key. Used only by remove, and requires
  /// feature_id.
  final String stateKey;

  /// A copy of this value with the given members replaced.
  FeatureStateSelector copyWith({
    String? sourceId,
    String? sourceLayerId,
    String? featureId,
    String? stateKey,
  }) => FeatureStateSelector(
    sourceId: sourceId ?? this.sourceId,
    sourceLayerId: sourceLayerId ?? this.sourceLayerId,
    featureId: featureId ?? this.featureId,
    stateKey: stateKey ?? this.stateKey,
  );

  @override
  List<Object?> get _members => [sourceId, sourceLayerId, featureId, stateKey];
  @override
  String toString() => _describe('FeatureStateSelector', const [
    'sourceId',
    'sourceLayerId',
    'featureId',
    'stateKey',
  ]);
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

  /// Prefetch zoom delta, at most 255.
  final int? prefetchZoomDelta;
  final double? lodMinRadius;
  final double? lodScale;
  final double? lodPitchThreshold;
  final double? lodZoomShift;
  final TileLodMode? lodMode;

  /// A copy of this value with the given members replaced.
  MapTileOptions copyWith({
    int? prefetchZoomDelta,
    double? lodMinRadius,
    double? lodScale,
    double? lodPitchThreshold,
    double? lodZoomShift,
    TileLodMode? lodMode,
  }) => MapTileOptions(
    prefetchZoomDelta: prefetchZoomDelta ?? this.prefetchZoomDelta,
    lodMinRadius: lodMinRadius ?? this.lodMinRadius,
    lodScale: lodScale ?? this.lodScale,
    lodPitchThreshold: lodPitchThreshold ?? this.lodPitchThreshold,
    lodZoomShift: lodZoomShift ?? this.lodZoomShift,
    lodMode: lodMode ?? this.lodMode,
  );

  @override
  List<Object?> get _members => [
    prefetchZoomDelta,
    lodMinRadius,
    lodScale,
    lodPitchThreshold,
    lodZoomShift,
    lodMode,
  ];
  @override
  String toString() => _describe('MapTileOptions', const [
    'prefetchZoomDelta',
    'lodMinRadius',
    'lodScale',
    'lodPitchThreshold',
    'lodZoomShift',
    'lodMode',
  ]);
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

  /// Geographic bounds that the camera center stays inside.
  final LatLngBounds? bounds;
  final double? minZoom;
  final double? maxZoom;
  final double? minPitch;
  final double? maxPitch;

  /// Selects the unbounded geographic constraint, which leaves every camera
  /// center unconstrained and lets the map pan freely across the antimeridian.
  /// This differs from world bounds of -90/-180 to 90/180, which clamp
  /// longitude to that range. Mutually exclusive with `BoundOptions.bounds`,
  /// which it leaves unread.
  final bool unbounded;

  /// A copy of this value with the given members replaced.
  BoundOptions copyWith({
    LatLngBounds? bounds,
    double? minZoom,
    double? maxZoom,
    double? minPitch,
    double? maxPitch,
    bool? unbounded,
  }) => BoundOptions(
    bounds: bounds ?? this.bounds,
    minZoom: minZoom ?? this.minZoom,
    maxZoom: maxZoom ?? this.maxZoom,
    minPitch: minPitch ?? this.minPitch,
    maxPitch: maxPitch ?? this.maxPitch,
    unbounded: unbounded ?? this.unbounded,
  );

  @override
  List<Object?> get _members => [
    bounds,
    minZoom,
    maxZoom,
    minPitch,
    maxPitch,
    unbounded,
  ];
  @override
  String toString() => _describe('BoundOptions', const [
    'bounds',
    'minZoom',
    'maxZoom',
    'minPitch',
    'maxPitch',
    'unbounded',
  ]);
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

  /// A copy of this value with the given members replaced.
  Vec3 copyWith({double? x, double? y, double? z}) =>
      Vec3(x: x ?? this.x, y: y ?? this.y, z: z ?? this.z);

  @override
  List<Object?> get _members => [x, y, z];
  @override
  String toString() => _describe('Vec3', const ['x', 'y', 'z']);
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

  /// A copy of this value with the given members replaced.
  Quaternion copyWith({double? x, double? y, double? z, double? w}) =>
      Quaternion(
        x: x ?? this.x,
        y: y ?? this.y,
        z: z ?? this.z,
        w: w ?? this.w,
      );

  @override
  List<Object?> get _members => [x, y, z, w];
  @override
  String toString() => _describe('Quaternion', const ['x', 'y', 'z', 'w']);
}

/// Free camera position and orientation in MapLibre Native camera space.
///
/// See `mln_free_camera_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class FreeCameraOptions extends _Value {
  const FreeCameraOptions({this.position, this.orientation});
  final Vec3? position;
  final Quaternion? orientation;

  /// A copy of this value with the given members replaced.
  FreeCameraOptions copyWith({Vec3? position, Quaternion? orientation}) =>
      FreeCameraOptions(
        position: position ?? this.position,
        orientation: orientation ?? this.orientation,
      );

  @override
  List<Object?> get _members => [position, orientation];
  @override
  String toString() =>
      _describe('FreeCameraOptions', const ['position', 'orientation']);
}

/// Immutable map state copied from the latest published generation.
///
/// See `mln_map_snapshot` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class MapSnapshot extends _Value {
  const MapSnapshot({
    this.debugOptions = MapDebugFlag.none,
    this.generation = 0,
    this.fullyLoaded = false,
    this.renderingStatsViewEnabled = false,
    this.gestureInProgress = false,
    this.transitionInProgress = false,
    this.eventMask = RuntimeEventFlag.none,
    this.latestRenderUpdateGeneration = 0,
    this.tile = const MapTileOptions(),
    this.bounds = const BoundOptions(),
    this.freeCamera = const FreeCameraOptions(),
    this.view = const MapView(),
  });

  /// Debug overlays that the map draws.
  final MapDebugFlag debugOptions;
  final int generation;

  /// True once every requested style and tile resource finished loading.
  final bool fullyLoaded;
  final bool renderingStatsViewEnabled;

  /// True while the map is inside a gesture.
  final bool gestureInProgress;

  /// True while a camera command with a duration is still animating.
  final bool transitionInProgress;
  final RuntimeEventFlag eventMask;

  /// Generation of the latest render update the map published. A rendered frame
  /// at or past it draws map state that includes every command this snapshot
  /// observes, though animations and resource loads finish in later frames.
  final int latestRenderUpdateGeneration;
  final MapTileOptions tile;
  final BoundOptions bounds;
  final FreeCameraOptions freeCamera;

  /// The map's camera, extent, projection mode, and viewport options.
  /// `MapProjectionHandle.create()` converts coordinates for this view.
  final MapView view;

  @override
  List<Object?> get _members => [
    debugOptions,
    generation,
    fullyLoaded,
    renderingStatsViewEnabled,
    gestureInProgress,
    transitionInProgress,
    eventMask,
    latestRenderUpdateGeneration,
    tile,
    bounds,
    freeCamera,
    view,
  ];
  @override
  String toString() => _describe('MapSnapshot', const [
    'debugOptions',
    'generation',
    'fullyLoaded',
    'renderingStatsViewEnabled',
    'gestureInProgress',
    'transitionInProgress',
    'eventMask',
    'latestRenderUpdateGeneration',
    'tile',
    'bounds',
    'freeCamera',
    'view',
  ]);
}

/// One stretchable interval along an image axis, in image pixels.
///
/// See `mln_image_stretch` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class ImageStretch extends _Value {
  const ImageStretch({this.from = 0, this.to = 0});
  final double from;
  final double to;

  /// A copy of this value with the given members replaced.
  ImageStretch copyWith({double? from, double? to}) =>
      ImageStretch(from: from ?? this.from, to: to ?? this.to);

  @override
  List<Object?> get _members => [from, to];
  @override
  String toString() => _describe('ImageStretch', const ['from', 'to']);
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

  /// A copy of this value with the given members replaced.
  ImageContent copyWith({
    double? left,
    double? top,
    double? right,
    double? bottom,
  }) => ImageContent(
    left: left ?? this.left,
    top: top ?? this.top,
    right: right ?? this.right,
    bottom: bottom ?? this.bottom,
  );

  @override
  List<Object?> get _members => [left, top, right, bottom];
  @override
  String toString() =>
      _describe('ImageContent', const ['left', 'top', 'right', 'bottom']);
}

/// One complete runtime style image.
///
/// See `mln_style_image_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleImageInfo extends _Value {
  StyleImageInfo({
    this.width = 0,
    this.height = 0,
    Uint8List? pixels,
    List<ImageStretch> stretchX = const [],
    List<ImageStretch> stretchY = const [],
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
  @override
  String toString() => _describe('StyleImageInfo', const [
    'width',
    'height',
    'pixels',
    'stretchX',
    'stretchY',
    'content',
    'textFitWidth',
    'textFitHeight',
    'pixelRatio',
    'sdf',
  ]);
}

/// One style layer.
///
/// See `mln_style_layer_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleLayerInfo extends _Value {
  const StyleLayerInfo({
    this.id = '',
    this.type = '',
    this.sourceId = '',
    this.sourceLayer = '',
    this.minZoom = 0,
    this.maxZoom = 0,
    this.visibility = const StyleLayerVisibility.fromRawValue(0),
  });

  /// Layer ID.
  final String id;

  /// The style-spec layer type string.
  final String type;

  /// Source ID. Empty for a layer type that takes no source.
  final String sourceId;

  /// Source-layer ID. Empty when the layer sets none.
  final String sourceLayer;

  /// Lowest zoom at which the layer draws; -INFINITY with no lower bound.
  final double minZoom;

  /// Highest zoom at which the layer draws; INFINITY with no upper bound.
  final double maxZoom;
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
  @override
  String toString() => _describe('StyleLayerInfo', const [
    'id',
    'type',
    'sourceId',
    'sourceLayer',
    'minZoom',
    'maxZoom',
    'visibility',
  ]);
}

/// Inline TileJSON metadata of a tile source.
///
/// See `mln_style_source_tile_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleSourceTileInfo extends _Value {
  StyleSourceTileInfo({
    List<String> tileUrls = const [],
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
  final StyleTileScheme scheme;

  @override
  List<Object?> get _members => [tileUrls, minZoom, maxZoom, scheme];
  @override
  String toString() => _describe('StyleSourceTileInfo', const [
    'tileUrls',
    'minZoom',
    'maxZoom',
    'scheme',
  ]);
}

/// Complete metadata of one style source.
///
/// See `mln_style_source_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleSourceInfo extends _Value {
  const StyleSourceInfo({
    this.id = '',
    this.type = const StyleSourceType.fromRawValue(0),
    this.isVolatile = false,
    this.attribution = '',
    this.url = '',
    this.tileJson,
    this.bounds,
    this.tileSize,
    this.vectorEncoding,
    this.rasterEncoding,
  });

  /// Source ID.
  final String id;
  final StyleSourceType type;

  /// Whether the source is marked volatile.
  final bool isVolatile;

  /// Attribution string, or empty when the source sets none.
  final String attribution;

  /// URL that the source loads from, or empty when it has none.
  final String url;

  /// Inline TileJSON metadata, when the source was defined with it.
  final StyleSourceTileInfo? tileJson;

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
    tileJson,
    bounds,
    tileSize,
    vectorEncoding,
    rasterEncoding,
  ];
  @override
  String toString() => _describe('StyleSourceInfo', const [
    'id',
    'type',
    'isVolatile',
    'attribution',
    'url',
    'tileJson',
    'bounds',
    'tileSize',
    'vectorEncoding',
    'rasterEncoding',
  ]);
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

  /// Transition duration in milliseconds. Must be finite, non-negative, and
  /// short enough to represent as a duration.
  final double? durationMs;

  /// Transition delay in milliseconds. Must be finite, non-negative, and short
  /// enough to represent as a duration.
  final double? delayMs;

  /// Whether symbol placement changes cross-fade.
  final bool? enablePlacementTransitions;

  /// A copy of this value with the given members replaced.
  StyleTransitionOptions copyWith({
    double? durationMs,
    double? delayMs,
    bool? enablePlacementTransitions,
  }) => StyleTransitionOptions(
    durationMs: durationMs ?? this.durationMs,
    delayMs: delayMs ?? this.delayMs,
    enablePlacementTransitions:
        enablePlacementTransitions ?? this.enablePlacementTransitions,
  );

  @override
  List<Object?> get _members => [
    durationMs,
    delayMs,
    enablePlacementTransitions,
  ];
  @override
  String toString() => _describe('StyleTransitionOptions', const [
    'durationMs',
    'delayMs',
    'enablePlacementTransitions',
  ]);
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

  /// Horizontally stretchable intervals.
  final List<ImageStretch>? stretchX;

  /// Vertically stretchable intervals.
  final List<ImageStretch>? stretchY;

  /// Content box used when icon-text-fit applies.
  final ImageContent? content;

  /// Absent selects STRETCH_OR_SHRINK.
  final StyleImageTextFit? textFitWidth;

  /// Absent selects STRETCH_OR_SHRINK.
  final StyleImageTextFit? textFitHeight;

  /// Sprite pixel ratio. Absent selects 1.
  final double? pixelRatio;

  /// Whether the image is a signed distance field icon. Absent selects false.
  final bool? sdf;

  /// A copy of this value with the given members replaced.
  StyleImageOptions copyWith({
    List<ImageStretch>? stretchX,
    List<ImageStretch>? stretchY,
    ImageContent? content,
    StyleImageTextFit? textFitWidth,
    StyleImageTextFit? textFitHeight,
    double? pixelRatio,
    bool? sdf,
  }) => StyleImageOptions(
    stretchX: stretchX ?? this.stretchX,
    stretchY: stretchY ?? this.stretchY,
    content: content ?? this.content,
    textFitWidth: textFitWidth ?? this.textFitWidth,
    textFitHeight: textFitHeight ?? this.textFitHeight,
    pixelRatio: pixelRatio ?? this.pixelRatio,
    sdf: sdf ?? this.sdf,
  );

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
  @override
  String toString() => _describe('StyleImageOptions', const [
    'stretchX',
    'stretchY',
    'content',
    'textFitWidth',
    'textFitHeight',
    'pixelRatio',
    'sdf',
  ]);
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

  /// A copy of this value with the given members replaced.
  CameraUpdate copyWith({
    CameraUpdateMode? mode,
    CameraOptions? camera,
    AnimationOptions? animation,
    GesturePhase? gesturePhase,
  }) => CameraUpdate(
    mode: mode ?? this.mode,
    camera: camera ?? this.camera,
    animation: animation ?? this.animation,
    gesturePhase: gesturePhase ?? this.gesturePhase,
  );

  @override
  List<Object?> get _members => [mode, camera, animation, gesturePhase];
  @override
  String toString() => _describe('CameraUpdate', const [
    'mode',
    'camera',
    'animation',
    'gesturePhase',
  ]);
}

final class RenderAbandonResult extends _Value {
  const RenderAbandonResult({
    this.disposition = const RenderAbandonDisposition.fromRawValue(0),
    this.quarantinedResourceCount = 0,
  });
  final RenderAbandonDisposition disposition;

  /// Backend resource groups intentionally retained until process exit.
  final int quarantinedResourceCount;

  @override
  List<Object?> get _members => [disposition, quarantinedResourceCount];
  @override
  String toString() => _describe('RenderAbandonResult', const [
    'disposition',
    'quarantinedResourceCount',
  ]);
}

/// Driver and target capabilities fixed for one attached render session.
///
/// See `mln_render_session_capabilities` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class RenderSessionCapabilities extends _Value {
  const RenderSessionCapabilities({
    required this.driver,
    this.textureRingDepth = 0,
    this.flags = RenderSessionCapabilityFlag.none,
  });
  final RenderDriverKind driver;

  /// Granted texture ring depth: the slot count of a session-owned ring, or the
  /// texture count of a borrowed one. Zero for a surface.
  final int textureRingDepth;
  final RenderSessionCapabilityFlag flags;

  @override
  List<Object?> get _members => [driver, textureRingDepth, flags];
  @override
  String toString() => _describe('RenderSessionCapabilities', const [
    'driver',
    'textureRingDepth',
    'flags',
  ]);
}

/// Any-thread render-session snapshot.
///
/// See `mln_render_session_snapshot` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
final class RenderSessionSnapshot extends _Value {
  const RenderSessionSnapshot({
    required this.state,
    this.extent = const LogicalExtent(),
    this.generation = 0,
    this.mapUpdateGeneration = 0,
    this.renderedUpdateGeneration = 0,
    this.frameGeneration = 0,
    this.acquiredFrameCount = 0,
    this.pendingChanges = false,
  });
  final RenderSessionState state;

  /// Logical extent applied by the driver.
  final LogicalExtent extent;

  /// Increases when the session's lifecycle state, target, or applied extent
  /// changes. An acquired texture frame reports the value it rendered under.
  final int generation;

  /// Generation of the latest map render update that the session has seen.
  final int mapUpdateGeneration;

  /// Map render update generation that the latest rendered frame drew. Zero
  /// before the first frame after attachment or a target replacement.
  final int renderedUpdateGeneration;

  /// Frame generation of the latest rendered frame, zero before the first.
  final int frameGeneration;

  /// Acquired frames that the host has not released or discarded.
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
  @override
  String toString() => _describe('RenderSessionSnapshot', const [
    'state',
    'extent',
    'generation',
    'mapUpdateGeneration',
    'renderedUpdateGeneration',
    'frameGeneration',
    'acquiredFrameCount',
    'pendingChanges',
  ]);
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

  /// A copy of this value with the given members replaced.
  ScreenBox copyWith({ScreenPoint? min, ScreenPoint? max}) =>
      ScreenBox(min: min ?? this.min, max: max ?? this.max);

  @override
  List<Object?> get _members => [min, max];
  @override
  String toString() => _describe('ScreenBox', const ['min', 'max']);
}

/// Screen-space line string in logical map pixels.
///
/// See `mln_screen_line_string` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
final class ScreenLineString extends _Value {
  ScreenLineString({List<ScreenPoint> points = const []})
    : points = List.unmodifiable(points);

  /// Points.
  final List<ScreenPoint> points;

  /// A copy of this value with the given members replaced.
  ScreenLineString copyWith({List<ScreenPoint>? points}) =>
      ScreenLineString(points: points ?? this.points);

  @override
  List<Object?> get _members => [points];
  @override
  String toString() => _describe('ScreenLineString', const ['points']);
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
  RenderedFeatureQueryOptions({List<String>? layerIds, this.filter})
    : layerIds = layerIds == null ? null : List.unmodifiable(layerIds);

  /// Optional style layer IDs. When absent, all rendered layers are queried.
  final List<String>? layerIds;

  /// UTF-8 MapLibre style-spec filter JSON, or empty for no filter.
  final Object? filter;

  /// A copy of this value with the given members replaced.
  RenderedFeatureQueryOptions copyWith({
    List<String>? layerIds,
    Object? filter = _unchanged,
  }) => RenderedFeatureQueryOptions(
    layerIds: layerIds ?? this.layerIds,
    filter: identical(filter, _unchanged) ? this.filter : filter,
  );

  @override
  List<Object?> get _members => [layerIds, filter];
  @override
  String toString() =>
      _describe('RenderedFeatureQueryOptions', const ['layerIds', 'filter']);
}

/// One rendered-feature query hit.
///
/// See `mln_rendered_feature` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
final class RenderedFeature extends _Value {
  const RenderedFeature({
    this.feature,
    this.layerId = '',
    this.sourceId = '',
    this.sourceLayerId = '',
    this.state,
  });
  final Object? feature;
  final String layerId;
  final String sourceId;
  final String sourceLayerId;
  final Object? state;

  @override
  List<Object?> get _members => [
    feature,
    layerId,
    sourceId,
    sourceLayerId,
    state,
  ];
  @override
  String toString() => _describe('RenderedFeature', const [
    'feature',
    'layerId',
    'sourceId',
    'sourceLayerId',
    'state',
  ]);
}

/// Options for source feature queries.
///
/// See `mln_source_feature_query_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
final class SourceFeatureQueryOptions extends _Value {
  SourceFeatureQueryOptions({List<String>? sourceLayerIds, this.filter})
    : sourceLayerIds = sourceLayerIds == null
          ? null
          : List.unmodifiable(sourceLayerIds);

  /// Optional source-layer IDs. Required by vector sources; ignored by GeoJSON.
  final List<String>? sourceLayerIds;

  /// UTF-8 MapLibre style-spec filter JSON, or empty for no filter.
  final Object? filter;

  /// A copy of this value with the given members replaced.
  SourceFeatureQueryOptions copyWith({
    List<String>? sourceLayerIds,
    Object? filter = _unchanged,
  }) => SourceFeatureQueryOptions(
    sourceLayerIds: sourceLayerIds ?? this.sourceLayerIds,
    filter: identical(filter, _unchanged) ? this.filter : filter,
  );

  @override
  List<Object?> get _members => [sourceLayerIds, filter];
  @override
  String toString() => _describe('SourceFeatureQueryOptions', const [
    'sourceLayerIds',
    'filter',
  ]);
}

/// One source-feature query hit.
///
/// See `mln_source_feature` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
final class SourceFeature extends _Value {
  const SourceFeature({this.feature, this.sourceLayerId = ''});
  final Object? feature;
  final String sourceLayerId;

  @override
  List<Object?> get _members => [feature, sourceLayerId];
  @override
  String toString() =>
      _describe('SourceFeature', const ['feature', 'sourceLayerId']);
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
  @override
  String toString() => _describe('TextureImageInfo', const [
    'width',
    'height',
    'stride',
    'byteLength',
  ]);
}

/// Texture readback.
///
/// See `mln_texture_readback_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class TextureReadbackResult extends _Value {
  TextureReadbackResult({Uint8List? data, this.info = const TextureImageInfo()})
    : data = Uint8List.fromList(data ?? const <int>[]).asUnmodifiableView();

  /// Pixel bytes.
  final Uint8List data;
  final TextureImageInfo info;

  @override
  List<Object?> get _members => [data, info];
  @override
  String toString() =>
      _describe('TextureReadbackResult', const ['data', 'info']);
}

/// One nonblocking request for a frame.
///
/// See `mln_frame_demand` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
final class FrameDemand extends _Value {
  const FrameDemand({this.flags = FrameDemandFlag.none, this.token = 0});

  /// Frame-demand policy. Zero renders only when a newer map update exists,
  /// without presenting.
  final FrameDemandFlag flags;

  /// Host identity returned with the terminal frame result. Any value except
  /// the maximum uint64_t value.
  final int token;

  /// A copy of this value with the given members replaced.
  FrameDemand copyWith({FrameDemandFlag? flags, int? token}) =>
      FrameDemand(flags: flags ?? this.flags, token: token ?? this.token);

  @override
  List<Object?> get _members => [flags, token];
  @override
  String toString() => _describe('FrameDemand', const ['flags', 'token']);
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
    this.errorMessage = '',
    this.mustRevalidate = false,
    this.modifiedUnixMs,
    this.expiresUnixMs,
    this.etag = '',
    this.retryAfterUnixMs,
  }) : bytes = Uint8List.fromList(bytes ?? const <int>[]).asUnmodifiableView();
  final ResourceResponseStatus status;
  final ResourceErrorReason errorReason;

  /// Response bytes.
  final Uint8List bytes;
  final String errorMessage;
  final bool mustRevalidate;
  final int? modifiedUnixMs;
  final int? expiresUnixMs;
  final String etag;
  final int? retryAfterUnixMs;

  /// A copy of this value with the given members replaced.
  ResourceResponse copyWith({
    ResourceResponseStatus? status,
    ResourceErrorReason? errorReason,
    Uint8List? bytes,
    String? errorMessage,
    bool? mustRevalidate,
    int? modifiedUnixMs,
    int? expiresUnixMs,
    String? etag,
    int? retryAfterUnixMs,
  }) => ResourceResponse(
    status: status ?? this.status,
    errorReason: errorReason ?? this.errorReason,
    bytes: bytes ?? this.bytes,
    errorMessage: errorMessage ?? this.errorMessage,
    mustRevalidate: mustRevalidate ?? this.mustRevalidate,
    modifiedUnixMs: modifiedUnixMs ?? this.modifiedUnixMs,
    expiresUnixMs: expiresUnixMs ?? this.expiresUnixMs,
    etag: etag ?? this.etag,
    retryAfterUnixMs: retryAfterUnixMs ?? this.retryAfterUnixMs,
  );

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
  @override
  String toString() => _describe('ResourceResponse', const [
    'status',
    'errorReason',
    'bytes',
    'errorMessage',
    'mustRevalidate',
    'modifiedUnixMs',
    'expiresUnixMs',
    'etag',
    'retryAfterUnixMs',
  ]);
}

/// Reports that MapLibre cancelled a C API resource provider request. The
/// callback runs asynchronously in the isolate and zone that registered it,
/// after native queues the call.
///
/// See `mln_resource_request_cancel_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
typedef ResourceRequestCancelCallback = void Function();

/// Options used when creating a runtime.
///
/// See `mln_runtime_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeOptions extends _Value {
  const RuntimeOptions({this.assetPath = '', this.cachePath = ''});

  /// Directory root for asset:// URLs. Copied during runtime creation. Empty
  /// selects `/android_asset` on Android and `.` elsewhere.
  final String assetPath;

  /// Cache database path. Copied during runtime creation. Empty selects an
  /// in-memory cache.
  final String cachePath;

  /// A copy of this value with the given members replaced.
  RuntimeOptions copyWith({String? assetPath, String? cachePath}) =>
      RuntimeOptions(
        assetPath: assetPath ?? this.assetPath,
        cachePath: cachePath ?? this.cachePath,
      );

  @override
  List<Object?> get _members => [assetPath, cachePath];
  @override
  String toString() =>
      _describe('RuntimeOptions', const ['assetPath', 'cachePath']);
}

/// Options used when creating a map.
///
/// See `mln_map_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class MapOptions extends _Value {
  const MapOptions({
    required this.initialExtent,
    this.mapMode = const MapMode.fromRawValue(0),
    this.fastPforEnabled = false,
    this.eventMask,
  });

  /// Initial logical extent. Required. Width and height must be nonzero, and
  /// scale_factor must be finite and positive. scale_factor is fixed for the
  /// map's lifetime.
  final LogicalExtent initialExtent;
  final MapMode mapMode;

  /// Decodes MapLibre Tile (MLT) tiles whose integer streams use FastPFOR
  /// encodings.
  final bool fastPforEnabled;

  /// Map-originated event types this map queues.
  final RuntimeEventFlag? eventMask;

  /// A copy of this value with the given members replaced.
  MapOptions copyWith({
    LogicalExtent? initialExtent,
    MapMode? mapMode,
    bool? fastPforEnabled,
    RuntimeEventFlag? eventMask,
  }) => MapOptions(
    initialExtent: initialExtent ?? this.initialExtent,
    mapMode: mapMode ?? this.mapMode,
    fastPforEnabled: fastPforEnabled ?? this.fastPforEnabled,
    eventMask: eventMask ?? this.eventMask,
  );

  @override
  List<Object?> get _members => [
    initialExtent,
    mapMode,
    fastPforEnabled,
    eventMask,
  ];
  @override
  String toString() => _describe('MapOptions', const [
    'initialExtent',
    'mapMode',
    'fastPforEnabled',
    'eventMask',
  ]);
}

/// Tile-pyramid offline region definition.
///
/// See `mln_offline_tile_pyramid_region_definition` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class OfflineTilePyramidRegionDefinition extends _Value {
  const OfflineTilePyramidRegionDefinition({
    this.styleUrl = '',
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

  /// Maximum zoom. Positive infinity lets each tile source use its own maximum
  /// zoom.
  final double maxZoom;
  final double pixelRatio;
  final bool includeIdeographs;

  /// A copy of this value with the given members replaced.
  OfflineTilePyramidRegionDefinition copyWith({
    String? styleUrl,
    LatLngBounds? bounds,
    double? minZoom,
    double? maxZoom,
    double? pixelRatio,
    bool? includeIdeographs,
  }) => OfflineTilePyramidRegionDefinition(
    styleUrl: styleUrl ?? this.styleUrl,
    bounds: bounds ?? this.bounds,
    minZoom: minZoom ?? this.minZoom,
    maxZoom: maxZoom ?? this.maxZoom,
    pixelRatio: pixelRatio ?? this.pixelRatio,
    includeIdeographs: includeIdeographs ?? this.includeIdeographs,
  );

  @override
  List<Object?> get _members => [
    styleUrl,
    bounds,
    minZoom,
    maxZoom,
    pixelRatio,
    includeIdeographs,
  ];
  @override
  String toString() => _describe('OfflineTilePyramidRegionDefinition', const [
    'styleUrl',
    'bounds',
    'minZoom',
    'maxZoom',
    'pixelRatio',
    'includeIdeographs',
  ]);
}

/// Geometry offline region definition.
///
/// See `mln_offline_geometry_region_definition` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class OfflineGeometryRegionDefinition extends _Value {
  const OfflineGeometryRegionDefinition({
    this.styleUrl = '',
    required this.geometry,
    this.minZoom = 0,
    this.maxZoom = 0,
    this.pixelRatio = 0,
    this.includeIdeographs = false,
  });

  /// Style URL. Copied during region creation.
  final String styleUrl;

  /// UTF-8 GeoJSON Geometry. Required.
  final Object? geometry;
  final double minZoom;

  /// Maximum zoom. Positive infinity lets each tile source use its own maximum
  /// zoom.
  final double maxZoom;
  final double pixelRatio;
  final bool includeIdeographs;

  /// A copy of this value with the given members replaced.
  OfflineGeometryRegionDefinition copyWith({
    String? styleUrl,
    Object? geometry = _unchanged,
    double? minZoom,
    double? maxZoom,
    double? pixelRatio,
    bool? includeIdeographs,
  }) => OfflineGeometryRegionDefinition(
    styleUrl: styleUrl ?? this.styleUrl,
    geometry: identical(geometry, _unchanged) ? this.geometry : geometry,
    minZoom: minZoom ?? this.minZoom,
    maxZoom: maxZoom ?? this.maxZoom,
    pixelRatio: pixelRatio ?? this.pixelRatio,
    includeIdeographs: includeIdeographs ?? this.includeIdeographs,
  );

  @override
  List<Object?> get _members => [
    styleUrl,
    geometry,
    minZoom,
    maxZoom,
    pixelRatio,
    includeIdeographs,
  ];
  @override
  String toString() => _describe('OfflineGeometryRegionDefinition', const [
    'styleUrl',
    'geometry',
    'minZoom',
    'maxZoom',
    'pixelRatio',
    'includeIdeographs',
  ]);
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

final class OfflineRegionDefinitionUnknown extends OfflineRegionDefinition {
  OfflineRegionDefinitionUnknown(this.tag, Uint8List rawRecord)
    : rawRecord = Uint8List.fromList(rawRecord).asUnmodifiableView(),
      super._();
  final int tag;
  final Uint8List rawRecord;
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
  @override
  String toString() =>
      _describe('OfflineRegionInfo', const ['id', 'definition', 'metadata']);
}

/// Offline region status snapshot.
///
/// See `mln_offline_region_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class OfflineRegionStatus extends _Value {
  const OfflineRegionStatus({
    this.downloadState = const OfflineRegionDownloadState.fromRawValue(0),
    this.completedResourceCount = 0,
    this.completedResourceSize = 0,
    this.completedTileCount = 0,
    this.requiredTileCount = 0,
    this.completedTileSize = 0,
    this.requiredResourceCount = 0,
    this.requiredResourceCountIsPrecise = false,
    this.complete = false,
  });
  final OfflineRegionDownloadState downloadState;
  final int completedResourceCount;
  final int completedResourceSize;
  final int completedTileCount;
  final int requiredTileCount;
  final int completedTileSize;
  final int requiredResourceCount;
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
  @override
  String toString() => _describe('OfflineRegionStatus', const [
    'downloadState',
    'completedResourceCount',
    'completedResourceSize',
    'completedTileCount',
    'requiredTileCount',
    'completedTileSize',
    'requiredResourceCount',
    'requiredResourceCountIsPrecise',
    'complete',
  ]);
}

/// Rendering statistics of one finished frame.
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
  @override
  String toString() => _describe('RenderingStats', const [
    'encodingTime',
    'renderingTime',
    'frameCount',
    'drawCallCount',
    'totalDrawCallCount',
  ]);
}

/// Payload for `RuntimeEventType.mapRenderFrameFinished`.
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
  final RenderMode mode;

  /// Whether MapLibre needs another frame after this one.
  final bool needsRepaint;

  /// Whether symbol placement changed during this frame.
  final bool placementChanged;
  final RenderingStats stats;

  @override
  List<Object?> get _members => [mode, needsRepaint, placementChanged, stats];
  @override
  String toString() => _describe('RuntimeEventRenderFrame', const [
    'mode',
    'needsRepaint',
    'placementChanged',
    'stats',
  ]);
}

/// Payload for `RuntimeEventType.mapRenderMapFinished`.
///
/// See `mln_runtime_event_render_map` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventRenderMap extends _Value {
  const RuntimeEventRenderMap({this.mode = const RenderMode.fromRawValue(0)});
  final RenderMode mode;

  @override
  List<Object?> get _members => [mode];
  @override
  String toString() => _describe('RuntimeEventRenderMap', const ['mode']);
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
  @override
  String toString() => _describe('TileId', const [
    'overscaledZ',
    'wrap',
    'canonicalZ',
    'canonicalX',
    'canonicalY',
  ]);
}

/// Payload for `RuntimeEventType.mapTileAction`.
///
/// See `mln_runtime_event_tile_action` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventTileAction extends _Value {
  const RuntimeEventTileAction({
    this.operation = const TileOperation.fromRawValue(0),
    this.tileId = const TileId(),
  });
  final TileOperation operation;
  final TileId tileId;

  @override
  List<Object?> get _members => [operation, tileId];
  @override
  String toString() =>
      _describe('RuntimeEventTileAction', const ['operation', 'tileId']);
}

/// Payload for `RuntimeEventType.offlineRegionStatusChanged`.
///
/// See `mln_runtime_event_offline_region_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventOfflineRegionStatus extends _Value {
  const RuntimeEventOfflineRegionStatus({
    this.regionId = 0,
    this.status = const OfflineRegionStatus(),
  });
  final int regionId;
  final OfflineRegionStatus status;

  @override
  List<Object?> get _members => [regionId, status];
  @override
  String toString() => _describe('RuntimeEventOfflineRegionStatus', const [
    'regionId',
    'status',
  ]);
}

/// Payload for `RuntimeEventType.offlineRegionResponseError`.
///
/// See `mln_runtime_event_offline_region_response_error` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventOfflineRegionResponseError extends _Value {
  const RuntimeEventOfflineRegionResponseError({
    this.regionId = 0,
    this.reason = const ResourceErrorReason.fromRawValue(0),
  });
  final int regionId;
  final ResourceErrorReason reason;

  @override
  List<Object?> get _members => [regionId, reason];
  @override
  String toString() => _describe(
    'RuntimeEventOfflineRegionResponseError',
    const ['regionId', 'reason'],
  );
}

/// Payload for `RuntimeEventType.offlineRegionTileCountLimitExceeded`.
///
/// See `mln_runtime_event_offline_region_tile_count_limit` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventOfflineRegionTileCountLimit extends _Value {
  const RuntimeEventOfflineRegionTileCountLimit({
    this.regionId = 0,
    this.limit = 0,
  });
  final int regionId;
  final int limit;

  @override
  List<Object?> get _members => [regionId, limit];
  @override
  String toString() => _describe(
    'RuntimeEventOfflineRegionTileCountLimit',
    const ['regionId', 'limit'],
  );
}

/// One delivered runtime event.
///
/// See `mln_runtime_event` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEvent {
  const RuntimeEvent({
    required this.type,
    this.sourceType = const RuntimeEventSourceType.fromRawValue(0),
    this.source = 0,
    this.generation = 0,
    this.code = 0,
    this.message = '',
    required this.payload,
  });
  final RuntimeEventType type;
  final RuntimeEventSourceType sourceType;

  /// Source handle: the `RuntimeHandle` or `MapHandle` that source_type
  /// selects.
  final int source;

  /// Map snapshot generation that the event reports, or zero when source_type
  /// is `RuntimeEventSourceType.runtime`.
  final int generation;

  /// Secondary event detail whose meaning type selects; see `RuntimeEventType`.
  final int code;

  /// UTF-8 message, empty when this event carries no message.
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

/// Receives selected events after their snapshots have been published. The
/// callback runs asynchronously in the isolate and zone that registered it,
/// after native queues the call.
///
/// See `mln_runtime_event_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
typedef RuntimeEventCallback = void Function(List<RuntimeEvent> events);

/// One HTTP header that a header rule supplies.
///
/// See `mln_http_header` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class HttpHeader extends _Value {
  const HttpHeader({required this.name, this.value = ''});

  /// Header field name. Required.
  final String name;
  final String value;

  /// A copy of this value with the given members replaced.
  HttpHeader copyWith({String? name, String? value}) =>
      HttpHeader(name: name ?? this.name, value: value ?? this.value);

  @override
  List<Object?> get _members => [name, value];
  @override
  String toString() => _describe('HttpHeader', const ['name', 'value']);
}

/// One rule of a native HTTP header transform.
///
/// See `mln_http_header_rule` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class HttpHeaderRule extends _Value {
  HttpHeaderRule({
    this.kinds = ResourceKindFlag.none,
    this.flags = UrlMatchFlag.exact,
    this.url = '',
    List<HttpHeader> headers = const [],
  }) : headers = List.unmodifiable(headers);

  /// The resource kinds to match. Zero selects every kind.
  final ResourceKindFlag kinds;
  final UrlMatchFlag flags;

  /// The URL or glob pattern to compare.
  final String url;

  /// The headers to set.
  final List<HttpHeader> headers;

  /// A copy of this value with the given members replaced.
  HttpHeaderRule copyWith({
    ResourceKindFlag? kinds,
    UrlMatchFlag? flags,
    String? url,
    List<HttpHeader>? headers,
  }) => HttpHeaderRule(
    kinds: kinds ?? this.kinds,
    flags: flags ?? this.flags,
    url: url ?? this.url,
    headers: headers ?? this.headers,
  );

  @override
  List<Object?> get _members => [kinds, flags, url, headers];
  @override
  String toString() =>
      _describe('HttpHeaderRule', const ['kinds', 'flags', 'url', 'headers']);
}

/// HTTP header transform state: a callback, or native header rules. The
/// callback runs asynchronously in the isolate and zone that registered it,
/// after native queues the call.
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
  const ResourceRange({this.start = 0, this.end = 0});

  /// First byte offset of the requested range.
  final int start;

  /// Last byte offset of the requested range, inclusive.
  final int end;

  @override
  List<Object?> get _members => [start, end];
  @override
  String toString() => _describe('ResourceRange', const ['start', 'end']);
}

final class ResourceRequestInfo extends _Value {
  ResourceRequestInfo({
    this.requestedUrl = '',
    this.resolvedUrl = '',
    this.kind = const ResourceKind.fromRawValue(0),
    this.loadingMethod = const ResourceLoadingMethod.fromRawValue(0),
    this.priority = const ResourcePriority.fromRawValue(0),
    this.usage = const ResourceUsage.fromRawValue(0),
    this.storagePolicy = const ResourceStoragePolicy.fromRawValue(0),
    this.range,
    this.priorModifiedUnixMs,
    this.priorExpiresUnixMs,
    this.priorEtag = '',
    Uint8List? priorData,
  }) : priorData = Uint8List.fromList(
         priorData ?? const <int>[],
       ).asUnmodifiableView();

  /// URL entering the network layer, before tile server normalization.
  final String requestedUrl;

  /// URL to fetch, after resource-kind normalization against the runtime's tile
  /// server options and API key.
  final String resolvedUrl;
  final ResourceKind kind;
  final ResourceLoadingMethod loadingMethod;
  final ResourcePriority priority;
  final ResourceUsage usage;
  final ResourceStoragePolicy storagePolicy;
  final ResourceRange? range;
  final int? priorModifiedUnixMs;
  final int? priorExpiresUnixMs;

  /// Entity tag of the cached copy, or empty when there is none.
  final String priorEtag;
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
  @override
  String toString() => _describe('ResourceRequestInfo', const [
    'requestedUrl',
    'resolvedUrl',
    'kind',
    'loadingMethod',
    'priority',
    'usage',
    'storagePolicy',
    'range',
    'priorModifiedUnixMs',
    'priorExpiresUnixMs',
    'priorEtag',
    'priorData',
  ]);
}

/// Intercepts a network resource request. The callback runs asynchronously in
/// the isolate and zone that registered it, after native queues the call.
///
/// See `mln_resource_provider_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
typedef ResourceProviderCallback =
    void Function(ResourceRequestInfo request, ResourceRequestHandle handle);

/// One request pattern that a resource provider claims.
///
/// See `mln_resource_route` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class ResourceRoute extends _Value {
  const ResourceRoute({
    this.kinds = ResourceKindFlag.none,
    this.flags = UrlMatchFlag.exact,
    this.url = '',
  });

  /// The resource kinds to match. Zero selects every kind.
  final ResourceKindFlag kinds;
  final UrlMatchFlag flags;

  /// The URL or glob pattern to compare.
  final String url;

  /// A copy of this value with the given members replaced.
  ResourceRoute copyWith({
    ResourceKindFlag? kinds,
    UrlMatchFlag? flags,
    String? url,
  }) => ResourceRoute(
    kinds: kinds ?? this.kinds,
    flags: flags ?? this.flags,
    url: url ?? this.url,
  );

  @override
  List<Object?> get _members => [kinds, flags, url];
  @override
  String toString() =>
      _describe('ResourceRoute', const ['kinds', 'flags', 'url']);
}

/// Resource provider state. The callback runs asynchronously in the isolate and
/// zone that registered it, after native queues the call.
///
/// See `mln_resource_provider` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class ResourceProvider {
  ResourceProvider({
    required this.callback,
    List<ResourceRoute> routes = const [],
  }) : routes = List.unmodifiable(routes);

  /// Answers each request that the routes leave to the provider.
  final ResourceProviderCallback callback;

  /// The requests that reach the provider; empty to claim every request.
  final List<ResourceRoute> routes;
}

/// Asks the host for the URL of one network resource request. The callback runs
/// asynchronously in the isolate and zone that registered it, after native
/// queues the call.
///
/// See `mln_resource_transform_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
typedef ResourceTransformCallback =
    void Function(
      ResourceKind kind,
      String url,
      ResourceTransformRequestHandle request,
    );

/// A style expression type.
///
/// See `mln_style_expression_type` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
final class StyleExpressionType extends _Value {
  const StyleExpressionType({
    this.kind = const StyleExpressionTypeKind.fromRawValue(0),
    this.itemKind = const StyleExpressionTypeKind.fromRawValue(0),
    this.itemCount = 0,
  });
  final StyleExpressionTypeKind kind;

  /// For an array, its items' kind; ignored for any other kind.
  final StyleExpressionTypeKind itemKind;

  /// For an array, its fixed length, or zero for any length.
  final int itemCount;

  /// A copy of this value with the given members replaced.
  StyleExpressionType copyWith({
    StyleExpressionTypeKind? kind,
    StyleExpressionTypeKind? itemKind,
    int? itemCount,
  }) => StyleExpressionType(
    kind: kind ?? this.kind,
    itemKind: itemKind ?? this.itemKind,
    itemCount: itemCount ?? this.itemCount,
  );

  @override
  List<Object?> get _members => [kind, itemKind, itemCount];
  @override
  String toString() =>
      _describe('StyleExpressionType', const ['kind', 'itemKind', 'itemCount']);
}

/// Options for parsing a style expression.
///
/// See `mln_style_expression_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
final class StyleExpressionOptions extends _Value {
  const StyleExpressionOptions({this.expectedType});

  /// The type that the expression must produce. A string type coerces the
  /// result to a string, as a string layer property does; every other type
  /// asserts it. Without it, the expression keeps the type it infers.
  final StyleExpressionType? expectedType;

  /// A copy of this value with the given members replaced.
  StyleExpressionOptions copyWith({StyleExpressionType? expectedType}) =>
      StyleExpressionOptions(expectedType: expectedType ?? this.expectedType);

  @override
  List<Object?> get _members => [expectedType];
  @override
  String toString() =>
      _describe('StyleExpressionOptions', const ['expectedType']);
}

/// What one evaluation reads.
///
/// See `mln_style_expression_inputs` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
final class StyleExpressionInputs extends _Value {
  StyleExpressionInputs({
    this.zoom,
    this.colorRampParameter,
    this.elevation,
    this.feature,
    this.featureState,
    this.globalState,
    this.canonicalTileId,
    List<String> availableImages = const [],
  }) : availableImages = List.unmodifiable(availableImages);

  /// The value of the zoom expression.
  final double? zoom;

  /// The value of the heatmap-density and line-progress expressions, and of the
  /// elevation expression in a color-relief layer.
  final double? colorRampParameter;

  /// The value of the elevation expression outside a color-relief layer.
  final double? elevation;

  /// One UTF-8 GeoJSON Feature that feature data expressions read.
  final Object? feature;

  /// A UTF-8 JSON object that the feature-state expression reads.
  final Object? featureState;

  /// A UTF-8 JSON object that the global-state expression reads.
  final Object? globalState;

  /// The tile that holds the feature, which the within and distance expressions
  /// need to place its geometry.
  final CanonicalTileId? canonicalTileId;

  /// The image IDs that the image expression reports as available. The
  /// expression reports any other ID as unavailable.
  final List<String> availableImages;

  /// A copy of this value with the given members replaced.
  StyleExpressionInputs copyWith({
    double? zoom,
    double? colorRampParameter,
    double? elevation,
    Object? feature = _unchanged,
    Object? featureState = _unchanged,
    Object? globalState = _unchanged,
    CanonicalTileId? canonicalTileId,
    List<String>? availableImages,
  }) => StyleExpressionInputs(
    zoom: zoom ?? this.zoom,
    colorRampParameter: colorRampParameter ?? this.colorRampParameter,
    elevation: elevation ?? this.elevation,
    feature: identical(feature, _unchanged) ? this.feature : feature,
    featureState: identical(featureState, _unchanged)
        ? this.featureState
        : featureState,
    globalState: identical(globalState, _unchanged)
        ? this.globalState
        : globalState,
    canonicalTileId: canonicalTileId ?? this.canonicalTileId,
    availableImages: availableImages ?? this.availableImages,
  );

  @override
  List<Object?> get _members => [
    zoom,
    colorRampParameter,
    elevation,
    feature,
    featureState,
    globalState,
    canonicalTileId,
    availableImages,
  ];
  @override
  String toString() => _describe('StyleExpressionInputs', const [
    'zoom',
    'colorRampParameter',
    'elevation',
    'feature',
    'featureState',
    'globalState',
    'canonicalTileId',
    'availableImages',
  ]);
}

/// What a parsed expression depends on.
///
/// See `mln_style_expression_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
final class StyleExpressionInfo extends _Value {
  const StyleExpressionInfo({
    this.isFeatureConstant = false,
    this.isZoomConstant = false,
  });

  /// Whether the result is the same for every feature and feature state.
  final bool isFeatureConstant;

  /// Whether the result is the same at every zoom.
  final bool isZoomConstant;

  @override
  List<Object?> get _members => [isFeatureConstant, isZoomConstant];
  @override
  String toString() => _describe('StyleExpressionInfo', const [
    'isFeatureConstant',
    'isZoomConstant',
  ]);
}
