// Owner handles: close-once, refused closes, and use across isolate hops.
import 'dart:isolate';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

import 'support/fixture.dart';

void main() {
  test(
    'closing a handle twice is safe and the second close does nothing',
    () async {
      final fixture = await openRuntime();
      final map = await fixture.openMap();
      final projection = await map.projectionCreate();

      projection.close();
      projection.close();
      expect(projection.isClosed, isTrue);

      // An asynchronous close hands a second caller the first close's future.
      final first = map.close();
      final second = map.close();
      await within(first, 'map close');
      await within(second, 'the second map close');
      expect(map.isClosed, isTrue);

      // A closed owner refuses use without reaching native code.
      expect(
        map.snapshotGet,
        throwsA(
          isA<InvalidStateException>()
              .having((error) => error.nativeStatusCode, 'status', isNull)
              .having(
                (error) => error.diagnostic,
                'diagnostic',
                'MapHandle is closed',
              ),
        ),
      );
    },
  );

  test('a refused close leaves the handle usable for a later close', () async {
    final runtime = runtimeCreate(runtimeOptionsDefault());
    final map = await within(
      runtime.mapCreate(mapOptionsDefault()),
      'map creation',
    );
    addTearDown(() async {
      await map.close();
      await runtime.close();
    });

    // The live map makes native refuse the release before teardown starts.
    expect(runtime.close, throwsA(isA<InvalidStateException>()));
    expect(runtime.isClosed, isFalse);
    await within(runtime.barrier(), 'a barrier on the refused runtime');

    await within(map.close(), 'map close');
    await within(runtime.close(), 'runtime close');
    expect(runtime.isClosed, isTrue);
  });

  test('handles stay usable after an isolate hop', () async {
    final fixture = await openRuntime();
    final map = await fixture.openMap();
    final before = map.snapshotGet();

    // Awaiting another isolate can resume this one on a different thread,
    // and every handle and the per-isolate diagnostic must survive that.
    await Isolate.run(() {});

    final camera = await within(map.cameraQuery(), 'camera query');
    expect(camera.generation, greaterThanOrEqualTo(before.generation));
    expect(
      () => fixture.runtime.setEventMask(
        const RuntimeEventMask.fromRawValue(1 << 40),
      ),
      throwsA(
        isA<InvalidArgumentException>().having(
          (error) => error.diagnostic,
          'diagnostic',
          isNotEmpty,
        ),
      ),
    );
  });
}
