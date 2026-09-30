// Completions: the shared receive port, conversion failures, rejected
// submissions, failed dispositions, and waits that give up.
import 'dart:async';
import 'dart:ffi';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.dart'
    show nativeDiagnostic;
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.g.dart'
    as raw;
import 'package:maplibre_native_ffi/src/internal/callback/completion.dart';
import 'package:maplibre_native_ffi/src/internal/memory/memory.dart';
import 'package:maplibre_native_ffi/src/runtime/runtime.dart'
    show adoptOwnedForTesting;
import 'package:test/test.dart';

import 'support/fixture.dart';

void main() {
  test(
    'a completion delivers once and disposes a value that fails to convert',
    () async {
      final fixture = await openRuntime();
      var decodes = 0;
      final creation = startNativeCompletion<void>(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_MAP,
        elementSize: sizeOf<Uint64>(),
        start: (completion) => withNativeArena((arena) {
          final options = arena<raw.mln_map_options>();
          options.ref = raw.mln_map_options_default();
          return raw.mln_map_create(
            fixture.runtime.identity.toSigned(64).toInt(),
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
      await within(fixture.runtime.barrier(), 'a barrier');
      expect(decodes, 1);

      // The runtime closes only once no map remains, so the copied map was
      // disposed with the record its conversion abandoned.
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
    var rejections = 0;
    expect(
      () => startNativeCompletion<void>(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_FLAT,
        elementSize: 0,
        // The null runtime is never live, so native refuses the barrier.
        start: (completion) =>
            raw.mln_runtime_barrier(0, completion, nativeDiagnostic),
        decode: (_) {},
        onRejected: () => rejections += 1,
      ),
      throwsA(isA<InvalidArgumentException>()),
    );
    expect(rejections, 1);

    // A later submission still completes through the shared port.
    final fixture = await openRuntime();
    await within(fixture.runtime.barrier(), 'a barrier');
  });

  test('a failed command reports its disposition as data', () async {
    final fixture = await openRuntime();
    final map = await fixture.openStyledMap();

    final completion = await within(
      map.removeStyleSource('missing-source'),
      'source removal',
    );
    expect(completion.disposition, CommandDisposition.failed);
    expect(completion.status, MaplibreStatus.notFound);
    expect(completion.diagnostic, isNotEmpty);
  });

  test('a wait that times out leaves its completion to arrive later', () async {
    final fixture = await openRuntime();
    final map = await fixture.openMap(
      const MapOptions(
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
    final cancelled = expectLater(image, throwsA(isA<CancelledException>()));
    await within(map.close(), 'map close');
    await within(cancelled, 'the cancelled still image');
  });
}
