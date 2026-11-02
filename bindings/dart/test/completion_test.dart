// Completions: discarded commands, conversion failures, failed commands, and
// waits that give up.
import 'dart:async';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.dart'
    show nativeDiagnostic;
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.g.dart'
    as raw;
import 'package:maplibre_native_ffi/src/internal/callback/delivery_queue.dart';
import 'package:test/test.dart';

import 'support/fixture.dart';

void main() {
  test(
    'discarded commands are fenced and synchronous rejection throws',
    () async {
      final fixture = await openRuntime();
      final map = await fixture.openMap();
      map.discarding.setDebugOptions(MapDebugFlag.tileBorders);
      // A later command on the same map completes after the discarded one.
      await within(map.requestRepaint(), 'a later command on the map');
      expect(map.getSnapshot().debugOptions, MapDebugFlag.tileBorders);
      expect(
        () => map.discarding.resize(
          LogicalExtent(width: 0, height: 8, scaleFactor: 1),
        ),
        throwsA(
          isA<MaplibreException>().having(
            (error) => error.status,
            'status',
            Status.invalidArgument,
          ),
        ),
      );
    },
  );

  test(
    'a completion delivers once and reports a value that fails to convert',
    () async {
      final fixture = await openRuntime();
      final map = await fixture.openMap();
      var decodes = 0;
      final camera = startNativeCompletion<void>(
        start: (completion) => raw.mln_map_get_camera(
          map.id.toSigned(64).toInt(),
          completion,
          nativeDiagnostic,
        ),
        decode: (_) {
          decodes += 1;
          throw StateError('the camera failed to convert');
        },
      );
      await expectLater(within(camera, 'the camera query'), throwsStateError);
      await within(map.requestRepaint(), 'a later command on the map');
      expect(decodes, 1);
    },
  );

  test(
    'a failed command fails its future with its status and generation',
    () async {
      final fixture = await openRuntime();
      final map = await fixture.openStyledMap();

      final MaplibreException error;
      try {
        await within(map.removeStyleSource('missing-source'), 'source removal');
        fail('the removal of a missing source unexpectedly committed');
      } on MaplibreException catch (caught) {
        error = caught;
      }
      expect(error.status, Status.notFound);
      expect(error.diagnostic, isNotEmpty);
      expect(error.generation, map.getSnapshot().generation);
    },
  );

  test('a wait that times out leaves its completion to arrive later', () async {
    final fixture = await openRuntime();
    final map = await fixture.openMap(
      MapOptions(
        mapMode: MapMode.static,
        initialExtent: LogicalExtent(width: 64, height: 64, scaleFactor: 1),
        eventMask: RuntimeEventFlag.all,
      ),
    );
    await expectCommitted(map.setStyleJson(emptyStyle));

    // A static map with no render session never produces the image, so the
    // request stays owed. A future cannot be cancelled in Dart; a waiter
    // gives up on it with a timeout instead.
    final image = map.requestStillImage();
    await expectLater(
      image.timeout(Duration.zero),
      throwsA(isA<TimeoutException>()),
    );

    // The binding still delivers the completion to the future the waiter
    // gave up on, once closing the map cancels the request.
    final cancelled = expectLater(
      image,
      throwsA(
        isA<MaplibreException>().having(
          (error) => error.status,
          'status',
          Status.cancelled,
        ),
      ),
    );
    await within(map.close(), 'map close');
    await within(cancelled, 'the cancelled still image');
  });
}
