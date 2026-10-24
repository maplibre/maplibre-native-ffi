// What the garbage collector may reclaim, and what the binding does with an
// owner it reclaims.
import 'dart:async';
import 'dart:ffi';
import 'dart:io';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.g.dart'
    as raw;
import 'package:maplibre_native_ffi/src/internal/memory/memory.dart';
import 'package:test/test.dart';

import 'support/collection.dart';
import 'support/fixture.dart';
import 'support/runtime_probes.dart';

/// A runtime, by identity and weak reference, that only its callbacks name.
typedef _Unreachable = (int, WeakReference<RuntimeHandle>);

/// Checks that native knows [runtime] while this still holds it: once the
/// caller drops it, any allocation may collect it.
_Unreachable _unreachable(RuntimeHandle runtime) {
  final identity = runtime.id.toSigned(64).toInt();
  expect(_runtimeIsLive(identity), isTrue);
  return (identity, WeakReference(runtime));
}

Future<_Unreachable> _unreachableRuntimeWithEvents() async {
  final runtime = runtimeCreate(runtimeOptionsDefault());
  await runtime.setEventHandler(
    RuntimeEventHandler(
      callback: (_) {
        runtime.id;
      },
    ),
  );
  return _unreachable(runtime);
}

Future<_Unreachable> _unreachableRuntimeWithProvider() async {
  final runtime = runtimeCreate(runtimeOptionsDefault());
  await runtime.setResourceProvider(
    routedProvider([styleRoute('capture://style')], (_, request) {
      runtime.id;
      request.close();
    }),
  );
  return _unreachable(runtime);
}

/// Creates a runtime and a map on it, and keeps only the map.
Future<(MapHandle, WeakReference<RuntimeHandle>)>
_mapWithoutItsRuntime() async {
  final runtime = runtimeCreate(runtimeOptionsDefault());
  final NewMap(:map, :ready) = runtime.createMap(mapOptionsDefault());
  await within(ready, 'map creation');
  return (map, WeakReference(runtime));
}

/// Creates a runtime and keeps only its identity and a weak reference to it.
_Unreachable _abandonedRuntime() =>
    _unreachable(runtimeCreate(runtimeOptionsDefault()));

/// Whether native still knows [runtime], asked through the raw C API, which
/// is the only way to name a runtime that no owner holds.
bool _runtimeIsLive(int runtime) => withNativeArena((arena) {
  final mask = arena<Uint64>();
  return raw.mln_runtime_get_event_mask(runtime, mask, nullptr) ==
      raw.MLN_STATUS_OK;
});

/// Collects what the binding writes to standard error.
final class _CapturedStderr implements Stdout {
  final lines = <String>[];

  @override
  void writeln([Object? object = '']) => lines.add('$object');

  @override
  dynamic noSuchMethod(Invocation invocation) => super.noSuchMethod(invocation);
}

final class _StderrOverrides extends IOOverrides {
  _StderrOverrides(this._stderr);
  final Stdout _stderr;

  @override
  Stdout get stderr => _stderr;
}

final class _WakeReceiver {
  final calls = StreamController<void>();

  void wake() => calls.add(null);
}

(WakeProbe, WakeProbe) _registerWakes(
  WakeRootsProbe roots,
  _WakeReceiver receiver,
  bool identicalCallback,
) {
  final callback = receiver.wake;
  return (
    roots.register(callback),
    roots.register(identicalCallback ? callback : receiver.wake),
  );
}

Future<void> _collectUnrootedObjects() {
  final control = WeakReference(Object());
  return collectGarbageUntil(() => control.target == null, 'a control object');
}

void main() {
  // Capture the leak reports from tests that abandon owners.
  late _CapturedStderr errors;
  setUp(() {
    errors = _CapturedStderr();
    IOOverrides.global = _StderrOverrides(errors);
    addTearDown(() => IOOverrides.global = null);
  });

  test(
    'an abandoned runtime is disposed and reported when collected',
    () async {
      final (identity, runtime) = _abandonedRuntime();
      final leak =
          'Leaked RuntimeHandle native handle 0x${identity.toRadixString(16)}; '
          'close it explicitly.';

      // The collection runs the owner's native finalizer, which disposes the
      // runtime without any Dart code on the stack. The binding's Dart
      // finalizer reports the leak afterwards, from the event loop.
      await collectGarbageUntil(
        () => runtime.target == null && errors.lines.contains(leak),
        'the abandoned runtime and its leak report',
      );
      expect(_runtimeIsLive(identity), isFalse);
    },
  );

  test('callback registrations do not keep their owners reachable', () async {
    final runtimes = [
      await _unreachableRuntimeWithEvents(),
      await _unreachableRuntimeWithProvider(),
    ];
    // Each runtime and the callback that captures it form a cycle through
    // the registration, which the collector reclaims as a whole.
    await collectGarbageUntil(
      () => runtimes.every((runtime) => runtime.$2.target == null),
      'runtimes whose callbacks capture them',
    );
    for (final (identity, _) in runtimes) {
      expect(_runtimeIsLive(identity), isFalse);
    }
  });

  test('a map keeps its runtime reachable', () async {
    final (map, runtime) = await _mapWithoutItsRuntime();
    // The same collections reclaim a runtime that nothing keeps.
    final control = WeakReference(runtimeCreate(runtimeOptionsDefault()));
    await collectGarbageUntil(
      () => control.target == null,
      'a runtime without a map',
    );
    expect(runtime.target, isNotNull);

    await within(map.close(), 'map close');
    await within(runtime.target!.close(), 'runtime close');
  });

  for (final identicalCallback in [false, true]) {
    test(
      'wake ports retain ${identicalCallback ? "identical callbacks" : "equal method tear-offs"} independently',
      () async {
        final roots = WakeRootsProbe();
        final receiver = _WakeReceiver();
        final calls = StreamIterator(receiver.calls.stream);
        final (first, second) = _registerWakes(
          roots,
          receiver,
          identicalCallback,
        );
        await _collectUnrootedObjects();
        second.notify();
        expect(await within(calls.moveNext(), 'the second wake'), isTrue);
        first.release();
        await _collectUnrootedObjects();
        second.notify();
        expect(await within(calls.moveNext(), 'the remaining wake'), isTrue);
        second.release();
        // Keep the owner reachable through both ports' last wakes.
        roots.register(() {}).release();
        await calls.cancel();
        await receiver.calls.close();
      },
    );
  }
}
