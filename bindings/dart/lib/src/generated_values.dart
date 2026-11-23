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

  /// Keeps the ambient cache's resources, but makes them stale: maps check each
  /// one with the server when they next use it, and show it meanwhile unless
  /// the server requires checking it first. Offline regions keep their
  /// resources as they are.
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

  /// A gesture that a gesture recognizer recognized.
  static const gesture = CameraChangeSource.fromRawValue(1);

  /// The glide and snap after a gesture recognizer's gesture.
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
  /// `MapHandle.cancelCameraTransitionsWithId()`, a gesture that a gesture
  /// recognizer begins or cancels, `MapHandle.setFreeCameraOptions()`, or a
  /// resize that moved the camera back inside its bounds stopped the change. A
  /// command that fails before it moves the camera also reports this.
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

/// How a graphics API's clip space holds depth.
///
/// See `mln_clip_depth_range` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class ClipDepthRange extends _Enum {
  const ClipDepthRange.fromRawValue(super.rawValue);

  /// Clip z runs from 0 to w, as in Metal, Vulkan, WebGPU, Direct3D, and OpenGL
  /// with `glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE)`.
  static const zeroToOne = ClipDepthRange.fromRawValue(1);

  /// Clip z runs from -w to w, as in OpenGL by default.
  static const negativeOneToOne = ClipDepthRange.fromRawValue(2);
  @override
  String toString() => switch (rawValue) {
    1 => 'ClipDepthRange.zeroToOne',
    2 => 'ClipDepthRange.negativeOneToOne',
    _ => 'ClipDepthRange($rawValue)',
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

/// What shows a D3D12 composition swap chain.
///
/// See `mln_d3d12_composition_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class D3d12CompositionKind extends _Enum {
  const D3d12CompositionKind.fromRawValue(super.rawValue);

  /// An `IDCompositionVisual`, whose content the session sets to its swap
  /// chain.
  static const visual = D3d12CompositionKind.fromRawValue(1);

  /// A XAML `SwapChainPanel`'s `ISwapChainPanelNative`, of WinUI or UWP, whose
  /// swap chain the session sets.
  static const swapChainPanel = D3d12CompositionKind.fromRawValue(2);
  @override
  String toString() => switch (rawValue) {
    1 => 'D3d12CompositionKind.visual',
    2 => 'D3d12CompositionKind.swapChainPanel',
    _ => 'D3d12CompositionKind($rawValue)',
  };
}

/// A format of depth and stencil textures, as a bit in a set.
///
/// See `mln_depth_stencil_format_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class DepthStencilFormatFlag extends _Flags<DepthStencilFormatFlag> {
  const DepthStencilFormatFlag.fromRawValue(super.rawValue);

  /// 24-bit normalized depth and 8-bit stencil: VK_FORMAT_D24_UNORM_S8_UINT,
  /// MTLPixelFormatDepth24Unorm_Stencil8, GL_DEPTH24_STENCIL8, or
  /// WGPUTextureFormat_Depth24PlusStencil8 (`depth24plus-stencil8`).
  static const depth24Stencil8 = DepthStencilFormatFlag.fromRawValue(1);

  /// 32-bit float depth and 8-bit stencil: VK_FORMAT_D32_SFLOAT_S8_UINT,
  /// MTLPixelFormatDepth32Float_Stencil8, GL_DEPTH32F_STENCIL8, or
  /// WGPUTextureFormat_Depth32FloatStencil8 (`depth32float-stencil8`).
  static const depth32FloatStencil8 = DepthStencilFormatFlag.fromRawValue(2);

  /// No flags.
  static const none = DepthStencilFormatFlag.fromRawValue(0);
  @override
  DepthStencilFormatFlag _of(int rawValue) =>
      DepthStencilFormatFlag.fromRawValue(rawValue);
  @override
  String toString() => _describe('DepthStencilFormatFlag', const {
    0: 'none',
    1: 'depth24Stencil8',
    2: 'depth32FloatStencil8',
  });
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

/// The part of the camera that a notch is on.
///
/// See `mln_gesture_axis` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
final class GestureAxis extends _Enum {
  const GestureAxis.fromRawValue(super.rawValue);
  static const zoom = GestureAxis.fromRawValue(0);
  static const bearing = GestureAxis.fromRawValue(1);
  static const pitch = GestureAxis.fromRawValue(2);
  @override
  String toString() => switch (rawValue) {
    0 => 'GestureAxis.zoom',
    1 => 'GestureAxis.bearing',
    2 => 'GestureAxis.pitch',
    _ => 'GestureAxis($rawValue)',
  };
}

/// What a one-pointer drag or a double tap does instead of its default, from
/// the pointer's held buttons or from held keys. A drag reads its modes when it
/// starts and keeps them until it ends, and a double tap when it lifts;
/// rotating wins over a box zoom.
///
/// See `mln_gesture_mode` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
final class GestureMode extends _Flags<GestureMode> {
  const GestureMode.fromRawValue(super.rawValue);

  /// Sideways turns the map and up and down tilts it.
  static const rotate = GestureMode.fromRawValue(1);

  /// Draws a box that the map zooms to when the pointer lifts.
  static const boxZoom = GestureMode.fromRawValue(2);

  /// A double tap zooms out instead of in.
  static const zoomOut = GestureMode.fromRawValue(4);

  /// No flags.
  static const none = GestureMode.fromRawValue(0);
  @override
  GestureMode _of(int rawValue) => GestureMode.fromRawValue(rawValue);
  @override
  String toString() => _describe('GestureMode', const {
    0: 'none',
    1: 'rotate',
    2: 'boxZoom',
    4: 'zoomOut',
  });
}

/// When a gesture recognizer next needs `GestureRecognizerHandle.tick()`.
///
/// See `mln_gesture_next_tick` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
final class GestureNextTick extends _Enum {
  const GestureNextTick.fromRawValue(super.rawValue);

  /// Not until the next input.
  static const none = GestureNextTick.fromRawValue(0);

  /// Every display frame, while a delta eases or a gesture, its glide, or its
  /// snap may reach a notch.
  static const frame = GestureNextTick.fromRawValue(1);

  /// At `next_tick_ms`, on the inputs' clock: when a press becomes a long
  /// press, or idle deltas end.
  static const at = GestureNextTick.fromRawValue(2);
  @override
  String toString() => switch (rawValue) {
    0 => 'GestureNextTick.none',
    1 => 'GestureNextTick.frame',
    2 => 'GestureNextTick.at',
    _ => 'GestureNextTick($rawValue)',
  };
}

/// A way to share a texture's memory with another API or process.
///
/// See `mln_gpu_export_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class GpuExportKind extends _Enum {
  const GpuExportKind.fromRawValue(super.rawValue);

  /// The texture isn't shared.
  static const none = GpuExportKind.fromRawValue(0);

  /// An `IOSurfaceRef`.
  static const ioSurface = GpuExportKind.fromRawValue(1);

  /// A Linux dmabuf, as file descriptors with a DRM format and modifier.
  static const dmaBuf = GpuExportKind.fromRawValue(2);

  /// An `AHardwareBuffer*`.
  static const androidHardwareBuffer = GpuExportKind.fromRawValue(3);

  /// A DXGI shared texture's NT handle.
  static const dxgiSharedHandle = GpuExportKind.fromRawValue(4);

  /// A Vulkan or OpenGL opaque memory file descriptor.
  static const opaqueFd = GpuExportKind.fromRawValue(5);
  @override
  String toString() => switch (rawValue) {
    0 => 'GpuExportKind.none',
    1 => 'GpuExportKind.ioSurface',
    2 => 'GpuExportKind.dmaBuf',
    3 => 'GpuExportKind.androidHardwareBuffer',
    4 => 'GpuExportKind.dxgiSharedHandle',
    5 => 'GpuExportKind.opaqueFd',
    _ => 'GpuExportKind($rawValue)',
  };
}

/// A way to share a texture's memory, as a bit in a set.
///
/// See `mln_gpu_export_kind_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class GpuExportKindFlag extends _Flags<GpuExportKindFlag> {
  const GpuExportKindFlag.fromRawValue(super.rawValue);
  static const ioSurface = GpuExportKindFlag.fromRawValue(1);
  static const dmaBuf = GpuExportKindFlag.fromRawValue(2);
  static const androidHardwareBuffer = GpuExportKindFlag.fromRawValue(4);
  static const dxgiSharedHandle = GpuExportKindFlag.fromRawValue(8);
  static const opaqueFd = GpuExportKindFlag.fromRawValue(16);

  /// No flags.
  static const none = GpuExportKindFlag.fromRawValue(0);
  @override
  GpuExportKindFlag _of(int rawValue) =>
      GpuExportKindFlag.fromRawValue(rawValue);
  @override
  String toString() => _describe('GpuExportKindFlag', const {
    0: 'none',
    1: 'ioSurface',
    2: 'dmaBuf',
    4: 'androidHardwareBuffer',
    8: 'dxgiSharedHandle',
    16: 'opaqueFd',
  });
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

  /// `handle` is a sync file descriptor. A descriptor of -1, as Vulkan exports
  /// for a semaphore that has already signaled, counts as
  /// `GpuSyncKind.cpuComplete`.
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

/// How a part's glide unfolds from its release speed.
///
/// See `mln_momentum_decay_model` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
final class MomentumDecayModel extends _Enum {
  const MomentumDecayModel.fromRawValue(super.rawValue);

  /// The speed halves every `half_life_ms`, until it drops to `stop_speed`.
  /// UIScrollView's normal deceleration is a half-life of 346 ms, and its fast
  /// deceleration 69 ms.
  static const friction = MomentumDecayModel.fromRawValue(0);

  /// The glide lasts `duration_ms` × (speed / `reference_speed`) to the power
  /// of `duration_exponent`, and follows `easing`, starting at the release
  /// speed. Its defaults are Android's OverScroller fling, in its
  /// density-independent pixels: a reference speed of 1000, 555.182 ms, an
  /// exponent of 0.7362676, the curve (0.175, 0.5, 0.35, 1), a minimum speed of
  /// 50 and a maximum of 8000.
  static const curve = MomentumDecayModel.fromRawValue(1);
  @override
  String toString() => switch (rawValue) {
    0 => 'MomentumDecayModel.friction',
    1 => 'MomentumDecayModel.curve',
    _ => 'MomentumDecayModel($rawValue)',
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

/// Where a platform-recognized pinch is.
///
/// See `mln_pinch_phase` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
final class PinchPhase extends _Enum {
  const PinchPhase.fromRawValue(super.rawValue);
  static const begin = PinchPhase.fromRawValue(0);
  static const update = PinchPhase.fromRawValue(1);

  /// The fingers lifted: the pinch glides on.
  static const end = PinchPhase.fromRawValue(2);

  /// The platform took the pinch away: nothing continues, and the camera's
  /// animations stop.
  static const cancel = PinchPhase.fromRawValue(3);
  @override
  String toString() => switch (rawValue) {
    0 => 'PinchPhase.begin',
    1 => 'PinchPhase.update',
    2 => 'PinchPhase.end',
    3 => 'PinchPhase.cancel',
    _ => 'PinchPhase($rawValue)',
  };
}

/// What kind of device a pointer is.
///
/// See `mln_pointer_kind` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
final class PointerKind extends _Enum {
  const PointerKind.fromRawValue(super.rawValue);
  static const mouse = PointerKind.fromRawValue(0);

  /// Two touches pinch, twist, and tilt, and a held second tap zooms.
  static const touch = PointerKind.fromRawValue(1);
  static const pen = PointerKind.fromRawValue(2);
  @override
  String toString() => switch (rawValue) {
    0 => 'PointerKind.mouse',
    1 => 'PointerKind.touch',
    2 => 'PointerKind.pen',
    _ => 'PointerKind($rawValue)',
  };
}

/// A projection that a style's
/// \[projection\](https://maplibre.org/maplibre-style-spec/projection/) names.
///
/// See `mln_projection_type` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class ProjectionType extends _Enum {
  const ProjectionType.fromRawValue(super.rawValue);
  static const mercator = ProjectionType.fromRawValue(0);

  /// Draws the map as a globe.
  static const verticalPerspective = ProjectionType.fromRawValue(1);

  /// Draws the map as a globe at low zooms, and hands over to Mercator between
  /// zoom 11 and 12.
  static const globe = ProjectionType.fromRawValue(2);
  @override
  String toString() => switch (rawValue) {
    0 => 'ProjectionType.mercator',
    1 => 'ProjectionType.verticalPerspective',
    2 => 'ProjectionType.globe',
    _ => 'ProjectionType($rawValue)',
  };
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

/// A render backend.
///
/// See `mln_render_backend` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
final class RenderBackend extends _Enum {
  const RenderBackend.fromRawValue(super.rawValue);
  static const metal = RenderBackend.fromRawValue(1);
  static const vulkan = RenderBackend.fromRawValue(2);
  static const openGl = RenderBackend.fromRawValue(3);
  static const webGpu = RenderBackend.fromRawValue(4);

  /// D3D12, on Windows. It compiles its shaders at run time with DXC, which
  /// your app ships: `dxcompiler.dll`, 1.8.2502 or later, beside your app's
  /// executable or in another directory of the DLL search path, which doesn't
  /// take the library's own directory. Without it, the library renders on
  /// another backend.
  static const d3d12 = RenderBackend.fromRawValue(5);
  @override
  String toString() => switch (rawValue) {
    1 => 'RenderBackend.metal',
    2 => 'RenderBackend.vulkan',
    3 => 'RenderBackend.openGl',
    4 => 'RenderBackend.webGpu',
    5 => 'RenderBackend.d3d12',
    _ => 'RenderBackend($rawValue)',
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
  static const d3d12 = RenderBackendFlag.fromRawValue(16);

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
    16: 'd3d12',
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

/// A render target kind, as a bit in a set.
///
/// See `mln_render_target_kind_flag` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class RenderTargetKindFlag extends _Flags<RenderTargetKindFlag> {
  const RenderTargetKindFlag.fromRawValue(super.rawValue);
  static const offscreen = RenderTargetKindFlag.fromRawValue(1);
  static const metalLayer = RenderTargetKindFlag.fromRawValue(2);
  static const win32Window = RenderTargetKindFlag.fromRawValue(4);
  static const xlibWindow = RenderTargetKindFlag.fromRawValue(8);
  static const waylandSurface = RenderTargetKindFlag.fromRawValue(16);
  static const androidWindow = RenderTargetKindFlag.fromRawValue(32);
  static const ohosWindow = RenderTargetKindFlag.fromRawValue(64);
  static const vulkanSurface = RenderTargetKindFlag.fromRawValue(512);
  static const openGlSurface = RenderTargetKindFlag.fromRawValue(1024);
  static const metalBorrowedTextures = RenderTargetKindFlag.fromRawValue(2048);
  static const vulkanBorrowedTextures = RenderTargetKindFlag.fromRawValue(4096);
  static const openGlBorrowedTextures = RenderTargetKindFlag.fromRawValue(8192);
  static const webGpuBorrowedTextures = RenderTargetKindFlag.fromRawValue(
    16384,
  );
  static const exportedRing = RenderTargetKindFlag.fromRawValue(32768);
  static const webCanvas = RenderTargetKindFlag.fromRawValue(65536);
  static const d3d12BorrowedTextures = RenderTargetKindFlag.fromRawValue(
    131072,
  );
  static const d3d12Composition = RenderTargetKindFlag.fromRawValue(262144);

  /// No flags.
  static const none = RenderTargetKindFlag.fromRawValue(0);
  @override
  RenderTargetKindFlag _of(int rawValue) =>
      RenderTargetKindFlag.fromRawValue(rawValue);
  @override
  String toString() => _describe('RenderTargetKindFlag', const {
    0: 'none',
    1: 'offscreen',
    2: 'metalLayer',
    4: 'win32Window',
    8: 'xlibWindow',
    16: 'waylandSurface',
    32: 'androidWindow',
    64: 'ohosWindow',
    512: 'vulkanSurface',
    1024: 'openGlSurface',
    2048: 'metalBorrowedTextures',
    4096: 'vulkanBorrowedTextures',
    8192: 'openGlBorrowedTextures',
    16384: 'webGpuBorrowedTextures',
    32768: 'exportedRing',
    65536: 'webCanvas',
    131072: 'd3d12BorrowedTextures',
    262144: 'd3d12Composition',
  });
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
  static const mapGlyphsRequested = RuntimeEventFlag.fromRawValue(524288);
  static const mapGlyphsLoaded = RuntimeEventFlag.fromRawValue(1048576);
  static const mapGlyphsError = RuntimeEventFlag.fromRawValue(2097152);
  static const mapSpriteRequested = RuntimeEventFlag.fromRawValue(4194304);
  static const mapSpriteLoaded = RuntimeEventFlag.fromRawValue(8388608);
  static const mapSpriteError = RuntimeEventFlag.fromRawValue(16777216);
  static const allMapEvents = RuntimeEventFlag.fromRawValue(33161214);
  static const allRuntimeEvents = RuntimeEventFlag.fromRawValue(393216);
  static const all = RuntimeEventFlag.fromRawValue(33554430);
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
    524288: 'mapGlyphsRequested',
    1048576: 'mapGlyphsLoaded',
    2097152: 'mapGlyphsError',
    4194304: 'mapSpriteRequested',
    8388608: 'mapSpriteLoaded',
    16777216: 'mapSpriteError',
    33161214: 'allMapEvents',
    393216: 'allRuntimeEvents',
    33554430: 'all',
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
  static const mapGlyphsRequested = RuntimeEventType.fromRawValue(19);
  static const mapGlyphsLoaded = RuntimeEventType.fromRawValue(20);
  static const mapGlyphsError = RuntimeEventType.fromRawValue(21);
  static const mapSpriteRequested = RuntimeEventType.fromRawValue(22);
  static const mapSpriteLoaded = RuntimeEventType.fromRawValue(23);
  static const mapSpriteError = RuntimeEventType.fromRawValue(24);
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
    19 => 'RuntimeEventType.mapGlyphsRequested',
    20 => 'RuntimeEventType.mapGlyphsLoaded',
    21 => 'RuntimeEventType.mapGlyphsError',
    22 => 'RuntimeEventType.mapSpriteRequested',
    23 => 'RuntimeEventType.mapSpriteLoaded',
    24 => 'RuntimeEventType.mapSpriteError',
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
  static const customGeometry = StyleSourceType.fromRawValue(7);
  static const customVector = StyleSourceType.fromRawValue(8);
  @override
  String toString() => switch (rawValue) {
    0 => 'StyleSourceType.unknown',
    1 => 'StyleSourceType.vector',
    2 => 'StyleSourceType.raster',
    3 => 'StyleSourceType.rasterDem',
    4 => 'StyleSourceType.geoJson',
    5 => 'StyleSourceType.image',
    6 => 'StyleSourceType.video',
    7 => 'StyleSourceType.customGeometry',
    8 => 'StyleSourceType.customVector',
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
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class VulkanImageOwner extends _Enum {
  const VulkanImageOwner.fromRawValue(super.rawValue);

  /// The image stays with the queue family of the device's queue.
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

/// The Android hardware buffer of an exported ring's acquired frame.
///
/// See `mln_android_hardware_buffer_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
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

/// The D3D12 texture of an acquired frame.
///
/// The depth members are set when the frame has a depth texture, as for
/// `MetalTextureFrame`. Otherwise they are zero.
///
/// See `mln_d3d12_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class D3d12TextureFrame extends _Value {
  const D3d12TextureFrame({
    this.width = 0,
    this.height = 0,
    this.scaleFactor = 0,
    this.slot = 0,
    this.resource = NativePointer.nullPointer,
    this.device = NativePointer.nullPointer,
    this.format = 0,
    this.state = 0,
    this.depthResource = NativePointer.nullPointer,
    this.depthFormat = 0,
    this.depthState = 0,
  });
  final int width;
  final int height;
  final double scaleFactor;

  /// The texture's index in the descriptor's `textures`, or in the ring.
  final int slot;

  /// The `ID3D12Resource`.
  final NativePointer resource;

  /// The resource's `ID3D12Device`.
  final NativePointer device;

  /// The resource's `DXGI_FORMAT`.
  final int format;

  /// The `D3D12_RESOURCE_STATES` that the frame left the resource in.
  final int state;

  /// The `ID3D12Resource` that holds the frame's depth and stencil.
  final NativePointer depthResource;

  /// The depth resource's `DXGI_FORMAT`.
  final int depthFormat;

  /// The `D3D12_RESOURCE_STATES` that the frame left the depth resource in.
  final int depthState;

  @override
  List<Object?> get _members => [
    width,
    height,
    scaleFactor,
    slot,
    resource,
    device,
    format,
    state,
    depthResource,
    depthFormat,
    depthState,
  ];
  @override
  String toString() => _describe('D3d12TextureFrame', const [
    'width',
    'height',
    'scaleFactor',
    'slot',
    'resource',
    'device',
    'format',
    'state',
    'depthResource',
    'depthFormat',
    'depthState',
  ]);
}

/// Where one plane of a dmabuf keeps its data.
///
/// See `mln_dma_buf_plane` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
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

/// The dmabuf of an exported ring's acquired frame.
///
/// See `mln_dma_buf_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
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

/// The DXGI shared texture of an exported ring's acquired frame.
///
/// See `mln_dxgi_shared_texture_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
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

/// The IOSurface of an exported ring's acquired frame.
///
/// See `mln_io_surface_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
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
/// The depth members are set when the frame has a depth texture: one of a
/// borrowed ring's `depth_textures`, or an exported ring's with
/// `RenderSessionAttachOptions.keepDepth`. Otherwise they are zero.
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
    this.depthTexture = NativePointer.nullPointer,
    this.depthPixelFormat = 0,
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

  /// The MTLTexture that holds the frame's depth and stencil.
  final NativePointer depthTexture;

  /// The depth texture's MTLPixelFormat.
  final int depthPixelFormat;

  @override
  List<Object?> get _members => [
    width,
    height,
    scaleFactor,
    slot,
    texture,
    device,
    pixelFormat,
    depthTexture,
    depthPixelFormat,
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
    'depthTexture',
    'depthPixelFormat',
  ]);
}

/// The memory of an exported ring's acquired frame, as an opaque file
/// descriptor.
///
/// The memory holds a 2D Vulkan image of `format`, with one mip level, one
/// layer, optimal tiling, and the usage VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
/// VK_IMAGE_USAGE_SAMPLED_BIT, and VK_IMAGE_USAGE_TRANSFER_SRC_BIT, which a
/// Vulkan image that imports it has too. In OpenGL, import it with
/// glImportMemoryFdEXT() and GL_HANDLE_TYPE_OPAQUE_FD_EXT, and make the texture
/// with GL_TEXTURE_TILING_EXT set to GL_OPTIMAL_TILING_EXT and
/// glTexStorageMem2DEXT() at offset 0. Each frame leaves the image in
/// VK_IMAGE_LAYOUT_GENERAL, released to VK_QUEUE_FAMILY_EXTERNAL.
///
/// With `RenderSessionAttachOptions.keepDepth`, on a device whose
/// `GpuDeviceCapabilities.depthExportKinds` has `GpuExportKind.opaqueFd`, the
/// depth members share the memory of the frame's depth image in the same way.
/// Otherwise `depth_fd` is -1 and the others are zero. The depth image is 2D,
/// of `depth_format`, with one mip level, one layer, optimal tiling, and the
/// usage VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
/// VK_IMAGE_USAGE_SAMPLED_BIT, and VK_IMAGE_USAGE_TRANSFER_SRC_BIT. Each frame
/// leaves it in VK_IMAGE_LAYOUT_GENERAL, released to VK_QUEUE_FAMILY_EXTERNAL.
/// In OpenGL, make its texture with the internal format that
/// `DepthStencilFormatFlag` names for `depth_format`.
///
/// See `mln_opaque_fd_frame` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
final class OpaqueFdFrame extends _Value {
  const OpaqueFdFrame({
    this.width = 0,
    this.height = 0,
    this.scaleFactor = 0,
    this.slot = 0,
    this.fd = 0,
    this.allocationSize = 0,
    this.dedicated = false,
    this.format = 0,
    this.depthFd = 0,
    this.depthAllocationSize = 0,
    this.depthDedicated = false,
    this.depthFormat = 0,
  });
  final int width;
  final int height;
  final double scaleFactor;

  /// The texture's index in the ring.
  final int slot;

  /// The file descriptor of the image's memory.
  final int fd;

  /// The size of the memory in bytes.
  final int allocationSize;

  /// Whether the memory is the image's own, which an import says, such as with
  /// GL_DEDICATED_MEMORY_OBJECT_EXT.
  final bool dedicated;

  /// The image's `VkFormat`: VK_FORMAT_R8G8B8A8_UNORM, OpenGL's GL_RGBA8.
  final int format;

  /// The file descriptor of the depth image's memory.
  final int depthFd;

  /// The size of the depth image's memory in bytes.
  final int depthAllocationSize;

  /// Whether the memory is the depth image's own.
  final bool depthDedicated;

  /// The depth image's `VkFormat`.
  final int depthFormat;

  @override
  List<Object?> get _members => [
    width,
    height,
    scaleFactor,
    slot,
    fd,
    allocationSize,
    dedicated,
    format,
    depthFd,
    depthAllocationSize,
    depthDedicated,
    depthFormat,
  ];
  @override
  String toString() => _describe('OpaqueFdFrame', const [
    'width',
    'height',
    'scaleFactor',
    'slot',
    'fd',
    'allocationSize',
    'dedicated',
    'format',
    'depthFd',
    'depthAllocationSize',
    'depthDedicated',
    'depthFormat',
  ]);
}

/// The OpenGL texture of an acquired frame.
///
/// The depth members are set when the frame has a depth texture, as for
/// `MetalTextureFrame`. Otherwise they are zero.
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
    this.depthTexture = 0,
    this.depthInternalFormat = 0,
  });
  final int width;
  final int height;
  final double scaleFactor;

  /// The texture's index in the descriptor's `textures`.
  final int slot;
  final int texture;
  final int target;

  /// The `GL_TEXTURE_2D` texture that holds the frame's depth and stencil.
  final int depthTexture;

  /// The depth texture's internal format, such as GL_DEPTH24_STENCIL8, or zero
  /// for a borrowed ring's.
  final int depthInternalFormat;

  @override
  List<Object?> get _members => [
    width,
    height,
    scaleFactor,
    slot,
    texture,
    target,
    depthTexture,
    depthInternalFormat,
  ];
  @override
  String toString() => _describe('OpenGlTextureFrame', const [
    'width',
    'height',
    'scaleFactor',
    'slot',
    'texture',
    'target',
    'depthTexture',
    'depthInternalFormat',
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

/// A style's projection at one zoom: `from` turning into `to`.
///
/// A zeroed definition is Mercator.
///
/// See `mln_projection_definition` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class ProjectionDefinition extends _Value {
  const ProjectionDefinition({
    this.from = const ProjectionType.fromRawValue(0),
    this.to = const ProjectionType.fromRawValue(0),
    this.transition = 0,
  });
  final ProjectionType from;
  final ProjectionType to;

  /// How far `from` has turned into `to`, from 0 to 1. Ignored when `from`
  /// equals `to`. A value outside that range holds at the nearer end.
  final double transition;

  /// A copy of this value with the given members replaced.
  ProjectionDefinition copyWith({
    ProjectionType? from,
    ProjectionType? to,
    double? transition,
  }) => ProjectionDefinition(
    from: from ?? this.from,
    to: to ?? this.to,
    transition: transition ?? this.transition,
  );

  @override
  List<Object?> get _members => [from, to, transition];
  @override
  String toString() =>
      _describe('ProjectionDefinition', const ['from', 'to', 'transition']);
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
    this.projection = const ProjectionDefinition(),
  });
  final Camera camera;
  final LogicalExtent extent;
  final ProjectionMode projectionMode;
  final MapViewport viewport;

  /// The style's projection at the camera's zoom.
  final ProjectionDefinition projection;

  /// A copy of this value with the given members replaced.
  MapView copyWith({
    Camera? camera,
    LogicalExtent? extent,
    ProjectionMode? projectionMode,
    MapViewport? viewport,
    ProjectionDefinition? projection,
  }) => MapView(
    camera: camera ?? this.camera,
    extent: extent ?? this.extent,
    projectionMode: projectionMode ?? this.projectionMode,
    viewport: viewport ?? this.viewport,
    projection: projection ?? this.projection,
  );

  @override
  List<Object?> get _members => [
    camera,
    extent,
    projectionMode,
    viewport,
    projection,
  ];
  @override
  String toString() => _describe('MapView', const [
    'camera',
    'extent',
    'projectionMode',
    'viewport',
    'projection',
  ]);
}

/// A vector in four dimensions, such as a point in homogeneous coordinates.
///
/// See `mln_vec4` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class Vec4 extends _Value {
  const Vec4({this.x = 0, this.y = 0, this.z = 0, this.w = 0});
  final double x;
  final double y;
  final double z;
  final double w;

  @override
  List<Object?> get _members => [x, y, z, w];
  @override
  String toString() => _describe('Vec4', const ['x', 'y', 'z', 'w']);
}

/// A 4×4 matrix, by its columns: it takes a vector to `x` times the vector's x,
/// plus `y` times its y, `z` times its z, and `w` times its w. In memory it is
/// 16 doubles in column-major order, as OpenGL, Metal, Vulkan, and WebGPU
/// shaders take a matrix.
///
/// See `mln_mat4` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class Mat4 extends _Value {
  const Mat4({
    this.x = const Vec4(),
    this.y = const Vec4(),
    this.z = const Vec4(),
    this.w = const Vec4(),
  });
  final Vec4 x;
  final Vec4 y;
  final Vec4 z;
  final Vec4 w;

  @override
  List<Object?> get _members => [x, y, z, w];
  @override
  String toString() => _describe('Mat4', const ['x', 'y', 'z', 'w']);
}

/// Where a frame drew the map, and the depth that it wrote there.
///
/// World coordinates are Web Mercator's. x runs from 0 at 180° west to 1 at
/// 180° east, and on past both into the copies of the world beside the first. y
/// runs from 0 at the north edge of the Mercator world, about 85.05° north, to
/// 1 at its south edge. z is the altitude in meters above sea level, at the
/// scale of the heights of the map's 3D layers, and the map draws its ground at
/// 0.
///
/// `matrix` takes a world point to the clip coordinates where the frame drew
/// it, with the camera's padding, roll, field of view, and center altitude, and
/// the map's viewport and projection mode. Clip y points up, as the map draws
/// with OpenGL, Metal, and WebGPU. The map negates clip y with Vulkan, so
/// negate it there too. At high zooms, single precision can't tell nearby world
/// points apart: subtract an origin near your geometry from its coordinates in
/// double precision, and fold the origin into the matrix, before you convert to
/// float.
///
/// Clip z follows `clip_depth_range`, the convention of the device's graphics
/// API. With the API's default depth range of 0 to 1, a fragment that you draw
/// through `matrix` gets the depth that the map's 3D layers, such as fill
/// extrusions, wrote at the same place: 0 at `near_distance` from the camera,
/// up to `far_depth` at `far_distance`. Depth-tested against the frame's depth,
/// your fragments hide behind the map's 3D layers where those stand in front of
/// them.
///
/// The frame's depth starts at 1. The map's 3D layers write the depth of each
/// of their fragments. A 2D layer below every 3D layer of the style that draws
/// opaque fragments, such as an opaque fill, writes one depth over all of them,
/// wherever they are: the higher the layer, the lower its depth, and every one
/// between `far_depth` and 1. Backgrounds, translucent fragments, and the 2D
/// layers above a 3D layer, such as labels, write no depth. So what you draw
/// through `matrix`, even on the ground, lies in front of every 2D layer.
///
/// While `globe` is true, the frame drew the map onto a globe, and its 2D
/// layers wrote no depth. `matrix` then is the map's Mercator one, which
/// doesn't place points on the globe.
///
/// See `mln_frame_projection` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class FrameProjection extends _Value {
  const FrameProjection({
    this.matrix = const Mat4(),
    required this.clipDepthRange,
    this.nearDistance = 0,
    this.farDistance = 0,
    this.farDepth = 0,
    this.globe = false,
  });
  final Mat4 matrix;
  final ClipDepthRange clipDepthRange;

  /// The distance from the camera to the plane where the depth that you get
  /// through `matrix` is 0, along the camera's view. Like `far_distance`, it is
  /// in pixels of the map at the camera's zoom, where the world is 512 × 2^zoom
  /// pixels wide.
  final double nearDistance;

  /// The distance to the plane where the depth is `far_depth`.
  final double farDistance;

  /// The depth at `far_distance`, below 1.
  final double farDepth;

  /// Whether the frame drew the map as a globe, alone or blending into
  /// Mercator.
  final bool globe;

  @override
  List<Object?> get _members => [
    matrix,
    clipDepthRange,
    nearDistance,
    farDistance,
    farDepth,
    globe,
  ];
  @override
  String toString() => _describe('FrameProjection', const [
    'matrix',
    'clipDepthRange',
    'nearDistance',
    'farDistance',
    'farDepth',
    'globe',
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
    required this.frameProjection,
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

  /// Where a rendered frame drew the map, for drawing into its frame with
  /// depth. All zero for a demand that rendered nothing.
  final FrameProjection frameProjection;
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
    frameProjection,
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
    'frameProjection',
    'mode',
  ]);
}

/// The Vulkan image of an acquired frame.
///
/// The depth members are set when the frame has a depth image, as for
/// `MetalTextureFrame`. Otherwise they are zero.
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
    this.depthImage = 0,
    this.depthImageView = 0,
    this.depthFormat = 0,
    this.depthLayout = 0,
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

  /// The VkImage that holds the frame's depth and stencil.
  final int depthImage;

  /// A VkImageView of the depth image's depth and stencil aspects.
  final int depthImageView;

  /// The depth image's VkFormat.
  final int depthFormat;

  /// The VkImageLayout that the frame left the depth image in.
  final int depthLayout;

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
    depthImage,
    depthImageView,
    depthFormat,
    depthLayout,
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
    'depthImage',
    'depthImageView',
    'depthFormat',
    'depthLayout',
  ]);
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

/// How one part of a gesture glides after release.
///
/// Speeds and distances are in the part's unit: logical pixels for pan, zoom
/// levels for zoom, and degrees for bearing and pitch, with speeds per second.
/// A setting whose bit isn't set keeps its current value. A new `model` starts
/// from its defaults: friction from the part's, which
/// `MapHandle.gestureRecognizerCreate()` lists, and a curve from Android's
/// fling. Settings of the other model are an error. `min_speed` is finite and
/// not negative, `duration_exponent` finite and above -1, and every other
/// number positive; the maximums may be infinite.
///
/// See `mln_momentum_decay` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
final class MomentumDecay extends _Value {
  const MomentumDecay({
    this.model,
    this.halfLifeMs,
    this.stopSpeed,
    this.referenceSpeed,
    this.durationMs,
    this.durationExponent,
    this.easing,
    this.minSpeed,
    this.maxSpeed,
    this.maxDistance,
  });
  final MomentumDecayModel? model;

  /// For `MomentumDecayModel.friction`.
  final double? halfLifeMs;

  /// For `MomentumDecayModel.friction`, the speed at which it ends.
  final double? stopSpeed;

  /// For `MomentumDecayModel.curve`, the speed that lasts `duration_ms`.
  final double? referenceSpeed;

  /// For `MomentumDecayModel.curve`.
  final double? durationMs;

  /// For `MomentumDecayModel.curve`, how the duration grows with the speed.
  final double? durationExponent;

  /// For `MomentumDecayModel.curve`, the share of the glide covered over the
  /// share of its duration. Its control points are between 0 and 1, and `x1`
  /// and `y1` above 0, so the glide starts moving and never passes its end.
  final UnitBezier? easing;

  /// A slower release doesn't glide.
  final double? minSpeed;

  /// A faster release counts as this.
  final double? maxSpeed;

  /// The farthest the part glides. A release that would glide farther counts as
  /// the speed that glides this far.
  final double? maxDistance;

  /// A copy of this value with the given members replaced.
  MomentumDecay copyWith({
    MomentumDecayModel? model,
    double? halfLifeMs,
    double? stopSpeed,
    double? referenceSpeed,
    double? durationMs,
    double? durationExponent,
    UnitBezier? easing,
    double? minSpeed,
    double? maxSpeed,
    double? maxDistance,
  }) => MomentumDecay(
    model: model ?? this.model,
    halfLifeMs: halfLifeMs ?? this.halfLifeMs,
    stopSpeed: stopSpeed ?? this.stopSpeed,
    referenceSpeed: referenceSpeed ?? this.referenceSpeed,
    durationMs: durationMs ?? this.durationMs,
    durationExponent: durationExponent ?? this.durationExponent,
    easing: easing ?? this.easing,
    minSpeed: minSpeed ?? this.minSpeed,
    maxSpeed: maxSpeed ?? this.maxSpeed,
    maxDistance: maxDistance ?? this.maxDistance,
  );

  @override
  List<Object?> get _members => [
    model,
    halfLifeMs,
    stopSpeed,
    referenceSpeed,
    durationMs,
    durationExponent,
    easing,
    minSpeed,
    maxSpeed,
    maxDistance,
  ];
  @override
  String toString() => _describe('MomentumDecay', const [
    'model',
    'halfLifeMs',
    'stopSpeed',
    'referenceSpeed',
    'durationMs',
    'durationExponent',
    'easing',
    'minSpeed',
    'maxSpeed',
    'maxDistance',
  ]);
}

/// How gestures move the map's content.
///
/// See `mln_gesture_pan_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
final class GesturePanOptions extends _Value {
  const GesturePanOptions({this.enabled, this.glides, this.momentum});

  /// Whether drags, two fingers moving together, and deltas pan.
  final bool? enabled;

  /// Whether a release glides on.
  final bool? glides;
  final MomentumDecay? momentum;

  /// A copy of this value with the given members replaced.
  GesturePanOptions copyWith({
    bool? enabled,
    bool? glides,
    MomentumDecay? momentum,
  }) => GesturePanOptions(
    enabled: enabled ?? this.enabled,
    glides: glides ?? this.glides,
    momentum: momentum ?? this.momentum,
  );

  @override
  List<Object?> get _members => [enabled, glides, momentum];
  @override
  String toString() =>
      _describe('GesturePanOptions', const ['enabled', 'glides', 'momentum']);
}

/// Where a part settles after a gesture that changed it.
///
/// The targets are every `spacing` from `offset`, in the part's unit. Bearing
/// targets lie within one turn from `offset`, and the bearing takes the shorter
/// way around to the nearest. Once the part's glide ends, or right away when it
/// has none, the part eases to the nearest target if it is within `tolerance`
/// of it, and otherwise stays where it is. A setting whose bit isn't set keeps
/// its current value.
///
/// See `mln_momentum_snap` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
final class MomentumSnap extends _Value {
  const MomentumSnap({
    this.spacing,
    this.offset,
    this.tolerance,
    this.durationMs,
  });

  /// Positive.
  final double? spacing;

  /// Finite.
  final double? offset;

  /// Not negative.
  final double? tolerance;

  /// Zero settles without easing.
  final double? durationMs;

  /// A copy of this value with the given members replaced.
  MomentumSnap copyWith({
    double? spacing,
    double? offset,
    double? tolerance,
    double? durationMs,
  }) => MomentumSnap(
    spacing: spacing ?? this.spacing,
    offset: offset ?? this.offset,
    tolerance: tolerance ?? this.tolerance,
    durationMs: durationMs ?? this.durationMs,
  );

  @override
  List<Object?> get _members => [spacing, offset, tolerance, durationMs];
  @override
  String toString() => _describe('MomentumSnap', const [
    'spacing',
    'offset',
    'tolerance',
    'durationMs',
  ]);
}

/// Values every `spacing` from `offset`, in the part's unit, that report an
/// `GestureEventNotch` as a gesture moves the part to or across one, for the
/// host to play as haptic feedback. Bearing notches lie within one turn from
/// the offset.
///
/// See `mln_gesture_notch` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
final class GestureNotch extends _Value {
  const GestureNotch({this.spacing = 0, this.offset = 0, this.kind = 0});

  /// Positive.
  final double spacing;

  /// Finite.
  final double offset;

  /// The host's name for the feedback, which the notch's events carry.
  final int kind;

  /// A copy of this value with the given members replaced.
  GestureNotch copyWith({double? spacing, double? offset, int? kind}) =>
      GestureNotch(
        spacing: spacing ?? this.spacing,
        offset: offset ?? this.offset,
        kind: kind ?? this.kind,
      );

  @override
  List<Object?> get _members => [spacing, offset, kind];
  @override
  String toString() =>
      _describe('GestureNotch', const ['spacing', 'offset', 'kind']);
}

/// How gestures move the zoom, the bearing, or the pitch.
///
/// See `mln_gesture_axis_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
final class GestureAxisOptions extends _Value {
  GestureAxisOptions({
    this.enabled,
    this.glides,
    this.momentum,
    this.snaps,
    this.snap,
    List<GestureNotch>? notches,
  }) : notches = notches == null ? null : List.unmodifiable(notches);

  /// Whether gestures move the part.
  final bool? enabled;

  /// Whether a release glides on.
  final bool? glides;
  final MomentumDecay? momentum;

  /// Whether the part settles once its glide ends.
  final bool? snaps;
  final MomentumSnap? snap;

  /// Where the part reports notches while gestures, their glides, and their
  /// snaps move it, replacing the ones it had. Several notches at one value
  /// report one event, with the kind of the first in the list.
  final List<GestureNotch>? notches;

  /// A copy of this value with the given members replaced.
  GestureAxisOptions copyWith({
    bool? enabled,
    bool? glides,
    MomentumDecay? momentum,
    bool? snaps,
    MomentumSnap? snap,
    List<GestureNotch>? notches,
  }) => GestureAxisOptions(
    enabled: enabled ?? this.enabled,
    glides: glides ?? this.glides,
    momentum: momentum ?? this.momentum,
    snaps: snaps ?? this.snaps,
    snap: snap ?? this.snap,
    notches: notches ?? this.notches,
  );

  @override
  List<Object?> get _members => [
    enabled,
    glides,
    momentum,
    snaps,
    snap,
    notches,
  ];
  @override
  String toString() => _describe('GestureAxisOptions', const [
    'enabled',
    'glides',
    'momentum',
    'snaps',
    'snap',
    'notches',
  ]);
}

/// How a gesture recognizer moves each part of the camera.
///
/// Settings whose bits aren't set keep their values. A new recognizer moves
/// every part, and glides each by friction with a half-life of 140 ms, a
/// minimum speed of 0, and no maximum speed or distance, with these exceptions:
///
/// - Pan stops at 10 logical pixels per second, needs a release of at least
///   200, and counts a release faster than 10000 as 10000.
/// - Zoom stops at 0.01 levels per second and glides at most 2.5 levels. Its
///   snap, when on, settles on the nearest whole level within half a level.
/// - Bearing stops at 1 degree per second and needs a release of at least 60.
///   It snaps to north within 7 degrees, and has a notch of kind 0 at north.
/// - Pitch stops at 1 degree per second and needs a release of at least 5. Its
///   snap, when on, settles flat within 7 degrees.
///
/// Snaps take 300 ms. A double tap zooms in a level and a two-finger tap out.
///
/// See `mln_gesture_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
final class GestureOptions extends _Value {
  GestureOptions({
    this.doubleTapZoom,
    this.twoFingerTapZoom,
    this.pan = const GesturePanOptions(),
    GestureAxisOptions? zoom,
    GestureAxisOptions? bearing,
    GestureAxisOptions? pitch,
  }) : zoom = zoom ?? GestureAxisOptions(),
       bearing = bearing ?? GestureAxisOptions(),
       pitch = pitch ?? GestureAxisOptions();

  /// Zoom levels that a double tap zooms in, eased; with `GestureMode.zoomOut`
  /// it zooms out instead. 0 leaves double taps to the host. Finite.
  final double? doubleTapZoom;

  /// Zoom levels that a two-finger tap zooms, eased; negative zooms out. 0
  /// leaves two-finger taps to the host. Finite.
  final double? twoFingerTapZoom;
  final GesturePanOptions pan;
  final GestureAxisOptions zoom;
  final GestureAxisOptions bearing;
  final GestureAxisOptions pitch;

  /// A copy of this value with the given members replaced.
  GestureOptions copyWith({
    double? doubleTapZoom,
    double? twoFingerTapZoom,
    GesturePanOptions? pan,
    GestureAxisOptions? zoom,
    GestureAxisOptions? bearing,
    GestureAxisOptions? pitch,
  }) => GestureOptions(
    doubleTapZoom: doubleTapZoom ?? this.doubleTapZoom,
    twoFingerTapZoom: twoFingerTapZoom ?? this.twoFingerTapZoom,
    pan: pan ?? this.pan,
    zoom: zoom ?? this.zoom,
    bearing: bearing ?? this.bearing,
    pitch: pitch ?? this.pitch,
  );

  @override
  List<Object?> get _members => [
    doubleTapZoom,
    twoFingerTapZoom,
    pan,
    zoom,
    bearing,
    pitch,
  ];
  @override
  String toString() => _describe('GestureOptions', const [
    'doubleTapZoom',
    'twoFingerTapZoom',
    'pan',
    'zoom',
    'bearing',
    'pitch',
  ]);
}

/// A pointer went down. The host sends no pointer whose buttons mean nothing,
/// such as a mouse's middle button.
///
/// See `mln_gesture_pointer_down` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
final class GesturePointerDown extends _Value {
  const GesturePointerDown({
    this.id = 0,
    this.kind = const PointerKind.fromRawValue(0),
    this.position = const ScreenPoint(),
    this.modes = GestureMode.none,
  });
  final int id;
  final PointerKind kind;
  final ScreenPoint position;

  /// The modes of the pointer's held buttons.
  final GestureMode modes;

  /// A copy of this value with the given members replaced.
  GesturePointerDown copyWith({
    int? id,
    PointerKind? kind,
    ScreenPoint? position,
    GestureMode? modes,
  }) => GesturePointerDown(
    id: id ?? this.id,
    kind: kind ?? this.kind,
    position: position ?? this.position,
    modes: modes ?? this.modes,
  );

  @override
  List<Object?> get _members => [id, kind, position, modes];
  @override
  String toString() => _describe('GesturePointerDown', const [
    'id',
    'kind',
    'position',
    'modes',
  ]);
}

/// A pointer moved, went up, or was taken away.
///
/// See `mln_gesture_pointer` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
final class GesturePointer extends _Value {
  const GesturePointer({this.id = 0, this.position = const ScreenPoint()});
  final int id;

  /// Unused for `GestureInputPointerCancel`.
  final ScreenPoint position;

  /// A copy of this value with the given members replaced.
  GesturePointer copyWith({int? id, ScreenPoint? position}) =>
      GesturePointer(id: id ?? this.id, position: position ?? this.position);

  @override
  List<Object?> get _members => [id, position];
  @override
  String toString() => _describe('GesturePointer', const ['id', 'position']);
}

/// Camera movement from anything but a pointer, such as a wheel, a trackpad
/// scroll, a dial, a crown, keys, or buttons, which the host converts into
/// camera units. Zooms and turns hold `position` in place.
///
/// A continuous delta, such as a trackpad scroll, applies at once. A discrete
/// one, such as a wheel detent or a key press, eases, and a quick run of them
/// merges into one ease that speeds up. Deltas are one gesture until they idle.
/// The recommended amounts are a third of a level per wheel detent, 1/300 of a
/// level per pixel of trackpad scrolling, and, for keys, 100 logical pixels of
/// pan, a level of zoom, 15 degrees of bearing, and 10 of pitch.
///
/// See `mln_gesture_delta` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
final class GestureDelta extends _Value {
  const GestureDelta({
    this.position = const ScreenPoint(),
    this.discrete = false,
    this.pan = const ScreenPoint(),
    this.zoom = 0,
    this.bearing = 0,
    this.pitch = 0,
  });
  final ScreenPoint position;
  final bool discrete;

  /// How far the map's content moves on the screen, in logical pixels.
  final ScreenPoint pan;

  /// Zoom levels to add.
  final double zoom;

  /// Degrees to turn clockwise.
  final double bearing;

  /// Degrees to tilt.
  final double pitch;

  /// A copy of this value with the given members replaced.
  GestureDelta copyWith({
    ScreenPoint? position,
    bool? discrete,
    ScreenPoint? pan,
    double? zoom,
    double? bearing,
    double? pitch,
  }) => GestureDelta(
    position: position ?? this.position,
    discrete: discrete ?? this.discrete,
    pan: pan ?? this.pan,
    zoom: zoom ?? this.zoom,
    bearing: bearing ?? this.bearing,
    pitch: pitch ?? this.pitch,
  );

  @override
  List<Object?> get _members => [position, discrete, pan, zoom, bearing, pitch];
  @override
  String toString() => _describe('GestureDelta', const [
    'position',
    'discrete',
    'pan',
    'zoom',
    'bearing',
    'pitch',
  ]);
}

/// A pinch the platform recognized, such as a macOS trackpad's magnify and
/// rotate or a browser's Ctrl+wheel. Each update zooms by `scale` (above 1
/// zooms in) and turns `rotation` degrees clockwise on the screen since the
/// last, around `center`. It glides on when it ends, as a touch pinch does.
///
/// See `mln_gesture_pinch` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
final class GesturePinch extends _Value {
  const GesturePinch({
    this.center = const ScreenPoint(),
    this.scale = 0,
    this.rotation = 0,
    this.phase = const PinchPhase.fromRawValue(0),
  });
  final ScreenPoint center;
  final double scale;
  final double rotation;
  final PinchPhase phase;

  /// A copy of this value with the given members replaced.
  GesturePinch copyWith({
    ScreenPoint? center,
    double? scale,
    double? rotation,
    PinchPhase? phase,
  }) => GesturePinch(
    center: center ?? this.center,
    scale: scale ?? this.scale,
    rotation: rotation ?? this.rotation,
    phase: phase ?? this.phase,
  );

  @override
  List<Object?> get _members => [center, scale, rotation, phase];
  @override
  String toString() =>
      _describe('GesturePinch', const ['center', 'scale', 'rotation', 'phase']);
}

/// One input, in logical pixels from the map's top-left corner.
///
/// When the view loses focus, the host cancels the pointers it sent and sends
/// empty modes.
///
/// See `mln_gesture_input` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
sealed class GestureInput {
  const GestureInput._();
  const factory GestureInput.pointerDown(GesturePointerDown value) =
      GestureInputPointerDown;
  const factory GestureInput.pointerMove(GesturePointer value) =
      GestureInputPointerMove;
  const factory GestureInput.pointerUp(GesturePointer value) =
      GestureInputPointerUp;
  const factory GestureInput.pointerCancel(GesturePointer value) =
      GestureInputPointerCancel;
  const factory GestureInput.modes(GestureMode value) = GestureInputModes;
  const factory GestureInput.delta(GestureDelta value) = GestureInputDelta;
  const factory GestureInput.pinch(GesturePinch value) = GestureInputPinch;
}

final class GestureInputPointerDown extends GestureInput {
  const GestureInputPointerDown(this.value) : super._();
  final GesturePointerDown value;
  @override
  bool operator ==(Object other) =>
      other is GestureInputPointerDown && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class GestureInputPointerMove extends GestureInput {
  const GestureInputPointerMove(this.value) : super._();
  final GesturePointer value;
  @override
  bool operator ==(Object other) =>
      other is GestureInputPointerMove && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class GestureInputPointerUp extends GestureInput {
  const GestureInputPointerUp(this.value) : super._();
  final GesturePointer value;
  @override
  bool operator ==(Object other) =>
      other is GestureInputPointerUp && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class GestureInputPointerCancel extends GestureInput {
  const GestureInputPointerCancel(this.value) : super._();
  final GesturePointer value;
  @override
  bool operator ==(Object other) =>
      other is GestureInputPointerCancel && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class GestureInputModes extends GestureInput {
  const GestureInputModes(this.value) : super._();
  final GestureMode value;
  @override
  bool operator ==(Object other) =>
      other is GestureInputModes && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class GestureInputDelta extends GestureInput {
  const GestureInputDelta(this.value) : super._();
  final GestureDelta value;
  @override
  bool operator ==(Object other) =>
      other is GestureInputDelta && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class GestureInputPinch extends GestureInput {
  const GestureInputPinch(this.value) : super._();
  final GesturePinch value;
  @override
  bool operator ==(Object other) =>
      other is GestureInputPinch && other.value == value;
  @override
  int get hashCode => value.hashCode;
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

/// A tap, at `position`, by the pointer `id`.
///
/// See `mln_gesture_tap` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
final class GestureTap extends _Value {
  const GestureTap({this.id = 0, this.position = const ScreenPoint()});

  /// Unused for `GestureEventTwoFingerTap`.
  final int id;
  final ScreenPoint position;

  @override
  List<Object?> get _members => [id, position];
  @override
  String toString() => _describe('GestureTap', const ['id', 'position']);
}

/// A notch that a part reached.
///
/// See `mln_gesture_notch_event` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
final class GestureNotchEvent extends _Value {
  const GestureNotchEvent({
    this.axis = const GestureAxis.fromRawValue(0),
    this.kind = 0,
    this.value = 0,
  });
  final GestureAxis axis;

  /// The notch's kind, from the options.
  final int kind;

  /// In the part's unit; a bearing from 0 up to 360 degrees.
  final double value;

  @override
  List<Object?> get _members => [axis, kind, value];
  @override
  String toString() =>
      _describe('GestureNotchEvent', const ['axis', 'kind', 'value']);
}

/// What a gesture means beyond moving the camera. Double taps and two-finger
/// taps also zoom, as the options say.
///
/// See `mln_gesture_event` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
sealed class GestureEvent {
  const GestureEvent._();
  const factory GestureEvent.tap(GestureTap value) = GestureEventTap;
  const factory GestureEvent.doubleTap(GestureTap value) =
      GestureEventDoubleTap;
  const factory GestureEvent.twoFingerTap(GestureTap value) =
      GestureEventTwoFingerTap;
  const factory GestureEvent.notch(GestureNotchEvent value) = GestureEventNotch;
  const factory GestureEvent.longPress(GestureTap value) =
      GestureEventLongPress;
}

final class GestureEventTap extends GestureEvent {
  const GestureEventTap(this.value) : super._();
  final GestureTap value;
  @override
  bool operator ==(Object other) =>
      other is GestureEventTap && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class GestureEventDoubleTap extends GestureEvent {
  const GestureEventDoubleTap(this.value) : super._();
  final GestureTap value;
  @override
  bool operator ==(Object other) =>
      other is GestureEventDoubleTap && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class GestureEventTwoFingerTap extends GestureEvent {
  const GestureEventTwoFingerTap(this.value) : super._();
  final GestureTap value;
  @override
  bool operator ==(Object other) =>
      other is GestureEventTwoFingerTap && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class GestureEventNotch extends GestureEvent {
  const GestureEventNotch(this.value) : super._();
  final GestureNotchEvent value;
  @override
  bool operator ==(Object other) =>
      other is GestureEventNotch && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class GestureEventLongPress extends GestureEvent {
  const GestureEventLongPress(this.value) : super._();
  final GestureTap value;
  @override
  bool operator ==(Object other) =>
      other is GestureEventLongPress && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class GestureEventUnknown extends GestureEvent {
  GestureEventUnknown(this.tag, Uint8List rawRecord)
    : rawRecord = Uint8List.fromList(rawRecord).asUnmodifiableView(),
      super._();
  final int tag;
  final Uint8List rawRecord;
}

/// What an input or a tick did.
///
/// See `mln_gesture_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
final class GestureResult extends _Value {
  GestureResult({
    this.handled = false,
    this.nextTick = const GestureNextTick.fromRawValue(0),
    this.nextTickMs = 0,
    this.boxZoomActive = false,
    this.boxZoom = const ScreenBox(),
    List<GestureEvent> events = const [],
  }) : events = List.unmodifiable(events);

  /// Whether the recognizer used the input, so the host stops the platform's
  /// default for it. Ticks report false.
  final bool handled;
  final GestureNextTick nextTick;

  /// For `GestureNextTick.at`.
  final double nextTickMs;

  /// Whether a box-zoom drag is drawing `box_zoom`, which the host shows, in
  /// logical pixels corner to corner.
  final bool boxZoomActive;
  final ScreenBox boxZoom;
  final List<GestureEvent> events;

  @override
  List<Object?> get _members => [
    handled,
    nextTick,
    nextTickMs,
    boxZoomActive,
    boxZoom,
    events,
  ];
  @override
  String toString() => _describe('GestureResult', const [
    'handled',
    'nextTick',
    'nextTickMs',
    'boxZoomActive',
    'boxZoom',
    'events',
  ]);
}

/// Your lock on a Vulkan queue that you and the render sessions of a GPU device
/// all submit to.
///
/// A session holds the lock only while it submits or presents to the queue, or
/// submits the empty batch that drains its work from the queue. It never holds
/// the lock while it waits. Set both `lock` and `unlock`, or neither when you
/// don't submit to the queue yourself.
///
/// Don't hold the lock while you call the library.
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

/// Options for `GpuDeviceHandle.create()`.
///
/// Name the GPU by at most one of `adapter_index`, `adapter_uuid`, and
/// `adapter_luid`. Without one, the device prefers a discrete GPU. An OpenGL
/// device takes none of them: it renders on the GPU that the platform picks.
///
/// See `mln_gpu_device_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
final class GpuDeviceOptions extends _Value {
  GpuDeviceOptions({
    this.backend,
    this.adapterIndex,
    Uint8List? adapterUuid,
    Uint8List? adapterLuid,
    this.openGlClientApi,
    this.openGlSingleContext,
    this.queueLock,
  }) : adapterUuid = adapterUuid == null
           ? null
           : Uint8List.fromList(adapterUuid).asUnmodifiableView(),
       adapterLuid = adapterLuid == null
           ? null
           : Uint8List.fromList(adapterLuid).asUnmodifiableView();

  /// The backend that the device renders with. By default, the platform's own:
  /// Metal on Apple platforms, Vulkan elsewhere, then D3D12 on Windows, or
  /// OpenGL where the system has neither, and in a browser the build's one
  /// backend: WebGPU or OpenGL (WebGL2).
  final RenderBackend? backend;

  /// The GPU to use, as an index into the backend's list of GPUs. OpenGL's list
  /// holds one GPU: the one that its display renders on.
  final int? adapterIndex;

  /// The 16-byte UUID of the GPU to use: Vulkan's
  /// `VkPhysicalDeviceIDProperties.deviceUUID`. Only a Vulkan device takes one.
  final Uint8List? adapterUuid;

  /// The 8-byte LUID of the GPU to use: on Windows, its DXGI adapter LUID,
  /// which Vulkan reports as `VkPhysicalDeviceIDProperties.deviceLUID` and
  /// D3D12 as `ID3D12Device::GetAdapterLuid`. Only a Vulkan or D3D12 device
  /// takes one.
  final Uint8List? adapterLuid;

  /// The OpenGL API that an OpenGL device renders with. An EGL device defaults
  /// to `OpenGlClientApi.gles`, and takes `OpenGlClientApi.gl` too on Linux and
  /// FreeBSD. A WGL device takes only `OpenGlClientApi.gl`, its default, and a
  /// WebGL device only `OpenGlClientApi.gles`, its default.
  final OpenGlClientApi? openGlClientApi;

  /// Whether the sessions of an EGL device take turns in one OpenGL context
  /// instead of having one each.
  final bool? openGlSingleContext;

  /// Your lock on a Vulkan device's queue, when you submit to the queue that
  /// `GpuDeviceHandle.getVulkanContext()` reports. Only a Vulkan device takes
  /// one.
  final QueueLock? queueLock;

  /// A copy of this value with the given members replaced.
  GpuDeviceOptions copyWith({
    RenderBackend? backend,
    int? adapterIndex,
    Uint8List? adapterUuid,
    Uint8List? adapterLuid,
    OpenGlClientApi? openGlClientApi,
    bool? openGlSingleContext,
    QueueLock? queueLock,
  }) => GpuDeviceOptions(
    backend: backend ?? this.backend,
    adapterIndex: adapterIndex ?? this.adapterIndex,
    adapterUuid: adapterUuid ?? this.adapterUuid,
    adapterLuid: adapterLuid ?? this.adapterLuid,
    openGlClientApi: openGlClientApi ?? this.openGlClientApi,
    openGlSingleContext: openGlSingleContext ?? this.openGlSingleContext,
    queueLock: queueLock ?? this.queueLock,
  );

  @override
  List<Object?> get _members => [
    backend,
    adapterIndex,
    adapterUuid,
    adapterLuid,
    openGlClientApi,
    openGlSingleContext,
    queueLock,
  ];
  @override
  String toString() => _describe('GpuDeviceOptions', const [
    'backend',
    'adapterIndex',
    'adapterUuid',
    'adapterLuid',
    'openGlClientApi',
    'openGlSingleContext',
    'queueLock',
  ]);
}

/// What a GPU device can do.
///
/// See `mln_gpu_device_capabilities` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
final class GpuDeviceCapabilities extends _Value {
  const GpuDeviceCapabilities({
    required this.backend,
    this.targetKinds = RenderTargetKindFlag.none,
    this.exportKinds = GpuExportKindFlag.none,
    this.syncKinds = GpuSyncKindFlag.none,
    this.depthStencilFormats = DepthStencilFormatFlag.none,
    this.depthExportKinds = GpuExportKindFlag.none,
  });
  final RenderBackend backend;

  /// The kinds of `RenderTarget` that sessions on the device render to.
  final RenderTargetKindFlag targetKinds;

  /// The ways that the device can share the textures of an exported ring. A
  /// kind is here only once the device has exported a texture of it, so a
  /// driver that offers an extension but can't allocate the memory reports no
  /// kind of it.
  final GpuExportKindFlag exportKinds;

  /// The kinds of `GpuSync` that the device can signal and wait on.
  final GpuSyncKindFlag syncKinds;

  /// The formats of the depth textures that a borrowed ring takes.
  final DepthStencilFormatFlag depthStencilFormats;

  /// The kinds of `export_kinds` whose rings also share each frame's depth
  /// texture, with `RenderSessionAttachOptions.keepDepth`. An IOSurface or a
  /// dmabuf holds no depth format, so only `GpuExportKind.opaqueFd` can be
  /// here.
  final GpuExportKindFlag depthExportKinds;

  @override
  List<Object?> get _members => [
    backend,
    targetKinds,
    exportKinds,
    syncKinds,
    depthStencilFormats,
    depthExportKinds,
  ];
  @override
  String toString() => _describe('GpuDeviceCapabilities', const [
    'backend',
    'targetKinds',
    'exportKinds',
    'syncKinds',
    'depthStencilFormats',
    'depthExportKinds',
  ]);
}

/// The D3D12 objects of a device, as `GpuDeviceHandle.getD3d12Context()`
/// reports them.
///
/// See `mln_gpu_device_d3d12_context_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
final class GpuDeviceD3d12ContextResult extends _Value {
  const GpuDeviceD3d12ContextResult({
    this.device = NativePointer.nullPointer,
    this.commandQueue = NativePointer.nullPointer,
  });

  /// The `ID3D12Device`.
  final NativePointer device;

  /// The direct `ID3D12CommandQueue` that sessions on the device submit to.
  final NativePointer commandQueue;

  @override
  List<Object?> get _members => [device, commandQueue];
  @override
  String toString() => _describe('GpuDeviceD3d12ContextResult', const [
    'device',
    'commandQueue',
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
  /// you submit to it too, give the device your `QueueLock`.
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

/// The Vulkan objects of a device, as `GpuDeviceHandle.getVulkanContext()`
/// reports them.
///
/// See `mln_gpu_device_vulkan_context_result` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
final class GpuDeviceVulkanContextResult extends _Value {
  const GpuDeviceVulkanContextResult({required this.context});
  final VulkanContextDescriptor context;

  @override
  List<Object?> get _members => [context];
  @override
  String toString() =>
      _describe('GpuDeviceVulkanContextResult', const ['context']);
}

/// Your D3D12 device and direct command queue, for
/// `GpuDeviceHandle.wrapD3d12()`.
///
/// See `mln_d3d12_device_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
final class D3d12DeviceDescriptor extends _Value {
  const D3d12DeviceDescriptor({
    required this.device,
    required this.commandQueue,
  });

  /// Your `ID3D12Device`.
  final NativePointer device;

  /// Your `ID3D12CommandQueue` of `D3D12_COMMAND_LIST_TYPE_DIRECT` on `device`,
  /// which sessions on the device submit to. Submit your reads of a borrowed
  /// ring's textures to it for `GpuSyncKind.queueOrder`.
  final NativePointer commandQueue;

  /// A copy of this value with the given members replaced.
  D3d12DeviceDescriptor copyWith({
    NativePointer? device,
    NativePointer? commandQueue,
  }) => D3d12DeviceDescriptor(
    device: device ?? this.device,
    commandQueue: commandQueue ?? this.commandQueue,
  );

  @override
  List<Object?> get _members => [device, commandQueue];
  @override
  String toString() =>
      _describe('D3d12DeviceDescriptor', const ['device', 'commandQueue']);
}

/// Your Metal device and command queue, for `GpuDeviceHandle.wrapMetal()`.
///
/// See `mln_metal_device_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
final class MetalDeviceDescriptor extends _Value {
  const MetalDeviceDescriptor({
    required this.device,
    this.commandQueue = NativePointer.nullPointer,
  });

  /// Your `MTLDevice`.
  final NativePointer device;

  /// Your `MTLCommandQueue` on `device`, or null for a queue of the library's.
  /// With your queue, borrowed rings can use `GpuSyncKind.queueOrder`.
  final NativePointer commandQueue;

  /// A copy of this value with the given members replaced.
  MetalDeviceDescriptor copyWith({
    NativePointer? device,
    NativePointer? commandQueue,
  }) => MetalDeviceDescriptor(
    device: device ?? this.device,
    commandQueue: commandQueue ?? this.commandQueue,
  );

  @override
  List<Object?> get _members => [device, commandQueue];
  @override
  String toString() =>
      _describe('MetalDeviceDescriptor', const ['device', 'commandQueue']);
}

/// Your WGL context, which each session's context shares objects with.
///
/// See `mln_wgl_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class WglContextDescriptor extends _Value {
  const WglContextDescriptor({
    required this.deviceContext,
    required this.shareContext,
    this.getProcAddress = NativePointer.nullPointer,
  });

  /// An `HDC` with the pixel format of `share_context`.
  final NativePointer deviceContext;

  /// Your `HGLRC`.
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

/// Your EGL context, which each session's context shares objects with.
///
/// Each session makes its context on `display` with `config` and the client API
/// and version of `share_context`.
///
/// See `mln_egl_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class EglContextDescriptor extends _Value {
  const EglContextDescriptor({
    required this.display,
    required this.config,
    required this.shareContext,
    this.getProcAddress = NativePointer.nullPointer,
  });

  /// An `EGLDisplay`, which you initialized.
  final NativePointer display;

  /// The `EGLConfig` of `share_context`.
  final NativePointer config;

  /// Your `EGLContext`.
  final NativePointer shareContext;

  /// Your `eglGetProcAddress`, or null to use the system's.
  final NativePointer getProcAddress;

  /// A copy of this value with the given members replaced.
  EglContextDescriptor copyWith({
    NativePointer? display,
    NativePointer? config,
    NativePointer? shareContext,
    NativePointer? getProcAddress,
  }) => EglContextDescriptor(
    display: display ?? this.display,
    config: config ?? this.config,
    shareContext: shareContext ?? this.shareContext,
    getProcAddress: getProcAddress ?? this.getProcAddress,
  );

  @override
  List<Object?> get _members => [display, config, shareContext, getProcAddress];
  @override
  String toString() => _describe('EglContextDescriptor', const [
    'display',
    'config',
    'shareContext',
    'getProcAddress',
  ]);
}

/// The WebGL2 context that an OpenGL device wraps.
///
/// See `mln_web_gl_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class WebGlContextDescriptor extends _Value {
  const WebGlContextDescriptor({required this.context});

  /// Your `WebGL2RenderingContext`.
  final int context;

  /// A copy of this value with the given members replaced.
  WebGlContextDescriptor copyWith({int? context}) =>
      WebGlContextDescriptor(context: context ?? this.context);

  @override
  List<Object?> get _members => [context];
  @override
  String toString() => _describe('WebGlContextDescriptor', const ['context']);
}

/// Your OpenGL context, for `GpuDeviceHandle.wrapOpenGl()`: the root of the
/// share group that the device's sessions join.
///
/// See `mln_open_gl_context_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
sealed class OpenGlContextDescriptor {
  const OpenGlContextDescriptor._();
  const factory OpenGlContextDescriptor.wgl(WglContextDescriptor value) =
      OpenGlContextDescriptorWgl;
  const factory OpenGlContextDescriptor.egl(EglContextDescriptor value) =
      OpenGlContextDescriptorEgl;
  const factory OpenGlContextDescriptor.webGl(WebGlContextDescriptor value) =
      OpenGlContextDescriptorWebGl;
}

final class OpenGlContextDescriptorWgl extends OpenGlContextDescriptor {
  const OpenGlContextDescriptorWgl(this.value) : super._();
  final WglContextDescriptor value;
  @override
  bool operator ==(Object other) =>
      other is OpenGlContextDescriptorWgl && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class OpenGlContextDescriptorEgl extends OpenGlContextDescriptor {
  const OpenGlContextDescriptorEgl(this.value) : super._();
  final EglContextDescriptor value;
  @override
  bool operator ==(Object other) =>
      other is OpenGlContextDescriptorEgl && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class OpenGlContextDescriptorWebGl extends OpenGlContextDescriptor {
  const OpenGlContextDescriptorWebGl(this.value) : super._();
  final WebGlContextDescriptor value;
  @override
  bool operator ==(Object other) =>
      other is OpenGlContextDescriptorWebGl && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

/// Your Vulkan device and queue, for `GpuDeviceHandle.wrapVulkan()`.
///
/// See `mln_vulkan_device_descriptor` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
final class VulkanDeviceDescriptor extends _Value {
  VulkanDeviceDescriptor({
    required this.context,
    this.queueLock,
    List<String> enabledExtensions = const [],
  }) : enabledExtensions = List.unmodifiable(enabledExtensions);
  final VulkanContextDescriptor context;

  /// Your lock on `context.graphics_queue`, when you submit to it yourself.
  /// Give every device that wraps the queue the same lock.
  final QueueLock? queueLock;

  /// The names of the device extensions that you enabled on `context.device`,
  /// such as "VK_KHR_external_semaphore_fd". Vulkan offers no way to ask a
  /// device which extensions it has, so the device offers the export kinds and
  /// `GpuSyncKind.syncFd` only when the extensions that they need are listed
  /// here, and none of them when the list is empty.
  final List<String> enabledExtensions;

  /// A copy of this value with the given members replaced.
  VulkanDeviceDescriptor copyWith({
    VulkanContextDescriptor? context,
    QueueLock? queueLock,
    List<String>? enabledExtensions,
  }) => VulkanDeviceDescriptor(
    context: context ?? this.context,
    queueLock: queueLock ?? this.queueLock,
    enabledExtensions: enabledExtensions ?? this.enabledExtensions,
  );

  @override
  List<Object?> get _members => [context, queueLock, enabledExtensions];
  @override
  String toString() => _describe('VulkanDeviceDescriptor', const [
    'context',
    'queueLock',
    'enabledExtensions',
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

/// The callbacks of a custom layer, which draws with your own GPU commands in
/// the map's frame.
///
/// A session runs the callbacks only when it can hand you its commands as one
/// of `CustomLayerRenderParametersPass`'s variants, so a session on another
/// backend, such as WebGPU outside a browser, doesn't draw the layer. Nor does
/// a browser WebGPU session yet: the library's WebGPU (wgpu) exposes no command
/// encoder or render pass to hand you. Leave unset the callbacks that you don't
/// need.
///
/// Runs on the thread that runs the session's graphics work: a library thread,
/// or yours with `RenderDriverKind.callerGraphicsThread`. It can't call the
/// library.
///
/// It can't call the library.
///
/// See `mln_custom_layer_host` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
final class CustomLayerHost {
  /// Creates a native registration from [prerender], [render],
  /// [deinitialize], [contextLost], [userData], and [releaseUserData],
  /// which native code in the host supplies. The binding copies them
  /// unchanged.
  ///
  /// The first call that carries the registration consumes it, and a later
  /// call throws an invalid-state error. Native code releases the context
  /// of a call that it accepts, and the binding calls [releaseUserData] for
  /// a call that native code does not accept.
  CustomLayerHost.native({
    required NativePointer prerender,
    required NativePointer render,
    required NativePointer deinitialize,
    required NativePointer contextLost,
    NativePointer userData = NativePointer.nullPointer,
    NativePointer releaseUserData = NativePointer.nullPointer,
  }) {
    nativeRegistrations[this] = NativeRegistration(
      [prerender, render, deinitialize, contextLost],
      userData,
      releaseUserData,
    );
  }
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

/// The end of a camera change.
///
/// See `mln_camera_transition_end` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class CameraTransitionEnd extends _Value {
  const CameraTransitionEnd({
    this.outcome = const CameraTransitionOutcome.fromRawValue(0),
    this.generation = 0,
  });
  final CameraTransitionOutcome outcome;

  /// The generation of the first map snapshot that shows where the change left
  /// the camera, or 0 when the map closed first.
  final int generation;

  @override
  List<Object?> get _members => [outcome, generation];
  @override
  String toString() =>
      _describe('CameraTransitionEnd', const ['outcome', 'generation']);
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
    this.maxDurationMs,
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

  /// The longest a fly animation may take. A fly animation whose duration, set
  /// or derived from `velocity`, is longer moves the camera at once instead,
  /// and so does one too slow for the library to time, without this field.
  /// Other animations ignore it.
  final double? maxDurationMs;
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
    double? maxDurationMs,
    UnitBezier? easing,
    int? transitionId,
    CameraTransitionHandler? endHandler,
  }) => AnimationOptions(
    durationMs: durationMs ?? this.durationMs,
    velocity: velocity ?? this.velocity,
    minZoom: minZoom ?? this.minZoom,
    maxDurationMs: maxDurationMs ?? this.maxDurationMs,
    easing: easing ?? this.easing,
    transitionId: transitionId ?? this.transitionId,
    endHandler: endHandler ?? this.endHandler,
  );

  @override
  List<Object?> get _members => [
    durationMs,
    velocity,
    minZoom,
    maxDurationMs,
    easing,
    transitionId,
    endHandler,
  ];
  @override
  String toString() => _describe('AnimationOptions', const [
    'durationMs',
    'velocity',
    'minZoom',
    'maxDurationMs',
    'easing',
    'transitionId',
    'endHandler',
  ]);
}

/// A change to the camera from where it is now.
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
  final AnimationOptions animation;

  /// A copy of this value with the given members replaced.
  CameraDelta copyWith({
    ScreenPoint? offset,
    double? scale,
    double? bearing,
    double? pitch,
    ScreenPoint? anchor,
    AnimationOptions? animation,
  }) => CameraDelta(
    offset: offset ?? this.offset,
    scale: scale ?? this.scale,
    bearing: bearing ?? this.bearing,
    pitch: pitch ?? this.pitch,
    anchor: anchor ?? this.anchor,
    animation: animation ?? this.animation,
  );

  @override
  List<Object?> get _members => [
    offset,
    scale,
    bearing,
    pitch,
    anchor,
    animation,
  ];
  @override
  String toString() => _describe('CameraDelta', const [
    'offset',
    'scale',
    'bearing',
    'pitch',
    'anchor',
    'animation',
  ]);
}

/// An image that the session allocates, renders at the map's scale factor, and
/// reads back with `RenderSessionHandle.readTexture()`.
///
/// See `mln_offscreen_target` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class OffscreenTarget extends _Value {
  const OffscreenTarget({required this.logicalSize});
  final LogicalSize logicalSize;

  /// A copy of this value with the given members replaced.
  OffscreenTarget copyWith({LogicalSize? logicalSize}) =>
      OffscreenTarget(logicalSize: logicalSize ?? this.logicalSize);

  @override
  List<Object?> get _members => [logicalSize];
  @override
  String toString() => _describe('OffscreenTarget', const ['logicalSize']);
}

/// A `CAMetalLayer` to present to.
///
/// The session sets the layer's device, pixel format, and drawable size.
///
/// See `mln_metal_layer_target` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class MetalLayerTarget extends _Value {
  const MetalLayerTarget({required this.logicalSize, required this.layer});
  final LogicalSize logicalSize;

  /// Your `CAMetalLayer*`.
  final NativePointer layer;

  /// A copy of this value with the given members replaced.
  MetalLayerTarget copyWith({LogicalSize? logicalSize, NativePointer? layer}) =>
      MetalLayerTarget(
        logicalSize: logicalSize ?? this.logicalSize,
        layer: layer ?? this.layer,
      );

  @override
  List<Object?> get _members => [logicalSize, layer];
  @override
  String toString() =>
      _describe('MetalLayerTarget', const ['logicalSize', 'layer']);
}

/// A Win32 window to present to.
///
/// On an OpenGL device, the session sets the window's pixel format, so don't
/// set one yourself. While the window is minimized, frame demands finish with
/// `RenderResult.targetNotReady`.
///
/// See `mln_win32_window_target` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class Win32WindowTarget extends _Value {
  const Win32WindowTarget({required this.logicalSize, required this.window});
  final LogicalSize logicalSize;

  /// Your `HWND`.
  final NativePointer window;

  /// A copy of this value with the given members replaced.
  Win32WindowTarget copyWith({
    LogicalSize? logicalSize,
    NativePointer? window,
  }) => Win32WindowTarget(
    logicalSize: logicalSize ?? this.logicalSize,
    window: window ?? this.window,
  );

  @override
  List<Object?> get _members => [logicalSize, window];
  @override
  String toString() =>
      _describe('Win32WindowTarget', const ['logicalSize', 'window']);
}

/// An X11 window to present to, through Xlib.
///
/// Keep the display connected until the session's detach completes.
///
/// See `mln_xlib_window_target` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class XlibWindowTarget extends _Value {
  const XlibWindowTarget({
    required this.logicalSize,
    required this.display,
    required this.window,
  });
  final LogicalSize logicalSize;

  /// Your `Display*`.
  final NativePointer display;

  /// Your `Window`, on `display`.
  final int window;

  /// A copy of this value with the given members replaced.
  XlibWindowTarget copyWith({
    LogicalSize? logicalSize,
    NativePointer? display,
    int? window,
  }) => XlibWindowTarget(
    logicalSize: logicalSize ?? this.logicalSize,
    display: display ?? this.display,
    window: window ?? this.window,
  );

  @override
  List<Object?> get _members => [logicalSize, display, window];
  @override
  String toString() =>
      _describe('XlibWindowTarget', const ['logicalSize', 'display', 'window']);
}

/// A Wayland surface to present to.
///
/// Keep the display connected until the session's detach completes. On an
/// OpenGL device, if you also use EGL on the display, initialize it before you
/// attach.
///
/// See `mln_wayland_surface_target` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class WaylandSurfaceTarget extends _Value {
  const WaylandSurfaceTarget({
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
  WaylandSurfaceTarget copyWith({
    LogicalSize? logicalSize,
    NativePointer? display,
    NativePointer? surface,
  }) => WaylandSurfaceTarget(
    logicalSize: logicalSize ?? this.logicalSize,
    display: display ?? this.display,
    surface: surface ?? this.surface,
  );

  @override
  List<Object?> get _members => [logicalSize, display, surface];
  @override
  String toString() => _describe('WaylandSurfaceTarget', const [
    'logicalSize',
    'display',
    'surface',
  ]);
}

/// An Android window to present to.
///
/// The session takes its own reference to the window, so you can release yours
/// once the call returns. When the platform destroys the window, such as in
/// `SurfaceHolder.Callback.surfaceDestroyed`, release the target with
/// `RenderSessionHandle.releaseTarget()`, or give the session another window
/// with `RenderSessionHandle.setTarget()`, without waiting for that to
/// complete. A frame to the destroyed window finishes with
/// `RenderResult.targetNotReady`, or with `RenderResult.rendered` if the window
/// went away while the frame was presenting.
///
/// See `mln_android_window_target` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class AndroidWindowTarget extends _Value {
  const AndroidWindowTarget({required this.logicalSize, required this.window});
  final LogicalSize logicalSize;

  /// Your `ANativeWindow*`.
  final NativePointer window;

  /// A copy of this value with the given members replaced.
  AndroidWindowTarget copyWith({
    LogicalSize? logicalSize,
    NativePointer? window,
  }) => AndroidWindowTarget(
    logicalSize: logicalSize ?? this.logicalSize,
    window: window ?? this.window,
  );

  @override
  List<Object?> get _members => [logicalSize, window];
  @override
  String toString() =>
      _describe('AndroidWindowTarget', const ['logicalSize', 'window']);
}

/// An OpenHarmony window to present to.
///
/// The session takes its own reference to the window, so you can release yours
/// once the call returns. When the platform destroys the window, such as in an
/// `XComponent`'s `onSurfaceDestroyed` callback, release the target or give the
/// session another window without waiting, as for an Android window.
///
/// See `mln_ohos_window_target` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class OhosWindowTarget extends _Value {
  const OhosWindowTarget({required this.logicalSize, required this.window});
  final LogicalSize logicalSize;

  /// Your `OHNativeWindow*`.
  final NativePointer window;

  /// A copy of this value with the given members replaced.
  OhosWindowTarget copyWith({
    LogicalSize? logicalSize,
    NativePointer? window,
  }) => OhosWindowTarget(
    logicalSize: logicalSize ?? this.logicalSize,
    window: window ?? this.window,
  );

  @override
  List<Object?> get _members => [logicalSize, window];
  @override
  String toString() =>
      _describe('OhosWindowTarget', const ['logicalSize', 'window']);
}

/// Your Vulkan surface to present to.
///
/// Make it with the instance of the Vulkan device that you attach on. The
/// device's queue family must support presenting to the surface. The session
/// creates the swapchain, and destroys it but not the surface.
///
/// See `mln_vulkan_surface_target` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class VulkanSurfaceTarget extends _Value {
  const VulkanSurfaceTarget({required this.logicalSize, required this.surface});
  final LogicalSize logicalSize;

  /// Your `VkSurfaceKHR`.
  final int surface;

  /// A copy of this value with the given members replaced.
  VulkanSurfaceTarget copyWith({LogicalSize? logicalSize, int? surface}) =>
      VulkanSurfaceTarget(
        logicalSize: logicalSize ?? this.logicalSize,
        surface: surface ?? this.surface,
      );

  @override
  List<Object?> get _members => [logicalSize, surface];
  @override
  String toString() =>
      _describe('VulkanSurfaceTarget', const ['logicalSize', 'surface']);
}

/// Your OpenGL surface to present to, with the context that
/// `GpuDeviceHandle.wrapOpenGl()` wrapped.
///
/// The session makes its context current on the surface for each frame, and
/// swaps the surface's buffers to present.
///
/// See `mln_open_gl_surface_target` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class OpenGlSurfaceTarget extends _Value {
  const OpenGlSurfaceTarget({required this.logicalSize, required this.surface});
  final LogicalSize logicalSize;

  /// Your `EGLSurface` on the context's display, with its config, or your `HDC`
  /// with the context's pixel format.
  final NativePointer surface;

  /// A copy of this value with the given members replaced.
  OpenGlSurfaceTarget copyWith({
    LogicalSize? logicalSize,
    NativePointer? surface,
  }) => OpenGlSurfaceTarget(
    logicalSize: logicalSize ?? this.logicalSize,
    surface: surface ?? this.surface,
  );

  @override
  List<Object?> get _members => [logicalSize, surface];
  @override
  String toString() =>
      _describe('OpenGlSurfaceTarget', const ['logicalSize', 'surface']);
}

/// A Metal texture of a borrowed ring.
///
/// See `mln_metal_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class MetalBorrowedTexture extends _Value {
  const MetalBorrowedTexture({required this.texture});

  /// Your `MTLTexture`.
  final NativePointer texture;

  /// A copy of this value with the given members replaced.
  MetalBorrowedTexture copyWith({NativePointer? texture}) =>
      MetalBorrowedTexture(texture: texture ?? this.texture);

  @override
  List<Object?> get _members => [texture];
  @override
  String toString() => _describe('MetalBorrowedTexture', const ['texture']);
}

/// A ring of your Metal textures to render into, on the Metal device that you
/// attach on.
///
/// `physical_width` and `physical_height` give the textures' size in pixels,
/// which is usually `logical_size` times the map's scale factor. The textures
/// must all have that size, the device's `MTLDevice`, one pixel format, one
/// sample, and `MTLTextureUsageRenderTarget`. The ring holds up to
/// `textureRingMaxDepth` distinct textures.
///
/// Give the ring `depth_textures` to draw into each frame depth-tested against
/// the map, as `FrameProjection` says: one for each of `textures`, at the same
/// index, or none. Each frame clears the one at its slot, renders the map's
/// depth and stencil into it, and stores them. They must have the size of
/// `textures`, one pixel format of `GpuDeviceCapabilities.depthStencilFormats`,
/// one sample, `MTLTextureUsageRenderTarget`, and a storage mode other than
/// `MTLStorageModeMemoryless`.
///
/// See `mln_metal_borrowed_textures_target` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class MetalBorrowedTexturesTarget extends _Value {
  MetalBorrowedTexturesTarget({
    required this.logicalSize,
    required this.physicalWidth,
    required this.physicalHeight,
    required List<MetalBorrowedTexture> textures,
    List<MetalBorrowedTexture> depthTextures = const [],
  }) : textures = List.unmodifiable(textures),
       depthTextures = List.unmodifiable(depthTextures);
  final LogicalSize logicalSize;
  final int physicalWidth;
  final int physicalHeight;
  final List<MetalBorrowedTexture> textures;
  final List<MetalBorrowedTexture> depthTextures;

  /// A copy of this value with the given members replaced.
  MetalBorrowedTexturesTarget copyWith({
    LogicalSize? logicalSize,
    int? physicalWidth,
    int? physicalHeight,
    List<MetalBorrowedTexture>? textures,
    List<MetalBorrowedTexture>? depthTextures,
  }) => MetalBorrowedTexturesTarget(
    logicalSize: logicalSize ?? this.logicalSize,
    physicalWidth: physicalWidth ?? this.physicalWidth,
    physicalHeight: physicalHeight ?? this.physicalHeight,
    textures: textures ?? this.textures,
    depthTextures: depthTextures ?? this.depthTextures,
  );

  @override
  List<Object?> get _members => [
    logicalSize,
    physicalWidth,
    physicalHeight,
    textures,
    depthTextures,
  ];
  @override
  String toString() => _describe('MetalBorrowedTexturesTarget', const [
    'logicalSize',
    'physicalWidth',
    'physicalHeight',
    'textures',
    'depthTextures',
  ]);
}

/// A Vulkan image of a borrowed ring.
///
/// See `mln_vulkan_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class VulkanBorrowedTexture extends _Value {
  const VulkanBorrowedTexture({required this.image, required this.imageView});

  /// Your `VkImage`.
  final int image;

  /// A 2D `VkImageView` of `image` with one mip level and one layer.
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

/// A ring of your Vulkan images to render into, on the Vulkan device that you
/// attach on.
///
/// `physical_width` and `physical_height` give the images' size in pixels,
/// which is usually `logical_size` times the map's scale factor. Each image
/// must have that size, `format`, and `VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT`,
/// and belong to the device's `VkDevice`. The ring holds up to
/// `textureRingMaxDepth` distinct images.
///
/// Give the ring `depth_textures` to draw into each frame depth-tested against
/// the map, as `FrameProjection` says: one for each of `textures`, at the same
/// index, or none. Each frame clears the one at its slot, renders the map's
/// depth and stencil into it, stores them, and leaves it in
/// `depth_final_layout`, with `owner` as for the color image. They must have
/// the size of `textures`, `depth_format`, one sample, and
/// `VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT`, and their views both the
/// depth and the stencil aspect.
///
/// See `mln_vulkan_borrowed_textures_target` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class VulkanBorrowedTexturesTarget extends _Value {
  VulkanBorrowedTexturesTarget({
    required this.logicalSize,
    required this.physicalWidth,
    required this.physicalHeight,
    required List<VulkanBorrowedTexture> textures,
    required this.format,
    required this.finalLayout,
    this.owner = const VulkanImageOwner.fromRawValue(0),
    List<VulkanBorrowedTexture> depthTextures = const [],
    this.depthFormat = 0,
    this.depthFinalLayout = 0,
  }) : textures = List.unmodifiable(textures),
       depthTextures = List.unmodifiable(depthTextures);
  final LogicalSize logicalSize;
  final int physicalWidth;
  final int physicalHeight;
  final List<VulkanBorrowedTexture> textures;

  /// The images' `VkFormat`.
  final int format;

  /// The `VkImageLayout` that each frame leaves its image in.
  final int finalLayout;
  final VulkanImageOwner owner;
  final List<VulkanBorrowedTexture> depthTextures;

  /// The depth images' `VkFormat`, of
  /// `GpuDeviceCapabilities.depthStencilFormats`.
  final int depthFormat;

  /// The `VkImageLayout` that each frame leaves its depth image in.
  final int depthFinalLayout;

  /// A copy of this value with the given members replaced.
  VulkanBorrowedTexturesTarget copyWith({
    LogicalSize? logicalSize,
    int? physicalWidth,
    int? physicalHeight,
    List<VulkanBorrowedTexture>? textures,
    int? format,
    int? finalLayout,
    VulkanImageOwner? owner,
    List<VulkanBorrowedTexture>? depthTextures,
    int? depthFormat,
    int? depthFinalLayout,
  }) => VulkanBorrowedTexturesTarget(
    logicalSize: logicalSize ?? this.logicalSize,
    physicalWidth: physicalWidth ?? this.physicalWidth,
    physicalHeight: physicalHeight ?? this.physicalHeight,
    textures: textures ?? this.textures,
    format: format ?? this.format,
    finalLayout: finalLayout ?? this.finalLayout,
    owner: owner ?? this.owner,
    depthTextures: depthTextures ?? this.depthTextures,
    depthFormat: depthFormat ?? this.depthFormat,
    depthFinalLayout: depthFinalLayout ?? this.depthFinalLayout,
  );

  @override
  List<Object?> get _members => [
    logicalSize,
    physicalWidth,
    physicalHeight,
    textures,
    format,
    finalLayout,
    owner,
    depthTextures,
    depthFormat,
    depthFinalLayout,
  ];
  @override
  String toString() => _describe('VulkanBorrowedTexturesTarget', const [
    'logicalSize',
    'physicalWidth',
    'physicalHeight',
    'textures',
    'format',
    'finalLayout',
    'owner',
    'depthTextures',
    'depthFormat',
    'depthFinalLayout',
  ]);
}

/// An OpenGL texture of a borrowed ring.
///
/// See `mln_open_gl_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
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

/// A ring of your OpenGL textures to render into, on an OpenGL device that
/// `GpuDeviceHandle.wrapOpenGl()` made from your context.
///
/// `physical_width` and `physical_height` give the textures' size in pixels,
/// which is usually `logical_size` times the map's scale factor. Each texture
/// must be in your context's share group, with RGBA8 storage of that size. The
/// ring holds up to `textureRingMaxDepth` distinct textures.
///
/// Give the ring `depth_textures` to draw into each frame depth-tested against
/// the map, as `FrameProjection` says: one for each of `textures`, at the same
/// index, or none. Each frame clears the one at its slot, and renders the map's
/// depth and stencil into it. They must be `GL_TEXTURE_2D` textures in the
/// share group, with storage of the size of `textures` in a format of
/// `GpuDeviceCapabilities.depthStencilFormats`.
///
/// See `mln_open_gl_borrowed_textures_target` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class OpenGlBorrowedTexturesTarget extends _Value {
  OpenGlBorrowedTexturesTarget({
    required this.logicalSize,
    required this.physicalWidth,
    required this.physicalHeight,
    required List<OpenGlBorrowedTexture> textures,
    required this.target,
    List<OpenGlBorrowedTexture> depthTextures = const [],
  }) : textures = List.unmodifiable(textures),
       depthTextures = List.unmodifiable(depthTextures);
  final LogicalSize logicalSize;
  final int physicalWidth;
  final int physicalHeight;
  final List<OpenGlBorrowedTexture> textures;

  /// The textures' target. It must be `GL_TEXTURE_2D`.
  final int target;
  final List<OpenGlBorrowedTexture> depthTextures;

  /// A copy of this value with the given members replaced.
  OpenGlBorrowedTexturesTarget copyWith({
    LogicalSize? logicalSize,
    int? physicalWidth,
    int? physicalHeight,
    List<OpenGlBorrowedTexture>? textures,
    int? target,
    List<OpenGlBorrowedTexture>? depthTextures,
  }) => OpenGlBorrowedTexturesTarget(
    logicalSize: logicalSize ?? this.logicalSize,
    physicalWidth: physicalWidth ?? this.physicalWidth,
    physicalHeight: physicalHeight ?? this.physicalHeight,
    textures: textures ?? this.textures,
    target: target ?? this.target,
    depthTextures: depthTextures ?? this.depthTextures,
  );

  @override
  List<Object?> get _members => [
    logicalSize,
    physicalWidth,
    physicalHeight,
    textures,
    target,
    depthTextures,
  ];
  @override
  String toString() => _describe('OpenGlBorrowedTexturesTarget', const [
    'logicalSize',
    'physicalWidth',
    'physicalHeight',
    'textures',
    'target',
    'depthTextures',
  ]);
}

/// A WebGPU texture of a borrowed ring.
///
/// See `mln_web_gpu_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class WebGpuBorrowedTexture extends _Value {
  const WebGpuBorrowedTexture({required this.texture});

  /// Your `GPUTexture`.
  final int texture;

  /// A copy of this value with the given members replaced.
  WebGpuBorrowedTexture copyWith({int? texture}) =>
      WebGpuBorrowedTexture(texture: texture ?? this.texture);

  @override
  List<Object?> get _members => [texture];
  @override
  String toString() => _describe('WebGpuBorrowedTexture', const ['texture']);
}

/// A ring of your WebGPU textures to render into, on a WebGPU device that wraps
/// yours.
///
/// `physical_width` and `physical_height` give the textures' size in pixels,
/// which is usually `logical_size` times the map's scale factor. Each texture
/// must be 2D with that size, `format`, one layer, one mip level, one sample,
/// and `GPUTextureUsage.RENDER_ATTACHMENT`. The ring holds up to
/// `textureRingMaxDepth` distinct textures.
///
/// Give the ring `depth_textures` to draw into each frame depth-tested against
/// the map, as `FrameProjection` says: one for each of `textures`, at the same
/// index, or none. Each frame clears the one at its slot, renders the map's
/// depth and stencil into it, and stores them. They must be 2D with the size of
/// `textures`, `depth_format`, one layer, one mip level, one sample, and
/// `GPUTextureUsage.RENDER_ATTACHMENT`.
///
/// See `mln_web_gpu_borrowed_textures_target` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class WebGpuBorrowedTexturesTarget extends _Value {
  WebGpuBorrowedTexturesTarget({
    required this.logicalSize,
    required this.physicalWidth,
    required this.physicalHeight,
    required List<WebGpuBorrowedTexture> textures,
    required this.format,
    List<WebGpuBorrowedTexture> depthTextures = const [],
    this.depthFormat = '',
  }) : textures = List.unmodifiable(textures),
       depthTextures = List.unmodifiable(depthTextures);
  final LogicalSize logicalSize;
  final int physicalWidth;
  final int physicalHeight;
  final List<WebGpuBorrowedTexture> textures;

  /// The textures' `GPUTextureFormat`, such as `rgba8unorm`.
  final String format;
  final List<WebGpuBorrowedTexture> depthTextures;

  /// The depth textures' `GPUTextureFormat`, of
  /// `GpuDeviceCapabilities.depthStencilFormats`.
  final String depthFormat;

  /// A copy of this value with the given members replaced.
  WebGpuBorrowedTexturesTarget copyWith({
    LogicalSize? logicalSize,
    int? physicalWidth,
    int? physicalHeight,
    List<WebGpuBorrowedTexture>? textures,
    String? format,
    List<WebGpuBorrowedTexture>? depthTextures,
    String? depthFormat,
  }) => WebGpuBorrowedTexturesTarget(
    logicalSize: logicalSize ?? this.logicalSize,
    physicalWidth: physicalWidth ?? this.physicalWidth,
    physicalHeight: physicalHeight ?? this.physicalHeight,
    textures: textures ?? this.textures,
    format: format ?? this.format,
    depthTextures: depthTextures ?? this.depthTextures,
    depthFormat: depthFormat ?? this.depthFormat,
  );

  @override
  List<Object?> get _members => [
    logicalSize,
    physicalWidth,
    physicalHeight,
    textures,
    format,
    depthTextures,
    depthFormat,
  ];
  @override
  String toString() => _describe('WebGpuBorrowedTexturesTarget', const [
    'logicalSize',
    'physicalWidth',
    'physicalHeight',
    'textures',
    'format',
    'depthTextures',
    'depthFormat',
  ]);
}

/// A DRM format and modifier that you can import.
///
/// See `mln_dma_buf_format` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
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

/// A ring of textures that the session allocates on its device and shares with
/// you, in one of the ways that you can import.
///
/// Each texture has the size in pixels of `logical_size` at the map's scale
/// factor. `RenderSessionHandle.resize()` gives the ring textures of the new
/// size, and a frame that you hold keeps its texture of the old size until you
/// release or discard it.
///
/// The ring shares its textures in the first kind of `export_kinds` that the
/// device exports, in this order, and `RenderSessionCapabilities.exportKind`
/// reports it:
///
/// - `GpuExportKind.ioSurface`: IOSurfaces of the pixel format
///   kCVPixelFormatType_32BGRA, which Metal names MTLPixelFormatBGRA8Unorm. Get
///   each frame's surface with `AcquiredFrameHandle.withIoSurface()`.
/// - `GpuExportKind.androidHardwareBuffer`: hardware buffers of the format
///   AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM, with the usage
///   AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT,
///   AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE, and the bits of
///   `android_hardware_buffer_usage`. Get each frame's buffer with
///   `AcquiredFrameHandle.withAndroidHardwareBuffer()`.
/// - `GpuExportKind.dxgiSharedHandle`: textures of the format
///   DXGI_FORMAT_B8G8R8A8_UNORM on the device's GPU, which bind as shader
///   resources and render targets: D3D11 textures on a Vulkan device, and
///   committed D3D12 resources in a shared heap on a D3D12 device, which each
///   frame leaves in D3D12_RESOURCE_STATE_COMMON. Open them on a D3D11 or D3D12
///   device of the same adapter, which you pick with
///   `GpuDeviceOptions.adapterLuid`. Get each frame's texture with
///   `AcquiredFrameHandle.withDxgiSharedTexture()`, and on a D3D12 device its
///   resource with `AcquiredFrameHandle.withD3d12Texture()`.
/// - `GpuExportKind.dmaBuf`: dmabufs of a format and modifier from
///   `dma_buf_formats`. Get each frame's buffer with
///   `AcquiredFrameHandle.withDmaBuf()`.
/// - `GpuExportKind.opaqueFd`: the memory of Vulkan images, which only a Vulkan
///   device or an OpenGL context with GL_EXT_memory_object_fd on the same GPU
///   and driver imports. To pick the GPU, create the device with
///   `GpuDeviceOptions.adapterUuid` set to your context's GL_DEVICE_UUID_EXT.
///   Get each frame's memory with `AcquiredFrameHandle.withOpaqueFd()`.
///
/// `GpuDeviceCapabilities.exportKinds` reports the kinds that a device exports.
/// Attach with `RenderDriverKind.coreWorker`. Without a device, the session
/// renders on the first of the platform's backends whose device exports one of
/// `export_kinds`.
///
/// Each frame hands over with `GpuSyncKind.cpuComplete` or with the sync kind
/// that crosses APIs on its platform: `GpuSyncKind.metalSharedEvent` for an
/// IOSurface, `GpuSyncKind.d3d12Fence` for a DXGI shared texture, and
/// `GpuSyncKind.syncFd` for the others, where the device has them. For a reader
/// that waits on neither, such as a Wayland compositor without explicit
/// synchronization or Flutter's `copyPixelBuffer`, leave them out of
/// `RenderSessionAttachOptions.producerSyncKinds`, so that each frame finishes
/// before you acquire it, and release each frame only once the reader is done
/// with its texture. Give the ring 3 textures, so the session can render while
/// you hold two frames.
///
/// See `mln_exported_ring_target` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class ExportedRingTarget extends _Value {
  ExportedRingTarget({
    required this.logicalSize,
    required this.textureCount,
    required this.exportKinds,
    List<DmaBufFormat> dmaBufFormats = const [],
    this.androidHardwareBufferUsage = 0,
  }) : dmaBufFormats = List.unmodifiable(dmaBufFormats);
  final LogicalSize logicalSize;

  /// How many textures the ring has, from 1 to `textureRingMaxDepth`.
  final int textureCount;

  /// The ways of sharing a texture that you can import.
  final GpuExportKindFlag exportKinds;

  /// The dmabuf formats and modifiers that you import, most preferred first,
  /// such as those that eglQueryDmaBufModifiersEXT() reports. A ring exports
  /// dmabufs only of one of them. The library renders only DRM_FORMAT_ABGR8888
  /// and DRM_FORMAT_ARGB8888. A modifier can give a buffer more planes than its
  /// format has, such as a plane of compression metadata, and the library takes
  /// any such modifier that its GPU can render into and share. The library
  /// takes no implicit modifier: it skips DRM_FORMAT_MOD_INVALID, so a list of
  /// only that one gets no dmabuf.
  final List<DmaBufFormat> dmaBufFormats;

  /// More `AHARDWAREBUFFER_USAGE_*` bits that your reads of a hardware buffer
  /// need, such as AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, or zero.
  final int androidHardwareBufferUsage;

  /// A copy of this value with the given members replaced.
  ExportedRingTarget copyWith({
    LogicalSize? logicalSize,
    int? textureCount,
    GpuExportKindFlag? exportKinds,
    List<DmaBufFormat>? dmaBufFormats,
    int? androidHardwareBufferUsage,
  }) => ExportedRingTarget(
    logicalSize: logicalSize ?? this.logicalSize,
    textureCount: textureCount ?? this.textureCount,
    exportKinds: exportKinds ?? this.exportKinds,
    dmaBufFormats: dmaBufFormats ?? this.dmaBufFormats,
    androidHardwareBufferUsage:
        androidHardwareBufferUsage ?? this.androidHardwareBufferUsage,
  );

  @override
  List<Object?> get _members => [
    logicalSize,
    textureCount,
    exportKinds,
    dmaBufFormats,
    androidHardwareBufferUsage,
  ];
  @override
  String toString() => _describe('ExportedRingTarget', const [
    'logicalSize',
    'textureCount',
    'exportKinds',
    'dmaBufFormats',
    'androidHardwareBufferUsage',
  ]);
}

/// A canvas to present to.
///
/// The session makes the canvas's "webgpu" or "webgl2" context with the
/// device's backend, and sets the canvas's size to `logical_size` at the map's
/// scale factor. Attached without a device, it renders with the build's
/// backend: a WebGPU build asks the browser for a device, and attaches once it
/// has one. For an OpenGL device that wraps your WebGL2 context, the canvas
/// must be the context's own.
///
/// See `mln_web_canvas_target` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class WebCanvasTarget extends _Value {
  const WebCanvasTarget({
    required this.logicalSize,
    required this.canvas,
    this.format = '',
  });
  final LogicalSize logicalSize;

  /// An `OffscreenCanvas`, such as one from
  /// `HTMLCanvasElement.transferControlToOffscreen()` that you posted to the
  /// library's Web Worker, or an `HTMLCanvasElement` when the library runs on
  /// the page's thread.
  final int canvas;

  /// For a WebGPU device, the `GPUTextureFormat` to configure the canvas with,
  /// such as `bgra8unorm`. Empty for a format that the device's adapter
  /// prefers.
  final String format;

  /// A copy of this value with the given members replaced.
  WebCanvasTarget copyWith({
    LogicalSize? logicalSize,
    int? canvas,
    String? format,
  }) => WebCanvasTarget(
    logicalSize: logicalSize ?? this.logicalSize,
    canvas: canvas ?? this.canvas,
    format: format ?? this.format,
  );

  @override
  List<Object?> get _members => [logicalSize, canvas, format];
  @override
  String toString() =>
      _describe('WebCanvasTarget', const ['logicalSize', 'canvas', 'format']);
}

/// A D3D12 texture of a borrowed ring.
///
/// See `mln_d3d12_borrowed_texture` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class D3d12BorrowedTexture extends _Value {
  const D3d12BorrowedTexture({required this.resource});

  /// Your `ID3D12Resource`.
  final NativePointer resource;

  /// A copy of this value with the given members replaced.
  D3d12BorrowedTexture copyWith({NativePointer? resource}) =>
      D3d12BorrowedTexture(resource: resource ?? this.resource);

  @override
  List<Object?> get _members => [resource];
  @override
  String toString() => _describe('D3d12BorrowedTexture', const ['resource']);
}

/// A ring of your D3D12 textures to render into, on the D3D12 device that you
/// attach on.
///
/// `physical_width` and `physical_height` give the textures' size in pixels,
/// which is usually `logical_size` times the map's scale factor. Each texture
/// must be a 2D resource of the device with that size, one mip level, one array
/// slice, one sample, `D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET`, and one typed
/// color format that the device renders into, such as
/// DXGI_FORMAT_R8G8B8A8_UNORM or DXGI_FORMAT_B8G8R8A8_UNORM. The ring holds up
/// to `textureRingMaxDepth` distinct textures, and references each.
///
/// Each texture is in `state` when the session has it, and each frame leaves it
/// there.
///
/// Give the ring `depth_textures` to draw into each frame depth-tested against
/// the map, as `FrameProjection` says: one for each of `textures`, at the same
/// index, or none. Each frame clears the one at its slot, renders the map's
/// depth and stencil into it, stores them, and leaves it in `depth_state`. They
/// must have the size of `textures`, one format of
/// `GpuDeviceCapabilities.depthStencilFormats` (DXGI_FORMAT_D24_UNORM_S8_UINT
/// or DXGI_FORMAT_D32_FLOAT_S8X24_UINT), one sample, and
/// `D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL`.
///
/// See `mln_d3d12_borrowed_textures_target` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class D3d12BorrowedTexturesTarget extends _Value {
  D3d12BorrowedTexturesTarget({
    required this.logicalSize,
    required this.physicalWidth,
    required this.physicalHeight,
    required List<D3d12BorrowedTexture> textures,
    this.state = 0,
    List<D3d12BorrowedTexture> depthTextures = const [],
    this.depthState = 0,
  }) : textures = List.unmodifiable(textures),
       depthTextures = List.unmodifiable(depthTextures);
  final LogicalSize logicalSize;
  final int physicalWidth;
  final int physicalHeight;
  final List<D3d12BorrowedTexture> textures;

  /// The textures' `D3D12_RESOURCE_STATES`.
  final int state;
  final List<D3d12BorrowedTexture> depthTextures;

  /// The depth textures' `D3D12_RESOURCE_STATES`.
  final int depthState;

  /// A copy of this value with the given members replaced.
  D3d12BorrowedTexturesTarget copyWith({
    LogicalSize? logicalSize,
    int? physicalWidth,
    int? physicalHeight,
    List<D3d12BorrowedTexture>? textures,
    int? state,
    List<D3d12BorrowedTexture>? depthTextures,
    int? depthState,
  }) => D3d12BorrowedTexturesTarget(
    logicalSize: logicalSize ?? this.logicalSize,
    physicalWidth: physicalWidth ?? this.physicalWidth,
    physicalHeight: physicalHeight ?? this.physicalHeight,
    textures: textures ?? this.textures,
    state: state ?? this.state,
    depthTextures: depthTextures ?? this.depthTextures,
    depthState: depthState ?? this.depthState,
  );

  @override
  List<Object?> get _members => [
    logicalSize,
    physicalWidth,
    physicalHeight,
    textures,
    state,
    depthTextures,
    depthState,
  ];
  @override
  String toString() => _describe('D3d12BorrowedTexturesTarget', const [
    'logicalSize',
    'physicalWidth',
    'physicalHeight',
    'textures',
    'state',
    'depthTextures',
    'depthState',
  ]);
}

/// A composition swap chain to present to, which your DirectComposition visual
/// or XAML `SwapChainPanel` shows.
///
/// The session renders on D3D12: on the D3D12 device that you attach on, or on
/// one that it makes without a device. It makes a flip-model swap chain for
/// composition of `logical_size` at the map's scale factor, with premultiplied
/// alpha, and sets it as the visual's content or the panel's swap chain. Commit
/// your DirectComposition device once the attach completes. A panel takes its
/// swap chain only on its UI thread: attach it there with
/// `RenderDriverKind.callerGraphicsThread`, and service the session there.
/// Scale the visual or the panel to show the swap chain at the display's scale.
/// The session references `object` until a replacement or release of the target
/// completes, or the session ends.
///
/// See `mln_d3d12_composition_target` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
final class D3d12CompositionTarget extends _Value {
  const D3d12CompositionTarget({
    required this.logicalSize,
    required this.kind,
    required this.object,
  });
  final LogicalSize logicalSize;
  final D3d12CompositionKind kind;

  /// Your `IDCompositionVisual*` or `ISwapChainPanelNative*`.
  final NativePointer object;

  /// A copy of this value with the given members replaced.
  D3d12CompositionTarget copyWith({
    LogicalSize? logicalSize,
    D3d12CompositionKind? kind,
    NativePointer? object,
  }) => D3d12CompositionTarget(
    logicalSize: logicalSize ?? this.logicalSize,
    kind: kind ?? this.kind,
    object: object ?? this.object,
  );

  @override
  List<Object?> get _members => [logicalSize, kind, object];
  @override
  String toString() => _describe('D3d12CompositionTarget', const [
    'logicalSize',
    'kind',
    'object',
  ]);
}

/// What a render session renders into: one kind of target, and its descriptor.
///
/// Every target has a size in logical pixels, which sets the map's size. The
/// session renders it at the map's scale factor. A borrowed or exported ring
/// hands each frame to you: acquire it with
/// `RenderSessionHandle.acquireFrame()`, and release it with your GPU's
/// synchronization once you have read it.
///
/// The session uses the objects that the descriptor names until a replacement
/// or release of the target completes, a detach or release of the session
/// completes, or an abandon of it returns, except as a kind's descriptor says.
/// A window target presents without waiting for the display, so demand at most
/// one frame per display refresh.
///
/// See `mln_render_target` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
sealed class RenderTarget {
  const RenderTarget._();
  const factory RenderTarget.offscreen(OffscreenTarget value) =
      RenderTargetOffscreen;
  const factory RenderTarget.metalLayer(MetalLayerTarget value) =
      RenderTargetMetalLayer;
  const factory RenderTarget.win32Window(Win32WindowTarget value) =
      RenderTargetWin32Window;
  const factory RenderTarget.xlibWindow(XlibWindowTarget value) =
      RenderTargetXlibWindow;
  const factory RenderTarget.waylandSurface(WaylandSurfaceTarget value) =
      RenderTargetWaylandSurface;
  const factory RenderTarget.androidWindow(AndroidWindowTarget value) =
      RenderTargetAndroidWindow;
  const factory RenderTarget.ohosWindow(OhosWindowTarget value) =
      RenderTargetOhosWindow;
  const factory RenderTarget.vulkanSurface(VulkanSurfaceTarget value) =
      RenderTargetVulkanSurface;
  const factory RenderTarget.openGlSurface(OpenGlSurfaceTarget value) =
      RenderTargetOpenGlSurface;
  const factory RenderTarget.metalBorrowedTextures(
    MetalBorrowedTexturesTarget value,
  ) = RenderTargetMetalBorrowedTextures;
  const factory RenderTarget.vulkanBorrowedTextures(
    VulkanBorrowedTexturesTarget value,
  ) = RenderTargetVulkanBorrowedTextures;
  const factory RenderTarget.openGlBorrowedTextures(
    OpenGlBorrowedTexturesTarget value,
  ) = RenderTargetOpenGlBorrowedTextures;
  const factory RenderTarget.webGpuBorrowedTextures(
    WebGpuBorrowedTexturesTarget value,
  ) = RenderTargetWebGpuBorrowedTextures;
  const factory RenderTarget.exportedRing(ExportedRingTarget value) =
      RenderTargetExportedRing;
  const factory RenderTarget.webCanvas(WebCanvasTarget value) =
      RenderTargetWebCanvas;
  const factory RenderTarget.d3d12BorrowedTextures(
    D3d12BorrowedTexturesTarget value,
  ) = RenderTargetD3d12BorrowedTextures;
  const factory RenderTarget.d3d12Composition(D3d12CompositionTarget value) =
      RenderTargetD3d12Composition;
}

final class RenderTargetOffscreen extends RenderTarget {
  const RenderTargetOffscreen(this.value) : super._();
  final OffscreenTarget value;
  @override
  bool operator ==(Object other) =>
      other is RenderTargetOffscreen && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class RenderTargetMetalLayer extends RenderTarget {
  const RenderTargetMetalLayer(this.value) : super._();
  final MetalLayerTarget value;
  @override
  bool operator ==(Object other) =>
      other is RenderTargetMetalLayer && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class RenderTargetWin32Window extends RenderTarget {
  const RenderTargetWin32Window(this.value) : super._();
  final Win32WindowTarget value;
  @override
  bool operator ==(Object other) =>
      other is RenderTargetWin32Window && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class RenderTargetXlibWindow extends RenderTarget {
  const RenderTargetXlibWindow(this.value) : super._();
  final XlibWindowTarget value;
  @override
  bool operator ==(Object other) =>
      other is RenderTargetXlibWindow && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class RenderTargetWaylandSurface extends RenderTarget {
  const RenderTargetWaylandSurface(this.value) : super._();
  final WaylandSurfaceTarget value;
  @override
  bool operator ==(Object other) =>
      other is RenderTargetWaylandSurface && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class RenderTargetAndroidWindow extends RenderTarget {
  const RenderTargetAndroidWindow(this.value) : super._();
  final AndroidWindowTarget value;
  @override
  bool operator ==(Object other) =>
      other is RenderTargetAndroidWindow && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class RenderTargetOhosWindow extends RenderTarget {
  const RenderTargetOhosWindow(this.value) : super._();
  final OhosWindowTarget value;
  @override
  bool operator ==(Object other) =>
      other is RenderTargetOhosWindow && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class RenderTargetVulkanSurface extends RenderTarget {
  const RenderTargetVulkanSurface(this.value) : super._();
  final VulkanSurfaceTarget value;
  @override
  bool operator ==(Object other) =>
      other is RenderTargetVulkanSurface && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class RenderTargetOpenGlSurface extends RenderTarget {
  const RenderTargetOpenGlSurface(this.value) : super._();
  final OpenGlSurfaceTarget value;
  @override
  bool operator ==(Object other) =>
      other is RenderTargetOpenGlSurface && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class RenderTargetMetalBorrowedTextures extends RenderTarget {
  const RenderTargetMetalBorrowedTextures(this.value) : super._();
  final MetalBorrowedTexturesTarget value;
  @override
  bool operator ==(Object other) =>
      other is RenderTargetMetalBorrowedTextures && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class RenderTargetVulkanBorrowedTextures extends RenderTarget {
  const RenderTargetVulkanBorrowedTextures(this.value) : super._();
  final VulkanBorrowedTexturesTarget value;
  @override
  bool operator ==(Object other) =>
      other is RenderTargetVulkanBorrowedTextures && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class RenderTargetOpenGlBorrowedTextures extends RenderTarget {
  const RenderTargetOpenGlBorrowedTextures(this.value) : super._();
  final OpenGlBorrowedTexturesTarget value;
  @override
  bool operator ==(Object other) =>
      other is RenderTargetOpenGlBorrowedTextures && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class RenderTargetWebGpuBorrowedTextures extends RenderTarget {
  const RenderTargetWebGpuBorrowedTextures(this.value) : super._();
  final WebGpuBorrowedTexturesTarget value;
  @override
  bool operator ==(Object other) =>
      other is RenderTargetWebGpuBorrowedTextures && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class RenderTargetExportedRing extends RenderTarget {
  const RenderTargetExportedRing(this.value) : super._();
  final ExportedRingTarget value;
  @override
  bool operator ==(Object other) =>
      other is RenderTargetExportedRing && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class RenderTargetWebCanvas extends RenderTarget {
  const RenderTargetWebCanvas(this.value) : super._();
  final WebCanvasTarget value;
  @override
  bool operator ==(Object other) =>
      other is RenderTargetWebCanvas && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class RenderTargetD3d12BorrowedTextures extends RenderTarget {
  const RenderTargetD3d12BorrowedTextures(this.value) : super._();
  final D3d12BorrowedTexturesTarget value;
  @override
  bool operator ==(Object other) =>
      other is RenderTargetD3d12BorrowedTextures && other.value == value;
  @override
  int get hashCode => value.hashCode;
}

final class RenderTargetD3d12Composition extends RenderTarget {
  const RenderTargetD3d12Composition(this.value) : super._();
  final D3d12CompositionTarget value;
  @override
  bool operator ==(Object other) =>
      other is RenderTargetD3d12Composition && other.value == value;
  @override
  int get hashCode => value.hashCode;
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
    this.keepDepth = false,
  });

  /// The thread that runs the session's graphics work, which depends on the
  /// target:
  ///
  /// - `RenderDriverKind.callerGraphicsThread` for a borrowed ring of WebGPU
  ///   textures.
  /// - `RenderDriverKind.coreWorker` for an offscreen target, a window, a
  ///   DirectComposition visual, or an exported ring.
  /// - `RenderDriverKind.callerGraphicsThread` for a XAML `SwapChainPanel`, on
  ///   the panel's UI thread.
  /// - Either one for a web canvas. In a browser both run on the thread that
  ///   loaded the library: `RenderDriverKind.coreWorker` in the library's own
  ///   tasks, and `RenderDriverKind.callerGraphicsThread` when you service the
  ///   session.
  /// - Either one for a Metal layer, a Vulkan surface, an OpenGL surface, or a
  ///   borrowed ring of Metal, Vulkan, D3D12, or OpenGL textures. A build may
  ///   drive these on `RenderDriverKind.coreWorker` only, and fails an attach
  ///   with `RenderDriverKind.callerGraphicsThread` then.
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
  /// again until a service runs. It's never called on your graphics thread
  /// during a service: the service runs the work that comes while it runs, so
  /// you can call `mln_render_session_service_driver_work()` from the wake.
  final Wake driverWorkWake;

  /// Whether an exported ring keeps the depth and stencil of each frame for
  /// you, so that you can draw into the frame depth-tested against the map, as
  /// `FrameProjection` says. The getter of the device's API, such as
  /// `AcquiredFrameHandle.withVulkanTexture()`, gets a frame's depth texture on
  /// the device, and `AcquiredFrameHandle.withOpaqueFd()` shares it across APIs
  /// when `GpuDeviceCapabilities.depthExportKinds` has the ring's kind. Keeping
  /// depth costs memory bandwidth, most on tile-based GPUs. A borrowed ring
  /// keeps depth in the textures that you give it instead, and the other
  /// targets keep none, so an attach to them with `keep_depth` fails with
  /// `Status.unsupported`.
  final bool keepDepth;

  /// A copy of this value with the given members replaced.
  RenderSessionAttachOptions copyWith({
    RenderDriverKind? driver,
    GpuSyncKindFlag? producerSyncKinds,
    Wake? frameWake,
    Wake? redrawWake,
    Wake? driverWorkWake,
    bool? keepDepth,
  }) => RenderSessionAttachOptions(
    driver: driver ?? this.driver,
    producerSyncKinds: producerSyncKinds ?? this.producerSyncKinds,
    frameWake: frameWake ?? this.frameWake,
    redrawWake: redrawWake ?? this.redrawWake,
    driverWorkWake: driverWorkWake ?? this.driverWorkWake,
    keepDepth: keepDepth ?? this.keepDepth,
  );

  @override
  List<Object?> get _members => [
    driver,
    producerSyncKinds,
    frameWake,
    redrawWake,
    driverWorkWake,
    keepDepth,
  ];
  @override
  String toString() => _describe('RenderSessionAttachOptions', const [
    'driver',
    'producerSyncKinds',
    'frameWake',
    'redrawWake',
    'driverWorkWake',
    'keepDepth',
  ]);
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
    this.globeRendering = false,
    this.globeFreeBearingEnabled = false,
  });
  final MapDebugFlag debugFlags;

  /// Grows with each change to the map's state.
  final int generation;

  /// Whether the style and every resource that the view needs have loaded.
  final bool fullyLoaded;
  final bool renderingStatsViewEnabled;

  /// Whether a gesture recognizer's gesture, or the glide and snap after it,
  /// moves the camera.
  final bool gestureInProgress;
  final bool transitionInProgress;
  final RuntimeEventFlag eventMask;

  /// Counts the render updates that the map has made for its render session.
  final int latestRenderUpdateGeneration;
  final MapTileSettings tile;
  final CameraBounds bounds;
  final FreeCamera freeCamera;
  final MapView view;

  /// Whether the map draws as a globe, alone or blending into Mercator.
  final bool globeRendering;

  /// The setting from `MapHandle.setGlobeFreeBearingEnabled()`.
  final bool globeFreeBearingEnabled;

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
    globeRendering,
    globeFreeBearingEnabled,
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
    'globeRendering',
    'globeFreeBearingEnabled',
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

  /// A GeoJSON source's options, with every field set.
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
  });
  final CameraUpdateMode mode;
  final CameraOptions camera;
  final AnimationOptions animation;

  /// A copy of this value with the given members replaced.
  CameraUpdate copyWith({
    CameraUpdateMode? mode,
    CameraOptions? camera,
    AnimationOptions? animation,
  }) => CameraUpdate(
    mode: mode ?? this.mode,
    camera: camera ?? this.camera,
    animation: animation ?? this.animation,
  );

  @override
  List<Object?> get _members => [mode, camera, animation];
  @override
  String toString() =>
      _describe('CameraUpdate', const ['mode', 'camera', 'animation']);
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
    this.exportKind = const GpuExportKind.fromRawValue(0),
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

  /// How an exported ring shares its textures, which says which getter gets an
  /// acquired frame's texture, or `GpuExportKind.none` for another target.
  final GpuExportKind exportKind;

  @override
  List<Object?> get _members => [
    driver,
    textureRingDepth,
    flags,
    producerSyncKind,
    consumerSyncKinds,
    exportKind,
  ];
  @override
  String toString() => _describe('RenderSessionCapabilities', const [
    'driver',
    'textureRingDepth',
    'flags',
    'producerSyncKind',
    'consumerSyncKinds',
    'exportKind',
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

/// Options for `RenderSessionHandle.setFramePacing()`.
///
/// See `mln_frame_pacing_options` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
final class FramePacingOptions extends _Value {
  const FramePacingOptions({this.maximumFramesPerSecond, this.view});

  /// The most frames per second that the library demands, greater than zero. It
  /// skips display refreshes to stay within it. By default, it can demand a
  /// frame at every refresh.
  final int? maximumFramesPerSecond;

  /// The `NSView*` on macOS, or the `UIView*` on iOS and tvOS, that shows the
  /// session's frames, to pace them to its window's display rather than the
  /// main one. On iOS and tvOS, set the pacing again when the scene moves to
  /// another screen, as `windowScene:didUpdateEffectiveGeometry:` reports.
  final NativePointer? view;

  /// A copy of this value with the given members replaced.
  FramePacingOptions copyWith({
    int? maximumFramesPerSecond,
    NativePointer? view,
  }) => FramePacingOptions(
    maximumFramesPerSecond:
        maximumFramesPerSecond ?? this.maximumFramesPerSecond,
    view: view ?? this.view,
  );

  @override
  List<Object?> get _members => [maximumFramesPerSecond, view];
  @override
  String toString() =>
      _describe('FramePacingOptions', const ['maximumFramesPerSecond', 'view']);
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
    this.noStore = false,
  }) : bytes = Uint8List.fromList(bytes ?? const <int>[]).asUnmodifiableView();
  final ResourceResponseStatus status;

  /// Why the request failed, for `ResourceResponseStatus.error`.
  final ResourceErrorReason errorReason;

  /// The resource, for `ResourceResponseStatus.ok`.
  final Uint8List bytes;

  /// A description of the error, for `ResourceResponseStatus.error`.
  final String errorMessage;

  /// Whether the library must not use this copy once it expires until it
  /// requests the resource again, not even while the server can't be reached,
  /// as `Cache-Control: must-revalidate` or `no-cache` says.
  final bool mustRevalidate;
  final int? modifiedUnixMs;

  /// When this copy stops being fresh. Without it, the copy never expires.
  final int? expiresUnixMs;

  /// Empty when the resource has no ETag.
  final String etag;

  /// When to retry, for `ResourceResponseStatus.error`.
  final int? retryAfterUnixMs;

  /// Whether the library must not store this copy, as `Cache-Control: no-store`
  /// says.
  final bool noStore;

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
    bool? noStore,
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
    noStore: noStore ?? this.noStore,
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
    noStore,
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
    'noStore',
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
    this.fallbackFontFaces = '',
    this.systemFontsEnabled,
    this.localIdeographsEnabled,
  });

  /// The map's first size, and its scale factor. Render sessions of the map
  /// render at its scale factor. `MapHandle.resize()` changes both, or
  /// `RenderSessionHandle.resize()` while a render session is attached.
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

  /// Fonts for the text that a style's own fonts don't draw. A JSON array of
  /// font files, each an object with a `url` and an optional `unicode-range`,
  /// as in a font face of a style's `font-faces`. They shape, in order, the
  /// text that glyph PBFs can't shape, such as Myanmar or Devanagari, or all
  /// the text of a style without glyph PBFs, and draw the glyphs that come from
  /// local fonts: those a style's PBFs lack, and CJK with
  /// `local_ideographs_enabled`. Empty for the system's fonts alone.
  final String fallbackFontFaces;

  /// Whether the map draws from the system's fonts what the style's fonts and
  /// `fallback_font_faces` don't: fonts that the platform's font matcher picks
  /// for the font stack's names, or for the character and the user's preferred
  /// languages. It shapes with them the text that glyph PBFs can't shape, and
  /// all the text of a style without glyph PBFs. Without it, the map does. The
  /// first map that uses them waits for the matcher to load. In the browser,
  /// the browser draws those glyphs in the font stack's families, then its
  /// sans-serif font, one at a time.
  final bool? systemFontsEnabled;

  /// Whether CJK ideographs, kana and hangul come from local fonts
  /// (`fallback_font_faces`, then the system's) before glyph PBFs, rather than
  /// from a glyph PBF downloaded for every few characters. Without it, they do.
  final bool? localIdeographsEnabled;

  /// A copy of this value with the given members replaced.
  MapOptions copyWith({
    LogicalExtent? initialExtent,
    MapMode? mapMode,
    bool? fastPforEnabled,
    RuntimeEventFlag? eventMask,
    bool? tileCacheEnabled,
    String? fallbackFontFaces,
    bool? systemFontsEnabled,
    bool? localIdeographsEnabled,
  }) => MapOptions(
    initialExtent: initialExtent ?? this.initialExtent,
    mapMode: mapMode ?? this.mapMode,
    fastPforEnabled: fastPforEnabled ?? this.fastPforEnabled,
    eventMask: eventMask ?? this.eventMask,
    tileCacheEnabled: tileCacheEnabled ?? this.tileCacheEnabled,
    fallbackFontFaces: fallbackFontFaces ?? this.fallbackFontFaces,
    systemFontsEnabled: systemFontsEnabled ?? this.systemFontsEnabled,
    localIdeographsEnabled:
        localIdeographsEnabled ?? this.localIdeographsEnabled,
  );

  @override
  List<Object?> get _members => [
    initialExtent,
    mapMode,
    fastPforEnabled,
    eventMask,
    tileCacheEnabled,
    fallbackFontFaces,
    systemFontsEnabled,
    localIdeographsEnabled,
  ];
  @override
  String toString() => _describe('MapOptions', const [
    'initialExtent',
    'mapMode',
    'fastPforEnabled',
    'eventMask',
    'tileCacheEnabled',
    'fallbackFontFaces',
    'systemFontsEnabled',
    'localIdeographsEnabled',
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

/// How long the resources that an offline region holds stay usable.
///
/// See `mln_offline_region_expiry` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
final class OfflineRegionExpiry extends _Value {
  const OfflineRegionExpiry({
    this.staleAfterUnixMs,
    this.usableOfflineUntilUnixMs,
    this.staleResourceCount = 0,
    this.unusableResourceCount = 0,
  });

  /// When the first of the region's resources goes stale, which may have
  /// passed. Maps show a stale resource while they check it with the server,
  /// unless it must be checked first, and the region's download refreshes it.
  /// Without it, no resource ever goes stale.
  final int? staleAfterUnixMs;

  /// When the first of the region's resources that the server requires checking
  /// once stale stops being usable without the server, which may have passed.
  /// Refresh the region before then to keep all of it usable offline. Without
  /// it, every resource stays usable offline.
  final int? usableOfflineUntilUnixMs;

  /// The resources that are stale now, tiles included.
  final int staleResourceCount;

  /// The stale resources that maps can't use now without the server.
  final int unusableResourceCount;

  @override
  List<Object?> get _members => [
    staleAfterUnixMs,
    usableOfflineUntilUnixMs,
    staleResourceCount,
    unusableResourceCount,
  ];
  @override
  String toString() => _describe('OfflineRegionExpiry', const [
    'staleAfterUnixMs',
    'usableOfflineUntilUnixMs',
    'staleResourceCount',
    'unusableResourceCount',
  ]);
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

/// The size of a diagnostic message in bytes, with its terminator.
///
/// See `MLN_DIAGNOSTIC_MESSAGE_CAPACITY` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
const int diagnosticMessageCapacity = 4096;

/// The token of the frame results of the demand that a session holds for an
/// image of `MapHandle.requestStillImage()`, which says how that demand runs.
/// Your own demands can't use it.
///
/// See `MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
const int frameDemandTokenStillImage = 0xFFFFFFFFFFFFFFFF;

/// The most events that one input or tick reports.
///
/// See `MLN_GESTURE_RESULT_EVENT_CAPACITY` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html).
const int gestureResultEventCapacity = 4;

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
