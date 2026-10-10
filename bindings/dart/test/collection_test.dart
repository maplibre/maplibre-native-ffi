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

/// A runtime, by identity and weak reference, that only its callbacks name.
typedef _Unreachable = (int, WeakReference<RuntimeHandle>);

/// Checks that native knows [runtime] while this still holds it: once the
/// caller drops it, any allocation may collect it.
_Unreachable _unreachable(RuntimeHandle runtime) {
  final identity = runtime.identity.toSigned(64).toInt();
  expect(_runtimeIsLive(identity), isTrue);
  return (identity, WeakReference(runtime));
}

_Unreachable _unreachableRuntimeWithWake() {
  late final RuntimeHandle runtime;
  runtime = runtimeCreate(
    RuntimeOptions(
      eventWake: Wake(
        callback: () {
          runtime.identity;
        },
      ),
    ),
  );
  return _unreachable(runtime);
}

Future<_Unreachable> _unreachableRuntimeWithProvider() async {
  final runtime = runtimeCreate(runtimeOptionsDefault());
  await runtime.setResourceProvider(
    routedProvider([styleRoute('capture://style')], (_, request) {
      runtime.identity;
      request.close();
    }),
  );
  return _unreachable(runtime);
}

/// Creates a runtime and a map on it, and keeps only the map.
Future<(MapHandle, WeakReference<RuntimeHandle>)>
_mapWithoutItsRuntime() async {
  final runtime = runtimeCreate(runtimeOptionsDefault());
  final map = await within(
    runtime.createMap(mapOptionsDefault()),
    'map creation',
  );
  return (map, WeakReference(runtime));
}

/// Starts a map creation and drops its future, keeping a weak reference to
/// the map it delivers.
Future<WeakReference<MapHandle>> _droppedMapCreation(
  RuntimeHandle runtime,
) async {
  WeakReference<MapHandle>? delivered;
  unawaited(
    runtime
        .createMap(mapOptionsDefault())
        .then((map) => delivered = WeakReference(map)),
  );
  // The creation's completion reaches the shared port before the barrier's.
  await within(runtime.barrier(), 'a barrier');
  return delivered!;
}

/// Creates a runtime and keeps only its identity and a weak reference to it.
_Unreachable _abandonedRuntime() =>
    _unreachable(runtimeCreate(runtimeOptionsDefault()));

/// Whether native still knows [runtime], asked through the raw C API, which
/// is the only way to name a runtime that no owner holds.
bool _runtimeIsLive(int runtime) => withNativeArena((arena) {
  final batch = arena<Uint64>();
  final status = raw.mln_runtime_drain_events(runtime, batch, nullptr);
  if (status == 0) raw.mln_event_batch_release(batch.value);
  return status == 0;
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

void main() {
  // Every test here abandons owners on purpose, so each one collects the leak
  // reports instead of writing them to the suite's output.
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
      _unreachableRuntimeWithWake(),
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

  test('a dropped creation is disposed once its owner is collected', () async {
    final runtime = runtimeCreate(runtimeOptionsDefault());
    addTearDown(runtime.close);
    final map = await _droppedMapCreation(runtime);
    expect(map.target, isNotNull);
    // The live map holds off the runtime's release.
    expect(runtime.close, throwsA(isA<InvalidStateException>()));

    // Collecting the map runs its native finalizer, which disposes the map
    // outside any callback, and then the runtime releases.
    await collectGarbageUntil(() => map.target == null, 'the dropped map');
    await within(runtime.barrier(), 'a barrier');
    await within(runtime.close(), 'runtime close');
  });
}
