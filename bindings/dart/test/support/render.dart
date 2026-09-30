/// An owned-texture render session on the build's backend, driven by its
/// wakes.
library;

import 'dart:async';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

import 'fixture.dart';
import 'graphics.dart';

const renderSize = 32;

final class RenderFixture {
  RenderFixture._(this.runtime, this.map, this.session, this.callerDriven);

  final RuntimeFixture runtime;
  final MapHandle map;
  final RenderSessionHandle session;
  final bool callerDriven;
  final _wakes = Signal();
  var _nextToken = 1;

  /// Attaches a session with [driver], or with the backend's own driver when
  /// null: the core worker, except on OpenGL, whose shared context takes the
  /// caller driver. Teardowns abandon and close the session before its map.
  static Future<RenderFixture> open({RenderDriverKind? driver}) async {
    final graphics = TestGraphics.open();
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
    final selected =
        driver ??
        (graphics.isOpengl
            ? RenderDriverKind.callerGraphicsThread
            : RenderDriverKind.coreWorker);
    late final RenderFixture fixture;
    final attachment = graphics.attachOwnedTexture(
      map,
      const RenderTargetExtent(
        width: renderSize,
        height: renderSize,
        scaleFactor: 1,
      ),
      RenderSessionAttachOptions(
        driver: selected,
        requestedTextureRingDepth: 2,
        frameWake: Wake(callback: () => fixture._wakes.notify()),
        driverWorkWake: Wake(callback: () => fixture._wakes.notify()),
      ),
    );
    fixture = RenderFixture._(
      runtime,
      map,
      attachment.session,
      selected == RenderDriverKind.callerGraphicsThread,
    );
    addTearDown(() {
      if (fixture.session.isClosed) return;
      try {
        fixture.session.abandon();
      } on MaplibreException {
        // A detached or abandoned session has nothing left to abandon.
      }
      fixture.session.close();
    });
    await fixture.drive(attachment.completed, 'attachment');
    return fixture;
  }

  /// Awaits [future], servicing a caller driver's work each time its wakes
  /// fire until the future completes.
  Future<T> drive<T>(Future<T> future, String what) async {
    if (!callerDriven) return within(future, what);
    var finished = false;
    final result = future.whenComplete(() {
      finished = true;
      _wakes.notify();
    });
    // Keeps an error from reaching the zone before the loop returns it.
    unawaited(result.then<void>((_) {}, onError: (_) {}));
    while (!finished) {
      session.serviceDriverWork(0);
      if (finished) break;
      await _wakes.wait(what);
    }
    return result;
  }

  /// Demands frames until one renders, and returns its result.
  Future<RenderFrameResult> renderFrame() async {
    while (true) {
      final token = BigInt.from(_nextToken++);
      session.requestFrame(
        FrameDemand(
          token: token,
          coalescingBoundary: BigInt.zero,
          timeoutNs: BigInt.zero,
        ),
      );
      final result = await _awaitResult(token);
      if (result.disposition == RenderResult.rendered) return result;
    }
  }

  Future<RenderFrameResult> _awaitResult(BigInt token) async {
    while (true) {
      if (callerDriven) session.serviceDriverWork(0);
      for (final result in _drainResults()) {
        if (result.token == token) return result;
      }
      await _wakes.wait('frame $token');
    }
  }

  List<RenderFrameResult> _drainResults() {
    final RenderFrameBatchHandle batch;
    try {
      batch = session.drainFrameResults();
    } on NotReadyException {
      return const [];
    }
    try {
      return [for (var i = 0; i < batch.count(); i++) batch.getValue(i)];
    } finally {
      batch.close();
    }
  }
}
