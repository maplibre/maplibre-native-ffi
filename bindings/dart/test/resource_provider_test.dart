// Resource providers: routes that decide which requests reach the provider,
// and queued requests that the binding hands over as owned handles.
import 'dart:async';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'support/runtime_probes.dart';
import 'package:test/test.dart';

import 'support/fixture.dart';

void main() {
  test('a routed provider answers the requests its routes match', () async {
    const styleUrl = 'custom://routed-style.json';
    const sourceUrl = 'custom://routed-source.json';
    final requested = <(String?, ResourceKind)>[];
    final sourceRequested = Completer<void>();
    final fixture = await openRuntime(
      provider: routedProvider(
        [
          // A route with the default kinds claims the style and its source alike.
          ResourceRoute(
            flags: UrlMatchFlags.glob,
            url: 'custom://routed-*.json',
          ),
        ],
        (request, handle) {
          requested.add((request.requestedUrl, request.kind));
          handle.complete(
            ResourceResponse(
              status: ResourceResponseStatus.ok,
              bytes: jsonBytes(
                request.kind == ResourceKind.style
                    ? '{"version":8,"sources":{"routed":{"type":"vector",'
                          '"url":"$sourceUrl"}},"layers":[]}'
                    : '{"tilejson":"3.0.0",'
                          '"tiles":["custom://unrouted/{z}/{x}/{y}.pbf"]}',
              ),
            ),
          );
          if (request.kind == ResourceKind.source) sourceRequested.complete();
        },
      ),
    );
    final map = await fixture.openMap();

    await expectCommitted(map.setStyleUrl(styleUrl));
    final loaded = await fixture.awaitEventType(
      RuntimeEventType.mapStyleLoaded,
    );
    expect(loaded.source, map.id);
    await within(sourceRequested.future, 'the source request');
    expect(requested, [
      (styleUrl, ResourceKind.style),
      (sourceUrl, ResourceKind.source),
    ]);
  });

  test('a provider answers a request after its callback returns', () async {
    const styleUrl = 'custom://deferred-style.json';
    final handed = Completer<(ResourceRequestInfo, ResourceRequestHandle)>();
    final fixture = await openRuntime(
      provider: routedProvider([
        styleRoute(styleUrl),
      ], (request, handle) => handed.complete((request, handle))),
    );
    final map = await fixture.openMap();

    await expectCommitted(map.setStyleUrl(styleUrl));
    final (request, handle) = await within(handed.future, 'the provider');
    expect(request.requestedUrl, styleUrl);
    expect(request.kind, ResourceKind.style);
    expect(handle.isCancelled(), isFalse);

    // A response the C API cannot represent is refused and leaves the
    // request open for a valid one.
    expect(
      () => handle.complete(
        ResourceResponse(
          status: ResourceResponseStatus.error,
          errorMessage: 'bad\u0000message',
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
    handle.complete(emptyStyleResponse);
    await fixture.awaitEventType(RuntimeEventType.mapStyleLoaded);

    // The accepted answer consumed the handle, so closing it does nothing and
    // native refuses a second answer or any other call on it.
    expect(handle.isClosed, isTrue);
    handle.close();
    final isInvalidState = throwsA(
      isA<MaplibreException>().having(
        (error) => error.status,
        'status',
        Status.invalidState,
      ),
    );
    expect(
      () => handle.complete(
        ResourceResponse(status: ResourceResponseStatus.noContent),
      ),
      isInvalidState,
    );
    expect(handle.passThrough, isInvalidState);
    expect(handle.isCancelled, isInvalidState);
  });

  test(
    'a provider callback error reaches its zone and fails the request',
    () async {
      const styleUrl = 'custom://throwing-style.json';
      final zoneErrors = <Object>[];
      final fixture = await runZonedGuarded(
        () => openRuntime(
          provider: routedProvider([
            styleRoute(styleUrl),
          ], (_, _) => throw StateError('provider failed')),
        ),
        (error, _) => zoneErrors.add(error),
      )!;
      final map = await fixture.openMap();

      await expectCommitted(map.setStyleUrl(styleUrl));
      // The binding closes the handle the throwing callback abandoned, which
      // fails the request instead of leaving the style load waiting.
      final failure = await fixture.awaitEventType(
        RuntimeEventType.mapLoadingFailed,
      );
      expect(failure.message, contains('released without a response'));
      expect(zoneErrors, [isA<StateError>()]);
    },
  );

  test('a cancel callback runs once for a request teardown discards', () async {
    const styleUrl = 'custom://cancelled-style.json';
    final handed = Completer<ResourceRequestHandle>();
    final cancelled = Completer<bool>();
    final zoneErrors = <Object>[];
    final fixture = await runZonedGuarded(
      () => openRuntime(
        provider: routedProvider([styleRoute(styleUrl)], (_, handle) {
          final alreadyCancelled = handle.setCancelCallback(
            ResourceRequestCancelHandler(
              callback: () {
                cancelled.complete(handle.isCancelled());
                throw StateError('cancel callback failed');
              },
            ),
          );
          expect(alreadyCancelled, isFalse);
          handed.complete(handle);
        }),
      ),
      (error, _) => zoneErrors.add(error),
    )!;
    final map = await fixture.openMap();
    await expectCommitted(map.setStyleUrl(styleUrl));
    final handle = await within(handed.future, 'the provider');
    final probe = singleRegistrationProbeForTesting(handle)!;
    expect(probe.closed, isFalse);

    await within(map.close(), 'map close');
    await within(fixture.runtime.close(), 'runtime close');
    expect(await within(cancelled.future, 'the cancel callback'), isTrue);
    // The retirement that follows the one delivery unroots the registration,
    // so no second delivery can arrive.
    await within(probe.released, 'the cancel registration release');
    expect(zoneErrors, [isA<StateError>()]);
    expect(singleRegistrationProbeForTesting(handle), isNull);
    handle.close();
  });

  test(
    'a cancel registration on a cancelled request is refused and not rooted',
    () async {
      const styleUrl = 'custom://late-cancel-style.json';
      final handed = Completer<ResourceRequestHandle>();
      final fixture = await openRuntime(
        provider: routedProvider([
          styleRoute(styleUrl),
        ], (_, handle) => handed.complete(handle)),
      );
      final map = await fixture.openMap();
      await expectCommitted(map.setStyleUrl(styleUrl));
      final handle = await within(handed.future, 'the provider');
      addTearDown(handle.close);

      await within(map.close(), 'map close');
      await within(fixture.runtime.close(), 'runtime close');
      expect(handle.isCancelled(), isTrue);

      // Native reports the cancellation through the registration's output
      // instead of storing the callback, so the binding keeps no root for it.
      expect(
        handle.setCancelCallback(ResourceRequestCancelHandler(callback: () {})),
        isTrue,
      );
      expect(singleRegistrationProbeForTesting(handle), isNull);
    },
  );
}
