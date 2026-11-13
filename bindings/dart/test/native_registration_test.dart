// Native registrations: wakes and queue locks whose function pointers and
// context native code in the host supplies. Only a Vulkan device takes a queue
// lock, so an accepted lock needs a device that wraps a Vulkan context from
// tests/graphics.
import 'dart:ffi';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

import 'support/fixture.dart';
import 'support/graphics.dart';
import 'support/render.dart';

typedef _ContextCallback = Void Function(Pointer<Void>);

final _size = LogicalSize(width: renderSize, height: renderSize);

/// Only a Vulkan device takes a queue lock.
final Object _vulkanSkip = buildAttachesVulkan
    ? false
    : 'Only a Vulkan device takes a queue lock';

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

RenderSessionAttachOptions _options({Wake frameWake = const Wake()}) =>
    RenderSessionAttachOptions(
      driver: RenderDriverKind.coreWorker,
      frameWake: frameWake,
    );

Matcher _throwsStatus(Status status) => throwsA(
  isA<MaplibreException>().having((error) => error.status, 'status', status),
);

void main() {
  test(
    'a native wake reaches native unchanged and only its first call',
    () async {
      final map = await openRenderMap(emptyStyle);
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
        () => attachOffscreenSession(map, _options(frameWake: rejected)),
        _throwsStatus(Status.invalidArgument),
      );
      await releases.wait('the release of the rejected wake');
      expect(releases.contexts, [1]);
      // Native code would reject the wake again with an invalid argument, so
      // an invalid state shows that the second call never reached it.
      expect(
        () => attachOffscreenSession(map, _options(frameWake: rejected)),
        _throwsStatus(Status.invalidState),
      );

      final session = attachOffscreenSession(
        map,
        _options(
          frameWake: Wake.native(
            callback: wakes.pointer,
            userData: const NativePointer(2),
            releaseUserData: releases.pointer,
          ),
        ),
      );
      addTearDown(() => releaseSession(session));
      await within(session.ready, 'attachment');

      session.demandFrame(FrameDemand(flags: FrameDemandFlag.force, token: 1));
      await wakes.wait('the frame wake');
      expect(wakes.contexts.toSet(), {2});

      // Native code releases the wake that it accepted once the session can
      // no longer call it.
      await within(session.release(), 'session release');
      await releases.wait('the release of the accepted wake');
      expect(releases.contexts, [1, 2]);
      wakes.close();
      releases.close();
    },
  );

  test('a native queue lock reaches native with its context', () async {
    final map = await openRenderMap(emptyStyle);
    // The session takes the lock around its queue calls, which this listener
    // answers at once. The host shares no queue with the session.
    final locks = _Recorder();
    final releases = _Recorder();
    final graphics = TestGraphics.open();
    final device = graphics.wrapDevice(queueLock: _queueLock(locks, releases));
    final session = graphics.attachBorrowedTexture(
      map,
      _size,
      _options(),
      device: device,
    );
    addTearDown(() => releaseSession(session));
    // The session keeps the device, and the device the lock, until the
    // session ends.
    device.release();
    await within(session.ready, 'attachment');
    expect(releases.contexts, isEmpty);
    await within(session.release(), 'session release');
    await releases.wait('the release of the accepted queue lock');
    expect(releases.contexts, [3]);
    locks.close();
    releases.close();
  }, skip: _vulkanSkip);

  test('a rejected native queue lock releases its context', () async {
    final locks = _Recorder();
    final releases = _Recorder();
    // This build lacks the backend, so native code rejects the device, and
    // the binding then releases the context of the rejected lock.
    expect(
      () => GpuDeviceHandle.create(
        options: GpuDeviceOptions(
          backend: _missingBackend(),
          queueLock: _queueLock(locks, releases),
        ),
      ),
      _throwsStatus(Status.unsupported),
    );
    await releases.wait('the release of the rejected queue lock');
    expect(releases.contexts, [3]);
    locks.close();
    releases.close();
  });
}

/// A backend that this build lacks.
RenderBackend _missingBackend() {
  final supported = supportedRenderBackendMask();
  for (final (backend, flag) in [
    (RenderBackend.metal, RenderBackendFlag.metal),
    (RenderBackend.vulkan, RenderBackendFlag.vulkan),
    (RenderBackend.openGl, RenderBackendFlag.openGl),
    (RenderBackend.webGpu, RenderBackendFlag.webGpu),
  ]) {
    if (!supported.contains(flag)) return backend;
  }
  fail('this build has every backend');
}

/// A queue lock whose callbacks both record on [locks], with a context that
/// [releases] records once native code releases it.
QueueLock _queueLock(_Recorder locks, _Recorder releases) => QueueLock.native(
  lock: locks.pointer,
  unlock: locks.pointer,
  userData: const NativePointer(3),
  releaseUserData: releases.pointer,
);
