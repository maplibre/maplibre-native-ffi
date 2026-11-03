// Resource providers: queued requests that the binding hands over as owned
// handles, and the errors that their callbacks throw.
import 'dart:async';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

import 'support/fixture.dart';

void main() {
  test('a provider answers a request after its callback returns', () async {
    const styleUrl = 'custom://deferred-style.json';
    final handed = Completer<(ResourceRequestInfo, ResourceRequestHandle)>();
    final fixture = await openRuntime(
      provider: routedProvider([
        styleRoute(styleUrl),
      ], (request, handle) => handed.complete((request, handle))),
    );
    final map = await fixture.openMap();

    final loaded = map.setStyleUrl(styleUrl);
    final (request, handle) = await within(handed.future, 'the provider');
    expect(request.requestedUrl, styleUrl);
    expect(request.kind, ResourceKind.style);
    expect(handle.isCancelled(), isFalse);

    handle.complete(emptyStyleResponse);
    await expectCommitted(loaded);

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

  test('a refused answer closes its request and fails it', () async {
    const styleUrl = 'custom://refused-style.json';
    final handed = Completer<ResourceRequestHandle>();
    final fixture = await openRuntime(
      provider: routedProvider([
        styleRoute(styleUrl),
      ], (_, handle) => handed.complete(handle)),
    );
    final map = await fixture.openMap();

    final loading = map.setStyleUrl(styleUrl);
    final handle = await within(handed.future, 'the provider');
    // A response the C API cannot represent is refused, and the refused
    // answer still ends the request.
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
    expect(handle.isClosed, isTrue);
    await expectLater(
      within(loading, 'a command'),
      throwsA(
        isA<MaplibreException>().having(
          (error) => error.status,
          'status',
          Status.nativeError,
        ),
      ),
    );
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

      // The binding closes the handle the throwing callback abandoned, which
      // fails the request instead of leaving the style load waiting.
      await expectLater(
        within(map.setStyleUrl(styleUrl), 'a command'),
        throwsA(
          isA<MaplibreException>().having(
            (error) => error.diagnostic,
            'diagnostic',
            contains('released without a response'),
          ),
        ),
      );
      expect(zoneErrors, [isA<StateError>()]);
    },
  );

  test('a cancel callback runs for a request that teardown discards', () async {
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
    map.discarding.setStyleUrl(styleUrl);
    final handle = await within(handed.future, 'the provider');

    await within(map.close(), 'map close');
    await within(fixture.runtime.close(), 'runtime close');
    expect(await within(cancelled.future, 'the cancel callback'), isTrue);
    expect(zoneErrors, [isA<StateError>()]);
    handle.close();
  });
}
