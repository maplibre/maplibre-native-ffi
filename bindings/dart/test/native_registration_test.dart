// Native registrations: wakes and queue locks whose function pointers and
// context native code in the host supplies.
import 'dart:ffi';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

import 'support/fixture.dart';
import 'support/graphics.dart';
import 'support/render.dart';

typedef _ContextCallback = Void Function(Pointer<Void>);

final _extent = LogicalExtent(
  width: renderSize,
  height: renderSize,
  scaleFactor: 1,
);

/// A WGL texture shares the host's context, which only a caller-driven session
/// can drive, and a Dart isolate services core-worker sessions only.
final Object _wglSkip = buildHasCoreWorkerTexture
    ? false
    : 'A WGL texture has no core-worker session for a Dart isolate to attach';

/// A native function that any thread may call with a context, which records
/// the context on this isolate.
final class _Recorder {
  _Recorder() {
    _callable.keepIsolateAlive = false;
  }

  late final _callable = NativeCallable<_ContextCallback>.listener(_record);
  final contexts = <int>[];
  final _signal = Signal();

  NativePointer get pointer => NativePointer(_callable.nativeFunction.address);

  void _record(Pointer<Void> context) {
    contexts.add(context.address);
    _signal.notify();
  }

  Future<void> wait(String what) => _signal.wait(what);

  /// Closes the function, which native code must no longer call.
  void close() => _callable.close();
}

RenderSessionAttachOptions _options({Wake? frameWake, QueueLock? queueLock}) =>
    RenderSessionAttachOptions(
      driver: RenderDriverKind.coreWorker,
      requestedTextureRingDepth: 1,
      frameWake: frameWake,
      queueLock: queueLock,
    );

Matcher _throwsStatus(Status status) => throwsA(
  isA<MaplibreException>().having((error) => error.status, 'status', status),
);

void main() {
  test(
    'a native wake reaches native unchanged and only its first call',
    () async {
      final map = await openRenderMap(emptyStyleJson);
      final graphics = TestGraphics.open();
      final wakes = _Recorder();
      final releases = _Recorder();

      // Native code rejects a disabled wake that carries a release, so the
      // binding releases the context itself.
      final rejected = Wake.native(
        callback: NativePointer.nullPointer,
        userData: const NativePointer(1),
        releaseUserData: releases.pointer,
      );
      expect(
        () => graphics.attachOwnedTexture(
          map,
          _extent,
          _options(frameWake: rejected),
        ),
        _throwsStatus(Status.invalidArgument),
      );
      await releases.wait('the release of the rejected wake');
      expect(releases.contexts, [1]);
      // Native code would reject the wake again with an invalid argument, so
      // an invalid state shows that the second call never reached it.
      expect(
        () => graphics.attachOwnedTexture(
          map,
          _extent,
          _options(frameWake: rejected),
        ),
        _throwsStatus(Status.invalidState),
      );

      final attachment = graphics.attachOwnedTexture(
        map,
        _extent,
        _options(
          frameWake: Wake.native(
            callback: wakes.pointer,
            userData: const NativePointer(2),
            releaseUserData: releases.pointer,
          ),
        ),
      );
      final session = attachment.session;
      addTearDown(() => releaseSession(session));
      await within(attachment.completed, 'attachment');

      session.requestFrame(
        FrameDemand(
          flags: const FrameDemandFlag.fromRawValue(0),
          token: BigInt.one,
        ),
      );
      await wakes.wait('the frame wake');
      expect(wakes.contexts.toSet(), {2});

      // Native code releases the wake that it accepted once the session can
      // no longer call it.
      await within(session.close(), 'session close');
      await releases.wait('the release of the accepted wake');
      expect(releases.contexts, [1, 2]);
      wakes.close();
      releases.close();
    },
    skip: _wglSkip,
  );

  test('a native queue lock reaches native with its context', () async {
    final map = await openRenderMap(emptyStyleJson);
    final graphics = TestGraphics.open();
    // A Vulkan session takes the lock around its queue calls, which this
    // listener answers at once. The host shares no queue with the session.
    final locks = _Recorder();
    final releases = _Recorder();
    final queueLock = QueueLock.native(
      lock: locks.pointer,
      unlock: locks.pointer,
      userData: const NativePointer(3),
      releaseUserData: releases.pointer,
    );

    if (buildAttachesVulkan) {
      final attachment = graphics.attachOwnedTexture(
        map,
        _extent,
        _options(queueLock: queueLock),
      );
      addTearDown(() => releaseSession(attachment.session));
      await within(attachment.completed, 'attachment');
      await within(attachment.session.close(), 'session close');
      await releases.wait('the release of the accepted queue lock');
    } else {
      // Only a Vulkan target takes a lock. Native code reaches that check
      // only with both lock callbacks set, and the binding then releases the
      // context of the rejected lock.
      expect(
        () => graphics.attachOwnedTexture(
          map,
          _extent,
          _options(queueLock: queueLock),
        ),
        _throwsStatus(Status.unsupported),
      );
      await releases.wait('the release of the rejected queue lock');
    }
    expect(releases.contexts, [3]);
    locks.close();
    releases.close();
  }, skip: _wglSkip);
}
