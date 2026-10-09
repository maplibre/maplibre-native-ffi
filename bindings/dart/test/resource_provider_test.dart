// Resource providers: native rule tables, and deferred callbacks whose
// requests the binding hands over as transferable decision handles.
import 'dart:async';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:maplibre_native_ffi/src/runtime/runtime.dart'
    show CallbackPortLifecycleProbe, singleCallbackPortProbeForTesting;
import 'package:test/test.dart';

import 'support/fixture.dart';

void main() {
  test('native provider rules answer a matching request inline', () async {
    const styleUrl = 'custom://inline-style.json';
    final fixture = await openRuntime(
      provider: ResourceProvider.resourceProviderRules(
        AdapterResourceProviderRules(
          rules: [
            AdapterResourceProviderRule(
              kind: ResourceKind.style.rawValue,
              requestedUrl: styleUrl,
              response: emptyStyleResponse,
            ),
          ],
        ),
      ),
    );
    final map = await fixture.openMap();

    await expectCommitted(map.setStyleUrl(styleUrl));
    final loaded = await fixture.awaitEventType(
      RuntimeEventType.mapStyleLoaded,
    );
    expect(loaded.source, map.identity);
  });

  test('a provider answers a request after its callback returns', () async {
    const styleUrl = 'custom://deferred-style.json';
    final handed = Completer<(ResourceRequest, ResourceRequestHandle)>();
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
    expect(handle.cancelled(), isFalse);

    // A response the C API cannot represent is refused and leaves the
    // request open for a valid one.
    expect(
      () => handle.complete(
        ResourceResponse(
          status: ResourceResponseStatus.error,
          errorMessage: 'bad\u0000message',
        ),
      ),
      throwsA(isA<InvalidArgumentException>()),
    );
    handle.complete(emptyStyleResponse);
    await fixture.awaitEventType(RuntimeEventType.mapStyleLoaded);

    // The request takes one answer, and the handle stays open until closed.
    expect(
      () => handle.complete(
        ResourceResponse(status: ResourceResponseStatus.noContent),
      ),
      throwsA(isA<InvalidStateException>()),
    );
    handle.close();
    handle.waitUntilRetired();
    handle.close();
    expect(handle.cancelled, throwsA(isA<InvalidStateException>()));
    expect(
      () => handle.complete(emptyStyleResponse),
      throwsA(isA<InvalidStateException>()),
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
          final alreadyCancelled = handle.setCancelCallback(() {
            cancelled.complete(handle.cancelled());
            throw StateError('cancel callback failed');
          });
          expect(alreadyCancelled, isFalse);
          handed.complete(handle);
        }),
      ),
      (error, _) => zoneErrors.add(error),
    )!;
    final map = await fixture.openMap();
    await expectCommitted(map.setStyleUrl(styleUrl));
    final handle = await within(handed.future, 'the provider');
    final probe = singleCallbackPortProbeForTesting(handle)!;

    // A request takes one cancel registration.
    expect(
      () => handle.setCancelCallback(() {}),
      throwsA(isA<InvalidStateException>()),
    );
    expect(probe.closed, isFalse);

    await within(map.close(), 'map close');
    await within(fixture.runtime.close(), 'runtime close');
    expect(await within(cancelled.future, 'the cancel callback'), isTrue);
    // The release that follows the one delivery closes the port, so no
    // second delivery can arrive.
    await within(probe.released, 'the cancel registration release');
    expect(zoneErrors, [isA<StateError>()]);
    expect(singleCallbackPortProbeForTesting(handle), isNull);

    // The cancelled request refuses an answer, and stays open until closed.
    expect(
      () => handle.complete(emptyStyleResponse),
      throwsA(isA<InvalidStateException>()),
    );
    expect(handle.isClosed, isFalse);
    handle.close();
    handle.waitUntilRetired();
  });

  test('a completed request releases its cancel registration unrun', () async {
    const styleUrl = 'custom://completed-style.json';
    final registered = Completer<CallbackPortLifecycleProbe>();
    var cancels = 0;
    final fixture = await openRuntime(
      provider: routedProvider([styleRoute(styleUrl)], (_, handle) {
        expect(handle.setCancelCallback(() => cancels += 1), isFalse);
        registered.complete(singleCallbackPortProbeForTesting(handle));
        handle.complete(emptyStyleResponse);
        handle.close();
      }),
    );
    final map = await fixture.openMap();

    await expectCommitted(map.setStyleUrl(styleUrl));
    final probe = await within(registered.future, 'the provider');
    await within(probe.released, 'the cancel registration release');
    expect(cancels, 0);
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
      expect(handle.cancelled(), isTrue);

      // Native reports the cancellation through the registration's output
      // instead of storing the callback, so the binding keeps no root for it.
      expect(handle.setCancelCallback(() {}), isTrue);
      expect(singleCallbackPortProbeForTesting(handle), isNull);
    },
  );
}
