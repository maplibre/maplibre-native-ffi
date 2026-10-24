// Owner handles: close-once, refused closes, and use across isolate hops.
import 'dart:isolate';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

import 'support/fixture.dart';

/// A handle describes itself by its type, id, and whether it is closed.
Matcher _describes(String typeName, int id, {required bool closed}) =>
    equals('$typeName(id=0x${id.toRadixString(16)}, closed=$closed)');

void main() {
  test('a second close is a no-op and the handle reports it', () async {
    final fixture = await openRuntime();
    final map = await fixture.openMap();
    final projection = mapProjectionCreate(map.getSnapshot().view);

    expect(projection.id, isNot(0));
    expect(projection.isClosed, isFalse);
    expect(
      projection.toString(),
      _describes('MapProjectionHandle', projection.id, closed: false),
    );
    projection.close();
    expect(projection.isClosed, isTrue);
    projection.close();
    expect(
      '$projection',
      _describes('MapProjectionHandle', projection.id, closed: true),
    );
  });

  test('a second dispose is a no-op and the handle reports it', () {
    final runtime = runtimeCreate(runtimeOptionsDefault());
    addTearDown(() => closeIfLive(runtime.close));

    expect(runtime.isClosed, isFalse);
    expect('$runtime', _describes('RuntimeHandle', runtime.id, closed: false));
    runtime.dispose();
    expect(runtime.isClosed, isTrue);
    runtime.dispose();
    expect('$runtime', _describes('RuntimeHandle', runtime.id, closed: true));
  });

  test('repeated asynchronous release reaches native', () async {
    final fixture = await openRuntime();
    final map = await fixture.openMap();

    await within(map.close(), 'map close');
    expect(map.isClosed, isTrue);
    expect(
      map.close,
      throwsA(
        isA<MaplibreException>().having(
          (error) => error.status,
          'status',
          Status.invalidState,
        ),
      ),
    );
    expect(
      map.getSnapshot,
      throwsA(
        isA<MaplibreException>().having(
          (error) => error.status,
          'status',
          Status.invalidState,
        ),
      ),
    );
  });

  test('a handle id names its events and outlives close', () async {
    final fixture = await openRuntime();
    final map = await fixture.openStyledMap();
    final loaded = await fixture.awaitEventType(
      RuntimeEventType.mapStyleLoaded,
    );
    expect(loaded.source, map.id);

    await within(map.close(), 'map close');
    // A host routes events that it drains after close by the id it kept.
    expect(map.id, loaded.source);
  });

  test('a refused close leaves the handle usable for a later close', () async {
    final runtime = runtimeCreate(runtimeOptionsDefault());
    final map = await within(
      runtime.createMap(mapOptionsDefault()),
      'map creation',
    );
    addTearDown(() async {
      await closeIfLive(map.close);
      await closeIfLive(runtime.close);
    });

    // The live map makes native refuse the release before teardown starts.
    expect(
      runtime.close,
      throwsA(
        isA<MaplibreException>().having(
          (error) => error.status,
          'status',
          Status.invalidState,
        ),
      ),
    );
    await within(
      runtime.clearResourceTransform(),
      'a command on the refused runtime',
    );

    await within(map.close(), 'map close');
    await within(runtime.close(), 'runtime close');
  });

  test('handles stay usable after an isolate hop', () async {
    final fixture = await openRuntime();
    final map = await fixture.openMap();
    final before = map.getSnapshot();

    // Awaiting another isolate can resume this one on a different thread,
    // and every handle and the per-isolate diagnostic must survive that.
    await Isolate.run(() {});

    final camera = await within(map.getCamera(), 'camera query');
    expect(camera.generation, greaterThanOrEqualTo(before.generation));
    expect(
      () => fixture.runtime.setEventMask(
        const RuntimeEventMask.fromRawValue(1 << 40),
      ),
      throwsA(
        isA<MaplibreException>()
            .having((error) => error.status, 'status', Status.invalidArgument)
            .having((error) => error.diagnostic, 'diagnostic', isNotEmpty),
      ),
    );
  });
}
