// Port-delivered callback registrations: each one roots its callback for as
// long as native holds it, and retires when native releases it.
import 'dart:async';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'support/runtime_probes.dart';
import 'package:test/test.dart';

import 'support/fixture.dart';

CustomGeometrySourceOptions _geometrySource({int? tileSize}) =>
    CustomGeometrySourceOptions(fetchTile: (_) {}, tileSize: tileSize);

void main() {
  test(
    'a callback root is released by a removal, a map close, or a refusal',
    () async {
      final fixture = await openRuntime();
      final map = await fixture.openStyledMap();

      await expectCommitted(
        map.addCustomGeometrySource('source', _geometrySource()),
      );
      final removed = singleCallbackPortProbeForTesting(map)!;
      expect(removed.closed, isFalse);
      await expectCommitted(map.removeStyleSource('source'));
      await within(removed.released, 'the release after a removal');
      expect(singleCallbackPortProbeForTesting(map), isNull);

      // A registration that never reaches native is released at once.
      expect(
        () => map.addCustomGeometrySource(
          'refused',
          _geometrySource(tileSize: -1),
        ),
        throwsA(isA<InvalidArgumentException>()),
      );
      expect(singleCallbackPortProbeForTesting(map), isNull);

      await expectCommitted(
        map.addCustomGeometrySource('source', _geometrySource()),
      );
      final closed = singleCallbackPortProbeForTesting(map)!;
      await within(map.close(), 'map close');
      await within(closed.released, 'the release after a map close');
    },
  );

  // A camera command carries its end handler two records deep in its input.
  // The handler's root lasts until native runs the handler once and releases
  // it, and a command that never reaches native roots nothing.
  test(
    'a camera end handler root is released after the handler runs',
    () async {
      final fixture = await openRuntime();
      final map = await fixture.openStyledMap();
      final ended = Completer<CameraTransitionEnd>();
      await expectCommitted(
        map.updateCamera(
          CameraUpdate(
            camera: const CameraOptions(zoom: 4),
            mode: CameraUpdateMode.ease,
            animation: AnimationOptions(
              durationMs: 60000,
              transitionId: BigInt.from(3),
              endHandler: CameraTransitionHandler(callback: ended.complete),
            ),
          ),
        ),
      );
      final probe = singleCallbackPortProbeForTesting(map)!;
      expect(probe.closed, isFalse);
      await expectCommitted(map.cancelCameraTransition(BigInt.from(3)));
      final end = await within(ended.future, 'the end handler');
      expect(end.outcome, CameraTransitionOutcome.cancelled);
      expect(end.generation, isNot(BigInt.zero));
      await within(probe.released, 'the release after the end');
      expect(singleCallbackPortProbeForTesting(map), isNull);

      expect(
        () => map.applyCameraDelta(
          CameraDelta(
            scale: -1,
            animation: AnimationOptions(
              endHandler: CameraTransitionHandler(callback: (_) {}),
            ),
          ),
        ),
        throwsA(isA<InvalidArgumentException>()),
      );
      expect(singleCallbackPortProbeForTesting(map), isNull);
    },
  );
}
