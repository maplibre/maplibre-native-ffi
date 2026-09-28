part of 'runtime.dart';

final class MapProjectionHandle with _GeneratedProjectionOperations {
  MapProjectionHandle._(NativeMapProjection handle)
    : _state = NativeHandleState(handle, 'MapProjectionHandle');

  @override
  final NativeHandleState<NativeMapProjection> _state;

  /// Whether this projection helper has been closed by the Dart binding.
  bool get isClosed => _state.isClosed;

  @override
  NativeMapProjection get _handle => _state.handle;
}

/// Render execution placement selected during attachment.
///
/// A target that requires one placement rejects the other during attachment.
final class RenderSessionAttachment {
  /// Creates an attachment from a session and its attachment completion.
  const RenderSessionAttachment(this.session, this.completed);

  /// The session, which is usable for driver work and for state reads while
  /// attachment runs.
  final RenderSessionHandle session;

  /// Completes after the selected driver initializes the target.
  ///
  /// A caller-graphics-thread session completes attachment only after the host
  /// services driver work. A failed attachment still requires
  /// [RenderSessionHandle.detach] or [RenderSessionHandle.abandon] before
  /// [RenderSessionHandle.close].
  final Future<void> completed;
}

/// A render session attached to one map and render target.
///
/// A map has at most one live session, and its style, sources, layers, and
/// camera remain after the session detaches. The session advances through its
/// selected [RenderDriverKind] rather than through a runtime pump.
///
/// The target setters below start ordered replacements of the session's
/// surface or caller-owned texture, as after a window recreation or a new host
/// allocation. A replacement keeps the session's rendering resources,
/// including loaded tiles, unless the scale factor changes, and map-owned
/// feature state survives either way. It changes the graphics resource only,
/// so the map viewport keeps following map creation and map resize. A setter
/// for a target kind that the session does not use reports an unsupported
/// status, and replacing a texture target while a frame is acquired reports an
/// invalid-state status.
final class RenderSessionHandle with _GeneratedRenderSessionOperations {
  RenderSessionHandle._(this._map, NativeRenderSession handle)
    : _state = NativeHandleState(handle, 'RenderSessionHandle');
  // Keeps the parent map reachable while the session lives.
  // ignore: unused_field
  final MapHandle _map;
  @override
  final NativeHandleState<NativeRenderSession> _state;
  @override
  NativeRenderSession get _handle => _state.handle;
  bool get isClosed => _state.isClosed;
}

final class ResourceRequestHandle with _GeneratedResourceRequestOperations {
  ResourceRequestHandle._(NativeResourceRequest handle)
    : _state = NativeHandleState(handle, 'ResourceRequestHandle');
  @override
  final NativeHandleState<NativeResourceRequest> _state;
  @override
  NativeResourceRequest get _handle => _state.handle;
  bool get isClosed => _state.isClosed;
  _ResourceRequestCancelState? _cancelRegistration;
}

/// Exposes a map's handle id for tests that must reach the C API with a raw id.
int mapHandleIdForTesting(MapHandle map) => map._state.handleId;

/// Exposes a runtime's handle id for tests that must reach the C API with a raw
/// id.
int runtimeHandleIdForTesting(RuntimeHandle runtime) => runtime._state.handleId;

/// Dart-owned premultiplied RGBA8 texture readback bytes.
final class AcquiredFrame with _GeneratedAcquiredFrameOperations {
  AcquiredFrame._(this._session, NativeAcquiredFrame handle)
    : _state = NativeHandleState(handle, 'AcquiredFrame');

  // Keeps the parent session reachable while the frame lives.
  // ignore: unused_field
  final RenderSessionHandle _session;
  @override
  final NativeHandleState<NativeAcquiredFrame> _state;
  @override
  NativeAcquiredFrame get _handle => _state.handle;
}

/// Render-session attachment operations on an any-thread map handle.
///
/// Every attachment copies its descriptor and its
/// [RenderSessionAttachOptions] before returning, and returns a
/// [RenderSessionAttachment] whose session is usable at once. The session
/// finishes initializing its target when the attachment's completion resolves.
/// The isolate that attaches a session receives its
/// [RenderSessionHandle.frameResultsReady] and
/// [RenderSessionHandle.driverWorkReady] notifications.
///
/// A target that requires one [RenderDriverKind] rejects the other during
/// attachment, which is why the defaults below differ per backend.
