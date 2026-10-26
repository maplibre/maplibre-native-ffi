/// Render sessions on the build's backend, driven by their core worker
/// and waited on through their frame wake. A session renders on a graphics
/// device that native code creates, or on an owned texture in a context from
/// `tests/graphics` when a test needs the frames that only such a session
/// exposes.
library;

import 'dart:async';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

import 'fixture.dart';
import 'graphics.dart';

const renderSize = 32;

/// How many frames a render test demands before it fails for want of one
/// that rendered.
const _frameAttempts = 8;

final _extent = LogicalExtent(
  width: renderSize,
  height: renderSize,
  scaleFactor: 1,
);

/// Opens a runtime and a map sized for the render targets, with [style]
/// committed. Teardowns close both.
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

/// Creates a graphics device, which a teardown releases.
GraphicsDeviceHandle openGraphicsDevice() {
  final device = GraphicsDeviceHandle.create();
  addTearDown(device.close);
  return device;
}

/// Attaches a texture of the render extent on a new graphics device. The
/// session keeps its own reference to the device, so the device handle closes
/// before this returns.
NewRenderSession attachDeviceTexture(
  MapHandle map,
  RenderSessionAttachOptions options,
) {
  final device = GraphicsDeviceHandle.create();
  try {
    return map.attachGraphicsDeviceTexture(
      device,
      GraphicsDeviceTextureDescriptor(extent: _extent),
      options,
    );
  } finally {
    device.close();
  }
}

/// A session driven by its core worker, which a teardown closes before the
/// map and any graphics context it uses.
final class WorkerSession {
  WorkerSession._(this.session);

  final RenderSessionHandle session;
  final _frames = Signal();
  var _nextToken = 1;

  /// Attaches a graphics device texture on [map], which grants readback.
  static Future<WorkerSession> attach(MapHandle map) =>
      _attach((options) => attachDeviceTexture(map, options));

  /// Attaches an owned texture on [map] in a context from `tests/graphics`,
  /// which exposes its frames on Metal and Vulkan.
  static Future<WorkerSession> attachOwnedTexture(MapHandle map) {
    final graphics = TestGraphics.open();
    return _attach(
      (options) => graphics.attachOwnedTexture(map, _extent, options),
    );
  }

  static Future<WorkerSession> _attach(
    NewRenderSession Function(RenderSessionAttachOptions options) attach,
  ) async {
    late final WorkerSession worker;
    final attachment = attach(
      RenderSessionAttachOptions(
        driver: RenderDriverKind.coreWorker,
        requestedTextureRingDepth: 1,
        frameWake: Wake(callback: () => worker._frames.notify()),
      ),
    );
    worker = WorkerSession._(attachment.session);
    addTearDown(() => releaseSession(worker.session));
    await within(attachment.ready, 'attachment');
    return worker;
  }

  /// Demands frames until one renders, and returns its result.
  Future<RenderFrameResult> renderFrame() async {
    for (var attempt = 0; attempt < _frameAttempts; attempt++) {
      final token = _nextToken++;
      _requestFrame(session, token);
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

/// Closes [session] in any state unless a test already did, and waits until
/// it retires, so a teardown that runs later may destroy its graphics.
Future<void> releaseSession(RenderSessionHandle session) =>
    closeIfLive(session.close);

/// Demands a forced frame, which renders whether or not the map changed.
void _requestFrame(RenderSessionHandle session, int token) {
  session.requestFrame(FrameDemand(flags: FrameDemandFlag.force, token: token));
}

/// Drains the session's frame results and returns the one for [token], or
/// null while it has not arrived.
RenderFrameResult? _takeResult(RenderSessionHandle session, int token) {
  for (final result in session.drainFrameResults()) {
    if (result.token == token) return result;
  }
  return null;
}
