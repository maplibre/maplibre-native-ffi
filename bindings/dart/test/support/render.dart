/// Render sessions on the backend under test, driven by their core worker
/// and waited on through their frame wake. A session renders on a graphics
/// device that native code creates, or on a borrowed texture in a context from
/// `tests/graphics` when a test needs the frames that only such a session
/// exposes.
library;

import 'dart:async';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

import 'backend.dart';
import 'fixture.dart';
import 'graphics.dart';

const renderSize = 32;

/// How many frames a render test demands before it fails for want of one
/// that rendered.
const _frameAttempts = 8;

final _size = LogicalSize(width: renderSize, height: renderSize);

/// Opens a runtime and a map sized for the render targets, with [style]
/// committed. Teardowns release both.
Future<MapHandle> openRenderMap(Object? style) async {
  final runtime = await openRuntime();
  final map = await runtime.openMap(
    MapOptions(
      initialExtent: LogicalExtent(
        width: renderSize,
        height: renderSize,
        scaleFactor: 1,
      ),
      eventMask: RuntimeEventFlag.all,
    ),
  );
  await expectCommitted(map.setStyleJson(style));
  return map;
}

/// An offscreen target of the render size.
final offscreenTarget = RenderTarget.offscreen(
  OffscreenTarget(logicalSize: _size),
);

/// Creates a GPU device of the backend under test that the library owns, which
/// a teardown releases.
GpuDeviceHandle openGpuDevice() {
  final device = createDeviceUnderTest();
  addTearDown(device.release);
  return device;
}

/// Attaches an offscreen session of the render size, on [device] or else on a
/// device of its own of the backend under test.
RenderSessionHandle attachOffscreenSession(
  MapHandle map,
  RenderSessionAttachOptions options, {
  GpuDeviceHandle? device,
}) {
  if (device != null) {
    return map.attach(offscreenTarget, options, device: device);
  }
  final own = createDeviceUnderTest();
  try {
    return map.attach(offscreenTarget, options, device: own);
  } finally {
    // The session keeps the device until the session ends.
    own.release();
  }
}

/// A session driven by its core worker, which a teardown releases before the
/// map and any graphics context it uses.
final class WorkerSession {
  WorkerSession._(this.session);

  final RenderSessionHandle session;
  final _frames = Signal();
  var _nextToken = 1;

  /// Attaches an offscreen session on [map], which grants readback.
  static Future<WorkerSession> attach(MapHandle map) =>
      _attach((options) => attachOffscreenSession(map, options));

  /// Attaches a borrowed texture on [map] in a context from `tests/graphics`,
  /// which exposes its frames on Metal and Vulkan.
  static Future<WorkerSession> attachBorrowedTexture(MapHandle map) {
    final graphics = TestGraphics.open();
    return _attach(
      (options) => graphics.attachBorrowedTexture(map, _size, options),
    );
  }

  static Future<WorkerSession> _attach(
    RenderSessionHandle Function(RenderSessionAttachOptions options) attach,
  ) async {
    late final WorkerSession worker;
    final session = attach(
      RenderSessionAttachOptions(
        driver: RenderDriverKind.coreWorker,
        frameWake: Wake(callback: () => worker._frames.notify()),
      ),
    );
    worker = WorkerSession._(session);
    addTearDown(() => releaseSession(session));
    await within(session.ready, 'attachment');
    return worker;
  }

  Future<RenderFrameResult> renderFrame() async {
    for (var attempt = 0; attempt < _frameAttempts; attempt++) {
      final token = _nextToken++;
      _demandFrame(session, token);
      var result = _takeResult(session, token);
      while (result == null) {
        await _frames.wait('frame $token');
        result = _takeResult(session, token);
      }
      if (result.disposition == RenderResult.rendered) return result;
    }
    fail('$_frameAttempts frame demands rendered nothing');
  }
}

/// Releases [session] in any state unless a test already did, and waits until
/// it retires, so a teardown that runs later may destroy its graphics.
Future<void> releaseSession(RenderSessionHandle session) =>
    releaseIfLive(session.release);

/// Demands a forced frame, which renders whether or not the map changed.
void _demandFrame(RenderSessionHandle session, int token) {
  session.demandFrame(FrameDemand(flags: FrameDemandFlag.force, token: token));
}

/// Drains the session's frame results and returns the one for [token], or
/// null while it has not arrived.
RenderFrameResult? _takeResult(RenderSessionHandle session, int token) {
  for (final result in session.drainFrameResults()) {
    if (result.token == token) return result;
  }
  return null;
}
