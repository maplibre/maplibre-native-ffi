// What a garbage collection must leave alive: callbacks that native can still
// call run after one.
import 'dart:async';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

import 'support/collection.dart';
import 'support/fixture.dart';
import 'support/runtime_probes.dart';

/// Creates a map whose runtime only the map and the runtime's event handler
/// name. The handler hands the runtime back once the map's style loads.
Future<MapHandle> _mapWithoutItsRuntime(
  Completer<RuntimeHandle> styleLoaded,
) async {
  final runtime = RuntimeHandle.create(RuntimeOptions());
  await runtime.setEventHandler((events) {
    if (!styleLoaded.isCompleted &&
        events.any((event) => event.type == RuntimeEventType.mapStyleLoaded)) {
      styleLoaded.complete(runtime);
    }
  });
  final NewMap(:map, :ready) = runtime.createMap(testMapOptions());
  await within(ready, 'map creation');
  return map;
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

void main() {
  test('a map keeps its runtime and the runtime callbacks alive', () async {
    final styleLoaded = Completer<RuntimeHandle>();
    final map = await _mapWithoutItsRuntime(styleLoaded);
    // A closure in the helper would share its context with the runtime.
    addTearDown(() => closeIfLive(map.close));
    await collectGarbage();

    // Only the runtime roots its event handler, so the handler runs only if
    // the map kept the runtime through the collection.
    await expectCommitted(map.setStyleJson(emptyStyle));
    final runtime = await within(styleLoaded.future, 'the style load event');
    await within(map.close(), 'map close');
    await within(runtime.close(), 'runtime close');
  });

  for (final identicalCallback in [false, true]) {
    test(
      'wake registrations retain ${identicalCallback ? "identical callbacks" : "equal method tear-offs"} independently',
      () async {
        final roots = WakeRootsProbe();
        final receiver = _WakeReceiver();
        final calls = StreamIterator(receiver.calls.stream);
        final (first, second) = _registerWakes(
          roots,
          receiver,
          identicalCallback,
        );
        await collectGarbage();
        second.notify();
        expect(await within(calls.moveNext(), 'the second wake'), isTrue);
        first.release();
        await collectGarbage();
        second.notify();
        expect(await within(calls.moveNext(), 'the remaining wake'), isTrue);
        second.release();
        // Keep the owner reachable through both registrations' last wakes.
        roots.register(() {}).release();
        await calls.cancel();
        await receiver.calls.close();
      },
    );
  }
}
