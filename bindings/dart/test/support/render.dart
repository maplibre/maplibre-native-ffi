/// Owned-texture render sessions on the build's backend, driven by their core
/// worker and waited on through their frame wake.
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

/// Opens a runtime and a map sized for the render targets, with [styleJson]
/// committed. Teardowns close both.
Future<MapHandle> openRenderMap(String styleJson) async {
  final runtime = await openRuntime();
  final map = await runtime.openMap(
    MapOptions(
      initialExtent: LogicalExtent(
        width: renderSize,
        height: renderSize,
        scaleFactor: 1,
      ),
      eventMask: RuntimeEventMask.all,
    ),
  );
  await expectCommitted(map.setStyleJson(jsonBytes(styleJson)));
  return map;
}

/// A session driven by its core worker, which a teardown closes before the
/// map and the graphics context it uses.
final class WorkerSession {
  WorkerSession._(this.session);

  final RenderSessionHandle session;
  final _frames = Signal();
  var _nextToken = 1;

  /// Attaches a session on [map]. On OpenGL, the session creates a dedicated
  /// EGL context, which grants readback with a ring of one texture.
  static Future<WorkerSession> attach(MapHandle map) async {
    final graphics = TestGraphics.open();
    late final WorkerSession worker;
    final attachment = graphics.attachOwnedTexture(
      map,
      _extent,
      RenderSessionAttachOptions(
        driver: RenderDriverKind.coreWorker,
        requestedTextureRingDepth: 1,
        frameWake: Wake(callback: () => worker._frames.notify()),
      ),
    );
    worker = WorkerSession._(attachment.session);
    addTearDown(() => releaseSession(worker.session));
    await within(attachment.completed, 'attachment');
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
  session.requestFrame(FrameDemand(flags: FrameDemandFlag.none, token: token));
}

/// Drains the session's frame results and returns the one for [token], or
/// null while it has not arrived.
RenderFrameResult? _takeResult(RenderSessionHandle session, int token) {
  final batch = session.drainFrameResults();
  if (batch == null) return null;
  try {
    for (final result in batch.getValue().results) {
      if (result.token == token) return result;
    }
    return null;
  } finally {
    batch.close();
  }
}
