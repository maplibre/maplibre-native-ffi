// Completions: the shared receive port, conversion failures, rejected
// submissions, failed commands, and waits that give up.
import 'dart:async';
import 'dart:ffi';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.dart'
    show nativeDiagnostic;
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.g.dart'
    as raw;
import 'package:maplibre_native_ffi/src/internal/callback/delivery_queue.dart';
import 'package:maplibre_native_ffi/src/internal/memory/memory.dart';
import 'support/runtime_probes.dart';
import 'package:test/test.dart';

import 'support/fixture.dart';

void main() {
  test(
    'discarded commands are fenced and synchronous rejection throws',
    () async {
      final fixture = await openRuntime();
      final map = await fixture.openMap();
      map.discarding.setDebugOptions(MapDebugOption.tileBorders);
      // A later command on the same map completes after the discarded one.
      await within(map.requestRepaint(), 'a later command on the map');
      expect(map.getSnapshot().debugOptions, MapDebugOption.tileBorders);
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
    'a completion delivers once and disposes a value that fails to convert',
    () async {
      final fixture = await openRuntime();
      var decodes = 0;
      final creation = startNativeCompletion<void>(
        start: (completion) => withNativeArena((arena) {
          final options = arena<raw.mln_map_options>();
          options.ref = raw.mln_map_options_default();
          return raw.mln_runtime_create_map(
            fixture.runtime.id.toSigned(64).toInt(),
            options,
            completion,
            nativeDiagnostic,
          );
        }),
        decode: (_) {
          decodes += 1;
          throw StateError('the created map failed to convert');
        },
      );
      await expectLater(within(creation, 'map creation'), throwsStateError);
      await within(
        fixture.runtime.clearResourceTransform(),
        'a later runtime command',
      );
      expect(decodes, 1);

      // The runtime closes only once no map remains, so the batch that held
      // the map disposed it when the conversion left it untaken.
      await within(fixture.runtime.close(), 'runtime close');
    },
  );

  test('an owner that fails to adopt its handle disposes the handle', () {
    final disposed = <int>[];
    expect(
      () => adoptOwnedForTesting<Object>(
        7,
        () => throw StateError('adoption failed'),
        disposed.add,
      ),
      throwsStateError,
    );
    expect(disposed, [7]);

    // When the disposal fails too, both errors surface with a retry.
    var attempts = 0;
    late NativeAdoptionFailure failure;
    try {
      adoptOwnedForTesting<Object>(
        7,
        () => throw StateError('adoption failed'),
        (_) {
          attempts += 1;
          if (attempts == 1) throw StateError('disposal failed');
        },
      );
      fail('the adoption unexpectedly succeeded');
    } on NativeAdoptionFailure catch (caught) {
      failure = caught;
    }
    expect(failure.adoptionError, isStateError);
    expect(failure.cleanupError, isStateError);
    failure.retryCleanup();
    expect(attempts, 2);
  });

  test('a submission native rejects frees its completion state', () async {
    final pending = pendingCompletionCountForTesting();
    // The null runtime is never live, so native refuses the command, and the
    // returned future reports it.
    final rejected = startNativeCompletion<void>(
      start: (completion) => raw.mln_runtime_clear_resource_transform(
        0,
        completion,
        nativeDiagnostic,
      ),
      decode: (_) {},
    );
    expect(pendingCompletionCountForTesting(), pending);
    await expectLater(
      rejected,
      throwsA(
        isA<MaplibreException>().having(
          (error) => error.status,
          'status',
          Status.invalidArgument,
        ),
      ),
    );

    // A later submission still completes through the shared port.
    final fixture = await openRuntime();
    await within(
      fixture.runtime.clearResourceTransform(),
      'a later runtime command',
    );
  });

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

  test(
    'a rejected release fails its future and leaves the owner open',
    () async {
      final fixture = await openRuntime();
      await fixture.openMap();
      // A runtime with a live map refuses its release.
      final release = fixture.runtime.close();
      await expectLater(
        release,
        throwsA(
          isA<MaplibreException>().having(
            (error) => error.status,
            'status',
            Status.invalidState,
          ),
        ),
      );
      await within(fixture.runtime.clearResourceTransform(), 'a later command');
    },
  );

  test('a wait that times out leaves its completion to arrive later', () async {
    final fixture = await openRuntime();
    final map = await fixture.openMap(
      MapOptions(
        mapMode: MapMode.static,
        initialExtent: LogicalExtent(width: 64, height: 64, scaleFactor: 1),
        eventMask: RuntimeEventMask.all,
      ),
    );
    await expectCommitted(map.setStyleJson(jsonBytes(emptyStyleJson)));

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
