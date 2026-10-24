// Queue-delivered callback registrations: each one roots its callbacks for as
// long as native holds it, and retires when native retires it.
import 'dart:async';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'support/runtime_probes.dart';
import 'package:test/test.dart';

import 'support/fixture.dart';

CustomGeometrySourceOptions _geometrySource({int? tileSize}) =>
    CustomGeometrySourceOptions(fetchTile: (_) {}, tileSize: tileSize);

void main() {
  test(
    'a cancel-only source surfaces native rejection without rooting',
    () async {
      final fixture = await openRuntime();
      final map = await fixture.openStyledMap();
      for (final add in [
        () => map.addCustomGeometrySource(
          'cancel-only-geometry',
          CustomGeometrySourceOptions(cancelTile: (_) {}),
        ),
        () => map.addCustomMvtVectorSource(
          'cancel-only-mvt',
          CustomMvtVectorSourceOptions(cancelTile: (_) {}),
        ),
      ]) {
        expect(
          add,
          throwsA(
            isA<MaplibreException>().having(
              (error) => error.status,
              'status',
              Status.invalidArgument,
            ),
          ),
        );
        expect(singleRegistrationProbeForTesting(map), isNull);
      }
    },
  );

  test(
    'a callback root is released by a removal, a map close, or a refusal',
    () async {
      final fixture = await openRuntime();
      final map = await fixture.openStyledMap();

      await expectCommitted(
        map.addCustomGeometrySource('source', _geometrySource()),
      );
      final removed = singleRegistrationProbeForTesting(map)!;
      expect(removed.closed, isFalse);
      await expectCommitted(map.removeStyleSource('source'));
      await within(removed.released, 'the release after a removal');
      expect(singleRegistrationProbeForTesting(map), isNull);

      // A registration that never reaches native is released at once.
      expect(
        () => map.addCustomGeometrySource(
          'refused',
          _geometrySource(tileSize: -1),
        ),
        throwsA(
          isA<MaplibreException>().having(
            (error) => error.status,
            'status',
            Status.invalidArgument,
          ),
        ),
      );
      expect(singleRegistrationProbeForTesting(map), isNull);

      await expectCommitted(
        map.addCustomGeometrySource('source', _geometrySource()),
      );
      final closed = singleRegistrationProbeForTesting(map)!;
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
            camera: CameraOptions(zoom: 4),
            mode: CameraUpdateMode.ease,
            animation: AnimationOptions(
              durationMs: 60000,
              transitionId: 3,
              endHandler: CameraTransitionHandler(callback: ended.complete),
            ),
          ),
        ),
      );
      final probe = singleRegistrationProbeForTesting(map)!;
      expect(probe.closed, isFalse);
      await expectCommitted(map.cancelCameraTransition(3));
      final end = await within(ended.future, 'the end handler');
      expect(end.outcome, CameraTransitionOutcome.cancelled);
      expect(end.generation, isNot(0));
      await within(probe.released, 'the release after the end');
      expect(singleRegistrationProbeForTesting(map), isNull);

      expect(
        () => map.applyCameraDelta(
          CameraDelta(
            scale: -1,
            animation: AnimationOptions(
              endHandler: CameraTransitionHandler(callback: (_) {}),
            ),
          ),
        ),
        throwsA(
          isA<MaplibreException>().having(
            (error) => error.status,
            'status',
            Status.invalidArgument,
          ),
        ),
      );
      expect(singleRegistrationProbeForTesting(map), isNull);
    },
  );
}
