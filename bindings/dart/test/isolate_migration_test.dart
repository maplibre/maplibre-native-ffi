import 'dart:isolate';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

void main() {
  test(
    'runtime and map remain usable after isolate execution and await',
    () async {
      final runtime = await RuntimeHandle.create(
        options: const RuntimeOptions(cachePath: ':memory:'),
      );
      final map = await runtime.createMap(
        options: const MapOptions(width: 64, height: 64),
      );

      await Isolate.run(() {});

      final before = map.snapshot();
      final commandId = map.requestRepaint();
      final camera = await map.queryCamera();
      expect(commandId, greaterThan(BigInt.zero));
      expect(camera.generation, greaterThanOrEqualTo(before.generation));

      await map.close();
      await runtime.close();
      expect(map.isClosed, isTrue);
      expect(runtime.isClosed, isTrue);
    },
  );

  test(
    'native execution progresses while the isolate stays responsive',
    () async {
      final runtime = await RuntimeHandle.create(
        options: const RuntimeOptions(cachePath: ':memory:'),
      );
      final map = await runtime.createMap();
      try {
        var timerRan = false;
        Future<void>.delayed(Duration.zero, () {
          timerRan = true;
        });
        final camera = await map.queryCamera();

        expect(timerRan, isTrue);
        expect(camera.generation, greaterThanOrEqualTo(BigInt.zero));
      } finally {
        await map.close();
        await runtime.close();
      }
    },
  );
}
