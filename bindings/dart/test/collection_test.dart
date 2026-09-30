// What the garbage collector may reclaim, and what a native finalizer does
// with an owner it reclaims.
import 'dart:async';
import 'dart:ffi';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.g.dart'
    as raw;
import 'package:maplibre_native_ffi/src/internal/memory/memory.dart';
import 'package:test/test.dart';

import 'support/collection.dart';
import 'support/fixture.dart';

WeakReference<RuntimeHandle> _unreachableRuntimeWithWake() {
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
  return WeakReference(runtime);
}

Future<WeakReference<RuntimeHandle>> _unreachableRuntimeWithProvider() async {
  final runtime = runtimeCreate(runtimeOptionsDefault());
  await runtime.setResourceProvider(
    routedProvider([styleRoute('capture://style')], (_, request) {
      runtime.identity;
      request.close();
    }),
  );
  return WeakReference(runtime);
}

/// Creates a runtime and a map on it, and keeps only the map.
Future<(MapHandle, WeakReference<RuntimeHandle>)>
_mapWithoutItsRuntime() async {
  final runtime = runtimeCreate(runtimeOptionsDefault());
  final map = await within(
    runtime.mapCreate(mapOptionsDefault()),
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
        .mapCreate(mapOptionsDefault())
        .then((map) => delivered = WeakReference(map)),
  );
  // The creation's completion reaches the shared port before the barrier's.
  await within(runtime.barrier(), 'a barrier');
  return delivered!;
}

/// Creates a runtime and keeps only its identity and a weak reference to it.
(int, WeakReference<RuntimeHandle>) _abandonedRuntime() {
  final runtime = runtimeCreate(runtimeOptionsDefault());
  return (runtime.identity.toSigned(64).toInt(), WeakReference(runtime));
}

/// Whether native still knows [runtime], asked through the raw C API, which
/// is the only way to name a runtime that no owner holds.
bool _runtimeIsLive(int runtime) => withNativeArena((arena) {
  final batch = arena<Uint64>();
  final status = raw.mln_runtime_drain_events(runtime, batch, nullptr);
  if (status == 0) raw.mln_event_batch_release(batch.value);
  return status == 0;
});

void main() {
  test(
    'an abandoned runtime is disposed when the collector reclaims it',
    () async {
      final (identity, runtime) = _abandonedRuntime();
      expect(_runtimeIsLive(identity), isTrue);

      // The collection runs the owner's native finalizer, which disposes the
      // runtime without any Dart code on the stack.
      await collectGarbageUntil(
        () => runtime.target == null,
        'the abandoned runtime',
      );
      expect(_runtimeIsLive(identity), isFalse);
    },
  );

  test('callback registrations do not keep their owners reachable', () async {
    final references = [
      _unreachableRuntimeWithWake(),
      await _unreachableRuntimeWithProvider(),
    ];
    await collectGarbageUntil(
      () => references.every((reference) => reference.target == null),
      'runtimes whose callbacks capture them',
    );
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
