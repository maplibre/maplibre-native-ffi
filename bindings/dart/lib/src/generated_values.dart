// Generated from the C headers by tools/bindgen. Do not edit.
part of 'values.dart';

/// A maintenance action on a runtime's cache database.
///
/// The cache database is the file that `RuntimeOptions.cachePath` names. It
/// holds the ambient cache, meaning the resources that maps loaded, and the
/// offline regions.
///
/// See `mln_cache_database_operation` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class CacheDatabaseOperation extends _Enum {
  const CacheDatabaseOperation.fromRawValue(super.rawValue);

  /// Deletes the ambient cache's resources and frees the space they used.
  /// Offline regions keep their resources.
  static const clear = CacheDatabaseOperation.fromRawValue(1);

  /// Keeps the ambient cache's resources, but makes maps check each one with
  /// the server before they use it again. Offline regions keep their resources
  /// as they are.
  static const invalidate = CacheDatabaseOperation.fromRawValue(2);

  /// Compacts the whole database file to free the disk space that it no longer
  /// uses.
  static const pack = CacheDatabaseOperation.fromRawValue(3);

  /// Deletes the whole database, offline regions included, and starts an empty
  /// one.
  static const reset = CacheDatabaseOperation.fromRawValue(4);
  @override
  String toString() => switch (rawValue) {
    1 => 'CacheDatabaseOperation.clear',
    2 => 'CacheDatabaseOperation.invalidate',
    3 => 'CacheDatabaseOperation.pack',
    4 => 'CacheDatabaseOperation.reset',
    _ => 'CacheDatabaseOperation($rawValue)',
  };
}

/// Whether a camera change jumps or animates.
///
/// See `mln_camera_change_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class CameraChangeMode extends _Enum {
  const CameraChangeMode.fromRawValue(super.rawValue);
  static const immediate = CameraChangeMode.fromRawValue(0);
  static const animated = CameraChangeMode.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'CameraChangeMode.immediate',
    1 => 'CameraChangeMode.animated',
    _ => 'CameraChangeMode($rawValue)',
  };
}

/// What moved the camera.
///
/// See `mln_camera_change_source` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class CameraChangeSource extends _Enum {
  const CameraChangeSource.fromRawValue(super.rawValue);

  /// A camera command outside a gesture, or the map itself, as when a resize
  /// keeps the camera within its bounds.
  static const command = CameraChangeSource.fromRawValue(0);

  /// A camera command with a gesture phase.
  static const gesture = CameraChangeSource.fromRawValue(1);

  /// The momentum and bearing snap that a gesture's end started.
  static const momentum = CameraChangeSource.fromRawValue(2);
  @override
  String toString() => switch (rawValue) {
    0 => 'CameraChangeSource.command',
    1 => 'CameraChangeSource.gesture',
    2 => 'CameraChangeSource.momentum',
    _ => 'CameraChangeSource($rawValue)',
  };
}

/// How a camera change ended.
///
/// A change that ends in more than one way reports
/// `CameraTransitionOutcome.cancelled` if any of it was cancelled, and
/// otherwise `CameraTransitionOutcome.replaced` if any of it was replaced.
///
/// See `mln_camera_transition_outcome` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraTransitionOutcome extends _Enum {
  const CameraTransitionOutcome.fromRawValue(super.rawValue);

  /// The camera finished the change.
  static const completed = CameraTransitionOutcome.fromRawValue(0);

  /// `MapHandle.cancelCameraTransitions()`,
  /// `MapHandle.cancelCameraTransitionsWithId()`, a gesture's
  /// `GesturePhase.begin` or `GesturePhase.cancel`,
  /// `MapHandle.setFreeCameraOptions()`, or a resize that moved the camera back
  /// inside its bounds stopped the change. A command that fails before it moves
  /// the camera also reports this.
  static const cancelled = CameraTransitionOutcome.fromRawValue(1);

  /// The map closed before the change ended.
  static const closed = CameraTransitionOutcome.fromRawValue(2);

  /// A later camera change set every field that a part of the change was still
  /// animating, so that part stopped. Its other parts keep animating, and the
  /// change reports this outcome when they end, unless one was cancelled.
  ///
  /// A later change that sets only some of those fields leaves the change
  /// animating the rest. A fly's center and zoom count as one field, and so do
  /// an anchored change's center and the fields that the anchor holds in place.
  static const replaced = CameraTransitionOutcome.fromRawValue(3);
  @override
  String toString() => switch (rawValue) {
    0 => 'CameraTransitionOutcome.completed',
    1 => 'CameraTransitionOutcome.cancelled',
    2 => 'CameraTransitionOutcome.closed',
    3 => 'CameraTransitionOutcome.replaced',
    _ => 'CameraTransitionOutcome($rawValue)',
  };
}

/// How a camera command moves the camera.
///
/// See `mln_camera_update_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraUpdateMode extends _Enum {
  const CameraUpdateMode.fromRawValue(super.rawValue);

  /// Moves the camera at once.
  static const jump = CameraUpdateMode.fromRawValue(0);

  /// Animates the camera straight to the target.
  static const ease = CameraUpdateMode.fromRawValue(1);

  /// Zooms out, travels to the target, and zooms back in.
  static const fly = CameraUpdateMode.fromRawValue(2);
  @override
  String toString() => switch (rawValue) {
    0 => 'CameraUpdateMode.jump',
    1 => 'CameraUpdateMode.ease',
    2 => 'CameraUpdateMode.fly',
    _ => 'CameraUpdateMode($rawValue)',
  };
}

/// How the map keeps the camera within the world or within its bounds.
///
/// See `mln_constrain_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class ConstrainMode extends _Enum {
  const ConstrainMode.fromRawValue(super.rawValue);
  static const none = ConstrainMode.fromRawValue(0);

  /// Keeps the world filling the view from top to bottom.
  static const heightOnly = ConstrainMode.fromRawValue(1);

  /// Keeps the world filling the view in both directions.
  static const widthAndHeight = ConstrainMode.fromRawValue(2);

  /// Keeps the whole view inside the bounds from `MapHandle.setBounds()`.
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

/// How a frame demand behaves.
///
/// Without flags, a demand draws a frame when the map changed since the last
/// one, and otherwise finishes with `RenderResult.noUpdate`. A session with a
/// surface target shows each frame that it draws.
///
/// See `mln_frame_demand_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
final class FrameDemandFlag extends _Flags<FrameDemandFlag> {
  const FrameDemandFlag.fromRawValue(super.rawValue);

  /// Draws a frame even when nothing changed since the last one. It can't be
  /// combined with `FrameDemandFlag.waitForUpdate`.
  static const force = FrameDemandFlag.fromRawValue(1);

  /// When the map has nothing new to draw, waits for its next update instead of
  /// finishing with `RenderResult.noUpdate`.
  static const waitForUpdate = FrameDemandFlag.fromRawValue(4);

  /// No flags.
  static const none = FrameDemandFlag.fromRawValue(0);
  @override
  FrameDemandFlag _of(int rawValue) => FrameDemandFlag.fromRawValue(rawValue);
  @override
  String toString() => _describe('FrameDemandFlag', const {
    0: 'none',
    1: 'force',
    4: 'waitForUpdate',
  });
}

/// Where a camera change falls in a gesture that the user makes.
///
/// `GesturePhase.begin` and `GesturePhase.update` mark a gesture in progress,
/// and `GesturePhase.end` and `GesturePhase.cancel` end it.
///
/// See `mln_gesture_phase` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class GesturePhase extends _Enum {
  const GesturePhase.fromRawValue(super.rawValue);

  /// The change isn't part of a gesture.
  static const none = GesturePhase.fromRawValue(0);

  /// The gesture starts. This cancels every running camera animation before the
  /// change applies.
  static const begin = GesturePhase.fromRawValue(1);

  /// The gesture continues. To start a gesture that keeps the running camera
  /// animations, start it with this phase instead of `GesturePhase.begin`.
  static const update = GesturePhase.fromRawValue(2);
  static const end = GesturePhase.fromRawValue(3);

  /// The platform cancelled the gesture. This also cancels every running camera
  /// animation after the change applies.
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

/// How one side waits for the other's GPU work on a shared texture.
///
/// A sync file descriptor or an OpenGL fence signals once, so when
/// `AcquiredFrameHandle.release()` succeeds, the session takes the one you
/// pass. A Vulkan timeline semaphore, Metal shared event, or D3D12 fence counts
/// up with each frame, so the one you pass stays yours. Keep it valid until a
/// detach or release of the session completes.
///
/// See `mln_gpu_sync_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class GpuSyncKind extends _Enum {
  const GpuSyncKind.fromRawValue(super.rawValue);

  /// The work has already finished, so there is nothing to wait for.
  static const cpuComplete = GpuSyncKind.fromRawValue(0);

  /// `handle` is a `GLsync` in the session's share group.
  static const openGlFence = GpuSyncKind.fromRawValue(1);

  /// The work was submitted to the queue that both sides share, so work
  /// submitted to that queue later runs after it.
  static const queueOrder = GpuSyncKind.fromRawValue(2);

  /// `handle` is a `VkSemaphore`. Wait until it reaches `value`.
  static const vulkanTimelineSemaphore = GpuSyncKind.fromRawValue(3);

  /// `handle` is a sync file descriptor.
  static const syncFd = GpuSyncKind.fromRawValue(4);

  /// `handle` is an `MTLSharedEvent`. Wait until it reaches `value`.
  static const metalSharedEvent = GpuSyncKind.fromRawValue(5);

  /// `handle` is a shared Windows handle to an `ID3D12Fence`. Wait until it
  /// reaches `value`.
  static const d3d12Fence = GpuSyncKind.fromRawValue(6);
  @override
  String toString() => switch (rawValue) {
    0 => 'GpuSyncKind.cpuComplete',
    1 => 'GpuSyncKind.openGlFence',
    2 => 'GpuSyncKind.queueOrder',
    3 => 'GpuSyncKind.vulkanTimelineSemaphore',
    4 => 'GpuSyncKind.syncFd',
    5 => 'GpuSyncKind.metalSharedEvent',
    6 => 'GpuSyncKind.d3d12Fence',
    _ => 'GpuSyncKind($rawValue)',
  };
}

/// A GPU sync kind, as a bit in a set.
///
/// See `mln_gpu_sync_kind_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class GpuSyncKindFlag extends _Flags<GpuSyncKindFlag> {
  const GpuSyncKindFlag.fromRawValue(super.rawValue);
  static const cpuComplete = GpuSyncKindFlag.fromRawValue(1);
  static const openGlFence = GpuSyncKindFlag.fromRawValue(2);
  static const queueOrder = GpuSyncKindFlag.fromRawValue(4);
  static const vulkanTimelineSemaphore = GpuSyncKindFlag.fromRawValue(8);
  static const syncFd = GpuSyncKindFlag.fromRawValue(16);
  static const metalSharedEvent = GpuSyncKindFlag.fromRawValue(32);
  static const d3d12Fence = GpuSyncKindFlag.fromRawValue(64);

  /// No flags.
  static const none = GpuSyncKindFlag.fromRawValue(0);
  @override
  GpuSyncKindFlag _of(int rawValue) => GpuSyncKindFlag.fromRawValue(rawValue);
  @override
  String toString() => _describe('GpuSyncKindFlag', const {
    0: 'none',
    1: 'cpuComplete',
    2: 'openGlFence',
    4: 'queueOrder',
    8: 'vulkanTimelineSemaphore',
    16: 'syncFd',
    32: 'metalSharedEvent',
    64: 'd3d12Fence',
  });
}

/// The part of the library that logged a message.
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

/// The severity of a log message.
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

/// A log severity, as a bit in a set.
///
/// See `mln_log_severity_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
final class LogSeverityFlag extends _Flags<LogSeverityFlag> {
  const LogSeverityFlag.fromRawValue(super.rawValue);
  static const info = LogSeverityFlag.fromRawValue(2);
  static const warning = LogSeverityFlag.fromRawValue(4);
  static const error = LogSeverityFlag.fromRawValue(8);

  /// The severities logged on a background thread until
  /// `setLogAsyncSeverityMask()` changes them.
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

/// A debug overlay that the map draws.
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

/// Why a map failed to load a style.
///
/// See `mln_map_load_error` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class MapLoadError extends _Enum {
  const MapLoadError.fromRawValue(super.rawValue);
  static const unknown = MapLoadError.fromRawValue(0);

  /// The style document isn't a valid style.
  static const styleParse = MapLoadError.fromRawValue(1);

  /// The style's request failed, such as with a 404 or a missing file.
  static const styleLoad = MapLoadError.fromRawValue(2);
  @override
  String toString() => switch (rawValue) {
    0 => 'MapLoadError.unknown',
    1 => 'MapLoadError.styleParse',
    2 => 'MapLoadError.styleLoad',
    _ => 'MapLoadError($rawValue)',
  };
}

/// When a map renders.
///
/// See `mln_map_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class MapMode extends _Enum {
  const MapMode.fromRawValue(super.rawValue);

  /// Renders whenever the map changes.
  static const continuous = MapMode.fromRawValue(0);

  /// Renders only when `MapHandle.requestStillImage()` asks for an image.
  static const static = MapMode.fromRawValue(1);

  /// Renders still images of single tiles, as `MapMode.static` does.
  static const tile = MapMode.fromRawValue(2);
  @override
  String toString() => switch (rawValue) {
    0 => 'MapMode.continuous',
    1 => 'MapMode.static',
    2 => 'MapMode.tile',
    _ => 'MapMode($rawValue)',
  };
}

/// How a part of a gesture's momentum slows to a stop.
///
/// See `mln_momentum_decay` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class MomentumDecay extends _Enum {
  const MomentumDecay.fromRawValue(super.rawValue);

  /// The share of the distance traveled is 1 - (1 - t)^`exponent`, where t goes
  /// from 0 to 1 over the momentum's duration. An exponent of 2 slows at a
  /// constant rate.
  static const power = MomentumDecay.fromRawValue(0);

  /// The speed falls to e^-`decay_rate` of itself each second, until it drops
  /// below `stop_speed`.
  static const exponential = MomentumDecay.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'MomentumDecay.power',
    1 => 'MomentumDecay.exponential',
    _ => 'MomentumDecay($rawValue)',
  };
}

/// Whether the library may use the network.
///
/// See `mln_network_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
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

/// The screen direction that north points to when the bearing is 0.
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

/// Whether an offline region is downloading.
///
/// See `mln_offline_region_download_state` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
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

/// The OpenGL API that a context renders with.
///
/// See `mln_open_gl_client_api` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class OpenGlClientApi extends _Enum {
  const OpenGlClientApi.fromRawValue(super.rawValue);
  static const unspecified = OpenGlClientApi.fromRawValue(0);
  static const gl = OpenGlClientApi.fromRawValue(1);
  static const gles = OpenGlClientApi.fromRawValue(2);
  @override
  String toString() => switch (rawValue) {
    0 => 'OpenGlClientApi.unspecified',
    1 => 'OpenGlClientApi.gl',
    2 => 'OpenGlClientApi.gles',
    _ => 'OpenGlClientApi($rawValue)',
  };
}

/// Whether a render session's OpenGL context shares objects with yours.
///
/// See `mln_open_gl_context_ownership` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class OpenGlContextOwnership extends _Enum {
  const OpenGlContextOwnership.fromRawValue(super.rawValue);

  /// The session's context shares objects with your `share_context`.
  static const shared = OpenGlContextOwnership.fromRawValue(0);

  /// The session's context shares objects with no other context.
  static const dedicated = OpenGlContextOwnership.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'OpenGlContextOwnership.shared',
    1 => 'OpenGlContextOwnership.dedicated',
    _ => 'OpenGlContextOwnership($rawValue)',
  };
}

/// An OpenGL context platform, as a bit in a set.
///
/// See `mln_open_gl_context_provider_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class OpenGlContextProviderFlag
    extends _Flags<OpenGlContextProviderFlag> {
  const OpenGlContextProviderFlag.fromRawValue(super.rawValue);
  static const wgl = OpenGlContextProviderFlag.fromRawValue(1);
  static const egl = OpenGlContextProviderFlag.fromRawValue(2);
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

/// What `RenderSessionHandle.abandon()` did with a session's graphics objects.
///
/// See `mln_render_abandon_disposition` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
final class RenderAbandonDisposition extends _Enum {
  const RenderAbandonDisposition.fromRawValue(super.rawValue);
  static const clean = RenderAbandonDisposition.fromRawValue(0);

  /// It kept some objects that it couldn't destroy safely. They stay allocated
  /// until the process exits.
  static const quarantined = RenderAbandonDisposition.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'RenderAbandonDisposition.clean',
    1 => 'RenderAbandonDisposition.quarantined',
    _ => 'RenderAbandonDisposition($rawValue)',
  };
}

/// A render backend, as a bit in a set.
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

/// The thread that runs a render session's graphics work.
///
/// See `mln_render_driver_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class RenderDriverKind extends _Enum {
  const RenderDriverKind.fromRawValue(super.rawValue);

  /// A library thread.
  static const coreWorker = RenderDriverKind.fromRawValue(1);

  /// Your thread, when it calls `mln_render_session_service_driver_work()`.
  static const callerGraphicsThread = RenderDriverKind.fromRawValue(2);
  @override
  String toString() => switch (rawValue) {
    1 => 'RenderDriverKind.coreWorker',
    2 => 'RenderDriverKind.callerGraphicsThread',
    _ => 'RenderDriverKind($rawValue)',
  };
}

/// Whether a frame drew the map complete, or lacked resources that are still
/// loading.
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

/// How a frame demand finished.
///
/// See `mln_render_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
final class RenderResult extends _Enum {
  const RenderResult.fromRawValue(super.rawValue);
  static const rendered = RenderResult.fromRawValue(0);

  /// The map had nothing new to draw.
  static const noUpdate = RenderResult.fromRawValue(1);

  /// The target couldn't take a frame, such as while a window is hidden or
  /// destroyed, or after `RenderSessionHandle.releaseTarget()`. Demand a frame
  /// again later.
  static const targetNotReady = RenderResult.fromRawValue(2);

  /// A later demand with the same flags replaced this one.
  static const superseded = RenderResult.fromRawValue(3);

  /// The session failed, and `RenderSessionHandle.getFailure()` reports why.
  /// Once `RenderSessionHandle.release()` completes, you can attach a new
  /// session.
  static const failed = RenderResult.fromRawValue(4);
  @override
  String toString() => switch (rawValue) {
    0 => 'RenderResult.rendered',
    1 => 'RenderResult.noUpdate',
    2 => 'RenderResult.targetNotReady',
    3 => 'RenderResult.superseded',
    4 => 'RenderResult.failed',
    _ => 'RenderResult($rawValue)',
  };
}

/// Something that a render session can do.
///
/// See `mln_render_session_capability_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class RenderSessionCapabilityFlag
    extends _Flags<RenderSessionCapabilityFlag> {
  const RenderSessionCapabilityFlag.fromRawValue(super.rawValue);

  /// `RenderSessionHandle.acquireFrame()` can take the frames it renders.
  static const frameAcquisition = RenderSessionCapabilityFlag.fromRawValue(1);

  /// `RenderSessionHandle.readTexture()` can copy its texture to memory.
  static const readback = RenderSessionCapabilityFlag.fromRawValue(2);

  /// It shows frames on a surface.
  static const presentation = RenderSessionCapabilityFlag.fromRawValue(4);

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
    4: 'presentation',
  });
}

/// Where a render session is in its life.
///
/// See `mln_render_session_state` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
final class RenderSessionState extends _Enum {
  const RenderSessionState.fromRawValue(super.rawValue);
  static const attaching = RenderSessionState.fromRawValue(1);
  static const attached = RenderSessionState.fromRawValue(2);
  static const detaching = RenderSessionState.fromRawValue(3);
  static const detached = RenderSessionState.fromRawValue(4);
  static const failed = RenderSessionState.fromRawValue(5);
  static const abandoned = RenderSessionState.fromRawValue(6);
  @override
  String toString() => switch (rawValue) {
    1 => 'RenderSessionState.attaching',
    2 => 'RenderSessionState.attached',
    3 => 'RenderSessionState.detaching',
    4 => 'RenderSessionState.detached',
    5 => 'RenderSessionState.failed',
    6 => 'RenderSessionState.abandoned',
    _ => 'RenderSessionState($rawValue)',
  };
}

/// Why a resource request failed.
///
/// See `mln_resource_error_reason` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class ResourceErrorReason extends _Enum {
  const ResourceErrorReason.fromRawValue(super.rawValue);

  /// A reason that no other value names.
  static const other = ResourceErrorReason.fromRawValue(0);
  static const notFound = ResourceErrorReason.fromRawValue(1);

  /// The server reported an error.
  static const server = ResourceErrorReason.fromRawValue(2);

  /// The server couldn't be reached.
  static const connection = ResourceErrorReason.fromRawValue(3);
  static const rateLimit = ResourceErrorReason.fromRawValue(4);
  @override
  String toString() => switch (rawValue) {
    0 => 'ResourceErrorReason.other',
    1 => 'ResourceErrorReason.notFound',
    2 => 'ResourceErrorReason.server',
    3 => 'ResourceErrorReason.connection',
    4 => 'ResourceErrorReason.rateLimit',
    _ => 'ResourceErrorReason($rawValue)',
  };
}

/// The kind of a resource that the library loads.
///
/// See `mln_resource_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
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

/// A resource kind, as a bit in a set.
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

/// Where a request may load its resource from.
///
/// See `mln_resource_loading_method` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class ResourceLoadingMethod extends _Enum {
  const ResourceLoadingMethod.fromRawValue(super.rawValue);

  /// The cache or the network.
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

/// The priority of a resource request.
///
/// See `mln_resource_priority` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
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

/// The outcome of a resource request.
///
/// See `mln_resource_response_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class ResourceResponseStatus extends _Enum {
  const ResourceResponseStatus.fromRawValue(super.rawValue);
  static const ok = ResourceResponseStatus.fromRawValue(0);
  static const error = ResourceResponseStatus.fromRawValue(1);

  /// The resource exists and is empty.
  static const noContent = ResourceResponseStatus.fromRawValue(2);

  /// The earlier copy that the request's `prior_` fields describe is current.
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

/// Whether the library keeps a loaded resource in its cache.
///
/// See `mln_resource_storage_policy` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
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

/// Whether a request loads a resource for a map or an offline region.
///
/// See `mln_resource_usage` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
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

/// An event type, as a bit in a set.
///
/// See `mln_runtime_event_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventFlag extends _Flags<RuntimeEventFlag> {
  const RuntimeEventFlag.fromRawValue(super.rawValue);
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
  static const mapGlyphsRequested = RuntimeEventFlag.fromRawValue(1048576);
  static const mapGlyphsLoaded = RuntimeEventFlag.fromRawValue(2097152);
  static const mapGlyphsError = RuntimeEventFlag.fromRawValue(4194304);
  static const mapSpriteRequested = RuntimeEventFlag.fromRawValue(8388608);
  static const mapSpriteLoaded = RuntimeEventFlag.fromRawValue(16777216);
  static const mapSpriteError = RuntimeEventFlag.fromRawValue(33554432);
  static const allMapEvents = RuntimeEventFlag.fromRawValue(66191358);
  static const allRuntimeEvents = RuntimeEventFlag.fromRawValue(917504);
  static const all = RuntimeEventFlag.fromRawValue(67108862);
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
    1048576: 'mapGlyphsRequested',
    2097152: 'mapGlyphsLoaded',
    4194304: 'mapGlyphsError',
    8388608: 'mapSpriteRequested',
    16777216: 'mapSpriteLoaded',
    33554432: 'mapSpriteError',
    66191358: 'allMapEvents',
    917504: 'allRuntimeEvents',
    67108862: 'all',
  });
}

/// The kind of handle that raised an event.
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

/// The kind of an event.
///
/// Only maps in `MapMode.continuous` raise
/// `RuntimeEventType.mapLoadingFinished`, `RuntimeEventType.mapIdle`, and the
/// render frame and render map events. Offline region events come only for the
/// regions and types that `RuntimeHandle.setOfflineRegionObserved()` selects.
///
/// See `mln_runtime_event_type` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventType extends _Enum {
  const RuntimeEventType.fromRawValue(super.rawValue);
  static const mapCameraWillChange = RuntimeEventType.fromRawValue(1);
  static const mapCameraIsChanging = RuntimeEventType.fromRawValue(2);
  static const mapCameraDidChange = RuntimeEventType.fromRawValue(3);
  static const mapStyleLoaded = RuntimeEventType.fromRawValue(4);

  /// The map started to load a style.
  static const mapLoadingStarted = RuntimeEventType.fromRawValue(5);

  /// The map loaded its style and everything that the current view needs, and
  /// rendered the result.
  static const mapLoadingFinished = RuntimeEventType.fromRawValue(6);

  /// The map failed to load a style.
  static const mapLoadingFailed = RuntimeEventType.fromRawValue(7);

  /// The map rendered a complete frame and has nothing left to load, animate,
  /// or redraw.
  static const mapIdle = RuntimeEventType.fromRawValue(8);
  static const mapRenderError = RuntimeEventType.fromRawValue(9);
  static const mapRenderFrameStarted = RuntimeEventType.fromRawValue(10);
  static const mapRenderFrameFinished = RuntimeEventType.fromRawValue(11);
  static const mapRenderMapStarted = RuntimeEventType.fromRawValue(12);
  static const mapRenderMapFinished = RuntimeEventType.fromRawValue(13);

  /// The style uses an image that it doesn't have.
  static const mapStyleImageMissing = RuntimeEventType.fromRawValue(14);

  /// A tile moved to another step of loading.
  static const mapTileAction = RuntimeEventType.fromRawValue(15);

  /// A source in the style changed, such as when its TileJSON loads.
  static const mapSourceChanged = RuntimeEventType.fromRawValue(16);
  static const offlineRegionStatusChanged = RuntimeEventType.fromRawValue(17);

  /// A resource that the region needs failed to download.
  static const offlineRegionResponseError = RuntimeEventType.fromRawValue(18);

  /// The region's download became inactive because the cache file's offline
  /// regions hold as many tiles from MapLibre's demo tile server as the limit
  /// allows. Only tiles of sources with a `maplibre://` URL count toward the
  /// limit.
  static const offlineRegionTileCountLimitExceeded =
      RuntimeEventType.fromRawValue(19);
  static const mapGlyphsRequested = RuntimeEventType.fromRawValue(20);
  static const mapGlyphsLoaded = RuntimeEventType.fromRawValue(21);
  static const mapGlyphsError = RuntimeEventType.fromRawValue(22);
  static const mapSpriteRequested = RuntimeEventType.fromRawValue(23);
  static const mapSpriteLoaded = RuntimeEventType.fromRawValue(24);
  static const mapSpriteError = RuntimeEventType.fromRawValue(25);
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
    20 => 'RuntimeEventType.mapGlyphsRequested',
    21 => 'RuntimeEventType.mapGlyphsLoaded',
    22 => 'RuntimeEventType.mapGlyphsError',
    23 => 'RuntimeEventType.mapSpriteRequested',
    24 => 'RuntimeEventType.mapSpriteLoaded',
    25 => 'RuntimeEventType.mapSpriteError',
    _ => 'RuntimeEventType($rawValue)',
  };
}

/// The result of a call or of asynchronous work.
///
/// See `mln_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class Status extends _Enum {
  const Status.fromRawValue(super.rawValue);
  static const ok = Status.fromRawValue(0);

  /// An argument is invalid, such as malformed JSON or a value out of range.
  static const invalidArgument = Status.fromRawValue(-1);

  /// The call doesn't fit the current state, such as removing a source that a
  /// layer still uses.
  static const invalidState = Status.fromRawValue(-2);

  /// The call came from a thread that can't make it.
  static const wrongThread = Status.fromRawValue(-3);

  /// The platform, the build, or the render target lacks the feature.
  static const unsupported = Status.fromRawValue(-4);

  /// The library or the platform hit an error, such as a failed graphics driver
  /// call.
  static const nativeError = Status.fromRawValue(-5);

  /// The work stopped before it finished, such as when its handle was released.
  static const cancelled = Status.fromRawValue(-6);

  /// Another call is still using the same object.
  static const busy = Status.fromRawValue(-7);

  /// The render target can't be drawn to anymore.
  static const targetLost = Status.fromRawValue(-8);

  /// Nothing has the given ID.
  static const notFound = Status.fromRawValue(-10);

  /// Something already has the given ID.
  static const alreadyExists = Status.fromRawValue(-11);
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
    -10 => 'Status.notFound',
    -11 => 'Status.alreadyExists',
    _ => 'Status($rawValue)',
  };
}

/// The type of a style expression's result, or of the items of an array result.
///
/// See `mln_style_expression_type_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
final class StyleExpressionTypeKind extends _Enum {
  const StyleExpressionTypeKind.fromRawValue(super.rawValue);

  /// Any type.
  static const value = StyleExpressionTypeKind.fromRawValue(0);
  static const number = StyleExpressionTypeKind.fromRawValue(1);
  static const string = StyleExpressionTypeKind.fromRawValue(2);
  static const boolean = StyleExpressionTypeKind.fromRawValue(3);
  static const color = StyleExpressionTypeKind.fromRawValue(4);
  static const object = StyleExpressionTypeKind.fromRawValue(5);
  static const formatted = StyleExpressionTypeKind.fromRawValue(6);
  static const image = StyleExpressionTypeKind.fromRawValue(7);
  static const padding = StyleExpressionTypeKind.fromRawValue(8);
  static const variableAnchorOffsetCollection =
      StyleExpressionTypeKind.fromRawValue(9);
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

/// How an image stretches to fit text along one axis.
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

/// Whether a layer shows.
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

/// How a raster DEM tile encodes elevation in its colors.
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

/// The type of a style source.
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
  static const customGeometry = StyleSourceType.fromRawValue(8);
  static const customVector = StyleSourceType.fromRawValue(9);
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
    8 => 'StyleSourceType.customGeometry',
    9 => 'StyleSourceType.customVector',
    _ => 'StyleSourceType($rawValue)',
  };
}

/// Whether a tileset counts tile rows from the north (XYZ) or the south (TMS).
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

/// The format of vector tiles.
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

/// How the map picks the level of detail of its tiles.
///
/// See `mln_tile_lod_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class TileLodMode extends _Enum {
  const TileLodMode.fromRawValue(super.rawValue);

  /// Uses the most detail at the center of the screen.
  static const defaultValue = TileLodMode.fromRawValue(0);

  /// Uses the most detail closest to the camera.
  static const distance = TileLodMode.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'TileLodMode.defaultValue',
    1 => 'TileLodMode.distance',
    _ => 'TileLodMode($rawValue)',
  };
}

/// A step in loading a tile.
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

/// How a URL pattern matches a request's URL.
///
/// A pattern matches the whole URL, query included. Without flags, it matches
/// only the URL that equals it exactly.
///
/// See `mln_url_match_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class UrlMatchFlag extends _Flags<UrlMatchFlag> {
  const UrlMatchFlag.fromRawValue(super.rawValue);

  /// The URL matches the pattern as a glob. `*` matches any characters except
  /// `/`, `**` matches any characters, and `?` matches one character except
  /// `/`. `\` makes the next character match only itself. For example,
  /// `https://*.example.com/tiles/**` matches every URL under `/tiles/` on each
  /// subdomain of `example.com`.
  static const glob = UrlMatchFlag.fromRawValue(1);

  /// No flags.
  static const none = UrlMatchFlag.fromRawValue(0);
  @override
  UrlMatchFlag _of(int rawValue) => UrlMatchFlag.fromRawValue(rawValue);
  @override
  String toString() => _describe('UrlMatchFlag', const {0: 'none', 1: 'glob'});
}

/// The vertical direction of the image that the map renders.
///
/// See `mln_viewport_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class ViewportMode extends _Enum {
  const ViewportMode.fromRawValue(super.rawValue);
  static const defaultValue = ViewportMode.fromRawValue(0);

  /// Renders the image upside down.
  static const flippedY = ViewportMode.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'ViewportMode.defaultValue',
    1 => 'ViewportMode.flippedY',
    _ => 'ViewportMode($rawValue)',
  };
}

/// The queue family that a borrowed image goes to after each frame.
///
/// With `VulkanImageOwner.external` or `VulkanImageOwner.foreign`, acquire the
/// image on your side before you read it. You don't need to release it back.
///
/// See `mln_vulkan_image_owner` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class VulkanImageOwner extends _Enum {
  const VulkanImageOwner.fromRawValue(super.rawValue);

  /// The image stays with the queue family of `graphics_queue_family_index`.
  static const graphicsQueue = VulkanImageOwner.fromRawValue(0);

  /// Each frame releases the image to VK_QUEUE_FAMILY_EXTERNAL.
  static const external = VulkanImageOwner.fromRawValue(1);

  /// Each frame releases the image to VK_QUEUE_FAMILY_FOREIGN_EXT.
  static const foreign = VulkanImageOwner.fromRawValue(2);
  @override
  String toString() => switch (rawValue) {
    0 => 'VulkanImageOwner.graphicsQueue',
    1 => 'VulkanImageOwner.external',
    2 => 'VulkanImageOwner.foreign',
    _ => 'VulkanImageOwner($rawValue)',
  };
}

/// How a render session gets its WebGL context.
///
/// See `mln_web_gl_context_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class WebGlContextKind extends _Enum {
  const WebGlContextKind.fromRawValue(super.rawValue);

  /// The session uses a context that you created with
  /// `emscripten_webgl_create_context()`.
  static const existing = WebGlContextKind.fromRawValue(0);

  /// The library transfers the canvas that `canvas_selector` names to a worker
  /// and creates the context there.
  static const transferredCanvas = WebGlContextKind.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'WebGlContextKind.existing',
    1 => 'WebGlContextKind.transferredCanvas',
    _ => 'WebGlContextKind($rawValue)',
  };
}

/// The Android hardware buffer of an acquired frame.
///
/// See `mln_android_hardware_buffer_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
final class AndroidHardwareBufferFrame extends _Value {
  const AndroidHardwareBufferFrame({
    this.width = 0,
    this.height = 0,
    this.scaleFactor = 0,
    this.slot = 0,
    this.buffer = NativePointer.nullPointer,
  });
  final int width;
  final int height;
  final double scaleFactor;

  /// The buffer's index in the ring.
  final int slot;

  /// The `AHardwareBuffer*`.
  final NativePointer buffer;

  @override
  List<Object?> get _members => [width, height, scaleFactor, slot, buffer];
  @override
  String toString() => _describe('AndroidHardwareBufferFrame', const [
    'width',
    'height',
    'scaleFactor',
    'slot',
    'buffer',
  ]);
}

/// Where one plane of a dmabuf keeps its data.
///
/// See `mln_dma_buf_plane` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/linux_8h.html).
final class DmaBufPlane extends _Value {
  const DmaBufPlane({this.fd = 0, this.offset = 0, this.stride = 0});

  /// The file descriptor of the dmabuf that holds the plane. The planes of a
  /// buffer often share one.
  final int fd;

  /// The bytes from the start of the dmabuf to the plane's first byte.
  final int offset;

  /// The bytes from the start of one row to the start of the next.
  final int stride;

  @override
  List<Object?> get _members => [fd, offset, stride];
  @override
  String toString() =>
      _describe('DmaBufPlane', const ['fd', 'offset', 'stride']);
}

/// The dmabuf of an acquired frame.
///
/// See `mln_dma_buf_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/linux_8h.html).
final class DmaBufFrame extends _Value {
  DmaBufFrame({
    this.width = 0,
    this.height = 0,
    this.scaleFactor = 0,
    this.slot = 0,
    this.fourcc = 0,
    this.modifier = 0,
    List<DmaBufPlane> planes = const [],
  }) : planes = List.unmodifiable(planes);
  final int width;
  final int height;
  final double scaleFactor;

  /// The buffer's index in the ring.
  final int slot;
  final int fourcc;
  final int modifier;

  /// The buffer's planes, in the order that its fourcc and modifier define.
  /// Import all of them, as EGL_EXT_image_dma_buf_import_modifiers planes 0 and
  /// up, or with one `zwp_linux_buffer_params_v1.add` call each.
  final List<DmaBufPlane> planes;

  @override
  List<Object?> get _members => [
    width,
    height,
    scaleFactor,
    slot,
    fourcc,
    modifier,
    planes,
  ];
  @override
  String toString() => _describe('DmaBufFrame', const [
    'width',
    'height',
    'scaleFactor',
    'slot',
    'fourcc',
    'modifier',
    'planes',
  ]);
}

/// The DXGI shared texture of an acquired frame.
///
/// See `mln_dxgi_shared_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/win32_8h.html).
final class DxgiSharedTextureFrame extends _Value {
  const DxgiSharedTextureFrame({
    this.width = 0,
    this.height = 0,
    this.scaleFactor = 0,
    this.slot = 0,
    this.format = 0,
    this.handle = NativePointer.nullPointer,
  });
  final int width;
  final int height;
  final double scaleFactor;

  /// The texture's index in the ring.
  final int slot;

  /// The texture's `DXGI_FORMAT`.
  final int format;

  /// The texture's shared NT `HANDLE`.
  final NativePointer handle;

  @override
  List<Object?> get _members => [
    width,
    height,
    scaleFactor,
    slot,
    format,
    handle,
  ];
  @override
  String toString() => _describe('DxgiSharedTextureFrame', const [
    'width',
    'height',
    'scaleFactor',
    'slot',
    'format',
    'handle',
  ]);
}

/// The IOSurface of an acquired frame.
///
/// See `mln_io_surface_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/apple_8h.html).
final class IoSurfaceFrame extends _Value {
  const IoSurfaceFrame({
    this.width = 0,
    this.height = 0,
    this.scaleFactor = 0,
    this.slot = 0,
    this.surface = NativePointer.nullPointer,
  });
  final int width;
  final int height;
  final double scaleFactor;

  /// The surface's index in the ring.
  final int slot;

  /// The `IOSurfaceRef`.
  final NativePointer surface;

  @override
  List<Object?> get _members => [width, height, scaleFactor, slot, surface];
  @override
  String toString() => _describe('IoSurfaceFrame', const [
    'width',
    'height',
    'scaleFactor',
    'slot',
    'surface',
  ]);
}

/// The Metal texture of an acquired frame.
///
/// See `mln_metal_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class MetalTextureFrame extends _Value {
  const MetalTextureFrame({
    this.width = 0,
    this.height = 0,
    this.scaleFactor = 0,
    this.slot = 0,
    this.texture = NativePointer.nullPointer,
    this.device = NativePointer.nullPointer,
    this.pixelFormat = 0,
  });
  final int width;
  final int height;
  final double scaleFactor;

  /// The texture's index in the descriptor's `textures`.
  final int slot;

  /// The MTLTexture.
  final NativePointer texture;

  /// The texture's MTLDevice.
  final NativePointer device;

  /// The texture's MTLPixelFormat.
  final int pixelFormat;

  @override
  List<Object?> get _members => [
    width,
    height,
    scaleFactor,
    slot,
    texture,
    device,
    pixelFormat,
  ];
  @override
  String toString() => _describe('MetalTextureFrame', const [
    'width',
    'height',
    'scaleFactor',
    'slot',
    'texture',
    'device',
    'pixelFormat',
  ]);
}

/// The OpenGL texture of an acquired frame.
///
/// See `mln_open_gl_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class OpenGlTextureFrame extends _Value {
  const OpenGlTextureFrame({
    this.width = 0,
    this.height = 0,
    this.scaleFactor = 0,
    this.slot = 0,
    this.texture = 0,
    this.target = 0,
  });
  final int width;
  final int height;
  final double scaleFactor;

  /// The texture's index in the descriptor's `textures`.
  final int slot;
  final int texture;
  final int target;

  @override
  List<Object?> get _members => [
    width,
    height,
    scaleFactor,
    slot,
    texture,
    target,
  ];
  @override
  String toString() => _describe('OpenGlTextureFrame', const [
    'width',
    'height',
    'scaleFactor',
    'slot',
    'texture',
    'target',
  ]);
}

/// A point in GPU work that the other side waits for before using a texture.
///
/// See `mln_gpu_sync` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class GpuSync extends _Value {
  const GpuSync({
    this.kind = const GpuSyncKind.fromRawValue(0),
    this.handle = 0,
    this.value = 0,
  });
  final GpuSyncKind kind;

  /// The object to wait on, for the kinds that name one.
  final int handle;
  final int value;

  /// A copy of this value with the given members replaced.
  GpuSync copyWith({GpuSyncKind? kind, int? handle, int? value}) => GpuSync(
    kind: kind ?? this.kind,
    handle: handle ?? this.handle,
    value: value ?? this.value,
  );

  @override
  List<Object?> get _members => [kind, handle, value];
  @override
  String toString() => _describe('GpuSync', const ['kind', 'handle', 'value']);
}

/// A geographic coordinate, in degrees.
///
/// See `mln_lat_lng` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class LatLng extends _Value {
  const LatLng({this.latitude = 0, this.longitude = 0});
  final double latitude;
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

/// Distances in logical pixels from each edge of the map.
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

/// A map's whole camera.
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

  /// The altitude of the center, in meters above sea level.
  final double centerAltitude;

  /// Insets from the map's edges. The camera centers within the rest.
  final EdgeInsets padding;
  final double zoom;

  /// The rotation from north, in degrees.
  final double bearing;

  /// The tilt toward the horizon, in degrees. 0 looks straight down.
  final double pitch;

  /// The roll, in degrees.
  final double roll;

  /// The vertical field of view, in degrees.
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

  /// Converts this `Camera` to `CameraOptions` that set each of its members.
  CameraOptions toOptions() => CameraOptions(
    center: center,
    centerAltitude: centerAltitude,
    padding: padding,
    zoom: zoom,
    bearing: bearing,
    pitch: pitch,
    roll: roll,
    fieldOfView: fieldOfView,
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

/// A point on the screen, in logical pixels from the map's top-left corner.
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

/// A camera, with only the members that `fields` selects.
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

  /// The altitude of the center, in meters above sea level.
  final double? centerAltitude;

  /// Insets from the map's edges. The camera centers within the rest.
  final EdgeInsets? padding;

  /// The screen point that a change to the zoom, bearing, or pitch keeps in
  /// place. A `center` overrides it.
  final ScreenPoint? anchor;
  final double? zoom;

  /// The rotation from north, in degrees.
  final double? bearing;

  /// The tilt toward the horizon, in degrees. 0 looks straight down.
  final double? pitch;

  /// The roll, in degrees.
  final double? roll;

  /// The vertical field of view, in degrees.
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

/// A size in logical pixels, and the physical pixels in each logical pixel.
///
/// See `mln_logical_extent` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class LogicalExtent extends _Value {
  const LogicalExtent({this.width = 0, this.height = 0, this.scaleFactor = 0});
  final int width;
  final int height;
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

/// A map's whole projection mode.
///
/// Its members mean the same as those of `ProjectionModeOptions`.
///
/// See `mln_projection_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class ProjectionMode extends _Value {
  const ProjectionMode({
    this.axonometric = false,
    this.xSkew = 0,
    this.ySkew = 0,
  });
  final bool axonometric;
  final double xSkew;
  final double ySkew;

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

/// A map's whole viewport.
///
/// Its members mean the same as those of `MapViewportOptions`.
///
/// See `mln_map_viewport` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class MapViewport extends _Value {
  const MapViewport({
    this.northOrientation = const NorthOrientation.fromRawValue(0),
    this.constrainMode = const ConstrainMode.fromRawValue(0),
    this.viewportMode = const ViewportMode.fromRawValue(0),
    this.frustumOffset = const EdgeInsets(),
  });
  final NorthOrientation northOrientation;
  final ConstrainMode constrainMode;
  final ViewportMode viewportMode;
  final EdgeInsets frustumOffset;

  /// A copy of this value with the given members replaced.
  MapViewport copyWith({
    NorthOrientation? northOrientation,
    ConstrainMode? constrainMode,
    ViewportMode? viewportMode,
    EdgeInsets? frustumOffset,
  }) => MapViewport(
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
  String toString() => _describe('MapViewport', const [
    'northOrientation',
    'constrainMode',
    'viewportMode',
    'frustumOffset',
  ]);
}

/// One view of a map: its camera, size, and projection.
///
/// See `mln_map_view` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class MapView extends _Value {
  const MapView({
    this.camera = const Camera(),
    this.extent = const LogicalExtent(),
    this.projectionMode = const ProjectionMode(),
    this.viewport = const MapViewport(),
  });
  final Camera camera;
  final LogicalExtent extent;
  final ProjectionMode projectionMode;
  final MapViewport viewport;

  /// A copy of this value with the given members replaced.
  MapView copyWith({
    Camera? camera,
    LogicalExtent? extent,
    ProjectionMode? projectionMode,
    MapViewport? viewport,
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

/// How one frame demand finished.
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
    this.mode = const RenderMode.fromRawValue(0),
  });
  final RenderResult disposition;
  final int token;

  /// The map update that a rendered frame shows. The frame includes a command's
  /// change when this is at least the command's
  /// `CommandCompletion.renderUpdateGeneration`.
  final int mapUpdateGeneration;
  final int generation;
  final int frameGeneration;
  final bool needsRepaint;

  /// The camera and extent that a rendered frame shows.
  final MapView view;
  final RenderMode mode;

  @override
  List<Object?> get _members => [
    disposition,
    token,
    mapUpdateGeneration,
    generation,
    frameGeneration,
    needsRepaint,
    view,
    mode,
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
    'mode',
  ]);
}

/// The Vulkan image of an acquired frame.
///
/// See `mln_vulkan_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class VulkanTextureFrame extends _Value {
  const VulkanTextureFrame({
    this.width = 0,
    this.height = 0,
    this.scaleFactor = 0,
    this.slot = 0,
    this.image = 0,
    this.imageView = 0,
    this.device = NativePointer.nullPointer,
    this.format = 0,
    this.layout = 0,
  });
  final int width;
  final int height;
  final double scaleFactor;

  /// The image's index in the descriptor's `textures`.
  final int slot;

  /// The VkImage.
  final int image;

  /// The VkImageView.
  final int imageView;

  /// The image's VkDevice.
  final NativePointer device;

  /// The image's VkFormat.
  final int format;

  /// The VkImageLayout that the frame left the image in.
  final int layout;

  @override
  List<Object?> get _members => [
    width,
    height,
    scaleFactor,
    slot,
    image,
    imageView,
    device,
    format,
    layout,
  ];
  @override
  String toString() => _describe('VulkanTextureFrame', const [
    'width',
    'height',
    'scaleFactor',
    'slot',
    'image',
    'imageView',
    'device',
    'format',
    'layout',
  ]);
}

/// Options for creating a frame pacer.
///
/// See `mln_frame_pacer_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
final class FramePacerOptions extends _Value {
  const FramePacerOptions({this.maximumFramesPerSecond});

  /// The most frames per second that the pacer demands, greater than zero. The
  /// pacer skips display refreshes to stay within it. By default, it can demand
  /// frames at every refresh.
  final int? maximumFramesPerSecond;

  /// A copy of this value with the given members replaced.
  FramePacerOptions copyWith({int? maximumFramesPerSecond}) =>
      FramePacerOptions(
        maximumFramesPerSecond:
            maximumFramesPerSecond ?? this.maximumFramesPerSecond,
      );

  @override
  List<Object?> get _members => [maximumFramesPerSecond];
  @override
  String toString() =>
      _describe('FramePacerOptions', const ['maximumFramesPerSecond']);
}

/// A tile's zoom level `z`, column `x`, and row `y`, with row 0 at the north.
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

/// A size in logical pixels, which the map's scale factor turns into pixels.
///
/// See `mln_logical_size` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class LogicalSize extends _Value {
  const LogicalSize({this.width = 0, this.height = 0});
  final int width;
  final int height;

  /// A copy of this value with the given members replaced.
  LogicalSize copyWith({int? width, int? height}) =>
      LogicalSize(width: width ?? this.width, height: height ?? this.height);

  @override
  List<Object?> get _members => [width, height];
  @override
  String toString() => _describe('LogicalSize', const ['width', 'height']);
}

/// Options for a GeoJSON
/// \[source\](https://maplibre.org/maplibre-style-spec/sources/#geojson).
///
/// Zoom levels are whole numbers.
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
  });
  final double? minZoom;
  final double? maxZoom;
  final double? tolerance;

  /// The last zoom level that has clusters, at most 30.
  final double? clusterMaxZoom;

  /// The `clusterProperties` object, or empty for none.
  final Object? clusterProperties;
  final int? buffer;
  final int? clusterRadius;

  /// At most 4294967295 on a 32-bit target.
  final int? clusterMinPoints;
  final bool? lineMetrics;
  final bool? cluster;

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
  ]);
}

/// A point in the Web Mercator projection, in meters.
///
/// See `mln_projected_meters` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class ProjectedMeters extends _Value {
  const ProjectedMeters({this.northing = 0, this.easting = 0});
  final double northing;
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

/// Receives a log message.
///
/// The callback runs asynchronously in the isolate and zone that registered it,
/// after native queues the call.
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

/// A log callback, for `setLogCallback()`.
///
/// The callback runs asynchronously in the isolate and zone that registered it,
/// after native queues the call.
///
/// See `mln_log_handler` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
final class LogHandler {
  const LogHandler({
    required this.callback,
    this.platformSeverities = LogSeverityFlag.none,
  });
  final LogCallback callback;

  /// The severities that also go to the platform's log, as they do without a
  /// callback. 0 sends none. A message of these severities goes to the
  /// platform's log even when the handler's queue can't accept it.
  final LogSeverityFlag platformSeverities;
}

/// Asks for the data of a tile in a custom source.
///
/// `map` and `sourceId` name the source that asks. Return quickly. Once the
/// data is ready, pass it to `MapHandle.setCustomGeometrySourceTileData()` or
/// `MapHandle.setCustomVectorSourceTileData()`. If it fails to load, report
/// that with `MapHandle.setCustomGeometrySourceTileError()` or
/// `MapHandle.setCustomVectorSourceTileError()`.
///
/// See `mln_custom_source_fetch_tile_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
typedef CustomSourceFetchTileCallback =
    void Function(MapHandle map, String sourceId, CanonicalTileId tileId);

/// Reports that a custom source no longer needs a tile.
///
/// `map` and `sourceId` name the source.
///
/// See `mln_custom_source_cancel_tile_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
typedef CustomSourceCancelTileCallback =
    void Function(MapHandle map, String sourceId, CanonicalTileId tileId);

/// The callbacks and options of a custom geometry source.
///
/// Zoom levels are whole numbers from 0 to 32.
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
  final CustomSourceFetchTileCallback fetchTile;
  final CustomSourceCancelTileCallback? cancelTile;
  final double? minZoom;
  final double? maxZoom;

  /// How far, in pixels, simplified geometry can stray from the original.
  final double? tolerance;

  /// The width of a tile in pixels, a power of two no larger than 8192.
  final int? tileSize;

  /// The width in pixels of the margin around a tile that holds geometry from
  /// beyond its edges. `buffer` times 8192 / `tile_size` must be at most 65535.
  final int? buffer;

  /// Whether to cut geometry at the outer edge of a tile's margin.
  final bool? clip;

  /// Whether to wrap geometry that crosses the antimeridian.
  final bool? wrap;
}

/// The callbacks and options of a custom vector source.
///
/// Zoom levels are whole numbers from 0 to 32.
///
/// See `mln_custom_vector_source_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class CustomVectorSourceOptions {
  const CustomVectorSourceOptions({
    required this.fetchTile,
    this.cancelTile,
    this.minZoom,
    this.maxZoom,
  });
  final CustomSourceFetchTileCallback fetchTile;
  final CustomSourceCancelTileCallback? cancelTile;
  final double? minZoom;
  final double? maxZoom;
}

/// An image of 8-bit RGBA pixels whose colors are multiplied by their alpha.
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
  final int width;
  final int height;

  /// The number of bytes from the start of one row to the start of the next, at
  /// least `width` \* 4.
  final int stride;

  /// The rows of pixels, `stride` bytes apart, from the top row down.
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

/// How one part of a gesture keeps moving after its release.
///
/// Speeds and distances are in the part's unit: logical pixels for pan, zoom
/// levels for zoom, and degrees for bearing and pitch. A setting whose bit
/// isn't set takes the part's default, which `GestureMomentum` lists. Every
/// setting is finite and not negative.
///
/// See `mln_momentum_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class MomentumOptions extends _Value {
  const MomentumOptions({
    this.decay,
    this.speedScale,
    this.minSpeed,
    this.maxSpeed,
    this.exponent,
    this.baseDurationMs,
    this.durationMsPerSpeed,
    this.decayRate,
    this.stopSpeed,
    this.durationScale,
    this.maxDurationMs,
    this.maxDistance,
  });
  final MomentumDecay? decay;

  /// The momentum starts at this multiple of the release speed.
  final double? speedScale;

  /// A slower release starts no momentum.
  final double? minSpeed;

  /// A faster release counts as this speed.
  final double? maxSpeed;

  /// For `MomentumDecay.power`. At least 1.
  final double? exponent;

  /// For `MomentumDecay.power`, the duration is `base_duration_ms` plus
  /// `duration_ms_per_speed` times the release speed.
  final double? baseDurationMs;
  final double? durationMsPerSpeed;

  /// Required for `MomentumDecay.exponential`, and positive.
  final double? decayRate;

  /// Required for `MomentumDecay.exponential`, and positive.
  final double? stopSpeed;

  /// Stretches the momentum in time without changing its starting speed, so it
  /// also scales the distance. 0 starts no momentum. A host can pass its
  /// platform's animation duration scale here.
  final double? durationScale;

  /// The longest duration, after `duration_scale` applies.
  final double? maxDurationMs;

  /// The farthest the momentum moves. A momentum that would move farther slows
  /// faster over the same duration.
  final double? maxDistance;

  /// A copy of this value with the given members replaced.
  MomentumOptions copyWith({
    MomentumDecay? decay,
    double? speedScale,
    double? minSpeed,
    double? maxSpeed,
    double? exponent,
    double? baseDurationMs,
    double? durationMsPerSpeed,
    double? decayRate,
    double? stopSpeed,
    double? durationScale,
    double? maxDurationMs,
    double? maxDistance,
  }) => MomentumOptions(
    decay: decay ?? this.decay,
    speedScale: speedScale ?? this.speedScale,
    minSpeed: minSpeed ?? this.minSpeed,
    maxSpeed: maxSpeed ?? this.maxSpeed,
    exponent: exponent ?? this.exponent,
    baseDurationMs: baseDurationMs ?? this.baseDurationMs,
    durationMsPerSpeed: durationMsPerSpeed ?? this.durationMsPerSpeed,
    decayRate: decayRate ?? this.decayRate,
    stopSpeed: stopSpeed ?? this.stopSpeed,
    durationScale: durationScale ?? this.durationScale,
    maxDurationMs: maxDurationMs ?? this.maxDurationMs,
    maxDistance: maxDistance ?? this.maxDistance,
  );

  @override
  List<Object?> get _members => [
    decay,
    speedScale,
    minSpeed,
    maxSpeed,
    exponent,
    baseDurationMs,
    durationMsPerSpeed,
    decayRate,
    stopSpeed,
    durationScale,
    maxDurationMs,
    maxDistance,
  ];
  @override
  String toString() => _describe('MomentumOptions', const [
    'decay',
    'speedScale',
    'minSpeed',
    'maxSpeed',
    'exponent',
    'baseDurationMs',
    'durationMsPerSpeed',
    'decayRate',
    'stopSpeed',
    'durationScale',
    'maxDurationMs',
    'maxDistance',
  ]);
}

/// Where the bearing settles after a gesture that changed it.
///
/// The targets are `target_count` bearings spaced evenly around the circle,
/// starting at `target_offset`. The bearing eases to the nearest one when it is
/// within `tolerance` of it, and otherwise stays where it is.
///
/// See `mln_bearing_snap` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class BearingSnap extends _Value {
  const BearingSnap({
    this.targetCount,
    this.targetOffset,
    this.tolerance,
    this.durationMs,
  });

  /// At least 1. Defaults to 1.
  final int? targetCount;

  /// In degrees. Defaults to 0, north.
  final double? targetOffset;

  /// In degrees, from 0 to 180. Defaults to 7.
  final double? tolerance;

  /// Defaults to 300.
  final double? durationMs;

  /// A copy of this value with the given members replaced.
  BearingSnap copyWith({
    int? targetCount,
    double? targetOffset,
    double? tolerance,
    double? durationMs,
  }) => BearingSnap(
    targetCount: targetCount ?? this.targetCount,
    targetOffset: targetOffset ?? this.targetOffset,
    tolerance: tolerance ?? this.tolerance,
    durationMs: durationMs ?? this.durationMs,
  );

  @override
  List<Object?> get _members => [
    targetCount,
    targetOffset,
    tolerance,
    durationMs,
  ];
  @override
  String toString() => _describe('BearingSnap', const [
    'targetCount',
    'targetOffset',
    'tolerance',
    'durationMs',
  ]);
}

/// How a gesture keeps moving the camera after its last contact lifts.
///
/// A part moves only when its velocity is set, and each part slows on its own
/// clock. The parts' defaults are:
///
/// | Part | Exponent | Speed scale | Base ms | ms per speed | Other | | -------
/// | -------- | ----------- | ------- | ------------ | --------------- | | pan
/// | 2 | 0.56 | 150 | 1/10.5 | min speed 250 | | zoom | 4 | 1 | 600 | 0 | max
/// 600 ms, 2.5 | | bearing | 4 | 1 | 600 | 0 | max 600 ms | | pitch | 4 | 1 |
/// 300 | 0 | min speed 5 |
///
/// Every part defaults to `MomentumDecay.power`, and the zoom moves at most 2.5
/// levels.
///
/// See `mln_gesture_momentum` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class GestureMomentum extends _Value {
  const GestureMomentum({
    this.panVelocity,
    this.zoomVelocity,
    this.bearingVelocity,
    this.pitchVelocity,
    this.zoomAnchor,
    this.bearingAnchor,
    this.pan = const MomentumOptions(),
    this.zoom = const MomentumOptions(),
    this.bearing = const MomentumOptions(),
    this.pitch = const MomentumOptions(),
    this.panFollowsZoom,
    this.bearingSnap,
  });

  /// How fast the map's content moved on the screen at release, in logical
  /// pixels per second.
  final ScreenPoint? panVelocity;

  /// In zoom levels per second.
  final double? zoomVelocity;

  /// In degrees per second.
  final double? bearingVelocity;

  /// In degrees per second.
  final double? pitchVelocity;

  /// The screen point that the zoom keeps in place. Without one, the zoom keeps
  /// the center of the view in place.
  final ScreenPoint? zoomAnchor;

  /// The screen point that the rotation keeps in place. Without one, the
  /// rotation turns around the center of the view.
  final ScreenPoint? bearingAnchor;
  final MomentumOptions pan;
  final MomentumOptions zoom;
  final MomentumOptions bearing;
  final MomentumOptions pitch;

  /// When pan and zoom momentum both start, the pan slows over the zoom's
  /// duration and curve, starts no faster, and moves no farther than it would
  /// alone. Defaults to true.
  final bool? panFollowsZoom;

  /// Where the bearing settles after a gesture that changed it, once the rest
  /// of the momentum ends. Without momentum, it settles at once. Without this,
  /// the bearing stays where the gesture leaves it.
  final BearingSnap? bearingSnap;

  /// A copy of this value with the given members replaced.
  GestureMomentum copyWith({
    ScreenPoint? panVelocity,
    double? zoomVelocity,
    double? bearingVelocity,
    double? pitchVelocity,
    ScreenPoint? zoomAnchor,
    ScreenPoint? bearingAnchor,
    MomentumOptions? pan,
    MomentumOptions? zoom,
    MomentumOptions? bearing,
    MomentumOptions? pitch,
    bool? panFollowsZoom,
    BearingSnap? bearingSnap,
  }) => GestureMomentum(
    panVelocity: panVelocity ?? this.panVelocity,
    zoomVelocity: zoomVelocity ?? this.zoomVelocity,
    bearingVelocity: bearingVelocity ?? this.bearingVelocity,
    pitchVelocity: pitchVelocity ?? this.pitchVelocity,
    zoomAnchor: zoomAnchor ?? this.zoomAnchor,
    bearingAnchor: bearingAnchor ?? this.bearingAnchor,
    pan: pan ?? this.pan,
    zoom: zoom ?? this.zoom,
    bearing: bearing ?? this.bearing,
    pitch: pitch ?? this.pitch,
    panFollowsZoom: panFollowsZoom ?? this.panFollowsZoom,
    bearingSnap: bearingSnap ?? this.bearingSnap,
  );

  @override
  List<Object?> get _members => [
    panVelocity,
    zoomVelocity,
    bearingVelocity,
    pitchVelocity,
    zoomAnchor,
    bearingAnchor,
    pan,
    zoom,
    bearing,
    pitch,
    panFollowsZoom,
    bearingSnap,
  ];
  @override
  String toString() => _describe('GestureMomentum', const [
    'panVelocity',
    'zoomVelocity',
    'bearingVelocity',
    'pitchVelocity',
    'zoomAnchor',
    'bearingAnchor',
    'pan',
    'zoom',
    'bearing',
    'pitch',
    'panFollowsZoom',
    'bearingSnap',
  ]);
}

/// A timing curve from (0, 0) to (1, 1), with the control points (`x1`, `y1`)
/// and (`x2`, `y2`).
///
/// `x1` and `x2` are between 0 and 1.
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

/// How fast each part of a camera change moves.
///
/// See `mln_camera_velocity` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraVelocity extends _Value {
  const CameraVelocity({
    this.pan = const ScreenPoint(),
    this.zoom = 0,
    this.bearing = 0,
    this.pitch = 0,
  });

  /// How fast the map's content moves on the screen, in logical pixels per
  /// second.
  final ScreenPoint pan;

  /// In zoom levels per second.
  final double zoom;

  /// In degrees per second.
  final double bearing;

  /// In degrees per second.
  final double pitch;

  @override
  List<Object?> get _members => [pan, zoom, bearing, pitch];
  @override
  String toString() =>
      _describe('CameraVelocity', const ['pan', 'zoom', 'bearing', 'pitch']);
}

/// The end of a camera change.
///
/// See `mln_camera_transition_end` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraTransitionEnd extends _Value {
  const CameraTransitionEnd({
    this.outcome = const CameraTransitionOutcome.fromRawValue(0),
    this.generation = 0,
    this.velocity = const CameraVelocity(),
  });
  final CameraTransitionOutcome outcome;

  /// The generation of the first map snapshot that shows where the change left
  /// the camera, or 0 when the map closed first.
  final int generation;

  /// How fast a gesture's momentum still moved when it was cancelled, so that
  /// the next gesture can carry it on. Zero for every other end.
  final CameraVelocity velocity;

  @override
  List<Object?> get _members => [outcome, generation, velocity];
  @override
  String toString() => _describe('CameraTransitionEnd', const [
    'outcome',
    'generation',
    'velocity',
  ]);
}

/// Receives the end of a camera change.
///
/// See `mln_camera_transition_end_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
typedef CameraTransitionEndCallback = void Function(CameraTransitionEnd end);

/// A callback for the end of a camera change, for `AnimationOptions`.
///
/// The callback runs once for each camera command, after every part of the
/// change that the command started has ended.
///
/// See `mln_camera_transition_handler` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraTransitionHandler {
  const CameraTransitionHandler({this.callback});
  final CameraTransitionEndCallback? callback;
}

/// How a camera change animates.
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
    this.endHandler = const CameraTransitionHandler(),
  });
  final double? durationMs;

  /// The average speed of a fly animation, in screens per second. `duration_ms`
  /// overrides it.
  final double? velocity;

  /// The zoom level at the peak of a fly animation.
  final double? minZoom;
  final UnitBezier? easing;

  /// An ID that `MapHandle.cancelCameraTransitionsWithId()` cancels the change
  /// by. Several changes can share one.
  final int? transitionId;
  final CameraTransitionHandler endHandler;

  /// A copy of this value with the given members replaced.
  AnimationOptions copyWith({
    double? durationMs,
    double? velocity,
    double? minZoom,
    UnitBezier? easing,
    int? transitionId,
    CameraTransitionHandler? endHandler,
  }) => AnimationOptions(
    durationMs: durationMs ?? this.durationMs,
    velocity: velocity ?? this.velocity,
    minZoom: minZoom ?? this.minZoom,
    easing: easing ?? this.easing,
    transitionId: transitionId ?? this.transitionId,
    endHandler: endHandler ?? this.endHandler,
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

/// A change to the camera from where it is now, for gestures.
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
    this.momentum,
    this.animation = const AnimationOptions(),
    this.gesturePhase = const GesturePhase.fromRawValue(0),
  });

  /// Moves the map's content this far on the screen, as a drag does.
  final ScreenPoint? offset;

  /// Multiplies the map's scale. 2 zooms in by one level.
  final double? scale;

  /// Degrees to add to the bearing.
  final double? bearing;

  /// Degrees to add to the pitch.
  final double? pitch;

  /// The screen point that `scale`, `bearing`, and `pitch` keep in place. It
  /// needs one of them, and it combines with `offset` only in a delta without a
  /// duration.
  final ScreenPoint? anchor;

  /// How the gesture keeps moving after this delta. Only a delta in the
  /// `GesturePhase.end` phase carries it.
  final GestureMomentum? momentum;
  final AnimationOptions animation;
  final GesturePhase gesturePhase;

  /// A copy of this value with the given members replaced.
  CameraDelta copyWith({
    ScreenPoint? offset,
    double? scale,
    double? bearing,
    double? pitch,
    ScreenPoint? anchor,
    GestureMomentum? momentum,
    AnimationOptions? animation,
    GesturePhase? gesturePhase,
  }) => CameraDelta(
    offset: offset ?? this.offset,
    scale: scale ?? this.scale,
    bearing: bearing ?? this.bearing,
    pitch: pitch ?? this.pitch,
    anchor: anchor ?? this.anchor,
    momentum: momentum ?? this.momentum,
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
    momentum,
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
    'momentum',
    'animation',
    'gesturePhase',
  ]);
}

/// A ring of Android hardware buffers that the library allocates, for
/// `MapHandle.attachAndroidHardwareBufferRing()`.
///
/// Each buffer has the map's size in pixels, `logical_size` times the map's
/// scale factor, and the format AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM. Its
/// usage is AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT,
/// AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE, and the bits of `usage`.
///
/// See `mln_android_hardware_buffer_ring_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
final class AndroidHardwareBufferRingDescriptor extends _Value {
  const AndroidHardwareBufferRingDescriptor({
    required this.logicalSize,
    required this.bufferCount,
    this.usage = 0,
  });
  final LogicalSize logicalSize;

  /// How many buffers the ring has, from 1 to 8.
  final int bufferCount;

  /// More `AHARDWAREBUFFER_USAGE_*` bits that your reads need, such as
  /// AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, or zero.
  final int usage;

  /// A copy of this value with the given members replaced.
  AndroidHardwareBufferRingDescriptor copyWith({
    LogicalSize? logicalSize,
    int? bufferCount,
    int? usage,
  }) => AndroidHardwareBufferRingDescriptor(
    logicalSize: logicalSize ?? this.logicalSize,
    bufferCount: bufferCount ?? this.bufferCount,
    usage: usage ?? this.usage,
  );

  @override
  List<Object?> get _members => [logicalSize, bufferCount, usage];
  @override
  String toString() => _describe('AndroidHardwareBufferRingDescriptor', const [
    'logicalSize',
    'bufferCount',
    'usage',
  ]);
}

/// Tells the caller that work is ready for it.
///
/// One call can stand for several pieces of ready work, so take all of it each
/// time.
///
/// The callback runs asynchronously in the isolate and zone that registered it,
/// after native queues the call.
///
/// See `mln_wake_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wake_8h.html).
typedef WakeCallback = void Function();

/// A callback that tells the caller when work is ready.
///
/// The callback runs asynchronously in the isolate and zone that registered it,
/// after native queues the call.
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

  /// Null to turn the wake off.
  final WakeCallback? callback;
}

/// Your lock on a Vulkan queue that you and a render session both submit to.
///
/// The session holds the lock only while it submits or presents to the queue.
/// Set both `lock` and `unlock`, or neither when you don't submit to the queue
/// yourself.
///
/// It can run on a library thread, and it can't call the library.
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

/// Options for attaching a render session to a map.
///
/// See `mln_render_session_attach_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class RenderSessionAttachOptions extends _Value {
  const RenderSessionAttachOptions({
    required this.driver,
    this.producerSyncKinds = GpuSyncKindFlag.none,
    this.frameWake = const Wake(),
    this.redrawWake = const Wake(),
    this.driverWorkWake = const Wake(),
    this.queueLock,
  });

  /// The thread that runs the session's graphics work, which depends on the
  /// target:
  ///
  /// - `RenderDriverKind.callerGraphicsThread` for a shared or dedicated EGL or
  ///   WGL context, an existing WebGL context, or a browser's WebGPU device.
  /// - `RenderDriverKind.coreWorker` for a transferred WebGL canvas, an
  ///   Android, OpenHarmony, or Win32 window, a Wayland surface, a ring of
  ///   Android hardware buffers, dmabufs, DXGI shared textures, or IOSurfaces,
  ///   or an offscreen session.
  /// - Either one for another Metal or Vulkan target.
  ///
  /// An attach with another driver fails with `Status.unsupported`.
  ///
  /// An isolate cannot service a caller-graphics-thread driver, because Dart
  /// can resume it on another native thread after any `await`. Host native code
  /// services such a session on the graphics thread by passing its `id` to
  /// `mln_render_session_service_driver_work()`.
  final RenderDriverKind driver;

  /// The kinds of `GpuSync` that you can wait on before you read an acquired
  /// frame. The session uses one of them that its target supports. Zero lets
  /// the session pick a kind that names no object to wait on.
  final GpuSyncKindFlag producerSyncKinds;

  /// Called when frame results are waiting for
  /// `RenderSessionHandle.drainFrameResults()`. It isn't called again until a
  /// drain takes them.
  final Wake frameWake;

  /// Called when the session has something new to draw. Demand a frame at your
  /// next display refresh. It isn't called again until you demand a frame, or
  /// while the session has no target.
  final Wake redrawWake;

  /// Called when a session driven by `RenderDriverKind.callerGraphicsThread`
  /// has work for `mln_render_session_service_driver_work()`. It isn't called
  /// again until a service runs.
  final Wake driverWorkWake;

  /// Your lock on the Vulkan queue, when you submit to it yourself.
  final QueueLock? queueLock;

  /// A copy of this value with the given members replaced.
  RenderSessionAttachOptions copyWith({
    RenderDriverKind? driver,
    GpuSyncKindFlag? producerSyncKinds,
    Wake? frameWake,
    Wake? redrawWake,
    Wake? driverWorkWake,
    QueueLock? queueLock,
  }) => RenderSessionAttachOptions(
    driver: driver ?? this.driver,
    producerSyncKinds: producerSyncKinds ?? this.producerSyncKinds,
    frameWake: frameWake ?? this.frameWake,
    redrawWake: redrawWake ?? this.redrawWake,
    driverWorkWake: driverWorkWake ?? this.driverWorkWake,
    queueLock: queueLock ?? this.queueLock,
  );

  @override
  List<Object?> get _members => [
    driver,
    producerSyncKinds,
    frameWake,
    redrawWake,
    driverWorkWake,
    queueLock,
  ];
  @override
  String toString() => _describe('RenderSessionAttachOptions', const [
    'driver',
    'producerSyncKinds',
    'frameWake',
    'redrawWake',
    'driverWorkWake',
    'queueLock',
  ]);
}

/// An Android window to present to, for `MapHandle.attachAndroidSurface()`.
///
/// Name the window by `window`, or by `jni_env` and `surface`. The session
/// takes its own reference to the window, so you can release yours once the
/// call returns.
///
/// See `mln_android_surface_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
final class AndroidSurfaceDescriptor extends _Value {
  const AndroidSurfaceDescriptor({
    required this.logicalSize,
    this.window = NativePointer.nullPointer,
    this.jniEnv = NativePointer.nullPointer,
    this.surface = NativePointer.nullPointer,
  });
  final LogicalSize logicalSize;

  /// Your `ANativeWindow*`, or null to name the window by `surface`.
  final NativePointer window;

  /// The calling thread's `JNIEnv*`, with `surface`.
  final NativePointer jniEnv;

  /// An `android.view.Surface`, with `jni_env`.
  final NativePointer surface;

  /// A copy of this value with the given members replaced.
  AndroidSurfaceDescriptor copyWith({
    LogicalSize? logicalSize,
    NativePointer? window,
    NativePointer? jniEnv,
    NativePointer? surface,
  }) => AndroidSurfaceDescriptor(
    logicalSize: logicalSize ?? this.logicalSize,
    window: window ?? this.window,
    jniEnv: jniEnv ?? this.jniEnv,
    surface: surface ?? this.surface,
  );

  @override
  List<Object?> get _members => [logicalSize, window, jniEnv, surface];
  @override
  String toString() => _describe('AndroidSurfaceDescriptor', const [
    'logicalSize',
    'window',
    'jniEnv',
    'surface',
  ]);
}

/// A DRM format and modifier that you can import.
///
/// See `mln_dma_buf_format` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/linux_8h.html).
final class DmaBufFormat extends _Value {
  const DmaBufFormat({this.fourcc = 0, this.modifier = 0});

  /// A DRM fourcc code, such as DRM_FORMAT_ABGR8888.
  final int fourcc;

  /// A DRM format modifier, such as DRM_FORMAT_MOD_LINEAR.
  final int modifier;

  /// A copy of this value with the given members replaced.
  DmaBufFormat copyWith({int? fourcc, int? modifier}) => DmaBufFormat(
    fourcc: fourcc ?? this.fourcc,
    modifier: modifier ?? this.modifier,
  );

  @override
  List<Object?> get _members => [fourcc, modifier];
  @override
  String toString() => _describe('DmaBufFormat', const ['fourcc', 'modifier']);
}

/// A ring of dmabufs that the library allocates, for
/// `MapHandle.attachDmaBufRing()`.
///
/// Each buffer has the map's size in pixels and a format and modifier from
/// `formats`. The library renders only DRM_FORMAT_ABGR8888 and
/// DRM_FORMAT_ARGB8888. A modifier can give a buffer more planes than its
/// format has, such as a plane of compression metadata, and the library takes
/// any such modifier that its GPU can render into and share.
///
/// See `mln_dma_buf_ring_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/linux_8h.html).
final class DmaBufRingDescriptor extends _Value {
  DmaBufRingDescriptor({
    required this.logicalSize,
    required this.bufferCount,
    required List<DmaBufFormat> formats,
  }) : formats = List.unmodifiable(formats);
  final LogicalSize logicalSize;

  /// How many buffers the ring has, from 1 to 8.
  final int bufferCount;

  /// The formats and modifiers that your reader imports, most preferred first,
  /// such as those that eglQueryDmaBufModifiersEXT() reports.
  final List<DmaBufFormat> formats;

  /// A copy of this value with the given members replaced.
  DmaBufRingDescriptor copyWith({
    LogicalSize? logicalSize,
    int? bufferCount,
    List<DmaBufFormat>? formats,
  }) => DmaBufRingDescriptor(
    logicalSize: logicalSize ?? this.logicalSize,
    bufferCount: bufferCount ?? this.bufferCount,
    formats: formats ?? this.formats,
  );

  @override
  List<Object?> get _members => [logicalSize, bufferCount, formats];
  @override
  String toString() => _describe('DmaBufRingDescriptor', const [
    'logicalSize',
    'bufferCount',
    'formats',
  ]);
}

/// A ring of DXGI shared textures that the library allocates, for
/// `MapHandle.attachDxgiSharedTextureRing()`.
///
/// Each texture is a D3D11 2D texture with the map's size in pixels and the
/// format DXGI_FORMAT_B8G8R8A8_UNORM. It binds as a shader resource and a
/// render target, and is shared through an NT handle.
///
/// See `mln_dxgi_shared_texture_ring_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/win32_8h.html).
final class DxgiSharedTextureRingDescriptor extends _Value {
  const DxgiSharedTextureRingDescriptor({
    required this.logicalSize,
    required this.textureCount,
  });
  final LogicalSize logicalSize;

  /// How many textures the ring has, from 1 to 8.
  final int textureCount;

  /// A copy of this value with the given members replaced.
  DxgiSharedTextureRingDescriptor copyWith({
    LogicalSize? logicalSize,
    int? textureCount,
  }) => DxgiSharedTextureRingDescriptor(
    logicalSize: logicalSize ?? this.logicalSize,
    textureCount: textureCount ?? this.textureCount,
  );

  @override
  List<Object?> get _members => [logicalSize, textureCount];
  @override
  String toString() => _describe('DxgiSharedTextureRingDescriptor', const [
    'logicalSize',
    'textureCount',
  ]);
}

/// The Metal device that a render session uses.
///
/// See `mln_metal_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class MetalContextDescriptor extends _Value {
  const MetalContextDescriptor({this.device = NativePointer.nullPointer});

  /// An `MTLDevice`.
  final NativePointer device;

  /// A copy of this value with the given members replaced.
  MetalContextDescriptor copyWith({NativePointer? device}) =>
      MetalContextDescriptor(device: device ?? this.device);

  @override
  List<Object?> get _members => [device];
  @override
  String toString() => _describe('MetalContextDescriptor', const ['device']);
}

/// A ring of IOSurfaces that the library allocates, for
/// `MapHandle.attachIoSurfaceRing()`.
///
/// Each surface has the map's size in pixels, `logical_size` times the map's
/// scale factor, and the pixel format kCVPixelFormatType_32BGRA, which Metal
/// names MTLPixelFormatBGRA8Unorm.
///
/// See `mln_io_surface_ring_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/apple_8h.html).
final class IoSurfaceRingDescriptor extends _Value {
  const IoSurfaceRingDescriptor({
    required this.logicalSize,
    required this.bufferCount,
    this.context = const MetalContextDescriptor(),
  });
  final LogicalSize logicalSize;

  /// How many surfaces the ring has, from 1 to 8.
  final int bufferCount;

  /// The MTLDevice to render with. A null device picks the system's default.
  final MetalContextDescriptor context;

  /// A copy of this value with the given members replaced.
  IoSurfaceRingDescriptor copyWith({
    LogicalSize? logicalSize,
    int? bufferCount,
    MetalContextDescriptor? context,
  }) => IoSurfaceRingDescriptor(
    logicalSize: logicalSize ?? this.logicalSize,
    bufferCount: bufferCount ?? this.bufferCount,
    context: context ?? this.context,
  );

  @override
  List<Object?> get _members => [logicalSize, bufferCount, context];
  @override
  String toString() => _describe('IoSurfaceRingDescriptor', const [
    'logicalSize',
    'bufferCount',
    'context',
  ]);
}

/// A Metal texture of a borrowed ring.
///
/// See `mln_metal_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class MetalBorrowedTexture extends _Value {
  const MetalBorrowedTexture({required this.texture});

  /// Your MTLTexture.
  final NativePointer texture;

  /// A copy of this value with the given members replaced.
  MetalBorrowedTexture copyWith({NativePointer? texture}) =>
      MetalBorrowedTexture(texture: texture ?? this.texture);

  @override
  List<Object?> get _members => [texture];
  @override
  String toString() => _describe('MetalBorrowedTexture', const ['texture']);
}

/// A ring of your Metal textures, for `MapHandle.attachMetalBorrowedTexture()`.
///
/// `logical_size` sets the map's size. `physical_width` and `physical_height`
/// give the textures' size in pixels, which is usually `logical_size` times the
/// map's scale factor. The textures must all have that size, one device, one
/// pixel format, one sample, and MTLTextureUsageRenderTarget. The ring holds up
/// to `textureRingMaxDepth` distinct textures.
///
/// The session uses the textures until a replacement or release of the target
/// completes, a detach or release of the session completes, or an abandon of it
/// returns.
///
/// See `mln_metal_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class MetalBorrowedTextureDescriptor extends _Value {
  MetalBorrowedTextureDescriptor({
    required this.logicalSize,
    required this.physicalWidth,
    required this.physicalHeight,
    required List<MetalBorrowedTexture> textures,
    this.commandQueue = NativePointer.nullPointer,
  }) : textures = List.unmodifiable(textures);
  final LogicalSize logicalSize;
  final int physicalWidth;
  final int physicalHeight;
  final List<MetalBorrowedTexture> textures;

  /// Your MTLCommandQueue on the textures' device, or null for a queue of the
  /// session's own. With your queue, frames can use `GpuSyncKind.queueOrder`.
  final NativePointer commandQueue;

  /// A copy of this value with the given members replaced.
  MetalBorrowedTextureDescriptor copyWith({
    LogicalSize? logicalSize,
    int? physicalWidth,
    int? physicalHeight,
    List<MetalBorrowedTexture>? textures,
    NativePointer? commandQueue,
  }) => MetalBorrowedTextureDescriptor(
    logicalSize: logicalSize ?? this.logicalSize,
    physicalWidth: physicalWidth ?? this.physicalWidth,
    physicalHeight: physicalHeight ?? this.physicalHeight,
    textures: textures ?? this.textures,
    commandQueue: commandQueue ?? this.commandQueue,
  );

  @override
  List<Object?> get _members => [
    logicalSize,
    physicalWidth,
    physicalHeight,
    textures,
    commandQueue,
  ];
  @override
  String toString() => _describe('MetalBorrowedTextureDescriptor', const [
    'logicalSize',
    'physicalWidth',
    'physicalHeight',
    'textures',
    'commandQueue',
  ]);
}

/// A Metal layer to render to, for `MapHandle.attachMetalSurface()`.
///
/// The session sets the layer's device, pixel format, and drawable size. It
/// uses the layer until a replacement or release of the target completes, a
/// detach or release of the session completes, or an abandon of it returns.
///
/// See `mln_metal_surface_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
final class MetalSurfaceDescriptor extends _Value {
  const MetalSurfaceDescriptor({
    required this.logicalSize,
    this.context = const MetalContextDescriptor(),
    required this.layer,
  });
  final LogicalSize logicalSize;

  /// The MTLDevice to render with. A null device picks the system's default.
  final MetalContextDescriptor context;

  /// Your CAMetalLayer.
  final NativePointer layer;

  /// A copy of this value with the given members replaced.
  MetalSurfaceDescriptor copyWith({
    LogicalSize? logicalSize,
    MetalContextDescriptor? context,
    NativePointer? layer,
  }) => MetalSurfaceDescriptor(
    logicalSize: logicalSize ?? this.logicalSize,
    context: context ?? this.context,
    layer: layer ?? this.layer,
  );

  @override
  List<Object?> get _members => [logicalSize, context, layer];
  @override
  String toString() => _describe('MetalSurfaceDescriptor', const [
    'logicalSize',
    'context',
    'layer',
  ]);
}

/// The image that `MapHandle.attachOffscreen()` renders. The session renders it
/// at the map's scale factor.
///
/// See `mln_offscreen_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
final class OffscreenDescriptor extends _Value {
  const OffscreenDescriptor({required this.logicalSize});
  final LogicalSize logicalSize;

  /// A copy of this value with the given members replaced.
  OffscreenDescriptor copyWith({LogicalSize? logicalSize}) =>
      OffscreenDescriptor(logicalSize: logicalSize ?? this.logicalSize);

  @override
  List<Object?> get _members => [logicalSize];
  @override
  String toString() => _describe('OffscreenDescriptor', const ['logicalSize']);
}

/// The WGL objects that a render session makes its OpenGL context from.
///
/// See `mln_wgl_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class WglContextDescriptor extends _Value {
  const WglContextDescriptor({
    required this.deviceContext,
    this.shareContext = NativePointer.nullPointer,
    this.getProcAddress = NativePointer.nullPointer,
  });

  /// An `HDC`.
  final NativePointer deviceContext;

  /// Your `HGLRC`, for a shared context.
  final NativePointer shareContext;

  /// Your `wglGetProcAddress`, or null to use the system's.
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

/// The EGL objects that a render session makes its OpenGL context from.
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

  /// An `EGLDisplay`.
  final NativePointer display;

  /// An `EGLConfig`.
  final NativePointer config;

  /// Your `EGLContext`, for a shared context.
  final NativePointer shareContext;

  /// The API of a dedicated context. A shared context uses the API of yours.
  final OpenGlClientApi clientApi;

  /// Your `eglGetProcAddress`, or null to use the system's.
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

/// The WebGL context or canvas that a render session uses.
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

  /// Your `EMSCRIPTEN_WEBGL_CONTEXT_HANDLE`, for an existing context.
  final int context;

  /// The canvas's CSS selector, for a transferred canvas.
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

/// How a render session gets the OpenGL context that it renders with.
///
/// Keep the objects that it names alive until a detach or release of the
/// session completes, or an abandon of it returns.
///
/// See `mln_open_gl_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class OpenGlContextDescriptor {
  const OpenGlContextDescriptor({
    this.ownership = const OpenGlContextOwnership.fromRawValue(0),
    required this.data,
  });
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

/// An OpenGL texture of a borrowed ring.
///
/// See `mln_open_gl_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class OpenGlBorrowedTexture extends _Value {
  const OpenGlBorrowedTexture({required this.texture});

  /// Your texture name.
  final int texture;

  /// A copy of this value with the given members replaced.
  OpenGlBorrowedTexture copyWith({int? texture}) =>
      OpenGlBorrowedTexture(texture: texture ?? this.texture);

  @override
  List<Object?> get _members => [texture];
  @override
  String toString() => _describe('OpenGlBorrowedTexture', const ['texture']);
}

/// A ring of your OpenGL textures, for
/// `MapHandle.attachOpenGlBorrowedTexture()`.
///
/// `logical_size` sets the map's size. `physical_width` and `physical_height`
/// give the textures' size in pixels, which is usually `logical_size` times the
/// map's scale factor. The context must use `OpenGlContextOwnership.shared`.
/// Each texture must be in your context's share group, with storage of that
/// size. The ring holds up to `textureRingMaxDepth` distinct textures.
///
/// The session uses the textures until a replacement or release of the target
/// completes, a detach or release of the session completes, or an abandon of it
/// returns.
///
/// See `mln_open_gl_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class OpenGlBorrowedTextureDescriptor extends _Value {
  OpenGlBorrowedTextureDescriptor({
    required this.logicalSize,
    required this.physicalWidth,
    required this.physicalHeight,
    required this.context,
    required List<OpenGlBorrowedTexture> textures,
    required this.target,
  }) : textures = List.unmodifiable(textures);
  final LogicalSize logicalSize;
  final int physicalWidth;
  final int physicalHeight;
  final OpenGlContextDescriptor context;
  final List<OpenGlBorrowedTexture> textures;

  /// The textures' target. It must be GL_TEXTURE_2D.
  final int target;

  /// A copy of this value with the given members replaced.
  OpenGlBorrowedTextureDescriptor copyWith({
    LogicalSize? logicalSize,
    int? physicalWidth,
    int? physicalHeight,
    OpenGlContextDescriptor? context,
    List<OpenGlBorrowedTexture>? textures,
    int? target,
  }) => OpenGlBorrowedTextureDescriptor(
    logicalSize: logicalSize ?? this.logicalSize,
    physicalWidth: physicalWidth ?? this.physicalWidth,
    physicalHeight: physicalHeight ?? this.physicalHeight,
    context: context ?? this.context,
    textures: textures ?? this.textures,
    target: target ?? this.target,
  );

  @override
  List<Object?> get _members => [
    logicalSize,
    physicalWidth,
    physicalHeight,
    context,
    textures,
    target,
  ];
  @override
  String toString() => _describe('OpenGlBorrowedTextureDescriptor', const [
    'logicalSize',
    'physicalWidth',
    'physicalHeight',
    'context',
    'textures',
    'target',
  ]);
}

/// An OpenGL surface to present to, for `MapHandle.attachOpenGlSurface()`.
///
/// The session uses the surface until a replacement or release of the target
/// completes, a detach or release of the session completes, or an abandon of it
/// returns.
///
/// See `mln_open_gl_surface_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
final class OpenGlSurfaceDescriptor extends _Value {
  const OpenGlSurfaceDescriptor({
    required this.logicalSize,
    required this.context,
    this.surface = NativePointer.nullPointer,
  });
  final LogicalSize logicalSize;
  final OpenGlContextDescriptor context;

  /// Your EGLSurface with EGL, or your HDC with WGL. WebGL doesn't use it.
  final NativePointer surface;

  /// A copy of this value with the given members replaced.
  OpenGlSurfaceDescriptor copyWith({
    LogicalSize? logicalSize,
    OpenGlContextDescriptor? context,
    NativePointer? surface,
  }) => OpenGlSurfaceDescriptor(
    logicalSize: logicalSize ?? this.logicalSize,
    context: context ?? this.context,
    surface: surface ?? this.surface,
  );

  @override
  List<Object?> get _members => [logicalSize, context, surface];
  @override
  String toString() => _describe('OpenGlSurfaceDescriptor', const [
    'logicalSize',
    'context',
    'surface',
  ]);
}

/// The Vulkan device and queue that a render session uses.
///
/// Keep these objects alive until a detach or release of the session completes,
/// or an abandon of it returns.
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

  /// A `VkInstance`.
  final NativePointer instance;

  /// A `VkPhysicalDevice`.
  final NativePointer physicalDevice;

  /// A `VkDevice`.
  final NativePointer device;

  /// A `VkQueue` from `graphics_queue_family_index` that supports graphics. If
  /// you submit to it too, set `RenderSessionAttachOptions.queueLock`.
  final NativePointer graphicsQueue;
  final int graphicsQueueFamilyIndex;

  /// Your `vkGetInstanceProcAddr`, or null to use the Vulkan loader's.
  final NativePointer getInstanceProcAddr;

  /// Your `vkGetDeviceProcAddr`, or null to look it up from the instance.
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

/// A Vulkan image of a borrowed ring.
///
/// See `mln_vulkan_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class VulkanBorrowedTexture extends _Value {
  const VulkanBorrowedTexture({required this.image, required this.imageView});

  /// Your VkImage.
  final int image;

  /// A 2D VkImageView of `image` with one mip level and one layer.
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

/// A ring of your Vulkan images, for `MapHandle.attachVulkanBorrowedTexture()`.
///
/// `logical_size` sets the map's size. `physical_width` and `physical_height`
/// give the images' size in pixels, which is usually `logical_size` times the
/// map's scale factor. Each image must have that size, `format`, and
/// VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, and belong to the context's device. The
/// ring holds up to `textureRingMaxDepth` distinct images.
///
/// The session uses the images until a replacement or release of the target
/// completes, a detach or release of the session completes, or an abandon of it
/// returns.
///
/// See `mln_vulkan_borrowed_texture_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class VulkanBorrowedTextureDescriptor extends _Value {
  VulkanBorrowedTextureDescriptor({
    required this.logicalSize,
    required this.physicalWidth,
    required this.physicalHeight,
    required this.context,
    required List<VulkanBorrowedTexture> textures,
    required this.format,
    required this.finalLayout,
    this.owner = const VulkanImageOwner.fromRawValue(0),
  }) : textures = List.unmodifiable(textures);
  final LogicalSize logicalSize;
  final int physicalWidth;
  final int physicalHeight;
  final VulkanContextDescriptor context;
  final List<VulkanBorrowedTexture> textures;

  /// The images' VkFormat.
  final int format;

  /// The VkImageLayout that each frame leaves its image in.
  final int finalLayout;
  final VulkanImageOwner owner;

  /// A copy of this value with the given members replaced.
  VulkanBorrowedTextureDescriptor copyWith({
    LogicalSize? logicalSize,
    int? physicalWidth,
    int? physicalHeight,
    VulkanContextDescriptor? context,
    List<VulkanBorrowedTexture>? textures,
    int? format,
    int? finalLayout,
    VulkanImageOwner? owner,
  }) => VulkanBorrowedTextureDescriptor(
    logicalSize: logicalSize ?? this.logicalSize,
    physicalWidth: physicalWidth ?? this.physicalWidth,
    physicalHeight: physicalHeight ?? this.physicalHeight,
    context: context ?? this.context,
    textures: textures ?? this.textures,
    format: format ?? this.format,
    finalLayout: finalLayout ?? this.finalLayout,
    owner: owner ?? this.owner,
  );

  @override
  List<Object?> get _members => [
    logicalSize,
    physicalWidth,
    physicalHeight,
    context,
    textures,
    format,
    finalLayout,
    owner,
  ];
  @override
  String toString() => _describe('VulkanBorrowedTextureDescriptor', const [
    'logicalSize',
    'physicalWidth',
    'physicalHeight',
    'context',
    'textures',
    'format',
    'finalLayout',
    'owner',
  ]);
}

/// A Vulkan surface to present to, for `MapHandle.attachVulkanSurface()`.
///
/// The device must have VK_KHR_swapchain enabled, and the queue family of
/// `graphics_queue_family_index` must support presenting to the surface. The
/// session creates the swapchain. It uses the surface until a replacement or
/// release of the target completes, a detach or release of the session
/// completes, or an abandon of it returns.
///
/// See `mln_vulkan_surface_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
final class VulkanSurfaceDescriptor extends _Value {
  const VulkanSurfaceDescriptor({
    required this.logicalSize,
    required this.context,
    required this.surface,
  });
  final LogicalSize logicalSize;
  final VulkanContextDescriptor context;

  /// Your VkSurfaceKHR.
  final int surface;

  /// A copy of this value with the given members replaced.
  VulkanSurfaceDescriptor copyWith({
    LogicalSize? logicalSize,
    VulkanContextDescriptor? context,
    int? surface,
  }) => VulkanSurfaceDescriptor(
    logicalSize: logicalSize ?? this.logicalSize,
    context: context ?? this.context,
    surface: surface ?? this.surface,
  );

  @override
  List<Object?> get _members => [logicalSize, context, surface];
  @override
  String toString() => _describe('VulkanSurfaceDescriptor', const [
    'logicalSize',
    'context',
    'surface',
  ]);
}

/// A Wayland surface to present to, for `MapHandle.attachWaylandSurface()`.
///
/// See `mln_wayland_surface_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wayland_8h.html).
final class WaylandSurfaceDescriptor extends _Value {
  const WaylandSurfaceDescriptor({
    required this.logicalSize,
    required this.display,
    required this.surface,
  });
  final LogicalSize logicalSize;

  /// Your `wl_display*`.
  final NativePointer display;

  /// Your `wl_surface*`, on `display`.
  final NativePointer surface;

  /// A copy of this value with the given members replaced.
  WaylandSurfaceDescriptor copyWith({
    LogicalSize? logicalSize,
    NativePointer? display,
    NativePointer? surface,
  }) => WaylandSurfaceDescriptor(
    logicalSize: logicalSize ?? this.logicalSize,
    display: display ?? this.display,
    surface: surface ?? this.surface,
  );

  @override
  List<Object?> get _members => [logicalSize, display, surface];
  @override
  String toString() => _describe('WaylandSurfaceDescriptor', const [
    'logicalSize',
    'display',
    'surface',
  ]);
}

/// A Win32 window to present to, for `MapHandle.attachWin32Window()`.
///
/// See `mln_win32_window_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/win32_8h.html).
final class Win32WindowDescriptor extends _Value {
  const Win32WindowDescriptor({
    required this.logicalSize,
    required this.window,
  });
  final LogicalSize logicalSize;

  /// Your `HWND`.
  final NativePointer window;

  /// A copy of this value with the given members replaced.
  Win32WindowDescriptor copyWith({
    LogicalSize? logicalSize,
    NativePointer? window,
  }) => Win32WindowDescriptor(
    logicalSize: logicalSize ?? this.logicalSize,
    window: window ?? this.window,
  );

  @override
  List<Object?> get _members => [logicalSize, window];
  @override
  String toString() =>
      _describe('Win32WindowDescriptor', const ['logicalSize', 'window']);
}

/// Options for finding the camera that fits an area.
///
/// See `mln_camera_fit_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraFitOptions extends _Value {
  const CameraFitOptions({this.padding, this.bearing, this.pitch});

  /// Space to leave between the area and each edge of the map. The left and
  /// right insets must add up to less than the map's width, and the top and
  /// bottom to less than its height.
  final EdgeInsets? padding;

  /// The rotation from north, in degrees.
  final double? bearing;

  /// The tilt toward the horizon, in degrees.
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

/// A rectangle of coordinates, from its southwest to its northeast corner.
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

/// A camera change that fits an area, for `MapHandle.fitCameraToLatLngBounds()`
/// and the other fit commands.
///
/// A jump uses only the end handler of `animation`, as in `CameraUpdate`.
///
/// See `mln_camera_fit_update` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraFitUpdate extends _Value {
  const CameraFitUpdate({
    this.mode = const CameraUpdateMode.fromRawValue(0),
    this.fit = const CameraFitOptions(),
    this.animation = const AnimationOptions(),
  });
  final CameraUpdateMode mode;
  final CameraFitOptions fit;
  final AnimationOptions animation;

  /// A copy of this value with the given members replaced.
  CameraFitUpdate copyWith({
    CameraUpdateMode? mode,
    CameraFitOptions? fit,
    AnimationOptions? animation,
  }) => CameraFitUpdate(
    mode: mode ?? this.mode,
    fit: fit ?? this.fit,
    animation: animation ?? this.animation,
  );

  @override
  List<Object?> get _members => [mode, fit, animation];
  @override
  String toString() =>
      _describe('CameraFitUpdate', const ['mode', 'fit', 'animation']);
}

/// A map's camera, with the snapshot generation that it matches.
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

/// The ID of a feature in a source, as the source's data gives it.
///
/// The map compares feature IDs as text. A number names the same feature as the
/// string of its shortest decimal form, so 7, 7.0, and "7" name one feature.
///
/// See `mln_feature_id` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
sealed class FeatureId {
  const FeatureId._();
  const factory FeatureId.stringValue(String value) = FeatureIdStringValue;
  const factory FeatureId.uintValue(int value) = FeatureIdUintValue;
  const factory FeatureId.intValue(int value) = FeatureIdIntValue;

  /// A finite number.
  const factory FeatureId.doubleValue(double value) = FeatureIdDoubleValue;
}

final class FeatureIdStringValue extends FeatureId {
  const FeatureIdStringValue(this.value) : super._();
  final String value;
  @override
  bool operator ==(Object other) =>
      other is FeatureIdStringValue && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class FeatureIdUintValue extends FeatureId {
  const FeatureIdUintValue(this.value) : super._();
  final int value;
  @override
  bool operator ==(Object other) =>
      other is FeatureIdUintValue && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class FeatureIdIntValue extends FeatureId {
  const FeatureIdIntValue(this.value) : super._();
  final int value;
  @override
  bool operator ==(Object other) =>
      other is FeatureIdIntValue && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class FeatureIdDoubleValue extends FeatureId {
  const FeatureIdDoubleValue(this.value) : super._();
  final double value;
  @override
  bool operator ==(Object other) =>
      other is FeatureIdDoubleValue && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

/// The features whose state to set, read, or remove.
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
  final String sourceId;

  /// The source layer, for a vector source. Empty for other sources.
  final String sourceLayerId;

  /// The feature's ID. Setting or reading state needs it. Without it,
  /// `MapHandle.removeFeatureState()` removes the state of every feature in the
  /// source layer.
  final FeatureId? featureId;

  /// The one key that `MapHandle.removeFeatureState()` removes. It needs
  /// `feature_id`. Empty removes every key.
  final String stateKey;

  /// A copy of this value with the given members replaced.
  FeatureStateSelector copyWith({
    String? sourceId,
    String? sourceLayerId,
    FeatureId? featureId,
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

/// All the settings for how a map picks the tiles to load.
///
/// Its members mean the same as those of `MapTileOptions`.
///
/// See `mln_map_tile_settings` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class MapTileSettings extends _Value {
  const MapTileSettings({
    this.prefetchZoomDelta = 0,
    this.lodMinRadius = 0,
    this.lodScale = 0,
    this.lodPitchThreshold = 0,
    this.lodZoomShift = 0,
    this.lodMode = const TileLodMode.fromRawValue(0),
  });
  final int prefetchZoomDelta;
  final double lodMinRadius;
  final double lodScale;

  /// The pitch, in degrees, above which the map lowers the detail.
  final double lodPitchThreshold;
  final double lodZoomShift;
  final TileLodMode lodMode;

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
  String toString() => _describe('MapTileSettings', const [
    'prefetchZoomDelta',
    'lodMinRadius',
    'lodScale',
    'lodPitchThreshold',
    'lodZoomShift',
    'lodMode',
  ]);
}

/// All the limits on a map's camera.
///
/// Its members mean the same as those of `BoundOptions`.
///
/// See `mln_camera_bounds` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraBounds extends _Value {
  const CameraBounds({
    this.bounds = const LatLngBounds(),
    this.unbounded = false,
    this.minZoom = 0,
    this.maxZoom = 0,
    this.minPitch = 0,
    this.maxPitch = 0,
  });

  /// The area that the camera's center stays within. When `unbounded` is true,
  /// it covers the whole world.
  final LatLngBounds bounds;

  /// Whether no area limits the camera's center.
  final bool unbounded;
  final double minZoom;
  final double maxZoom;

  /// The lowest pitch, in degrees.
  final double minPitch;

  /// The highest pitch, in degrees.
  final double maxPitch;

  @override
  List<Object?> get _members => [
    bounds,
    unbounded,
    minZoom,
    maxZoom,
    minPitch,
    maxPitch,
  ];
  @override
  String toString() => _describe('CameraBounds', const [
    'bounds',
    'unbounded',
    'minZoom',
    'maxZoom',
    'minPitch',
    'maxPitch',
  ]);
}

/// A vector in three dimensions.
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

/// A rotation, as a quaternion.
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

/// A map camera's whole position and orientation in three dimensions.
///
/// Its members mean the same as those of `FreeCameraOptions`.
///
/// See `mln_free_camera` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class FreeCamera extends _Value {
  const FreeCamera({
    this.position = const Vec3(),
    this.orientation = const Quaternion(),
  });
  final Vec3 position;
  final Quaternion orientation;

  @override
  List<Object?> get _members => [position, orientation];
  @override
  String toString() =>
      _describe('FreeCamera', const ['position', 'orientation']);
}

/// A map's state, as `MapHandle.getSnapshot()` reads it.
///
/// See `mln_map_snapshot` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class MapSnapshot extends _Value {
  const MapSnapshot({
    this.debugFlags = MapDebugFlag.none,
    this.generation = 0,
    this.fullyLoaded = false,
    this.renderingStatsViewEnabled = false,
    this.gestureInProgress = false,
    this.transitionInProgress = false,
    this.eventMask = RuntimeEventFlag.none,
    this.latestRenderUpdateGeneration = 0,
    this.tile = const MapTileSettings(),
    this.bounds = const CameraBounds(),
    this.freeCamera = const FreeCamera(),
    this.view = const MapView(),
  });
  final MapDebugFlag debugFlags;

  /// Grows with each change to the map's state.
  final int generation;

  /// Whether the style and every resource that the view needs have loaded.
  final bool fullyLoaded;
  final bool renderingStatsViewEnabled;
  final bool gestureInProgress;
  final bool transitionInProgress;
  final RuntimeEventFlag eventMask;

  /// Counts the render updates that the map has made for its render session.
  final int latestRenderUpdateGeneration;
  final MapTileSettings tile;
  final CameraBounds bounds;
  final FreeCamera freeCamera;
  final MapView view;

  @override
  List<Object?> get _members => [
    debugFlags,
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
    'debugFlags',
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

/// A range of image pixels, from `from` to `to`, that stretches to fit text.
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

/// The area of an image, in image pixels, that text fits inside.
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

/// An image in the map's style.
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
    this.keepUnused = false,
  }) : pixels = Uint8List.fromList(
         pixels ?? const <int>[],
       ).asUnmodifiableView(),
       stretchX = List.unmodifiable(stretchX),
       stretchY = List.unmodifiable(stretchY);
  final int width;
  final int height;

  /// The premultiplied RGBA pixels, in rows of `width` \* 4 bytes, from the top
  /// row down.
  final Uint8List pixels;
  final List<ImageStretch> stretchX;
  final List<ImageStretch> stretchY;
  final ImageContent? content;
  final StyleImageTextFit? textFitWidth;
  final StyleImageTextFit? textFitHeight;
  final double pixelRatio;
  final bool sdf;
  final bool keepUnused;

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
    keepUnused,
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
    'keepUnused',
  ]);
}

/// A layer in the map's style.
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
  final String id;

  /// The layer's
  /// \[type\](https://maplibre.org/maplibre-style-spec/layers/#type), such as
  /// `fill`.
  final String type;
  final String sourceId;
  final String sourceLayer;

  /// The zoom level where the layer starts to show, or negative infinity when
  /// the layer doesn't set one.
  final double minZoom;

  /// The zoom level where the layer stops showing, or infinity when the layer
  /// doesn't set one.
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

/// The tiles of a source, as the style lists them.
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
  final List<String> tileUrls;
  final double minZoom;
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

/// A source in the map's style.
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
    this.geoJson,
  });
  final String id;
  final StyleSourceType type;
  final bool isVolatile;
  final String attribution;
  final String url;

  /// The tiles that the style lists in the source itself, instead of at a URL.
  final StyleSourceTileInfo? tileJson;
  final LatLngBounds? bounds;
  final int? tileSize;
  final StyleVectorTileEncoding? vectorEncoding;
  final StyleRasterDemEncoding? rasterEncoding;

  /// A GeoJSON source's options, with every field set. Data prepared with them
  /// can replace the source's data.
  final GeoJsonSourceOptions? geoJson;

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
    geoJson,
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
    'geoJson',
  ]);
}

/// The default
/// \[transition\](https://maplibre.org/maplibre-style-spec/transition/) for
/// changes to a style's paint properties.
///
/// See `mln_style_transition_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class StyleTransitionOptions extends _Value {
  const StyleTransitionOptions({
    this.durationMs,
    this.delayMs,
    this.enablePlacementTransitions,
  });
  final double? durationMs;
  final double? delayMs;

  /// Whether symbols fade in and out as the map places them.
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

/// A projection's camera, as `MapProjectionHandle.getCamera()` reports it.
///
/// See `mln_map_projection_camera_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
final class MapProjectionCameraResult extends _Value {
  const MapProjectionCameraResult({this.camera = const Camera()});
  final Camera camera;

  @override
  List<Object?> get _members => [camera];
  @override
  String toString() => _describe('MapProjectionCameraResult', const ['camera']);
}

/// Limits on the camera, for `MapHandle.setBounds()`.
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

  /// The area that the camera's center stays within.
  final LatLngBounds? bounds;
  final double? minZoom;
  final double? maxZoom;

  /// The lowest pitch, in degrees.
  final double? minPitch;

  /// The highest pitch, in degrees.
  final double? maxPitch;

  /// Removes the area that limits the camera's center.
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

/// The camera's position and orientation in three dimensions.
///
/// See `mln_free_camera_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class FreeCameraOptions extends _Value {
  const FreeCameraOptions({this.position, this.orientation});

  /// The camera's position in Web Mercator units, where the world is 1 wide,
  /// (0, 0) is its northwest corner, and (1, 1) is its southeast corner. `z` is
  /// the altitude in the same units.
  final Vec3? position;

  /// The camera's rotation from looking straight down with north at the top of
  /// the screen.
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

/// Settings that skew the map or draw it without perspective.
///
/// See `mln_projection_mode_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class ProjectionModeOptions extends _Value {
  const ProjectionModeOptions({this.axonometric, this.xSkew, this.ySkew});

  /// Whether the map draws without perspective.
  final bool? axonometric;
  final double? xSkew;
  final double? ySkew;

  /// A copy of this value with the given members replaced.
  ProjectionModeOptions copyWith({
    bool? axonometric,
    double? xSkew,
    double? ySkew,
  }) => ProjectionModeOptions(
    axonometric: axonometric ?? this.axonometric,
    xSkew: xSkew ?? this.xSkew,
    ySkew: ySkew ?? this.ySkew,
  );

  @override
  List<Object?> get _members => [axonometric, xSkew, ySkew];
  @override
  String toString() => _describe('ProjectionModeOptions', const [
    'axonometric',
    'xSkew',
    'ySkew',
  ]);
}

/// Options for a style image, as the
/// \[sprite\](https://maplibre.org/maplibre-style-spec/sprite/) spec describes
/// them.
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
    this.keepUnused,
  }) : stretchX = stretchX == null ? null : List.unmodifiable(stretchX),
       stretchY = stretchY == null ? null : List.unmodifiable(stretchY);

  /// The ranges of columns that stretch, in increasing order without overlap.
  final List<ImageStretch>? stretchX;

  /// The ranges of rows that stretch, in increasing order without overlap.
  final List<ImageStretch>? stretchY;
  final ImageContent? content;
  final StyleImageTextFit? textFitWidth;
  final StyleImageTextFit? textFitHeight;
  final double? pixelRatio;
  final bool? sdf;

  /// Keeps the image after no layer uses it. Without this, the map can remove
  /// an image that it asked for through
  /// `RuntimeEventType.mapStyleImageMissing`.
  final bool? keepUnused;

  /// A copy of this value with the given members replaced.
  StyleImageOptions copyWith({
    List<ImageStretch>? stretchX,
    List<ImageStretch>? stretchY,
    ImageContent? content,
    StyleImageTextFit? textFitWidth,
    StyleImageTextFit? textFitHeight,
    double? pixelRatio,
    bool? sdf,
    bool? keepUnused,
  }) => StyleImageOptions(
    stretchX: stretchX ?? this.stretchX,
    stretchY: stretchY ?? this.stretchY,
    content: content ?? this.content,
    textFitWidth: textFitWidth ?? this.textFitWidth,
    textFitHeight: textFitHeight ?? this.textFitHeight,
    pixelRatio: pixelRatio ?? this.pixelRatio,
    sdf: sdf ?? this.sdf,
    keepUnused: keepUnused ?? this.keepUnused,
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
    keepUnused,
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
    'keepUnused',
  ]);
}

/// How the map picks the tiles to load.
///
/// The `lod_` members lower the level of detail away from the camera, so the
/// map loads fewer tiles when the pitch is high.
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

  /// How many zoom levels lower the map also loads tiles for, so it can show a
  /// coarse map while the current tiles load. 0 loads only the current tiles.
  final int? prefetchZoomDelta;

  /// The radius around the view point, in tiles, that keeps full detail.
  final double? lodMinRadius;

  /// Scales the distance to the view point. Above 1 lowers the detail.
  final double? lodScale;

  /// The pitch, in degrees, above which the map lowers the detail.
  final double? lodPitchThreshold;

  /// Zoom levels to add for picking the detail. Below 0 loads fewer tiles.
  final double? lodZoomShift;

  /// `TileLodMode.distance` ignores `lod_min_radius`.
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

/// How the map orients and constrains its view.
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

/// A camera change for `MapHandle.updateCamera()`.
///
/// A jump uses only the end handler of `animation`, and the handler runs as
/// soon as the camera moves.
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

/// Options for `OffscreenDeviceHandle.create()`.
///
/// See `mln_offscreen_device_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
final class OffscreenDeviceOptions extends _Value {
  const OffscreenDeviceOptions({
    this.adapterIndex,
    this.singleContext,
    this.clientApi,
  });

  /// The GPU to use, as an index into the platform's list of GPUs.
  final int? adapterIndex;

  /// Whether the device's sessions take turns in one OpenGL context instead of
  /// having one each. Only an EGL device reads it.
  final bool? singleContext;

  /// The OpenGL API that the device's sessions render with. An EGL device
  /// defaults to `OpenGlClientApi.gles`, and takes `OpenGlClientApi.gl` too on
  /// Linux and FreeBSD. A WGL device takes only `OpenGlClientApi.gl`, its
  /// default, and a WebGL device only `OpenGlClientApi.gles`, its default.
  /// Other devices take neither.
  final OpenGlClientApi? clientApi;

  /// A copy of this value with the given members replaced.
  OffscreenDeviceOptions copyWith({
    int? adapterIndex,
    bool? singleContext,
    OpenGlClientApi? clientApi,
  }) => OffscreenDeviceOptions(
    adapterIndex: adapterIndex ?? this.adapterIndex,
    singleContext: singleContext ?? this.singleContext,
    clientApi: clientApi ?? this.clientApi,
  );

  @override
  List<Object?> get _members => [adapterIndex, singleContext, clientApi];
  @override
  String toString() => _describe('OffscreenDeviceOptions', const [
    'adapterIndex',
    'singleContext',
    'clientApi',
  ]);
}

/// The outcome of `RenderSessionHandle.abandon()`.
///
/// See `mln_render_abandon_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
final class RenderAbandonResult extends _Value {
  const RenderAbandonResult({
    this.disposition = const RenderAbandonDisposition.fromRawValue(0),
    this.quarantinedResourceCount = 0,
  });
  final RenderAbandonDisposition disposition;
  final int quarantinedResourceCount;

  @override
  List<Object?> get _members => [disposition, quarantinedResourceCount];
  @override
  String toString() => _describe('RenderAbandonResult', const [
    'disposition',
    'quarantinedResourceCount',
  ]);
}

/// A demand for one frame.
///
/// See `mln_frame_demand` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
final class FrameDemand extends _Value {
  const FrameDemand({this.flags = FrameDemandFlag.none, this.token = 0});
  final FrameDemandFlag flags;

  /// A value of your choice that the demand's frame result carries.
  final int token;

  /// A copy of this value with the given members replaced.
  FrameDemand copyWith({FrameDemandFlag? flags, int? token}) =>
      FrameDemand(flags: flags ?? this.flags, token: token ?? this.token);

  @override
  List<Object?> get _members => [flags, token];
  @override
  String toString() => _describe('FrameDemand', const ['flags', 'token']);
}

/// What an attached render session supports.
///
/// See `mln_render_session_capabilities` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class RenderSessionCapabilities extends _Value {
  const RenderSessionCapabilities({
    required this.driver,
    this.textureRingDepth = 0,
    this.flags = RenderSessionCapabilityFlag.none,
    this.producerSyncKind = const GpuSyncKind.fromRawValue(0),
    this.consumerSyncKinds = GpuSyncKindFlag.none,
  });
  final RenderDriverKind driver;

  /// How many textures the session's texture ring has.
  final int textureRingDepth;
  final RenderSessionCapabilityFlag flags;

  /// The kind of `GpuSync` that `AcquiredFrameHandle.withProducerSync()`
  /// reports.
  final GpuSyncKind producerSyncKind;

  /// The kinds of `GpuSync` that `AcquiredFrameHandle.release()` accepts. They
  /// depend on the target and on what the device supports.
  final GpuSyncKindFlag consumerSyncKinds;

  @override
  List<Object?> get _members => [
    driver,
    textureRingDepth,
    flags,
    producerSyncKind,
    consumerSyncKinds,
  ];
  @override
  String toString() => _describe('RenderSessionCapabilities', const [
    'driver',
    'textureRingDepth',
    'flags',
    'producerSyncKind',
    'consumerSyncKinds',
  ]);
}

/// The state of a render session at one moment.
///
/// See `mln_render_session_snapshot` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
final class RenderSessionSnapshot extends _Value {
  const RenderSessionSnapshot({
    required this.state,
    this.extent = const LogicalExtent(),
    this.generation = 0,
    this.frameGeneration = 0,
    this.acquiredFrameCount = 0,
    this.failureStatus = const Status.fromRawValue(0),
    this.hasTarget = false,
  });
  final RenderSessionState state;

  /// The target's logical size, and the map's scale factor.
  final LogicalExtent extent;
  final int generation;
  final int frameGeneration;
  final int acquiredFrameCount;

  /// `Status.ok` until the session fails. Then `Status.targetLost` when it
  /// failed because it lost its target, or `Status.nativeError` when it failed
  /// for another reason.
  final Status failureStatus;

  /// Whether the session is attached and has a target to render to. It's false
  /// from when `RenderSessionHandle.releaseTarget()` completes until you give
  /// the session a new target.
  final bool hasTarget;

  @override
  List<Object?> get _members => [
    state,
    extent,
    generation,
    frameGeneration,
    acquiredFrameCount,
    failureStatus,
    hasTarget,
  ];
  @override
  String toString() => _describe('RenderSessionSnapshot', const [
    'state',
    'extent',
    'generation',
    'frameGeneration',
    'acquiredFrameCount',
    'failureStatus',
    'hasTarget',
  ]);
}

/// A rectangle of the screen, from its `min` corner to its `max` corner.
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

/// A line through points on the screen.
///
/// See `mln_screen_line_string` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
final class ScreenLineString extends _Value {
  ScreenLineString({List<ScreenPoint> points = const []})
    : points = List.unmodifiable(points);
  final List<ScreenPoint> points;

  /// A copy of this value with the given members replaced.
  ScreenLineString copyWith({List<ScreenPoint>? points}) =>
      ScreenLineString(points: points ?? this.points);

  @override
  List<Object?> get _members => [points];
  @override
  String toString() => _describe('ScreenLineString', const ['points']);
}

/// The screen area that a rendered feature query covers.
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

/// Options for a rendered feature query.
///
/// See `mln_rendered_feature_query_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
final class RenderedFeatureQueryOptions extends _Value {
  RenderedFeatureQueryOptions({List<String>? layerIds, this.filter})
    : layerIds = layerIds == null ? null : List.unmodifiable(layerIds);

  /// The layers to query. Without them, the query covers every layer.
  final List<String>? layerIds;

  /// A \[filter\](https://maplibre.org/maplibre-style-spec/expressions/) that
  /// each feature must match. Empty matches every feature.
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

/// A feature that a rendered feature query found.
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

  /// The feature, as a GeoJSON feature.
  final Object? feature;

  /// The layer that drew the feature.
  final String layerId;
  final String sourceId;

  /// The feature's source layer, or empty when its source has none.
  final String sourceLayerId;

  /// The feature's state, as a JSON object.
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

/// Options for a source feature query.
///
/// See `mln_source_feature_query_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
final class SourceFeatureQueryOptions extends _Value {
  SourceFeatureQueryOptions({List<String>? sourceLayerIds, this.filter})
    : sourceLayerIds = sourceLayerIds == null
          ? null
          : List.unmodifiable(sourceLayerIds);

  /// The source layers to query. A vector source finds features only in the
  /// source layers named here.
  final List<String>? sourceLayerIds;

  /// A \[filter\](https://maplibre.org/maplibre-style-spec/expressions/) that
  /// each feature must match. Empty matches every feature.
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

/// A feature that a source feature query found.
///
/// See `mln_source_feature` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
final class SourceFeature extends _Value {
  const SourceFeature({this.feature, this.sourceLayerId = ''});

  /// The feature, as a GeoJSON feature.
  final Object? feature;

  /// The feature's source layer, or empty when its source has none.
  final String sourceLayerId;

  @override
  List<Object?> get _members => [feature, sourceLayerId];
  @override
  String toString() =>
      _describe('SourceFeature', const ['feature', 'sourceLayerId']);
}

/// The layout of the pixels that `RenderSessionHandle.readTexture()` reads.
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
  final int width;
  final int height;

  /// The bytes from the start of one row to the start of the next.
  final int stride;
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

/// The pixels that `RenderSessionHandle.readTexture()` reads.
///
/// See `mln_texture_readback_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class TextureReadbackResult extends _Value {
  TextureReadbackResult({Uint8List? data, this.info = const TextureImageInfo()})
    : data = Uint8List.fromList(data ?? const <int>[]).asUnmodifiableView();

  /// Premultiplied RGBA pixels with 8 bits per channel, in rows from top to
  /// bottom.
  final Uint8List data;
  final TextureImageInfo info;

  @override
  List<Object?> get _members => [data, info];
  @override
  String toString() =>
      _describe('TextureReadbackResult', const ['data', 'info']);
}

/// A new Android window for `RenderSessionHandle.setAndroidSurfaceTarget()`.
///
/// Name the window by `window`, or by `jni_env` and `surface`. The session
/// takes its own reference to the window, so you can release yours once the
/// call returns.
///
/// See `mln_android_surface_replacement` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
final class AndroidSurfaceReplacement extends _Value {
  const AndroidSurfaceReplacement({
    required this.logicalSize,
    this.window = NativePointer.nullPointer,
    this.jniEnv = NativePointer.nullPointer,
    this.surface = NativePointer.nullPointer,
  });
  final LogicalSize logicalSize;

  /// Your `ANativeWindow*`, or null to name the window by `surface`.
  final NativePointer window;

  /// The calling thread's `JNIEnv*`, with `surface`.
  final NativePointer jniEnv;

  /// An `android.view.Surface`, with `jni_env`.
  final NativePointer surface;

  /// A copy of this value with the given members replaced.
  AndroidSurfaceReplacement copyWith({
    LogicalSize? logicalSize,
    NativePointer? window,
    NativePointer? jniEnv,
    NativePointer? surface,
  }) => AndroidSurfaceReplacement(
    logicalSize: logicalSize ?? this.logicalSize,
    window: window ?? this.window,
    jniEnv: jniEnv ?? this.jniEnv,
    surface: surface ?? this.surface,
  );

  @override
  List<Object?> get _members => [logicalSize, window, jniEnv, surface];
  @override
  String toString() => _describe('AndroidSurfaceReplacement', const [
    'logicalSize',
    'window',
    'jniEnv',
    'surface',
  ]);
}

/// New Metal textures for a borrowed ring, for
/// `RenderSessionHandle.setMetalBorrowedTextureTarget()`.
///
/// The members and the textures follow the rules of
/// `MetalBorrowedTextureDescriptor`. The ring keeps its depth, device, and
/// command queue, so give it as many textures as it attached with, on the same
/// device.
///
/// See `mln_metal_borrowed_texture_replacement` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class MetalBorrowedTextureReplacement extends _Value {
  MetalBorrowedTextureReplacement({
    required this.logicalSize,
    required this.physicalWidth,
    required this.physicalHeight,
    required List<MetalBorrowedTexture> textures,
  }) : textures = List.unmodifiable(textures);
  final LogicalSize logicalSize;
  final int physicalWidth;
  final int physicalHeight;
  final List<MetalBorrowedTexture> textures;

  /// A copy of this value with the given members replaced.
  MetalBorrowedTextureReplacement copyWith({
    LogicalSize? logicalSize,
    int? physicalWidth,
    int? physicalHeight,
    List<MetalBorrowedTexture>? textures,
  }) => MetalBorrowedTextureReplacement(
    logicalSize: logicalSize ?? this.logicalSize,
    physicalWidth: physicalWidth ?? this.physicalWidth,
    physicalHeight: physicalHeight ?? this.physicalHeight,
    textures: textures ?? this.textures,
  );

  @override
  List<Object?> get _members => [
    logicalSize,
    physicalWidth,
    physicalHeight,
    textures,
  ];
  @override
  String toString() => _describe('MetalBorrowedTextureReplacement', const [
    'logicalSize',
    'physicalWidth',
    'physicalHeight',
    'textures',
  ]);
}

/// A new Metal layer for `RenderSessionHandle.setMetalSurfaceTarget()`.
///
/// The session renders to it with the device that it attached with. It uses the
/// layer until a replacement or release of the target completes, a detach or
/// release of the session completes, or an abandon of it returns.
///
/// See `mln_metal_surface_replacement` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
final class MetalSurfaceReplacement extends _Value {
  const MetalSurfaceReplacement({
    required this.logicalSize,
    required this.layer,
  });
  final LogicalSize logicalSize;

  /// Your CAMetalLayer.
  final NativePointer layer;

  /// A copy of this value with the given members replaced.
  MetalSurfaceReplacement copyWith({
    LogicalSize? logicalSize,
    NativePointer? layer,
  }) => MetalSurfaceReplacement(
    logicalSize: logicalSize ?? this.logicalSize,
    layer: layer ?? this.layer,
  );

  @override
  List<Object?> get _members => [logicalSize, layer];
  @override
  String toString() =>
      _describe('MetalSurfaceReplacement', const ['logicalSize', 'layer']);
}

/// New OpenGL textures for a borrowed ring, for
/// `RenderSessionHandle.setOpenGlBorrowedTextureTarget()`.
///
/// The members and the textures follow the rules of
/// `OpenGlBorrowedTextureDescriptor`. The ring keeps its depth, context, and
/// texture target, so give it as many textures as it attached with, in the same
/// share group.
///
/// See `mln_open_gl_borrowed_texture_replacement` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class OpenGlBorrowedTextureReplacement extends _Value {
  OpenGlBorrowedTextureReplacement({
    required this.logicalSize,
    required this.physicalWidth,
    required this.physicalHeight,
    required List<OpenGlBorrowedTexture> textures,
  }) : textures = List.unmodifiable(textures);
  final LogicalSize logicalSize;
  final int physicalWidth;
  final int physicalHeight;
  final List<OpenGlBorrowedTexture> textures;

  /// A copy of this value with the given members replaced.
  OpenGlBorrowedTextureReplacement copyWith({
    LogicalSize? logicalSize,
    int? physicalWidth,
    int? physicalHeight,
    List<OpenGlBorrowedTexture>? textures,
  }) => OpenGlBorrowedTextureReplacement(
    logicalSize: logicalSize ?? this.logicalSize,
    physicalWidth: physicalWidth ?? this.physicalWidth,
    physicalHeight: physicalHeight ?? this.physicalHeight,
    textures: textures ?? this.textures,
  );

  @override
  List<Object?> get _members => [
    logicalSize,
    physicalWidth,
    physicalHeight,
    textures,
  ];
  @override
  String toString() => _describe('OpenGlBorrowedTextureReplacement', const [
    'logicalSize',
    'physicalWidth',
    'physicalHeight',
    'textures',
  ]);
}

/// A new OpenGL surface for `RenderSessionHandle.setOpenGlSurfaceTarget()`.
///
/// The session presents to it with the context that it attached with. It uses
/// the surface until a replacement or release of the target completes, a detach
/// or release of the session completes, or an abandon of it returns.
///
/// See `mln_open_gl_surface_replacement` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
final class OpenGlSurfaceReplacement extends _Value {
  const OpenGlSurfaceReplacement({
    required this.logicalSize,
    this.surface = NativePointer.nullPointer,
  });
  final LogicalSize logicalSize;

  /// Your EGLSurface with EGL, or your HDC with WGL. WebGL doesn't use it, so
  /// leave it null.
  final NativePointer surface;

  /// A copy of this value with the given members replaced.
  OpenGlSurfaceReplacement copyWith({
    LogicalSize? logicalSize,
    NativePointer? surface,
  }) => OpenGlSurfaceReplacement(
    logicalSize: logicalSize ?? this.logicalSize,
    surface: surface ?? this.surface,
  );

  @override
  List<Object?> get _members => [logicalSize, surface];
  @override
  String toString() =>
      _describe('OpenGlSurfaceReplacement', const ['logicalSize', 'surface']);
}

/// New Vulkan images for a borrowed ring, for
/// `RenderSessionHandle.setVulkanBorrowedTextureTarget()`.
///
/// The members and the images follow the rules of
/// `VulkanBorrowedTextureDescriptor`. The ring keeps its depth, context,
/// `format`, `final_layout`, and `owner`, so give it as many images as it
/// attached with, in that format, on the same device.
///
/// See `mln_vulkan_borrowed_texture_replacement` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class VulkanBorrowedTextureReplacement extends _Value {
  VulkanBorrowedTextureReplacement({
    required this.logicalSize,
    required this.physicalWidth,
    required this.physicalHeight,
    required List<VulkanBorrowedTexture> textures,
  }) : textures = List.unmodifiable(textures);
  final LogicalSize logicalSize;
  final int physicalWidth;
  final int physicalHeight;
  final List<VulkanBorrowedTexture> textures;

  /// A copy of this value with the given members replaced.
  VulkanBorrowedTextureReplacement copyWith({
    LogicalSize? logicalSize,
    int? physicalWidth,
    int? physicalHeight,
    List<VulkanBorrowedTexture>? textures,
  }) => VulkanBorrowedTextureReplacement(
    logicalSize: logicalSize ?? this.logicalSize,
    physicalWidth: physicalWidth ?? this.physicalWidth,
    physicalHeight: physicalHeight ?? this.physicalHeight,
    textures: textures ?? this.textures,
  );

  @override
  List<Object?> get _members => [
    logicalSize,
    physicalWidth,
    physicalHeight,
    textures,
  ];
  @override
  String toString() => _describe('VulkanBorrowedTextureReplacement', const [
    'logicalSize',
    'physicalWidth',
    'physicalHeight',
    'textures',
  ]);
}

/// A new Vulkan surface for `RenderSessionHandle.setVulkanSurfaceTarget()`.
///
/// The session presents to it with the context that it attached with, so the
/// queue family of that context's `graphics_queue_family_index` must support
/// presenting to the surface. It uses the surface until a replacement or
/// release of the target completes, a detach or release of the session
/// completes, or an abandon of it returns.
///
/// See `mln_vulkan_surface_replacement` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
final class VulkanSurfaceReplacement extends _Value {
  const VulkanSurfaceReplacement({
    required this.logicalSize,
    required this.surface,
  });
  final LogicalSize logicalSize;

  /// Your VkSurfaceKHR.
  final int surface;

  /// A copy of this value with the given members replaced.
  VulkanSurfaceReplacement copyWith({LogicalSize? logicalSize, int? surface}) =>
      VulkanSurfaceReplacement(
        logicalSize: logicalSize ?? this.logicalSize,
        surface: surface ?? this.surface,
      );

  @override
  List<Object?> get _members => [logicalSize, surface];
  @override
  String toString() =>
      _describe('VulkanSurfaceReplacement', const ['logicalSize', 'surface']);
}

/// A new Wayland surface for `RenderSessionHandle.setWaylandSurfaceTarget()`.
///
/// See `mln_wayland_surface_replacement` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wayland_8h.html).
final class WaylandSurfaceReplacement extends _Value {
  const WaylandSurfaceReplacement({
    required this.logicalSize,
    required this.surface,
  });
  final LogicalSize logicalSize;

  /// Your `wl_surface*`, on the display that the session attached with.
  final NativePointer surface;

  /// A copy of this value with the given members replaced.
  WaylandSurfaceReplacement copyWith({
    LogicalSize? logicalSize,
    NativePointer? surface,
  }) => WaylandSurfaceReplacement(
    logicalSize: logicalSize ?? this.logicalSize,
    surface: surface ?? this.surface,
  );

  @override
  List<Object?> get _members => [logicalSize, surface];
  @override
  String toString() =>
      _describe('WaylandSurfaceReplacement', const ['logicalSize', 'surface']);
}

/// A new Win32 window for `RenderSessionHandle.setWin32WindowTarget()`.
///
/// See `mln_win32_window_replacement` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/win32_8h.html).
final class Win32WindowReplacement extends _Value {
  const Win32WindowReplacement({
    required this.logicalSize,
    required this.window,
  });
  final LogicalSize logicalSize;

  /// Your `HWND`.
  final NativePointer window;

  /// A copy of this value with the given members replaced.
  Win32WindowReplacement copyWith({
    LogicalSize? logicalSize,
    NativePointer? window,
  }) => Win32WindowReplacement(
    logicalSize: logicalSize ?? this.logicalSize,
    window: window ?? this.window,
  );

  @override
  List<Object?> get _members => [logicalSize, window];
  @override
  String toString() =>
      _describe('Win32WindowReplacement', const ['logicalSize', 'window']);
}

/// The answer to a resource request.
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

  /// Why the request failed, for `ResourceResponseStatus.error`.
  final ResourceErrorReason errorReason;

  /// The resource, for `ResourceResponseStatus.ok`.
  final Uint8List bytes;

  /// A description of the error, for `ResourceResponseStatus.error`.
  final String errorMessage;

  /// Whether the library must request the resource again before it reuses this
  /// copy.
  final bool mustRevalidate;
  final int? modifiedUnixMs;
  final int? expiresUnixMs;

  /// Empty when the resource has no ETag.
  final String etag;

  /// When to retry, for `ResourceResponseStatus.error`.
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

/// Reports that the library no longer needs a resource request.
///
/// The request no longer accepts an answer, so stop working on it and release
/// it.
///
/// See `mln_resource_request_cancel_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
typedef ResourceRequestCancelCallback = void Function();

/// A cancel callback, for `ResourceRequestHandle.setCancelCallback()`.
///
/// See `mln_resource_request_cancel_handler` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class ResourceRequestCancelHandler {
  const ResourceRequestCancelHandler({required this.callback});
  final ResourceRequestCancelCallback callback;
}

/// Why a resource transform fails a request.
///
/// See `mln_resource_transform_error` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class ResourceTransformError extends _Value {
  const ResourceTransformError({
    this.reason = const ResourceErrorReason.fromRawValue(0),
    this.message = '',
    this.retryAfterUnixMs,
  });
  final ResourceErrorReason reason;
  final String message;

  /// When to retry, for `ResourceErrorReason.rateLimit`.
  final int? retryAfterUnixMs;

  /// A copy of this value with the given members replaced.
  ResourceTransformError copyWith({
    ResourceErrorReason? reason,
    String? message,
    int? retryAfterUnixMs,
  }) => ResourceTransformError(
    reason: reason ?? this.reason,
    message: message ?? this.message,
    retryAfterUnixMs: retryAfterUnixMs ?? this.retryAfterUnixMs,
  );

  @override
  List<Object?> get _members => [reason, message, retryAfterUnixMs];
  @override
  String toString() => _describe('ResourceTransformError', const [
    'reason',
    'message',
    'retryAfterUnixMs',
  ]);
}

/// Options for `RuntimeHandle.create()`.
///
/// See `mln_runtime_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeOptions extends _Value {
  const RuntimeOptions({this.assetPath = '', this.cachePath = ''});

  /// The directory that `asset://` URLs load from. Empty uses the working
  /// directory, or the app's assets on Android.
  final String assetPath;

  /// The path of the cache database file, which holds the ambient cache and
  /// offline regions. Its directory must be writable. Empty keeps the cache in
  /// memory.
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

/// Options for `RuntimeHandle.createMap()`.
///
/// See `mln_map_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class MapOptions extends _Value {
  const MapOptions({
    required this.initialExtent,
    this.mapMode = const MapMode.fromRawValue(0),
    this.fastPforEnabled = false,
    this.eventMask,
    this.tileCacheEnabled,
  });

  /// The map's first size, and its scale factor. The scale factor stays fixed
  /// for the map's life, and every render session of the map renders at it.
  final LogicalExtent initialExtent;
  final MapMode mapMode;

  /// Whether the map decodes MapLibre Tiles (MLT) that use FastPFOR integer
  /// encoding. Without it, such tiles fail to load.
  final bool fastPforEnabled;

  /// The events that the map reports. Without it, the map reports them all.
  final RuntimeEventFlag? eventMask;

  /// Whether the map keeps tiles in memory after they leave the view, so they
  /// show again without loading. Without it, the map keeps them.
  final bool? tileCacheEnabled;

  /// A copy of this value with the given members replaced.
  MapOptions copyWith({
    LogicalExtent? initialExtent,
    MapMode? mapMode,
    bool? fastPforEnabled,
    RuntimeEventFlag? eventMask,
    bool? tileCacheEnabled,
  }) => MapOptions(
    initialExtent: initialExtent ?? this.initialExtent,
    mapMode: mapMode ?? this.mapMode,
    fastPforEnabled: fastPforEnabled ?? this.fastPforEnabled,
    eventMask: eventMask ?? this.eventMask,
    tileCacheEnabled: tileCacheEnabled ?? this.tileCacheEnabled,
  );

  @override
  List<Object?> get _members => [
    initialExtent,
    mapMode,
    fastPforEnabled,
    eventMask,
    tileCacheEnabled,
  ];
  @override
  String toString() => _describe('MapOptions', const [
    'initialExtent',
    'mapMode',
    'fastPforEnabled',
    'eventMask',
    'tileCacheEnabled',
  ]);
}

/// An offline region that covers a rectangle of the map.
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

  /// The style whose resources the region holds. Maps use the region when they
  /// load the same style URL.
  final String styleUrl;
  final LatLngBounds bounds;
  final double minZoom;

  /// INFINITY covers every zoom level that the style's sources have.
  final double maxZoom;

  /// The pixel ratio of the images to download, such as 2 for dense screens.
  final double pixelRatio;

  /// Whether the region holds the glyphs of CJK ideographs.
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

/// An offline region that covers a GeoJSON geometry.
///
/// The members other than `geometry` mean the same as those of
/// `OfflineTilePyramidRegionDefinition`.
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
  final String styleUrl;
  final Object? geometry;
  final double minZoom;
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

/// The area, zoom range, and style that an offline region covers.
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

/// An offline region in the runtime's cache file.
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
  final Uint8List metadata;

  @override
  List<Object?> get _members => [id, definition, metadata];
  @override
  String toString() =>
      _describe('OfflineRegionInfo', const ['id', 'definition', 'metadata']);
}

/// The download progress of an offline region.
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

  /// The resources downloaded so far, tiles included.
  final int completedResourceCount;

  /// The size of the downloaded resources in bytes, tiles included.
  final int completedResourceSize;
  final int completedTileCount;
  final int requiredTileCount;

  /// The size of the downloaded tiles in bytes.
  final int completedTileSize;

  /// The resources that the region needs, tiles included. Until
  /// `required_resource_count_is_precise` is true, this is only a lower bound.
  final int requiredResourceCount;
  final bool requiredResourceCountIsPrecise;

  /// Whether the region has downloaded every resource it needs.
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

/// The details of `RuntimeEventType.mapCameraWillChange` and
/// `RuntimeEventType.mapCameraDidChange`.
///
/// See `mln_runtime_event_camera_change` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventCameraChange extends _Value {
  const RuntimeEventCameraChange({
    this.mode = const CameraChangeMode.fromRawValue(0),
    this.source = const CameraChangeSource.fromRawValue(0),
  });
  final CameraChangeMode mode;
  final CameraChangeSource source;

  @override
  List<Object?> get _members => [mode, source];
  @override
  String toString() =>
      _describe('RuntimeEventCameraChange', const ['mode', 'source']);
}

/// The details of `RuntimeEventType.mapLoadingFailed`.
///
/// See `mln_runtime_event_loading_failed` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventLoadingFailed extends _Value {
  const RuntimeEventLoadingFailed({
    this.error = const MapLoadError.fromRawValue(0),
  });
  final MapLoadError error;

  @override
  List<Object?> get _members => [error];
  @override
  String toString() => _describe('RuntimeEventLoadingFailed', const ['error']);
}

/// Statistics about the frames that a map renders.
///
/// See `mln_rendering_stats` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RenderingStats extends _Value {
  const RenderingStats({
    this.encodingTimeMs = 0,
    this.renderingTimeMs = 0,
    this.frameCount = 0,
    this.drawCallCount = 0,
    this.totalDrawCallCount = 0,
  });

  /// The time that the frame took to encode, in milliseconds.
  final double encodingTimeMs;

  /// The time that the frame took to render, in milliseconds.
  final double renderingTimeMs;

  /// The frames rendered so far.
  final int frameCount;

  /// The draw calls in the frame.
  final int drawCallCount;

  /// The draw calls in every frame so far.
  final int totalDrawCallCount;

  @override
  List<Object?> get _members => [
    encodingTimeMs,
    renderingTimeMs,
    frameCount,
    drawCallCount,
    totalDrawCallCount,
  ];
  @override
  String toString() => _describe('RenderingStats', const [
    'encodingTimeMs',
    'renderingTimeMs',
    'frameCount',
    'drawCallCount',
    'totalDrawCallCount',
  ]);
}

/// The details of `RuntimeEventType.mapRenderFrameFinished`.
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
  final bool needsRepaint;
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

/// The details of `RuntimeEventType.mapStyleImageMissing`.
///
/// See `mln_runtime_event_style_image` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventStyleImage extends _Value {
  const RuntimeEventStyleImage({this.imageId = ''});
  final String imageId;

  @override
  List<Object?> get _members => [imageId];
  @override
  String toString() => _describe('RuntimeEventStyleImage', const ['imageId']);
}

/// The ID of a tile.
///
/// `canonical_x` and `canonical_y` place the tile in the source's grid at zoom
/// `canonical_z`. `overscaled_z` is the zoom that the map draws the tile at,
/// and is greater when the source has no tiles that deep. `wrap` says which
/// copy of the world holds the tile: zero for the main one, negative to the
/// west, and positive to the east.
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

/// The details of `RuntimeEventType.mapTileAction`.
///
/// See `mln_runtime_event_tile_action` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventTileAction extends _Value {
  const RuntimeEventTileAction({
    this.sourceId = '',
    this.operation = const TileOperation.fromRawValue(0),
    this.tileId = const TileId(),
  });

  /// The source that the tile belongs to.
  final String sourceId;
  final TileOperation operation;
  final TileId tileId;

  @override
  List<Object?> get _members => [sourceId, operation, tileId];
  @override
  String toString() => _describe('RuntimeEventTileAction', const [
    'sourceId',
    'operation',
    'tileId',
  ]);
}

/// The details of `RuntimeEventType.mapSourceChanged`.
///
/// See `mln_runtime_event_source_changed` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventSourceChanged extends _Value {
  const RuntimeEventSourceChanged({this.sourceId = ''});
  final String sourceId;

  @override
  List<Object?> get _members => [sourceId];
  @override
  String toString() =>
      _describe('RuntimeEventSourceChanged', const ['sourceId']);
}

/// The details of `RuntimeEventType.offlineRegionStatusChanged`.
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

/// The details of `RuntimeEventType.offlineRegionResponseError`.
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

/// The details of `RuntimeEventType.offlineRegionTileCountLimitExceeded`.
///
/// See `mln_runtime_event_offline_region_tile_count_limit` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventOfflineRegionTileCountLimit extends _Value {
  const RuntimeEventOfflineRegionTileCountLimit({
    this.regionId = 0,
    this.limit = 0,
  });
  final int regionId;

  /// The number of demo server tiles that the cache file's offline regions may
  /// hold together.
  final int limit;

  @override
  List<Object?> get _members => [regionId, limit];
  @override
  String toString() => _describe(
    'RuntimeEventOfflineRegionTileCountLimit',
    const ['regionId', 'limit'],
  );
}

/// The details of a glyph event.
///
/// The event covers the glyphs from `first` to `last`, both included.
///
/// See `mln_runtime_event_glyphs` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventGlyphs extends _Value {
  RuntimeEventGlyphs({
    List<String> fontStack = const [],
    this.first = 0,
    this.last = 0,
    this.fontFace = false,
  }) : fontStack = List.unmodifiable(fontStack);

  /// The fonts that the glyphs come from, in order of preference.
  final List<String> fontStack;
  final int first;
  final int last;
  final bool fontFace;

  @override
  List<Object?> get _members => [fontStack, first, last, fontFace];
  @override
  String toString() => _describe('RuntimeEventGlyphs', const [
    'fontStack',
    'first',
    'last',
    'fontFace',
  ]);
}

/// The details of a sprite event.
///
/// `id` and `url` are both empty when the style has no sprite.
///
/// See `mln_runtime_event_sprite` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventSprite extends _Value {
  const RuntimeEventSprite({this.id = '', this.url = ''});
  final String id;
  final String url;

  @override
  List<Object?> get _members => [id, url];
  @override
  String toString() => _describe('RuntimeEventSprite', const ['id', 'url']);
}

/// An event that a runtime or one of its maps raised.
///
/// See `mln_runtime_event` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEvent {
  const RuntimeEvent({
    required this.type,
    this.sourceType = const RuntimeEventSourceType.fromRawValue(0),
    this.source = 0,
    this.generation = 0,
    this.message = '',
    required this.payload,
  });
  final RuntimeEventType type;
  final RuntimeEventSourceType sourceType;

  /// The runtime or map that raised the event, as `source_type` says.
  final int source;

  /// For a map event, a map snapshot with this generation or a later one
  /// includes what the event reports. Zero for a runtime event.
  final int generation;

  /// The error's text for an event that reports a failure, or empty.
  final String message;
  final RuntimeEventPayload payload;
}

sealed class RuntimeEventPayload {
  const RuntimeEventPayload._();
}

final class RuntimeEventPayloadCameraChange extends RuntimeEventPayload {
  const RuntimeEventPayloadCameraChange(this.value) : super._();
  final RuntimeEventCameraChange value;
}

final class RuntimeEventPayloadLoadingFailed extends RuntimeEventPayload {
  const RuntimeEventPayloadLoadingFailed(this.value) : super._();
  final RuntimeEventLoadingFailed value;
}

final class RuntimeEventPayloadRenderFrame extends RuntimeEventPayload {
  const RuntimeEventPayloadRenderFrame(this.value) : super._();
  final RuntimeEventRenderFrame value;
}

final class RuntimeEventPayloadStyleImage extends RuntimeEventPayload {
  const RuntimeEventPayloadStyleImage(this.value) : super._();
  final RuntimeEventStyleImage value;
}

final class RuntimeEventPayloadTileAction extends RuntimeEventPayload {
  const RuntimeEventPayloadTileAction(this.value) : super._();
  final RuntimeEventTileAction value;
}

final class RuntimeEventPayloadSourceChanged extends RuntimeEventPayload {
  const RuntimeEventPayloadSourceChanged(this.value) : super._();
  final RuntimeEventSourceChanged value;
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

final class RuntimeEventPayloadGlyphs extends RuntimeEventPayload {
  const RuntimeEventPayloadGlyphs(this.value) : super._();
  final RuntimeEventGlyphs value;
}

final class RuntimeEventPayloadSprite extends RuntimeEventPayload {
  const RuntimeEventPayloadSprite(this.value) : super._();
  final RuntimeEventSprite value;
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

/// Receives a batch of events.
///
/// See `mln_runtime_event_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
typedef RuntimeEventCallback = void Function(List<RuntimeEvent> events);

/// An event callback, for `RuntimeHandle.setEventHandler()`.
///
/// See `mln_runtime_event_handler` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class RuntimeEventHandler {
  const RuntimeEventHandler({required this.callback});
  final RuntimeEventCallback callback;
}

/// An HTTP header.
///
/// See `mln_http_header` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class HttpHeader extends _Value {
  const HttpHeader({required this.name, this.value = ''});
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

/// Headers to add to the HTTP requests whose URL matches a pattern.
///
/// See `mln_http_header_rule` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class HttpHeaderRule extends _Value {
  HttpHeaderRule({
    this.kinds = ResourceKindFlag.none,
    this.flags = UrlMatchFlag.none,
    this.url = '',
    List<HttpHeader> headers = const [],
  }) : headers = List.unmodifiable(headers);

  /// The kinds of resource that the rule covers. Zero covers every kind.
  final ResourceKindFlag kinds;
  final UrlMatchFlag flags;
  final String url;
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

/// Headers to add to HTTP requests, for
/// `RuntimeHandle.setHttpHeaderTransform()`.
///
/// See `mln_http_header_transform` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class HttpHeaderTransform {
  HttpHeaderTransform({List<HttpHeaderRule> rules = const []})
    : rules = List.unmodifiable(rules);

  /// The first rule that matches a request adds its headers, and no other.
  final List<HttpHeaderRule> rules;
}

/// The bytes from `start` to `end` of a resource, both included.
///
/// See `mln_resource_range` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class ResourceRange extends _Value {
  const ResourceRange({this.start = 0, this.end = 0});
  final int start;
  final int end;

  @override
  List<Object?> get _members => [start, end];
  @override
  String toString() => _describe('ResourceRange', const ['start', 'end']);
}

/// A request that a resource provider serves.
///
/// When the cache holds an earlier copy of the resource, the `prior_` fields
/// describe it.
///
/// See `mln_resource_request_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
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

  /// The URL as the style names it.
  final String requestedUrl;

  /// The URL to load, after the library expands any shorthand in
  /// `requested_url` and before any resource transform. Routes match this URL.
  final String resolvedUrl;
  final ResourceKind kind;
  final ResourceLoadingMethod loadingMethod;
  final ResourcePriority priority;
  final ResourceUsage usage;
  final ResourceStoragePolicy storagePolicy;

  /// The only bytes of the resource that the request wants.
  final ResourceRange? range;
  final int? priorModifiedUnixMs;
  final int? priorExpiresUnixMs;

  /// Empty when the earlier copy has no ETag.
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

/// Serves a network request in place of the library.
///
/// Answer with `ResourceRequestHandle.complete()`, or hand the request back to
/// the library with `ResourceRequestHandle.passThrough()`. Either can come now
/// or later, from any thread. A request released without an answer fails.
///
/// See `mln_resource_provider_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
typedef ResourceProviderCallback =
    void Function(ResourceRequestInfo request, ResourceRequestHandle handle);

/// The requests that a resource provider serves.
///
/// See `mln_resource_route` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class ResourceRoute extends _Value {
  const ResourceRoute({
    this.kinds = ResourceKindFlag.none,
    this.flags = UrlMatchFlag.none,
    this.url = '',
  });

  /// The kinds of resource that the route covers. Zero covers every kind.
  final ResourceKindFlag kinds;
  final UrlMatchFlag flags;
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

/// A resource provider, for `RuntimeHandle.setResourceProvider()`.
///
/// See `mln_resource_provider` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class ResourceProvider {
  ResourceProvider({
    required this.callback,
    List<ResourceRoute> routes = const [],
  }) : routes = List.unmodifiable(routes);
  final ResourceProviderCallback callback;

  /// The requests that the provider serves. Without routes, it serves every
  /// network request.
  final List<ResourceRoute> routes;
}

/// Rewrites the URL of a network request before the library sends it.
///
/// The request waits until you answer with
/// `ResourceTransformRequestHandle.complete()` or
/// `ResourceTransformRequestHandle.fail()`. To keep `url`, complete with it.
/// Releasing `request` without an answer fails the request. You can answer now
/// or later, from any thread.
///
/// See `mln_resource_transform_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
typedef ResourceTransformCallback =
    void Function(
      ResourceKind kind,
      String url,
      ResourceTransformRequestHandle request,
    );

/// A query parameter that a resource transform rule adds to a URL.
///
/// Pass `name` and `value` unencoded. The library percent-encodes every byte of
/// them except the letters, digits, `-`, `.`, `_`, and `~`, and adds them as
/// `name=value`.
///
/// See `mln_url_query_parameter` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class UrlQueryParameter extends _Value {
  const UrlQueryParameter({required this.name, this.value = ''});
  final String name;
  final String value;

  /// A copy of this value with the given members replaced.
  UrlQueryParameter copyWith({String? name, String? value}) =>
      UrlQueryParameter(name: name ?? this.name, value: value ?? this.value);

  @override
  List<Object?> get _members => [name, value];
  @override
  String toString() => _describe('UrlQueryParameter', const ['name', 'value']);
}

/// Query parameters to add to the URLs of requests that match a pattern.
///
/// The library adds the parameters, in order, at the end of the URL's query and
/// before any fragment. It keeps the parameters that the URL already has, even
/// one with the same name.
///
/// See `mln_resource_transform_rule` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class ResourceTransformRule extends _Value {
  ResourceTransformRule({
    this.kinds = ResourceKindFlag.none,
    this.flags = UrlMatchFlag.none,
    this.url = '',
    List<UrlQueryParameter> parameters = const [],
  }) : parameters = List.unmodifiable(parameters);

  /// The kinds of resource that the rule covers. Zero covers every kind.
  final ResourceKindFlag kinds;
  final UrlMatchFlag flags;
  final String url;
  final List<UrlQueryParameter> parameters;

  /// A copy of this value with the given members replaced.
  ResourceTransformRule copyWith({
    ResourceKindFlag? kinds,
    UrlMatchFlag? flags,
    String? url,
    List<UrlQueryParameter>? parameters,
  }) => ResourceTransformRule(
    kinds: kinds ?? this.kinds,
    flags: flags ?? this.flags,
    url: url ?? this.url,
    parameters: parameters ?? this.parameters,
  );

  @override
  List<Object?> get _members => [kinds, flags, url, parameters];
  @override
  String toString() => _describe('ResourceTransformRule', const [
    'kinds',
    'flags',
    'url',
    'parameters',
  ]);
}

/// How to rewrite request URLs, for `RuntimeHandle.setResourceTransform()`.
///
/// Set either `callback` or `rules`, not both.
///
/// See `mln_resource_transform` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
final class ResourceTransform {
  ResourceTransform({
    this.callback,
    List<ResourceTransformRule> rules = const [],
  }) : rules = List.unmodifiable(rules);
  final ResourceTransformCallback? callback;

  /// The first rule that matches a request adds its parameters, and no other. A
  /// request that no rule matches keeps its URL.
  final List<ResourceTransformRule> rules;
}

/// The type of a style expression's result.
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

  /// The type of each item when `kind` is `StyleExpressionTypeKind.array`. It
  /// can't be an array itself.
  final StyleExpressionTypeKind itemKind;

  /// The length of an array, or 0 for any length.
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

  /// The type that the result must have.
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

/// The inputs that a style expression reads while it evaluates.
///
/// See `mln_style_expression_inputs` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
final class StyleExpressionInputs extends _Value {
  StyleExpressionInputs({
    this.zoom,
    this.colorRampParameter,
    this.feature,
    this.featureState,
    this.globalState,
    this.canonicalTileId,
    List<String> availableImages = const [],
  }) : availableImages = List.unmodifiable(availableImages);
  final double? zoom;

  /// The value of `heatmap-density`, `line-progress`, and `elevation`.
  final double? colorRampParameter;

  /// The feature to evaluate against, as a GeoJSON feature, or empty for none.
  final Object? feature;

  /// The feature's state, as a JSON object, or empty for none.
  final Object? featureState;

  /// The values that `global-state` reads, as a JSON object, or empty for none.
  final Object? globalState;

  /// The tile that holds the feature. `within` and `distance` need it to read
  /// the feature's geometry.
  final CanonicalTileId? canonicalTileId;

  /// The IDs of the images that `image` treats as available.
  final List<String> availableImages;

  /// A copy of this value with the given members replaced.
  StyleExpressionInputs copyWith({
    double? zoom,
    double? colorRampParameter,
    Object? feature = _unchanged,
    Object? featureState = _unchanged,
    Object? globalState = _unchanged,
    CanonicalTileId? canonicalTileId,
    List<String>? availableImages,
  }) => StyleExpressionInputs(
    zoom: zoom ?? this.zoom,
    colorRampParameter: colorRampParameter ?? this.colorRampParameter,
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
    'feature',
    'featureState',
    'globalState',
    'canonicalTileId',
    'availableImages',
  ]);
}

/// What a style expression's result depends on.
///
/// See `mln_style_expression_info` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
final class StyleExpressionInfo extends _Value {
  const StyleExpressionInfo({
    this.isFeatureConstant = false,
    this.isZoomConstant = false,
  });

  /// True when the result doesn't depend on the feature or its state.
  final bool isFeatureConstant;

  /// True when the result doesn't depend on the zoom level.
  final bool isZoomConstant;

  @override
  List<Object?> get _members => [isFeatureConstant, isZoomConstant];
  @override
  String toString() => _describe('StyleExpressionInfo', const [
    'isFeatureConstant',
    'isZoomConstant',
  ]);
}

/// Options for `VulkanDeviceHandle.create()`.
///
/// See `mln_vulkan_device_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html).
final class VulkanDeviceOptions extends _Value {
  VulkanDeviceOptions({
    this.presentation,
    Uint8List? deviceUuid,
    Uint8List? deviceLuid,
  }) : deviceUuid = deviceUuid == null
           ? null
           : Uint8List.fromList(deviceUuid).asUnmodifiableView(),
       deviceLuid = deviceLuid == null
           ? null
           : Uint8List.fromList(deviceLuid).asUnmodifiableView();

  /// Whether the device can present to a window. The instance then enables
  /// VK_KHR_surface and the platform's surface extensions, and the device
  /// enables VK_KHR_swapchain. Defaults to false.
  final bool? presentation;

  /// The 16-byte `deviceUUID` of the GPU to use, as
  /// `VkPhysicalDeviceIDProperties` reports it.
  final Uint8List? deviceUuid;

  /// The 8-byte `deviceLUID` of the GPU to use, as
  /// `VkPhysicalDeviceIDProperties` reports it. On Windows, this is the GPU's
  /// DXGI adapter LUID.
  final Uint8List? deviceLuid;

  /// A copy of this value with the given members replaced.
  VulkanDeviceOptions copyWith({
    bool? presentation,
    Uint8List? deviceUuid,
    Uint8List? deviceLuid,
  }) => VulkanDeviceOptions(
    presentation: presentation ?? this.presentation,
    deviceUuid: deviceUuid ?? this.deviceUuid,
    deviceLuid: deviceLuid ?? this.deviceLuid,
  );

  @override
  List<Object?> get _members => [presentation, deviceUuid, deviceLuid];
  @override
  String toString() => _describe('VulkanDeviceOptions', const [
    'presentation',
    'deviceUuid',
    'deviceLuid',
  ]);
}

/// A Vulkan device's context, as `VulkanDeviceHandle.getContext()` reports it.
///
/// See `mln_vulkan_device_context_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html).
final class VulkanDeviceContextResult extends _Value {
  const VulkanDeviceContextResult({required this.context});
  final VulkanContextDescriptor context;

  @override
  List<Object?> get _members => [context];
  @override
  String toString() =>
      _describe('VulkanDeviceContextResult', const ['context']);
}

/// The size of a diagnostic message in bytes, with its terminator.
///
/// See `MLN_DIAGNOSTIC_MESSAGE_CAPACITY` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
const int diagnosticMessageCapacity = 4096;

/// The token of the frame results that a session adds when it renders an image
/// for `MapHandle.requestStillImage()`. Your own demands can't use it.
///
/// See `MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
const int frameDemandTokenStillImage = 0xFFFFFFFFFFFFFFFF;

/// The most textures that a render session's texture ring can have.
///
/// See `MLN_TEXTURE_RING_MAX_DEPTH` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
const int textureRingMaxDepth = 8;

/// The null Vulkan non-dispatchable handle.
///
/// See `MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
const int vulkanNonDispatchableHandleNull = 0;
