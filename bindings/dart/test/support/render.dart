/// Owned-texture render sessions on the build's backend.
///
/// A core-worker session is waited on through its frame wake. A caller-driven
/// session belongs to the native thread that first services it, and Dart may
/// resume an isolate on another thread after any await, so this isolate
/// services one only inside a single synchronous stretch: from its context
/// becoming current, through attachment and every frame, to its detach.
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

const _extent = RenderTargetExtent(
  width: renderSize,
  height: renderSize,
  scaleFactor: 1,
);

/// Opens a runtime and a map sized for the render targets, with [styleJson]
/// committed. Teardowns close both.
Future<MapHandle> openRenderMap(String styleJson) async {
  final runtime = await openRuntime();
  final map = await runtime.openMap(
    const MapOptions(
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
      shared: false,
    );
    worker = WorkerSession._(attachment.session);
    addTearDown(() => _release(worker.session));
    await within(attachment.completed, 'attachment');
    return worker;
  }

  /// Demands frames until one renders, and returns its result.
  Future<RenderFrameResult> renderFrame() async {
    for (var attempt = 0; attempt < _frameAttempts; attempt++) {
      final token = _requestFrame(session, _nextToken++);
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

/// A caller-driven session that the current synchronous stretch services.
final class CallerDrivenSession {
  CallerDrivenSession._(this.session);

  final RenderSessionHandle session;
  var _nextToken = 1;

  /// Services driver work until [done] holds, failing after [waitDeadline].
  ///
  /// The loop never yields: a wake would reach this isolate only through its
  /// event loop, and the next service could then run on another thread.
  void serviceUntil(bool Function() done, String what) {
    final clock = Stopwatch()..start();
    while (true) {
      session.serviceDriverWork(0);
      if (done()) return;
      if (clock.elapsed > waitDeadline) fail('timed out servicing $what');
    }
  }

  /// Demands frames until one renders, and returns its result.
  RenderFrameResult renderFrame() {
    for (var attempt = 0; attempt < _frameAttempts; attempt++) {
      final token = _requestFrame(session, _nextToken++);
      RenderFrameResult? result;
      serviceUntil(
        () => (result = _takeResult(session, token)) != null,
        'frame $token',
      );
      if (result!.disposition == RenderResult.rendered) return result!;
    }
    fail('$_frameAttempts frame demands rendered nothing');
  }
}

/// Runs [body] on a caller-driven session attached to [map], all in one
/// synchronous stretch that also creates and destroys the graphics context.
///
/// The stretch services the work [body] queued and detaches the session after
/// [body] returns, or abandons it when [body] throws, and closes it. The attachment's and the detach's completions
/// arrive through ports afterwards, without any more driver work, and the
/// returned future awaits them before it returns what [body] returned.
Future<T> withCallerDrivenSession<T>(
  MapHandle map,
  T Function(CallerDrivenSession driven) body,
) async {
  final completions = <Future<void>>[];
  final T result;
  try {
    result = _callerDrivenStretch(map, body, completions);
  } catch (_) {
    // The failure that ended the stretch is the one to report.
    for (final completion in completions) {
      completion.ignore();
    }
    rethrow;
  }
  await within(Future.wait(completions), 'the session completions');
  return result;
}

T _callerDrivenStretch<T>(
  MapHandle map,
  T Function(CallerDrivenSession driven) body,
  List<Future<void>> completions,
) {
  final graphics = TestGraphics.create();
  try {
    if (graphics.isOpengl) graphics.makeCurrent();
    final attachment = graphics.attachOwnedTexture(
      map,
      _extent,
      const RenderSessionAttachOptions(
        driver: RenderDriverKind.callerGraphicsThread,
        requestedTextureRingDepth: 2,
      ),
      shared: true,
    );
    completions.add(attachment.completed);
    final driven = CallerDrivenSession._(attachment.session);
    try {
      driven.serviceUntil(
        () => _state(driven.session) != RenderSessionState.attaching,
        'attachment',
      );
      expect(_state(driven.session), RenderSessionState.attached);
      final result = body(driven);
      // Work that [body] submitted, such as a readback, runs before the
      // detach, whose submission retires the rendered frame.
      driven.session.serviceDriverWork(0);
      completions.add(driven.session.detach());
      driven.serviceUntil(
        () => _state(driven.session) == RenderSessionState.detached,
        'detach',
      );
      return result;
    } finally {
      _release(driven.session);
    }
  } finally {
    graphics.close();
  }
}

RenderSessionState _state(RenderSessionHandle session) =>
    session.getSnapshot().state;

/// Abandons [session] unless it already released its target, then closes it.
void _release(RenderSessionHandle session) {
  if (session.isClosed) return;
  try {
    session.abandon();
  } on MaplibreException {
    // A detached or abandoned session has nothing left to abandon.
  }
  session.close();
}

BigInt _requestFrame(RenderSessionHandle session, int value) {
  final token = BigInt.from(value);
  session.requestFrame(
    FrameDemand(
      token: token,
      coalescingBoundary: BigInt.zero,
      timeoutNs: BigInt.zero,
    ),
  );
  return token;
}

/// Drains the session's frame results and returns the one for [token], or
/// null while it has not arrived.
RenderFrameResult? _takeResult(RenderSessionHandle session, BigInt token) {
  final RenderFrameBatchHandle batch;
  try {
    batch = session.drainFrameResults();
  } on NotReadyException {
    return null;
  }
  try {
    for (var index = 0; index < batch.count(); index++) {
      final result = batch.getValue(index);
      if (result.token == token) return result;
    }
    return null;
  } finally {
    batch.close();
  }
}
