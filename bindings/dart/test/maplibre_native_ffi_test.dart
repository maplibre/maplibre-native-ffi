import 'generated_workflows.dart';
import 'dart:async';
import 'dart:convert';
import 'dart:ffi';
import 'dart:isolate';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';
import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.g.dart'
    as raw;
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.dart'
    show expectedCAbiVersion;
import 'package:maplibre_native_ffi/src/runtime/runtime.dart'
    show logCallbackStateForTesting;
import 'package:test/test.dart';

const _emptyStyleJson = '{"version":8,"sources":{},"layers":[]}';

/// Every event type but style-loaded, the type a host most plausibly clears
/// while still expecting custom-geometry callback state to be released.
final _maskWithoutStyleLoaded = RuntimeEventMask.fromRawValue(
  RuntimeEventMask.all.rawValue & ~RuntimeEventMask.mapStyleLoaded.rawValue,
);

Uint8List _jsonBytes(String value) => Uint8List.fromList(utf8.encode(value));

Future<void> _expectCommandFailure(
  Future<CommandCompletion> future,
  MaplibreStatus status,
) async {
  final completion = await future;
  expect(completion.disposition, CommandDisposition.failed);
  expect(completion.status, status);
  expect(completion.diagnostic, isNotEmpty);
}

/// Dispatches one record through the registered adapter log callback, the way
/// MapLibre's logging threads do, and reports the consume value native code
/// sees.
int _dispatchLogRecord({
  required int severity,
  required int event,
  required int code,
  required String message,
}) {
  final nativeMessage = message.toNativeUtf8();
  try {
    return raw.mln_adapter_log_callback(
      logCallbackStateForTesting().cast<Void>(),
      severity,
      event,
      code,
      nativeMessage.cast<Char>(),
    );
  } finally {
    malloc.free(nativeMessage);
  }
}

void main() {
  test('global state uses style defaults and resets on replacement', () async {
    final runtime = runtimeCreate(runtimeOptionsDefault());
    addTearDown(runtime.close);
    final map = await runtime.createMap();
    addTearDown(map.close);
    Uint8List bytes(String value) => Uint8List.fromList(utf8.encode(value));
    final rejected = await map.setGlobalStateProperty('theme', bytes('true'));
    expect(rejected.disposition, CommandDisposition.failed);
    expect(rejected.status, MaplibreStatus.invalidState);
    await map.setStyleJson(
      bytes(
        '{"version":8,"sources":{},"layers":[],"state":{"theme":{"default":"light"}}}',
      ),
    );
    expect(utf8.decode(await map.getGlobalState()), '{"theme":"light"}');
    await map.setGlobalStateProperty(
      'theme',
      bytes('["dark",{"enabled":true}]'),
    );
    final snapshot = await map.getGlobalState();
    await map.setGlobalStateProperty('theme', bytes('null'));
    expect(utf8.decode(await map.getGlobalState()), '{"theme":"light"}');
    expect(utf8.decode(snapshot), '{"theme":["dark",{"enabled":true}]}');
    await map.setStyleJson(bytes('{"version":8,"sources":{},"layers":[]}'));
    expect(utf8.decode(await map.getGlobalState()), '{}');
  });

  test('map options carry FastPFOR decoding to native', () async {
    expect(const MapOptions().fastPforEnabled, isFalse);
    expect(const MapOptions(fastPforEnabled: true), isNot(const MapOptions()));

    final runtime = runtimeCreate(runtimeOptionsDefault());
    final map = await runtime.createMap(
      options: const MapOptions(
        initialExtent: LogicalExtent(width: 64, height: 64, scaleFactor: 1),
        eventMask: RuntimeEventMask.all,
        fastPforEnabled: true,
      ),
    );
    expect(
      map.snapshotGet().logicalExtent,
      const LogicalExtent(width: 64, height: 64, scaleFactor: 1),
    );
    await map.close();
    await runtime.close();
  });
  test('one drain takes every event a style load queued', () async {
    final runtime = runtimeCreate(runtimeOptionsDefault());
    addTearDown(runtime.close);

    // A fresh runtime has nothing queued.
    expect(runtime.drainCopiedEvents(), isEmpty);

    final map = await runtime.createMap();
    addTearDown(map.close);
    await _expectCommandCommitted(
      map.setStyleJson(_jsonBytes(_emptyStyleJson)),
    );
    await runtime.barrier();

    // A style load reports several event types, and one drain takes them all.
    final batch = runtime.drainCopiedEvents();
    expect(batch.length, greaterThan(1));
    final types = batch.map((event) => event.type);
    expect(types, contains(RuntimeEventType.mapStyleLoaded));
    expect(types, contains(RuntimeEventType.mapRenderUpdateAvailable));
    expect(runtime.drainCopiedEvents(), isEmpty);
  });

  test('both handles report and narrow their event masks', () async {
    final runtime = runtimeCreate(runtimeOptionsDefault());
    addTearDown(runtime.close);
    final map = await runtime.createMap();
    addTearDown(map.close);

    // The default options select every event type.
    expect(runtime.eventMask, RuntimeEventMask.all);
    expect(map.snapshotGet().eventMask, RuntimeEventMask.all);

    // A host reads the mask, clears one bit, and writes it back; every other
    // bit survives.
    final withoutIdle = RuntimeEventMask.fromRawValue(
      map.snapshotGet().eventMask.rawValue & ~RuntimeEventMask.mapIdle.rawValue,
    );
    await _expectCommandCommitted(map.setEventMask(withoutIdle));
    final narrowed = map.snapshotGet().eventMask;
    expect(narrowed, withoutIdle);
    expect(narrowed.contains(RuntimeEventMask.mapIdle), isFalse);
    expect(narrowed.contains(RuntimeEventMask.mapStyleLoaded), isTrue);

    runtime.setEventMask(withoutIdle);
    expect(runtime.eventMask, withoutIdle);

    const outsideAll = RuntimeEventMask.fromRawValue(1 << 40);
    expect(
      () => map.setEventMask(outsideAll),
      throwsA(isA<InvalidArgumentException>()),
    );
    expect(
      () => runtime.setEventMask(outsideAll),
      throwsA(isA<InvalidArgumentException>()),
    );
    // A rejected mask leaves the installed one in place.
    expect(map.snapshotGet().eventMask, withoutIdle);
    expect(runtime.eventMask, withoutIdle);
  });

  test(
    'a narrowed map mask keeps cleared event types out of a batch',
    () async {
      final runtime = runtimeCreate(runtimeOptionsDefault());
      addTearDown(runtime.close);
      final map = await runtime.createMap();
      addTearDown(map.close);
      map.setEventMask(
        RuntimeEventMask.mapStyleLoaded | RuntimeEventMask.mapLoadingFailed,
      );

      map.setStyleJson(_jsonBytes(_emptyStyleJson));
      final types = <RuntimeEventType>{};
      await _waitUntil(() {
        types.addAll(runtime.drainCopiedEvents().map((event) => event.type));
        return types.contains(RuntimeEventType.mapStyleLoaded);
      });
      expect(types, isNot(contains(RuntimeEventType.mapRenderUpdateAvailable)));
    },
  );

  // The release callback the C API invokes is what tells this binding a style
  // replacement detached a source, so a host that reads no style-loaded events
  // still gets its callback root retired.
  test(
    'a style replacement releases a source with style loads unselected',
    () async {
      const sourceId = 'dart-released-source';
      final runtime = runtimeCreate(runtimeOptionsDefault());
      addTearDown(runtime.close);
      final map = await runtime.createMap(
        options: MapOptions(
          initialExtent: const LogicalExtent(
            width: 256,
            height: 256,
            scaleFactor: 1,
          ),
          eventMask: _maskWithoutStyleLoaded,
        ),
      );
      addTearDown(map.close);
      await _expectCommandCommitted(
        map.setStyleJson(_jsonBytes(_emptyStyleJson)),
      );
      await map.addCustomGeometrySource(
        sourceId,
        CustomGeometrySourceOptions(fetchTile: (_) {}),
      );
      final probe = singleCallbackPortProbeForTesting(map)!;

      // The mask reads back as the host set it, because the binding selects
      // nothing of its own.
      expect(map.snapshotGet().eventMask, _maskWithoutStyleLoaded);

      await _expectCommandCommitted(
        map.setStyleJson(_jsonBytes(_emptyStyleJson)),
      );
      final types = <RuntimeEventType>{};
      await _waitUntil(() {
        types.addAll(runtime.drainCopiedEvents().map((event) => event.type));
        return probe.retirementQueued;
      });
      expect(types, isNot(contains(RuntimeEventType.mapStyleLoaded)));
      expect(singleCallbackPortProbeForTesting(map), isNull);
      await _waitUntil(() => probe.closed);
    },
  );

  test(
    'a style replacement releases a custom MVT vector source with style loads '
    'unselected',
    () async {
      const sourceId = 'dart-released-mvt-source';
      final runtime = runtimeCreate(runtimeOptionsDefault());
      addTearDown(runtime.close);
      final map = await runtime.createMap(
        options: MapOptions(
          initialExtent: const LogicalExtent(
            width: 256,
            height: 256,
            scaleFactor: 1,
          ),
          eventMask: _maskWithoutStyleLoaded,
        ),
      );
      addTearDown(map.close);
      await _expectCommandCommitted(
        map.setStyleJson(_jsonBytes(_emptyStyleJson)),
      );
      await map.addCustomMvtVectorSource(
        sourceId,
        CustomMvtVectorSourceOptions(fetchTile: (_) {}),
      );
      final probe = singleCallbackPortProbeForTesting(map)!;

      expect(map.snapshotGet().eventMask, _maskWithoutStyleLoaded);

      await _expectCommandCommitted(
        map.setStyleJson(_jsonBytes(_emptyStyleJson)),
      );
      final types = <RuntimeEventType>{};
      await _waitUntil(() {
        types.addAll(runtime.drainCopiedEvents().map((event) => event.type));
        return probe.retirementQueued;
      });
      expect(types, isNot(contains(RuntimeEventType.mapStyleLoaded)));
      expect(singleCallbackPortProbeForTesting(map), isNull);
      await _waitUntil(() => probe.closed);
    },
  );

  test('process-global APIs cross the native C ABI', () {
    expect(cVersion(), expectedCAbiVersion);
    // The host library was built with at least one renderer, and it reports
    // the Opengl context providers that renderer can use.
    expect(supportedRenderBackendMask().rawValue, isNot(0));
    if (supportedRenderBackendMask().contains(RenderBackendFlag.opengl)) {
      expect(openglSupportedContextProviderMask().rawValue, isNot(0));
    }

    final meters = projectedMetersForLatLng(const LatLng(0, 0));
    expect(meters.northing.isFinite, isTrue);
    expect(meters.easting.isFinite, isTrue);
    expect(latLngForProjectedMeters(meters).latitude, closeTo(0, 0.0001));

    final status = networkStatusGet();
    expect(
      status.rawValue,
      isIn([NetworkStatus.online.rawValue, NetworkStatus.offline.rawValue]),
    );
    networkStatusSet(status);
    final logRecords = <LogRecord>[];
    logSetCallback(logRecords.add);
    logSetAsyncSeverityMask(LogSeverityMask.defaultValue);
    logSetAsyncSeverityMask(LogSeverityMask.defaultValue);
    logClearCallback();
  });

  test('render target extents report their physical size through native', () {
    final size = renderTargetExtentPhysicalSize(
      const RenderTargetExtent(width: 65, height: 33, scaleFactor: 1.5),
    );
    expect(size.$1, 98);
    expect(size.$2, 50);
  });

  test('process-global log callbacks retire across isolates', () async {
    logSetCallback((_) {}, consume: true);
    // The registration is process-global, so another isolate clears the one
    // this isolate installed.
    await Isolate.run(_clearLogCallback);
    await Future<void>.delayed(Duration.zero);

    // Native code dispatches to no callback, so it consumes nothing.
    expect(
      _dispatchLogRecord(
        severity: LogSeverity.info.rawValue,
        event: LogEvent.general.rawValue,
        code: 404,
        message: 'after cross-isolate clear',
      ),
      0,
    );

    logSetCallback((_) {});
    logClearCallback();
  });

  test('log callback replacement and clear change native delivery', () async {
    final first = <LogRecord>[];
    final replacement = <LogRecord>[];

    logSetCallback(first.add, consume: true);
    expect(
      _dispatchLogRecord(
        severity: LogSeverity.info.rawValue,
        event: LogEvent.general.rawValue,
        code: 101,
        message: 'first',
      ),
      1,
    );
    await _waitUntil(() => first.isNotEmpty);
    expect(first.single.code, 101);
    expect(first.single.message, 'first');

    logSetCallback(replacement.add);
    expect(
      _dispatchLogRecord(
        severity: LogSeverity.warning.rawValue,
        event: LogEvent.setup.rawValue,
        code: 202,
        message: 'replacement',
      ),
      0,
    );
    await _waitUntil(() => replacement.isNotEmpty);
    expect(first, hasLength(1));
    expect(replacement.single.code, 202);
    expect(replacement.single.message, 'replacement');

    logClearCallback();
    expect(
      _dispatchLogRecord(
        severity: LogSeverity.error.rawValue,
        event: LogEvent.render.rawValue,
        code: 303,
        message: 'cleared',
      ),
      0,
    );
    expect(replacement, hasLength(1));
  });

  test(
    'native provider rules complete matching style requests inline',
    () async {
      const styleUrl = 'custom://dart-inline-provider-style.json';
      final runtime = runtimeCreate(runtimeOptionsDefault());
      runtime.setResourceProvider(
        ResourceProvider.resourceProviderRules(
          AdapterResourceProviderRules(
            rules: [
              AdapterResourceProviderRule(
                kind: ResourceKind.style.rawValue,
                requestedUrl: styleUrl,
                response: ResourceResponse(
                  status: ResourceResponseStatus.ok,
                  bytes: Uint8List.fromList(_emptyStyleJson.codeUnits),
                ),
              ),
            ],
          ),
        ),
      );
      final map = await runtime.createMap();

      map.setStyleUrl(styleUrl);
      final event = await _waitUntilEvent(
        runtime,
        (candidate) =>
            candidate.type == RuntimeEventType.mapStyleLoaded &&
            candidate.sourceType == RuntimeEventSourceType.map,
      );
      expect(event.source, map.identity);

      await map.close();
      await runtime.close();
    },
  );

  test('unmatched provider routes pass through to native loading', () async {
    const unmatchedUrl = 'custom://dart-provider-pass-through.json';
    final runtime = runtimeCreate(runtimeOptionsDefault());
    var providerCalls = 0;
    runtime.setQueuedResourceProvider(
      QueuedResourceProvider(
        routes: [
          AdapterQueuedResourceProviderRoute(
            kind: ResourceKind.style.rawValue,
            url: 'custom://different-style.json',
          ),
        ],
        callback: (_, handle) {
          providerCalls += 1;
          handle.close();
        },
      ),
    );
    final map = await runtime.createMap();

    map.setStyleUrl(unmatchedUrl);
    final event = await _waitUntilEvent(
      runtime,
      (candidate) => candidate.type == RuntimeEventType.mapLoadingFailed,
    );
    expect(event.sourceType, RuntimeEventSourceType.map);
    expect(providerCalls, 0);

    await map.close();
    await runtime.close();
  });

  test('queued resource provider callbacks cross the native C ABI', () async {
    const styleUrl = 'custom://dart-provider-style.json';
    final runtime = runtimeCreate(runtimeOptionsDefault());
    final requests = <ResourceRequest>[];
    late ResourceRequestHandle ownerToken;

    runtime.setQueuedResourceProvider(
      QueuedResourceProvider(
        routes: [
          AdapterQueuedResourceProviderRoute(
            kind: ResourceKind.style.rawValue,
            url: styleUrl,
          ),
        ],
        callback: (request, handle) {
          requests.add(request);
          expect(request.requestedUrl, styleUrl);
          expect(request.kind, ResourceKind.style);
          expect(handle.cancelled(), isFalse);
          expect(
            () => handle.complete(
              ResourceResponse(
                status: ResourceResponseStatus.error,
                errorMessage: 'bad\u0000message',
              ),
            ),
            throwsA(isA<InvalidArgumentException>()),
          );
          ownerToken = handle;
          handle.complete(
            ResourceResponse(
              status: ResourceResponseStatus.ok,
              bytes: Uint8List.fromList(_emptyStyleJson.codeUnits),
            ),
          );
          expect(handle.isClosed, isFalse);
          expect(
            () => handle.complete(
              ResourceResponse(status: ResourceResponseStatus.noContent),
            ),
            throwsA(isA<InvalidStateException>()),
          );
        },
      ),
    );

    final map = await runtime.createMap();
    map.setStyleUrl(styleUrl);
    await _waitUntil(() => requests.isNotEmpty);

    ownerToken.close();
    ownerToken.waitUntilRetired();
    expect(
      () => ownerToken.cancelled(),
      throwsA(isA<InvalidArgumentException>()),
    );
    ownerToken.close();
    await map.close();
    await runtime.close();
  });

  // a configured URI-scheme alias reaches the provider as the alias,
  // alongside the URL the built-in network path would have fetched.
  test('queued resource provider sees scheme alias and resolved URL', () async {
    const aliasUrl = 'maplibre://maps/style';
    final runtime = runtimeCreate(runtimeOptionsDefault());
    final requests = <ResourceRequest>[];

    runtime.setQueuedResourceProvider(
      QueuedResourceProvider(
        routes: [
          AdapterQueuedResourceProviderRoute(
            kind: ResourceKind.style.rawValue,
            url: aliasUrl,
            flags: raw
                .mln_adapter_resource_route_flags
                .MLN_ADAPTER_RESOURCE_ROUTE_USE_REQUESTED_URL,
          ),
        ],
        callback: (request, handle) {
          requests.add(request);
          handle.complete(
            ResourceResponse(
              status: ResourceResponseStatus.ok,
              bytes: Uint8List.fromList(_emptyStyleJson.codeUnits),
            ),
          );
          handle.close();
        },
      ),
    );

    final map = await runtime.createMap();
    map.setStyleUrl(aliasUrl);
    await _waitUntil(() => requests.isNotEmpty);

    expect(requests.first.requestedUrl, aliasUrl);
    expect(
      requests.first.resolvedUrl,
      'https://demotiles.maplibre.org/style.json',
    );

    await map.close();
    await runtime.close();
  });

  // a glob route claims a URL family whose members are known only
  // when they are requested.
  test('queued resource provider glob routes claim a URL family', () async {
    const origin = 'custom://dart-provider-glob/';
    final runtime = runtimeCreate(runtimeOptionsDefault());
    final claimed = <String>[];

    runtime.setQueuedResourceProvider(
      QueuedResourceProvider(
        routes: [
          AdapterQueuedResourceProviderRoute(
            kind: raw.MLN_ADAPTER_RESOURCE_KIND_ANY,
            url: '$origin**',
            flags: raw
                .mln_adapter_resource_route_flags
                .MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB,
          ),
        ],
        callback: (request, handle) {
          claimed.add(request.resolvedUrl!);
          handle.complete(
            ResourceResponse(
              status: ResourceResponseStatus.ok,
              bytes: Uint8List.fromList(_emptyStyleJson.codeUnits),
            ),
          );
          handle.close();
        },
      ),
    );

    final map = await runtime.createMap();
    map.setStyleUrl('${origin}unenumerated/style.json');
    await _waitUntilEvent(
      runtime,
      (candidate) => candidate.type == RuntimeEventType.mapStyleLoaded,
    );

    // A pattern matches the complete URL, so a URL that merely contains the
    // route's origin stays with native loading.
    map.setStyleUrl('custom://elsewhere/${origin}style.json');
    await _waitUntilEvent(
      runtime,
      (candidate) => candidate.type == RuntimeEventType.mapLoadingFailed,
    );

    expect(claimed, ['${origin}unenumerated/style.json']);

    await map.close();
    await runtime.close();
  });

  // a route picks which of the request's two URLs it compares, so a
  // configured URI-scheme alias is reachable by the alias and by the URL the
  // built-in network path would have fetched.
  test(
    'queued resource provider routes pick requested or resolved URL',
    () async {
      const aliasUrl = 'maplibre://maps/style';
      const normalizedUrl = 'https://demotiles.maplibre.org/style.json';

      Future<ResourceRequest> claimedBy(
        AdapterQueuedResourceProviderRoute route,
      ) async {
        final runtime = runtimeCreate(runtimeOptionsDefault());
        final requests = <ResourceRequest>[];
        runtime.setQueuedResourceProvider(
          QueuedResourceProvider(
            routes: [route],
            callback: (request, handle) {
              requests.add(request);
              handle.complete(
                ResourceResponse(
                  status: ResourceResponseStatus.ok,
                  bytes: Uint8List.fromList(_emptyStyleJson.codeUnits),
                ),
              );
              handle.close();
            },
          ),
        );
        final map = await runtime.createMap();
        map.setStyleUrl(aliasUrl);
        await _waitUntil(() => requests.isNotEmpty);
        await map.close();
        await runtime.close();
        return requests.single;
      }

      final byResolved = await claimedBy(
        AdapterQueuedResourceProviderRoute(
          kind: ResourceKind.style.rawValue,
          url: normalizedUrl,
        ),
      );
      expect(byResolved.requestedUrl, aliasUrl);

      final byRequested = await claimedBy(
        AdapterQueuedResourceProviderRoute(
          kind: ResourceKind.style.rawValue,
          url: aliasUrl,
          flags: raw
              .mln_adapter_resource_route_flags
              .MLN_ADAPTER_RESOURCE_ROUTE_USE_REQUESTED_URL,
        ),
      );
      expect(byRequested.resolvedUrl, normalizedUrl);
    },
  );

  test('cancelled transferred requests complete terminally', () async {
    const styleUrl = 'custom://dart-provider-cancelled.json';
    final runtime = runtimeCreate(runtimeOptionsDefault());
    ResourceRequestHandle? token;

    runtime.setQueuedResourceProvider(
      QueuedResourceProvider(
        routes: [
          AdapterQueuedResourceProviderRoute(
            kind: ResourceKind.style.rawValue,
            url: styleUrl,
          ),
        ],
        callback: (_, handle) {
          token = handle;
        },
      ),
    );

    final map = await runtime.createMap();
    map.setStyleUrl(styleUrl);
    await _waitUntil(() => token != null);
    final liveToken = token!;

    await map.close();
    await runtime.close();
    await _waitUntil(liveToken.cancelled);
    expect(
      () => liveToken.complete(
        ResourceResponse(status: ResourceResponseStatus.noContent),
      ),
      throwsA(isA<InvalidStateException>()),
    );
    liveToken.close();
    liveToken.waitUntilRetired();
  });

  test('transferred response validation preserves the live token', () async {
    const styleUrl = 'custom://dart-provider-token-validation.json';
    final runtime = runtimeCreate(runtimeOptionsDefault());
    ResourceRequestHandle? token;

    runtime.setQueuedResourceProvider(
      QueuedResourceProvider(
        routes: [
          AdapterQueuedResourceProviderRoute(
            kind: ResourceKind.style.rawValue,
            url: styleUrl,
          ),
        ],
        callback: (_, handle) {
          token = handle;
        },
      ),
    );
    final map = await runtime.createMap();
    map.setStyleUrl(styleUrl);
    await _waitUntil(() => token != null);

    final liveToken = token!;
    expect(
      () => liveToken.complete(
        ResourceResponse(
          status: ResourceResponseStatus.error,
          errorMessage: 'bad\u0000message',
        ),
      ),
      throwsA(isA<InvalidArgumentException>()),
    );
    liveToken.complete(
      ResourceResponse(
        status: ResourceResponseStatus.ok,
        bytes: Uint8List.fromList(_emptyStyleJson.codeUnits),
      ),
    );
    // A released id is rejected rather than naming a later request.
    expect(
      () => liveToken.complete(
        ResourceResponse(status: ResourceResponseStatus.noContent),
      ),
      throwsA(isA<InvalidStateException>()),
    );
    liveToken.close();

    await map.close();
    await runtime.close();
  });

  test('queued resource provider callback exceptions are contained', () async {
    const styleUrl = 'custom://dart-provider-throws.json';
    final runtime = runtimeCreate(runtimeOptionsDefault());
    var calls = 0;

    runtime.setQueuedResourceProvider(
      QueuedResourceProvider(
        routes: [
          AdapterQueuedResourceProviderRoute(
            kind: ResourceKind.style.rawValue,
            url: styleUrl,
          ),
        ],
        callback: (_, _) {
          calls += 1;
          throw StateError('provider failed');
        },
      ),
    );

    final map = await runtime.createMap();
    map.setStyleUrl(styleUrl);
    await _waitUntil(() => calls > 0);

    // The binding fails the request the throwing callback abandoned, so the
    // style load reports its failure rather than hanging.
    final failure = await _waitUntilEvent(
      runtime,
      (candidate) => candidate.type == RuntimeEventType.mapLoadingFailed,
    );
    expect(failure.message, isNotNull);

    await map.close();
    await runtime.close();
  });

  test('closed resource request handles reject further use', () async {
    const styleUrl = 'custom://dart-provider-closed-handle.json';
    final runtime = runtimeCreate(runtimeOptionsDefault());
    var callbackFinished = false;
    var cancelledRejected = false;
    var completionRejected = false;
    var repeatedCloseSucceeded = false;

    runtime.setQueuedResourceProvider(
      QueuedResourceProvider(
        routes: [
          AdapterQueuedResourceProviderRoute(
            kind: ResourceKind.style.rawValue,
            url: styleUrl,
          ),
        ],
        callback: (_, handle) {
          handle.close();
          try {
            handle.cancelled();
          } on InvalidArgumentException {
            cancelledRejected = true;
          }
          try {
            handle.complete(
              ResourceResponse(status: ResourceResponseStatus.noContent),
            );
          } on InvalidArgumentException {
            completionRejected = true;
          }
          handle.close();
          repeatedCloseSucceeded = true;
          callbackFinished = true;
        },
      ),
    );
    final map = await runtime.createMap();
    map.setStyleUrl(styleUrl);
    await _waitUntil(() => callbackFinished);

    expect(cancelledRejected, isTrue);
    expect(completionRejected, isTrue);
    expect(repeatedCloseSucceeded, isTrue);

    await map.close();
    await runtime.close();
  });

  // a cancel callback registered on a handled request runs once when
  // MapLibre discards the request, may release the request from inside, and
  // keeps an exception it throws inside the binding.
  test(
    'resource request cancel callbacks report a discarded request',
    () async {
      const styleUrl = 'custom://dart-provider-cancel-reported.json';
      final runtime = runtimeCreate(runtimeOptionsDefault());
      ResourceRequestHandle? token;
      var cancels = 0;
      var cancelledInsideCallback = false;
      Object? insideError;

      runtime.setQueuedResourceProvider(
        QueuedResourceProvider(
          routes: [
            AdapterQueuedResourceProviderRoute(
              kind: ResourceKind.style.rawValue,
              url: styleUrl,
            ),
          ],
          callback: (_, handle) {
            token = handle;
            handle.setCancelCallback(() {
              cancels += 1;
              try {
                cancelledInsideCallback = handle.cancelled();
              } catch (error) {
                insideError = error;
              }
              handle.close();
              throw StateError('cancel callback failed');
            });
          },
        ),
      );

      final map = await runtime.createMap();
      map.setStyleUrl(styleUrl);
      await _waitUntil(() => token != null);
      final liveToken = token!;
      expect(
        () => liveToken.setCancelCallback(() {}),
        throwsA(isA<InvalidStateException>()),
      );

      await map.close();
      await runtime.close();
      await _waitUntil(() => cancels > 0);

      // A second delivery would arrive on a later turn, so settle before
      // asserting the callback ran once.
      await Future<void>.delayed(const Duration(milliseconds: 50));
      expect(insideError, isNull);
      expect(cancelledInsideCallback, isTrue);
      expect(cancels, 1);
      expect(
        () => liveToken.setCancelCallback(() {}),
        throwsA(isA<InvalidArgumentException>()),
      );
    },
  );

  // a request the provider completed is never reported as cancelled.
  test('resource request cancel callbacks end at completion', () async {
    const styleUrl = 'custom://dart-provider-cancel-completed.json';
    final runtime = runtimeCreate(runtimeOptionsDefault());
    var cancels = 0;

    runtime.setQueuedResourceProvider(
      QueuedResourceProvider(
        routes: [
          AdapterQueuedResourceProviderRoute(
            kind: ResourceKind.style.rawValue,
            url: styleUrl,
          ),
        ],
        callback: (_, handle) {
          handle.setCancelCallback(() => cancels += 1);
          handle.complete(
            ResourceResponse(
              status: ResourceResponseStatus.ok,
              bytes: Uint8List.fromList(_emptyStyleJson.codeUnits),
            ),
          );
        },
      ),
    );

    final map = await runtime.createMap();
    map.setStyleUrl(styleUrl);
    await _waitUntilEvent(
      runtime,
      (candidate) => candidate.type == RuntimeEventType.mapStyleLoaded,
    );

    await map.close();
    await runtime.close();

    expect(cancels, 0);
  });

  // a registration on a request MapLibre already cancelled runs the
  // callback before registration returns.
  test('cancel registration reports an already cancelled request', () async {
    const styleUrl = 'custom://dart-provider-cancel-late.json';
    final runtime = runtimeCreate(runtimeOptionsDefault());
    ResourceRequestHandle? token;
    var cancels = 0;

    runtime.setQueuedResourceProvider(
      QueuedResourceProvider(
        routes: [
          AdapterQueuedResourceProviderRoute(
            kind: ResourceKind.style.rawValue,
            url: styleUrl,
          ),
        ],
        callback: (_, handle) {
          token = handle;
        },
      ),
    );

    final map = await runtime.createMap();
    map.setStyleUrl(styleUrl);
    await _waitUntil(() => token != null);
    final liveToken = token!;

    await map.close();
    await runtime.close();
    await _waitUntil(liveToken.cancelled);

    expect(liveToken.setCancelCallback(() => cancels += 1), isTrue);
    expect(cancels, 0);
    await Future<void>.delayed(const Duration(milliseconds: 50));
    expect(cancels, 0);

    liveToken.close();
  });

  test(
    'projection is synchronous and observes only earlier commands',
    () async {
      final runtime = runtimeCreate(runtimeOptionsDefault());
      final map = await runtime.createMap(
        options: const MapOptions(
          initialExtent: LogicalExtent(width: 256, height: 256, scaleFactor: 1),
          eventMask: RuntimeEventMask.all,
        ),
      );
      addTearDown(runtime.close);
      addTearDown(map.close);
      // A projection created after a camera command observes that command.
      map.updateCamera(
        CameraUpdate(
          camera: const CameraOptions(center: LatLng(10, 20), zoom: 3),
        ),
      );
      final projection = await map.projectionCreate();
      addTearDown(projection.close);
      {
        final created = projection.getCamera();
        expect(created.zoom, closeTo(3, 0.0001));
        expect(created.center!.latitude, closeTo(10, 0.0001));
        expect(created.center!.longitude, closeTo(20, 0.0001));

        // A synchronous conversion round-trip returns to the same coordinate.
        const coordinate = LatLng(10, 20);
        final pixel = projection.pixelForLatLng(coordinate);
        expect(pixel.x.isFinite, isTrue);
        final roundTrip = projection.latLngForPixel(pixel);
        expect(roundTrip.latitude, closeTo(coordinate.latitude, 1e-6));
        expect(roundTrip.longitude, closeTo(coordinate.longitude, 1e-6));

        // A setter is applied before it returns, so it changes later
        // conversions, and the map's own camera stays untouched.
        final before = projection.pixelForLatLng(const LatLng(0, 0));
        projection.setCamera(
          const CameraOptions(center: LatLng(1, 1), zoom: 2),
        );
        expect(projection.getCamera().zoom, closeTo(2, 0.0001));
        final after = projection.pixelForLatLng(const LatLng(0, 0));
        expect(after == before, isFalse);
        expect((await map.cameraQuery()).camera.zoom, closeTo(3, 0.0001));

        // A later map command never reaches an existing projection.
        map.updateCamera(CameraUpdate(camera: const CameraOptions(zoom: 9)));
        await map.cameraQuery();
        expect(projection.getCamera().zoom, closeTo(2, 0.0001));

        // Projection calls remain valid when Dart resumes the isolate on
        // another native thread.
        await Isolate.run(() {});
        projection.setVisibleCoordinates(const [
          LatLng(-1, -1),
          LatLng(1, 1),
        ], const EdgeInsets());
        projection.setVisibleGeometry(
          _jsonBytes('{"type":"Point","coordinates":[0,0]}'),
          const EdgeInsets(),
        );
        await map.close();
        await runtime.close();

        // A projection owns its copy of the transform, so every accessor and
        // setter keeps working after the map and runtime are gone.
        expect(projection.getCamera().center, isNotNull);
        expect(
          projection.pixelForLatLng(const LatLng(0, 0)).x.isFinite,
          isTrue,
        );
        projection.setCamera(
          const CameraOptions(center: LatLng(2, 2), zoom: 5),
        );
        expect(projection.getCamera().zoom, closeTo(5, 0.0001));

        projection.close();
        expect(projection.isClosed, isTrue);
      }
    },
  );

  test(
    'unwrapped coordinate conversions preserve visible world copies',
    () async {
      final runtime = runtimeCreate(runtimeOptionsDefault());
      final map = await runtime.createMap(
        options: const MapOptions(
          initialExtent: LogicalExtent(
            width: 1024,
            height: 512,
            scaleFactor: 1,
          ),
          eventMask: RuntimeEventMask.all,
        ),
      );
      await _expectCommandCommitted(
        map.updateCamera(
          CameraUpdate(
            camera: const CameraOptions(center: LatLng(0, 180), zoom: 0),
          ),
        ),
      );
      const points = [ScreenPoint(0, 256), ScreenPoint(1024, 256)];

      final wrapped = await map.latLngsForPixels(points);
      final unwrapped = await map.latLngsForPixelsUnwrapped(points);
      expect(
        wrapped.every(
          (coordinate) =>
              coordinate.longitude >= -180 && coordinate.longitude <= 180,
        ),
        isTrue,
      );
      expect(unwrapped[1].longitude - unwrapped[0].longitude, greaterThan(360));
      expect(
        (await map.latLngForPixel(points[1])).longitude,
        inInclusiveRange(-180, 180),
      );
      final right = await map.latLngForPixelUnwrapped(points[1]);
      expect(right.longitude, closeTo(unwrapped[1].longitude, 1e-10));

      final projection = await map.projectionCreate();
      expect(
        projection.latLngForPixel(points[1]).longitude,
        inInclusiveRange(-180, 180),
      );
      expect(
        projection.latLngForPixelUnwrapped(points[1]).longitude,
        closeTo(right.longitude, 1e-10),
      );

      projection.close();
      await map.close();
      await runtime.close();
    },
  );

  test('style layer entries survive replacement and map close', () async {
    final runtime = runtimeCreate(runtimeOptionsDefault());
    final map = await runtime.createMap();
    await map.setStyleJson(
      _jsonBytes(
        '{"version":8,"sources":{},"layers":[{"id":"first","type":"background"},{"id":"second","type":"background"}]}',
      ),
    );
    final entries = await map.listStyleLayers();
    await map.setStyleJson(_jsonBytes(_emptyStyleJson));
    expect(await map.listStyleLayers(), isEmpty);
    await map.close();
    await runtime.close();
    expect(entries.map((entry) => entry.id), ['first', 'second']);
    expect(entries.map((entry) => entry.type), ['background', 'background']);
    expect(
      entries.every(
        (entry) => entry.sourceId == null && entry.sourceLayer == null,
      ),
      isTrue,
    );
  });

  test(
    'meters per pixel matches the projection and halves per zoom level',
    () async {
      final runtime = runtimeCreate(runtimeOptionsDefault());
      final map = await runtime.createMap(
        options: const MapOptions(
          initialExtent: LogicalExtent(width: 512, height: 512, scaleFactor: 1),
          eventMask: RuntimeEventMask.all,
        ),
      );
      addTearDown(() async {
        await map.close();
        await runtime.close();
      });
      await map.updateCamera(
        CameraUpdate(
          camera: const CameraOptions(center: LatLng(0, 0), zoom: 3),
        ),
      );
      final projection = await map.projectionCreate();
      addTearDown(projection.close);

      final metersPerPixel = await map.metersPerPixelAtLatitude(45);
      expect(
        projection.metersPerPixelAtLatitude(45),
        closeTo(metersPerPixel, 1e-10),
      );
      await map.updateCamera(
        CameraUpdate(camera: const CameraOptions(zoom: 4)),
      );
      expect(
        await map.metersPerPixelAtLatitude(45),
        closeTo(metersPerPixel / 2, 1e-10),
      );

      expect(
        () => map.metersPerPixelAtLatitude(91),
        throwsA(isA<InvalidArgumentException>()),
      );
      expect(
        () => projection.metersPerPixelAtLatitude(91),
        throwsA(isA<InvalidArgumentException>()),
      );
    },
  );

  test('custom geometry tile callbacks reach their isolate', () async {
    final deliveredTiles = <CanonicalTileId>[];
    final callback =
        NativeCallable<
          raw.mln_custom_geometry_source_tile_callbackFunction
        >.listener((Pointer<Void> _, raw.mln_canonical_tile_id tileId) {
          deliveredTiles.add(
            CanonicalTileId(z: tileId.z, x: tileId.x, y: tileId.y),
          );
        });
    final tileId = Struct.create<raw.mln_canonical_tile_id>();
    tileId.z = 3;
    tileId.x = 4;
    tileId.y = 5;

    callback.nativeFunction
        .asFunction<void Function(Pointer<Void>, raw.mln_canonical_tile_id)>()(
      nullptr,
      tileId,
    );
    await _waitUntil(() => deliveredTiles.isNotEmpty);

    expect(deliveredTiles.single.z, 3);
    expect(deliveredTiles.single.x, 4);
    expect(deliveredTiles.single.y, 5);
    callback.close();
  });

  test('loaded style document and URL read back what was loaded', () async {
    final runtime = runtimeCreate(runtimeOptionsDefault());
    final map = await runtime.createMap();
    try {
      // Nothing parsed and nothing requested yet.
      expect(await map.loadedStyleJson(), isEmpty);
      expect(await map.styleUrl(), '');

      // The document reads back byte-for-byte, so it can be reloaded unchanged.
      map.setStyleJson(_jsonBytes(_emptyStyleJson));
      expect(await map.loadedStyleJson(), _jsonBytes(_emptyStyleJson));
      // Inline JSON clears the URL.
      expect(await map.styleUrl(), '');

      // The URL is request state, recorded before the load can succeed, while
      // the document still reports the style that last parsed.
      map.setStyleUrl('https://example.com/style.json');
      expect(await map.styleUrl(), 'https://example.com/style.json');
      expect(await map.loadedStyleJson(), _jsonBytes(_emptyStyleJson));
    } finally {
      await map.close();
      await runtime.close();
    }
  });

  test('a removal and a map close each release a callback root', () async {
    const sourceId = 'dart-lifecycle-source';
    final runtime = runtimeCreate(runtimeOptionsDefault());
    final map = await runtime.createMap();
    await _expectCommandCommitted(
      map.setStyleJson(_jsonBytes(_emptyStyleJson)),
    );

    await map.addCustomGeometrySource(
      sourceId,
      CustomGeometrySourceOptions(fetchTile: (_) {}),
    );
    final removedProbe = singleCallbackPortProbeForTesting(map)!;
    await _expectCommandCommitted(map.removeStyleSource(sourceId));
    await _waitUntil(() => removedProbe.retirementQueued);
    expect(singleCallbackPortProbeForTesting(map), isNull);

    await map.addCustomGeometrySource(
      sourceId,
      CustomGeometrySourceOptions(fetchTile: (_) {}),
    );
    final closeProbe = singleCallbackPortProbeForTesting(map)!;
    await map.close();
    await runtime.close();
    expect(closeProbe.retirementQueued, isTrue);
    await _waitUntil(() => removedProbe.closed && closeProbe.closed);
  });

  test('a custom MVT vector source removal and map close each release a '
      'callback root', () async {
    const sourceId = 'dart-mvt-lifecycle-source';
    final runtime = runtimeCreate(runtimeOptionsDefault());
    final map = await runtime.createMap();
    await _expectCommandCommitted(
      map.setStyleJson(_jsonBytes(_emptyStyleJson)),
    );

    await map.addCustomMvtVectorSource(
      sourceId,
      CustomMvtVectorSourceOptions(fetchTile: (_) {}),
    );
    final removedProbe = singleCallbackPortProbeForTesting(map)!;
    await _expectCommandCommitted(map.removeStyleSource(sourceId));
    await _waitUntil(() => removedProbe.retirementQueued);
    expect(singleCallbackPortProbeForTesting(map), isNull);

    await map.addCustomMvtVectorSource(
      sourceId,
      CustomMvtVectorSourceOptions(fetchTile: (_) {}),
    );
    final closeProbe = singleCallbackPortProbeForTesting(map)!;
    await map.close();
    await runtime.close();
    expect(closeProbe.retirementQueued, isTrue);
    await _waitUntil(() => removedProbe.closed && closeProbe.closed);
  });

  test('feature state round-trips through the map store', () async {
    final runtime = runtimeCreate(runtimeOptionsDefault());
    final map = await runtime.createMap();
    try {
      await _expectCommandCommitted(
        map.setStyleJson(_jsonBytes(_emptyStyleJson)),
      );
      final selector = FeatureStateSelector(
        sourceId: 'dart-feature-state-source',
        featureId: 'feature-1',
      );

      // The map store answers reads without a render session or a loaded
      // source, and missing state reads back as an empty object.
      expect(
        jsonDecode(utf8.decode(await map.getFeatureState(selector))),
        <String, Object?>{},
      );

      await _expectCommandCommitted(
        map.setFeatureState(selector, _jsonBytes('{"hover":true,"rank":2}')),
      );
      expect(jsonDecode(utf8.decode(await map.getFeatureState(selector))), {
        'hover': true,
        'rank': 2,
      });

      // Removing one key leaves the rest of the feature's state.
      await _expectCommandCommitted(
        map.removeFeatureState(
          FeatureStateSelector(
            sourceId: 'dart-feature-state-source',
            featureId: 'feature-1',
            stateKey: 'hover',
          ),
        ),
      );
      expect(jsonDecode(utf8.decode(await map.getFeatureState(selector))), {
        'rank': 2,
      });

      // A source-wide removal clears the remaining state.
      await _expectCommandCommitted(
        map.removeFeatureState(
          FeatureStateSelector(sourceId: 'dart-feature-state-source'),
        ),
      );
      expect(
        jsonDecode(utf8.decode(await map.getFeatureState(selector))),
        <String, Object?>{},
      );

      // A selector without a feature ID cannot name state to set, and a state
      // key without a feature ID cannot name state to remove.
      expect(
        () => map.setFeatureState(
          FeatureStateSelector(sourceId: 'dart-feature-state-source'),
          _jsonBytes('{"hover":true}'),
        ),
        throwsA(isA<InvalidArgumentException>()),
      );
      expect(
        () => map.removeFeatureState(
          FeatureStateSelector(
            sourceId: 'dart-feature-state-source',
            stateKey: 'hover',
          ),
        ),
        throwsA(isA<InvalidArgumentException>()),
      );
      // Feature state must be one JSON object.
      expect(
        () => map.setFeatureState(selector, _jsonBytes('[1,2]')),
        throwsA(isA<InvalidArgumentException>()),
      );
    } finally {
      await map.close();
      await runtime.close();
    }
  });

  test('nine-patch style images round-trip through the native C ABI', () async {
    final runtime = runtimeCreate(runtimeOptionsDefault());
    final map = await runtime.createMap();
    try {
      map.setStyleJson(_jsonBytes(_emptyStyleJson));
      final image = PremultipliedRgba8Image(
        width: 2,
        height: 2,
        stride: 8,
        pixels: Uint8List(16),
      );
      map.setStyleImage(
        'patch',
        image,
        options: StyleImageOptions(
          stretchX: const [ImageStretch(0, 1)],
          stretchY: const [ImageStretch(0, 1), ImageStretch(1, 2)],
          content: const ImageContent(
            left: 0.5,
            top: 0.5,
            right: 1.5,
            bottom: 1.5,
          ),
          textFitHeight: StyleImageTextFit.proportional,
        ),
      );

      final info = await map.getStyleImageInfo('patch');
      expect(info, isNotNull);
      expect(info!.info.stretchXCount, 1);
      expect(info.info.stretchYCount, 2);
      expect(info.info.content?.right, 1.5);
      // An absent text fit stays distinguishable from a present default.
      expect(info.info.textFitWidth, isNull);
      expect(info.info.textFitHeight, StyleImageTextFit.proportional);

      final stretches = await map.copyStyleImageStretches('patch');
      expect(stretches, isNotNull);
      expect(stretches!.stretchX, [const ImageStretch(0, 1)]);
      expect(stretches.stretchY, [
        const ImageStretch(0, 1),
        const ImageStretch(1, 2),
      ]);
      expect(await map.copyStyleImageStretches('missing'), isNull);

      // A backwards interval is rejected by C.
      expect(
        () => map.setStyleImage(
          'bad',
          image,
          options: StyleImageOptions(stretchX: const [ImageStretch(2, 1)]),
        ),
        throwsA(isA<InvalidArgumentException>()),
      );

      // Options snapshot the caller's lists, so later mutation cannot reach them.
      final callerStretches = [const ImageStretch(0, 1)];
      final snapshotted = StyleImageOptions(stretchX: callerStretches);
      callerStretches.add(const ImageStretch(1, 2));
      expect(snapshotted.stretchX, [const ImageStretch(0, 1)]);
    } finally {
      await map.close();
      await runtime.close();
    }
  });

  test('layer base accessors round-trip through the native C ABI', () async {
    final runtime = runtimeCreate(runtimeOptionsDefault());
    final map = await runtime.createMap();
    try {
      await _expectCommandCommitted(
        map.setStyleJson(
          _jsonBytes(
            '{"version":8,"sources":{"geo":{"type":"geojson","data":'
            '{"type":"FeatureCollection","features":[]}}},"layers":['
            '{"id":"bg","type":"background"},'
            '{"id":"fill","type":"fill","source":"geo"}]}',
          ),
        ),
      );

      expect(await map.copyLayerSourceLayer('fill'), isNull);
      await map.setLayerSourceLayer('fill', sourceLayer: 'roads');
      expect(await map.copyLayerSourceLayer('fill'), 'roads');
      expect(await map.copyLayerSourceId('fill'), 'geo');

      // A layer type that takes no source rejects a source-layer command and
      // preserves its previous value.
      await _expectCommandFailure(
        map.setLayerSourceLayer('bg', sourceLayer: 'roads'),
        MaplibreStatus.invalidArgument,
      );
      expect(await map.copyLayerSourceId('bg'), isNull);

      // An unset zoom range crosses the boundary as infinities, and the
      // reported source ID and source-layer sizes feed the copy operations.
      var fillInfo = (await map.getStyleLayerInfo('fill'))!;
      expect(fillInfo.info.type, 'fill');
      expect(fillInfo.info.minZoom, double.negativeInfinity);
      expect(fillInfo.info.maxZoom, double.infinity);
      expect(fillInfo.info.visibility, StyleLayerVisibility.visible);
      expect(fillInfo.sourceId, 'geo');
      expect(fillInfo.sourceLayer, 'roads');

      map.setLayerMinZoom('fill', 4);
      map.setLayerMaxZoom('fill', 12.5);
      map.setLayerVisibility('fill', StyleLayerVisibility.none);
      fillInfo = (await map.getStyleLayerInfo('fill'))!;
      expect(fillInfo.info.minZoom, 4);
      expect(fillInfo.info.maxZoom, 12.5);
      expect(fillInfo.info.visibility, StyleLayerVisibility.none);

      // A layer that carries no source reports absent source fields.
      final bgInfo = (await map.getStyleLayerInfo('bg'))!;
      expect(bgInfo.info.type, 'background');
      expect(bgInfo.sourceId, isNull);
      expect(bgInfo.sourceLayer, isNull);

      // A missing layer reports null rather than metadata.
      expect(await map.getStyleLayerInfo('missing'), isNull);
    } finally {
      await map.close();
      await runtime.close();
    }
  });

  test(
    'style transition options round-trip through the native C ABI',
    () async {
      const transitionStyleJson =
          '{"version":8,"transition":{"duration":750,"delay":100},'
          '"sources":{},"layers":[]}';
      final runtime = runtimeCreate(runtimeOptionsDefault());
      final map = await runtime.createMap();
      try {
        // A map with no style yet reports no duration or delay. The placement
        // flag always reports, because native always holds a value for it.
        final empty = await map.getStyleTransitionOptions();
        expect(empty.durationMs, isNull);
        expect(empty.delayMs, isNull);
        expect(empty.enablePlacementTransitions, isTrue);

        // The style parser fills in its own 300ms duration for a style that
        // declares no transition.
        map.setStyleJson(_jsonBytes(_emptyStyleJson));
        final parsed = await map.getStyleTransitionOptions();
        expect(parsed.durationMs, 300);
        expect(parsed.delayMs, isNull);

        map.setStyleJson(_jsonBytes(transitionStyleJson));
        final declared = await map.getStyleTransitionOptions();
        expect(declared.durationMs, 750);
        expect(declared.delayMs, 100);
        expect(declared.enablePlacementTransitions, isTrue);

        // A present zero stays distinguishable from an absent field, and an
        // absent field clears what the style declared rather than merging.
        const options = StyleTransitionOptions(
          durationMs: 0,
          enablePlacementTransitions: false,
        );
        map.setStyleTransitionOptions(options);
        expect(await map.getStyleTransitionOptions(), options);
        expect(
          (await map.getStyleTransitionOptions()).hashCode,
          options.hashCode,
        );

        // Omitting the flag leaves the cross-fade on rather than clearing it.
        map.setStyleTransitionOptions(
          const StyleTransitionOptions(durationMs: 250),
        );
        expect(
          (await map.getStyleTransitionOptions()).enablePlacementTransitions,
          isTrue,
        );

        // Loading a style replaces the override with what that style declares.
        map.setStyleJson(_jsonBytes(transitionStyleJson));
        expect(await map.getStyleTransitionOptions(), declared);

        final rejectedCommand = map.setStyleTransitionOptions(
          const StyleTransitionOptions(delayMs: -1),
        );
        await _expectCommandFailure(
          rejectedCommand,
          MaplibreStatus.invalidArgument,
        );
        // The next ordered read observes the last committed options unchanged.
        expect(await map.getStyleTransitionOptions(), declared);
      } finally {
        await map.close();
        await runtime.close();
      }
    },
  );

  test(
    'runtime rules, ambient cache, and offline regions cross the native C ABI',
    () async {
      final cacheSizeRuntime = runtimeCreate(
        const RuntimeOptions(cachePath: ':memory:'),
      );
      // An out-of-domain unsigned value is rejected before crossing into C.
      expect(
        () => cacheSizeRuntime.setMaximumAmbientCacheSize(BigInt.from(-1)),
        throwsA(isA<InvalidArgumentException>()),
      );
      await cacheSizeRuntime.setMaximumAmbientCacheSize(BigInt.zero);
      await cacheSizeRuntime.close();

      final runtime = runtimeCreate(runtimeOptionsDefault());
      expect(runtime.isClosed, isFalse);
      expect(
        () => runtime.setResourceTransform(
          ResourceTransform.resourceRewriteRules(
            AdapterResourceRewriteRules(
              rules: [
                AdapterResourceRewriteRule(
                  kind: 0xffffffff,
                  url: 'https://example.com/original\u0000truncated',
                  replacementUrl: 'https://example.com/replacement',
                ),
              ],
            ),
          ),
        ),
        throwsA(isA<InvalidArgumentException>()),
      );
      expect(
        () => runtime.setResourceTransform(
          ResourceTransform.resourceRewriteRules(
            AdapterResourceRewriteRules(
              rules: [
                AdapterResourceRewriteRule(
                  kind: 0xffffffff,
                  url: 'https://example.com/original',
                  replacementUrl:
                      'https://example.com/replacement\u0000truncated',
                ),
              ],
            ),
          ),
        ),
        throwsA(isA<InvalidArgumentException>()),
      );
      expect(
        () => runtime.setHttpHeaderTransform(
          HttpHeaderTransform.httpHeaderTransformRules(
            AdapterHttpHeaderTransformRules(
              rules: [
                AdapterHttpHeaderTransformRule(
                  kind: 0xffffffff,
                  url: 'https://example.com/',
                  headers: [
                    AdapterHttpHeader(name: 'Bad\u0000Name', value: 'secret'),
                  ],
                ),
              ],
            ),
          ),
        ),
        throwsA(isA<InvalidArgumentException>()),
      );
      expect(
        () => runtime.setHttpHeaderTransform(
          HttpHeaderTransform.httpHeaderTransformRules(
            AdapterHttpHeaderTransformRules(
              rules: [
                AdapterHttpHeaderTransformRule(
                  kind: 0xffffffff,
                  url: 'https://example.com/',
                  headers: [
                    AdapterHttpHeader(name: 'Range\u0000tail', value: 'secret'),
                  ],
                ),
              ],
            ),
          ),
        ),
        throwsA(isA<InvalidArgumentException>()),
      );
      runtime.setHttpHeaderTransform(
        HttpHeaderTransform.httpHeaderTransformRules(
          AdapterHttpHeaderTransformRules(
            rules: [
              AdapterHttpHeaderTransformRule(
                kind: 0xffffffff,
                url: 'https://example.com/**',
                flags: 1,
                headers: [AdapterHttpHeader(name: 'X-Test', value: 'café')],
              ),
            ],
          ),
        ),
      );
      runtime.clearHttpHeaderTransform();
      expect(
        () => runtime.setResourceProvider(
          ResourceProvider.resourceProviderRules(
            AdapterResourceProviderRules(
              rules: [
                AdapterResourceProviderRule(
                  kind: 0xffffffff,
                  requestedUrl: 'https://example.com/provider\u0000truncated',
                  response: ResourceResponse(status: ResourceResponseStatus.ok),
                ),
              ],
            ),
          ),
        ),
        throwsA(isA<InvalidArgumentException>()),
      );
      expect(
        () => runtime.setResourceProvider(
          ResourceProvider.resourceProviderRules(
            AdapterResourceProviderRules(
              rules: [
                AdapterResourceProviderRule(
                  kind: 0xffffffff,
                  requestedUrl: 'https://example.com/provider-error-message',
                  response: ResourceResponse(
                    status: ResourceResponseStatus.error,
                    errorMessage: 'bad\u0000message',
                  ),
                ),
              ],
            ),
          ),
        ),
        throwsA(isA<InvalidArgumentException>()),
      );
      expect(
        () => runtime.setResourceProvider(
          ResourceProvider.resourceProviderRules(
            AdapterResourceProviderRules(
              rules: [
                AdapterResourceProviderRule(
                  kind: 0xffffffff,
                  requestedUrl: 'https://example.com/provider-etag',
                  response: ResourceResponse(
                    status: ResourceResponseStatus.ok,
                    etag: 'etag\u0000tail',
                  ),
                ),
              ],
            ),
          ),
        ),
        throwsA(isA<InvalidArgumentException>()),
      );
      expect(
        () => runtime.setQueuedResourceProvider(
          QueuedResourceProvider(
            routes: [
              AdapterQueuedResourceProviderRoute(
                kind: raw.MLN_ADAPTER_RESOURCE_KIND_ANY,
                url: 'https://example.com/provider\u0000x',
              ),
            ],
            callback: (_, _) {},
          ),
        ),
        throwsA(isA<InvalidArgumentException>()),
      );
      await expectLater(
        () => runtime.createMap(
          options: const MapOptions(
            initialExtent: LogicalExtent(
              width: -1,
              height: 512,
              scaleFactor: 1,
            ),
          ),
        ),
        throwsA(isA<InvalidArgumentException>()),
      );
      runtime.setResourceTransform(
        ResourceTransform.resourceRewriteRules(
          AdapterResourceRewriteRules(
            rules: [
              AdapterResourceRewriteRule(
                kind: ResourceKind.unknown.rawValue,
                url: 'https://example.com/style.json',
                replacementUrl: 'https://example.com/rewritten-style.json',
              ),
            ],
          ),
        ),
      );
      runtime.clearResourceTransform();
      runtime.setResourceProvider(
        ResourceProvider.resourceProviderRules(
          AdapterResourceProviderRules(
            rules: [
              AdapterResourceProviderRule(
                kind: ResourceKind.style.rawValue,
                requestedUrl: 'https://example.com/provider-style.json',
                response: ResourceResponse(
                  status: ResourceResponseStatus.ok,
                  bytes: Uint8List.fromList([123]),
                ),
              ),
            ],
          ),
        ),
      );
      runtime.setQueuedResourceProvider(
        QueuedResourceProvider(
          routes: [
            AdapterQueuedResourceProviderRoute(
              kind: ResourceKind.style.rawValue,
              url: 'https://example.com/provider-style.json',
            ),
          ],
          callback: (request, handle) {
            expect(request.kind, ResourceKind.style);
            handle.complete(
              ResourceResponse(
                status: ResourceResponseStatus.ok,
                bytes: Uint8List.fromList([123]),
              ),
            );
          },
        ),
      );
      await runtime.runAmbientCacheOperation(AmbientCacheOperation.clear);
      expect(await runtime.offlineRegionsList(), isEmpty);
      final offlineRegion = await runtime.offlineRegionCreate(
        const OfflineRegionDefinition.tilePyramid(
          OfflineTilePyramidRegionDefinition(
            styleUrl: 'https://example.com/style.json',
            bounds: LatLngBounds(
              southwest: LatLng(-1, -1),
              northeast: LatLng(1, 1),
            ),
            minZoom: 0,
            maxZoom: 1,
            pixelRatio: 1,
          ),
        ),
        Uint8List.fromList([1, 2, 3]),
      );
      expect(offlineRegion.metadata, [1, 2, 3]);

      // Every region operation reports not found for an unknown region ID.
      final missingRegion = offlineRegion.id + 1000;
      expect(await runtime.offlineRegionGet(missingRegion), isNull);
      await expectLater(
        runtime.offlineRegionUpdateMetadata(
          missingRegion,
          Uint8List.fromList([1]),
        ),
        throwsA(isA<NotFoundException>()),
      );
      await expectLater(
        runtime.offlineRegionGetStatus(missingRegion),
        throwsA(isA<NotFoundException>()),
      );
      await expectLater(
        runtime.offlineRegionSetObserved(missingRegion, true),
        throwsA(isA<NotFoundException>()),
      );
      await expectLater(
        runtime.offlineRegionSetDownloadState(
          missingRegion,
          OfflineRegionDownloadState.inactive,
        ),
        throwsA(isA<NotFoundException>()),
      );
      await expectLater(
        runtime.offlineRegionInvalidate(missingRegion),
        throwsA(isA<NotFoundException>()),
      );
      await expectLater(
        runtime.offlineRegionDelete(missingRegion),
        throwsA(isA<NotFoundException>()),
      );

      runtime.setResourceProvider(
        ResourceProvider.resourceProviderRules(
          AdapterResourceProviderRules(rules: const []),
        ),
      );
      runtime.setQueuedResourceProvider(
        QueuedResourceProvider(
          routes: const [],
          callback: (_, handle) => handle.close(),
        ),
      );
      runtime.clearResourceProvider();
      final operationAfterClose = runtime.runAmbientCacheOperation(
        AmbientCacheOperation.clear,
      );
      await runtime.close();
      expect(runtime.isClosed, isTrue);
      // A command accepted before the release still reaches a terminal result.
      await operationAfterClose;
    },
  );

  test('map snapshots fence the commands that committed them', () async {
    final runtime = runtimeCreate(runtimeOptionsDefault());
    final map = await runtime.createMap(
      options: const MapOptions(
        mapMode: MapMode.static,
        initialExtent: LogicalExtent(width: 256, height: 256, scaleFactor: 1),
        eventMask: RuntimeEventMask.all,
      ),
    );
    expect(map.isClosed, isFalse);
    expect(
      map.snapshotGet().logicalExtent,
      const LogicalExtent(width: 256, height: 256, scaleFactor: 1),
    );
    await _expectCommandCommitted(
      map.setStyleJson(_jsonBytes(_emptyStyleJson)),
    );
    // A static map has no repaint loop, and the rejection is synchronous.
    expect(() => map.requestRepaint(), throwsA(isA<InvalidStateException>()));
    // The scale factor is fixed at creation, so only width and height change.
    expect(
      () => map.resize(
        const LogicalExtent(width: 64, height: 64, scaleFactor: 2),
      ),
      throwsA(isA<InvalidArgumentException>()),
    );
    await _expectCommandCommitted(
      map.resize(const LogicalExtent(width: 64, height: 32, scaleFactor: 1)),
    );
    expect(
      map.snapshotGet().logicalExtent,
      const LogicalExtent(width: 64, height: 32, scaleFactor: 1),
    );

    var throwingLogCalls = 0;
    logSetCallback((_) {
      throwingLogCalls += 1;
      throw StateError('log callback failure');
    });
    map.dumpDebugLogs();
    await _waitUntil(() => throwingLogCalls > 0);
    logClearCallback();
    final copiedEvents = runtime.drainCopiedEvents();
    final styleLoadedEvent = copiedEvents.firstWhere(
      (event) => event.type == RuntimeEventType.mapStyleLoaded,
    );
    expect(styleLoadedEvent.sourceType, RuntimeEventSourceType.map);
    expect(styleLoadedEvent.source, map.identity);
    expect(runtime.drainCopiedEvents(), isEmpty);

    // A committed command reports the published snapshot generation, and a
    // snapshot at or past that generation observes the commit.
    final debugFinished = await map.setDebugOptions(MapDebugOption.tileBorders);
    expect(debugFinished.disposition, CommandDisposition.committed);
    expect(debugFinished.generation, greaterThan(BigInt.zero));
    final debugSnapshot = map.snapshotGet();
    expect(
      debugSnapshot.generation,
      greaterThanOrEqualTo(debugFinished.generation),
    );
    expect(
      debugSnapshot.debugOptions.contains(MapDebugOption.tileBorders),
      isTrue,
    );
    map.setDebugOptions(const MapDebugOption.fromRawValue(0));

    // Each new snapshot field round-trips through its set command.
    final statsFinished = await map.setRenderingStatsViewEnabled(true);
    final statsSnapshot = map.snapshotGet();
    expect(statsSnapshot.renderingStatsViewEnabled, isTrue);
    expect(
      statsSnapshot.generation,
      greaterThanOrEqualTo(statsFinished.generation),
    );
    map.setRenderingStatsViewEnabled(false);
    await _expectCommandCommitted(
      map.setViewportOptions(
        const MapViewportOptions(viewportMode: ViewportMode.flippedY),
      ),
    );
    expect(map.snapshotGet().viewport.viewportMode, ViewportMode.flippedY);
    map.setViewportOptions(
      const MapViewportOptions(viewportMode: ViewportMode.defaultValue),
    );
    await _expectCommandCommitted(
      map.setTileOptions(const MapTileOptions(prefetchZoomDelta: 0)),
    );
    expect(map.snapshotGet().tile.prefetchZoomDelta, 0);
    const cameraBounds = LatLngBounds(
      southwest: LatLng(-10, -20),
      northeast: LatLng(10, 20),
    );
    await _expectCommandCommitted(
      map.setBounds(
        const BoundOptions(bounds: cameraBounds, minZoom: 0, maxZoom: 24),
      ),
    );
    expect(map.snapshotGet().bounds.bounds, cameraBounds);
    await _expectCommandCommitted(
      map.setBounds(const BoundOptions(unbounded: true)),
    );
    expect(map.snapshotGet().bounds.unbounded, isTrue);
    await _expectCommandCommitted(
      map.setProjectionMode(
        const ProjectionMode(axonometric: true, xSkew: 0.25),
      ),
    );
    final axonometric = map.snapshotGet().projectionMode;
    expect(axonometric.axonometric, isTrue);
    expect(axonometric.xSkew, closeTo(0.25, 1e-9));
    const orientation = Quaternion(
      0,
      0,
      0.7071067811865476,
      0.7071067811865476,
    );
    await _expectCommandCommitted(
      map.setFreeCameraOptions(
        const FreeCameraOptions(orientation: orientation),
      ),
    );
    final storedOrientation = map.snapshotGet().freeCamera.orientation!;
    expect(storedOrientation.z, closeTo(orientation.z, 1e-9));
    expect(storedOrientation.w, closeTo(orientation.w, 1e-9));

    await map.close();
    await runtime.close();
  });

  test('gesture phases bracket the camera writes they carry', () async {
    final runtime = runtimeCreate(runtimeOptionsDefault());
    final map = await runtime.createMap();
    expect(map.snapshotGet().gestureInProgress, isFalse);

    await _expectCommandCommitted(
      map.updateCamera(
        CameraUpdate(
          camera: const CameraOptions(zoom: 2),
          gesturePhase: GesturePhase.begin,
        ),
      ),
    );
    expect(map.snapshotGet().gestureInProgress, isTrue);

    await _expectCommandCommitted(
      map.updateCamera(
        CameraUpdate(
          camera: const CameraOptions(zoom: 3),
          gesturePhase: GesturePhase.update,
        ),
      ),
    );
    expect(map.snapshotGet().gestureInProgress, isTrue);

    await _expectCommandCommitted(
      map.updateCamera(
        CameraUpdate(
          camera: const CameraOptions(zoom: 4),
          gesturePhase: GesturePhase.end,
        ),
      ),
    );
    expect(map.snapshotGet().gestureInProgress, isFalse);
    expect((await map.cameraQuery()).camera.zoom, closeTo(4, 0.0001));

    await map.close();
    await runtime.close();
  });

  test('ordered camera commands and queries observe map state', () async {
    final runtime = runtimeCreate(runtimeOptionsDefault());
    final map = await runtime.createMap(
      options: const MapOptions(
        mapMode: MapMode.static,
        initialExtent: LogicalExtent(width: 256, height: 256, scaleFactor: 1),
        eventMask: RuntimeEventMask.all,
      ),
    );
    await _expectCommandCommitted(
      map.setStyleJson(_jsonBytes(_emptyStyleJson)),
    );

    final jumpCommand = map.updateCamera(
      CameraUpdate(camera: const CameraOptions(center: LatLng(0, 0), zoom: 1)),
    );
    final camera = await map.cameraQuery();
    expect((await jumpCommand).disposition, CommandDisposition.committed);
    expect(camera.camera.center, const LatLng(0, 0));
    expect(camera.camera.zoom, closeTo(1, 0.0001));
    runtime.drainCopiedEvents();
    final transitionId = (BigInt.one << 63) - BigInt.one;
    map.updateCamera(
      CameraUpdate(
        camera: const CameraOptions(zoom: 2),
        mode: CameraUpdateMode.ease,
        animation: AnimationOptions(durationMs: 0, transitionId: transitionId),
      ),
    );
    await runtime.barrier();
    final cameraEvents = runtime.drainCopiedEvents();
    final transitionEvent = cameraEvents.firstWhere(
      (event) => event.type == RuntimeEventType.mapCameraTransitionFinished,
    );
    expect(
      (transitionEvent.payload as RuntimeEventPayloadCameraTransitionFinished)
          .value
          .transitionId,
      transitionId,
    );
    expect(
      cameraEvents
          .where((event) => event.type == RuntimeEventType.mapCameraDidChange)
          .map((event) => CameraChangeMode.fromRawValue(event.code)),
      contains(CameraChangeMode.immediate),
    );

    // The published camera snapshot carries the same state the ordered query
    // answers, and the generation it was published at.
    final published = map.cameraSnapshotGet();
    expect(published.$1.zoom, closeTo(2, 0.0001));
    expect(published.$2, greaterThan(BigInt.zero));
    expect((await map.cameraQuery()).camera.zoom, closeTo(2, 0.0001));

    expect(
      (await map.cameraForLatLngBounds(
        const LatLngBounds(southwest: LatLng(-1, -1), northeast: LatLng(1, 1)),
      )).zoom,
      isNotNull,
    );
    expect(
      (await map.cameraForLatLngs(const [LatLng(-1, -1), LatLng(1, 1)])).zoom,
      isNotNull,
    );
    expect(
      (await map.latLngBoundsForCamera(
        const CameraOptions(center: LatLng(0, 0)),
      )).southwest.latitude.isFinite,
      isTrue,
    );
    final centerPixel = await map.pixelForLatLng(const LatLng(0, 0));
    expect(centerPixel.x.isFinite, isTrue);
    expect((await map.latLngForPixel(centerPixel)).latitude.isFinite, isTrue);
    expect(await map.pixelsForLatLngs(const [LatLng(0, 0)]), hasLength(1));
    expect(await map.latLngsForPixels([centerPixel]), hasLength(1));
    final projection = await map.projectionCreate();
    final projectionCamera = projection.getCamera();
    expect(projectionCamera.center, isNotNull);
    expect(projection.pixelForLatLng(const LatLng(0, 0)).x.isFinite, isTrue);
    expect(
      projection.latLngForPixel(const ScreenPoint(0, 0)).latitude.isFinite,
      isTrue,
    );
    projection.setCamera(const CameraOptions(center: LatLng(1, 1), zoom: 2));
    expect(projection.getCamera().zoom, closeTo(2, 0.0001));
    projection.close();
    expect(projection.isClosed, isTrue);

    await map.close();
    await runtime.close();
  });

  test(
    'a superseding camera update finishes the transition it replaced',
    () async {
      final runtime = runtimeCreate(runtimeOptionsDefault());
      final map = await runtime.createMap();
      await _expectCommandCommitted(
        map.setStyleJson(_jsonBytes(_emptyStyleJson)),
      );
      runtime.drainCopiedEvents();

      final transitionId = BigInt.from(4242);
      await _expectCommandCommitted(
        map.updateCamera(
          CameraUpdate(
            camera: const CameraOptions(zoom: 6),
            mode: CameraUpdateMode.ease,
            animation: AnimationOptions(
              durationMs: 5000,
              transitionId: transitionId,
            ),
          ),
        ),
      );
      await runtime.barrier();
      expect(
        runtime.drainCopiedEvents().where(
          (event) => event.type == RuntimeEventType.mapCameraTransitionFinished,
        ),
        isEmpty,
      );

      // A jump replaces the running transition, which reports its end with the
      // identity the replaced transition carried.
      await _expectCommandCommitted(
        map.updateCamera(CameraUpdate(camera: const CameraOptions(zoom: 1))),
      );
      await runtime.barrier();
      final finished = runtime
          .drainCopiedEvents()
          .where(
            (event) =>
                event.type == RuntimeEventType.mapCameraTransitionFinished,
          )
          .toList();
      expect(finished, hasLength(1));
      expect(
        (finished.single.payload as RuntimeEventPayloadCameraTransitionFinished)
            .value
            .transitionId,
        transitionId,
      );

      await map.close();
      await runtime.close();
    },
  );

  test('cancelTransitions ends the transition that was running', () async {
    final runtime = runtimeCreate(runtimeOptionsDefault());
    final map = await runtime.createMap();
    await _expectCommandCommitted(
      map.setStyleJson(_jsonBytes(_emptyStyleJson)),
    );
    runtime.drainCopiedEvents();

    final transitionId = BigInt.from(77);
    await _expectCommandCommitted(
      map.updateCamera(
        CameraUpdate(
          camera: const CameraOptions(zoom: 9),
          mode: CameraUpdateMode.fly,
          animation: AnimationOptions(
            durationMs: 5000,
            transitionId: transitionId,
          ),
        ),
      ),
    );
    await _expectCommandCommitted(map.cancelTransitions());
    await runtime.barrier();

    final finished = runtime
        .drainCopiedEvents()
        .where(
          (event) => event.type == RuntimeEventType.mapCameraTransitionFinished,
        )
        .toList();
    expect(finished, hasLength(1));
    expect(
      (finished.single.payload as RuntimeEventPayloadCameraTransitionFinished)
          .value
          .transitionId,
      transitionId,
    );

    // Cancelling with nothing running commits and changes nothing.
    await _expectCommandCommitted(map.cancelTransitions());

    await map.close();
    await runtime.close();
  });

  test('closing a map cancels the commands it still owes', () async {
    final runtime = runtimeCreate(runtimeOptionsDefault());
    final map = await runtime.createMap(
      options: const MapOptions(
        mapMode: MapMode.static,
        initialExtent: LogicalExtent(width: 256, height: 256, scaleFactor: 1),
        eventMask: RuntimeEventMask.all,
      ),
    );
    await _expectCommandCommitted(
      map.setStyleJson(_jsonBytes(_emptyStyleJson)),
    );

    // A static map with no render session never produces the image, so the
    // request is still owed when the map closes.
    final pending = map.requestStillImage();
    // The expectation is attached before the close, because the cancellation
    // reaches the future while the close is still running.
    final cancelled = expectLater(pending, throwsA(isA<CancelledException>()));
    await map.close();
    await cancelled;

    await runtime.close();
  });

  test('render target attachment rejects invalid descriptors', () async {
    final runtime = runtimeCreate(runtimeOptionsDefault());
    final map = await runtime.createMap();
    expect(
      () => map.metalSurfaceAttach(
        const MetalSurfaceDescriptor(
          extent: RenderTargetExtent(width: -1, height: 16),
          context: MetalContextDescriptor(device: NativePointer.nullPointer),
          layer: NativePointer.nullPointer,
        ),
        renderSessionAttachOptionsDefault(),
      ),
      throwsA(isA<InvalidArgumentException>()),
    );
    expect(
      () => map.metalSurfaceAttach(
        const MetalSurfaceDescriptor(
          extent: RenderTargetExtent(width: 16, height: 16, scaleFactor: 1),
          context: MetalContextDescriptor(device: NativePointer.nullPointer),
          layer: NativePointer.nullPointer,
        ),
        renderSessionAttachOptionsDefault(),
      ),
      throwsA(isA<MaplibreException>()),
    );
    expect(
      () => map.metalOwnedTextureAttach(
        const MetalOwnedTextureDescriptor(
          extent: RenderTargetExtent(width: 16, height: 16, scaleFactor: 1),
          context: MetalContextDescriptor(device: NativePointer.nullPointer),
        ),
        renderSessionAttachOptionsDefault(),
      ),
      throwsA(isA<MaplibreException>()),
    );
    expect(
      () => map.openglOwnedTextureAttach(
        const OpenglOwnedTextureDescriptor(
          extent: RenderTargetExtent(width: -1, height: 16),
          context: OpenglContextDescriptor(
            ownership: OpenglContextOwnership.shared,
            data: OpenglContextDescriptorDataEgl(EglContextDescriptor()),
          ),
        ),
        renderSessionAttachOptionsDefault(),
      ),
      throwsA(isA<InvalidArgumentException>()),
    );
    expect(
      () => map.openglOwnedTextureAttach(
        const OpenglOwnedTextureDescriptor(
          extent: RenderTargetExtent(width: 16, height: 16, scaleFactor: 1),
          context: OpenglContextDescriptor(
            ownership: OpenglContextOwnership.shared,
            data: OpenglContextDescriptorDataEgl(EglContextDescriptor()),
          ),
        ),
        renderSessionAttachOptionsDefault(),
      ),
      throwsA(isA<MaplibreException>()),
    );
    expect(
      () => map.openglBorrowedTextureAttach(
        const OpenglBorrowedTextureDescriptor(
          extent: RenderTargetExtent(width: 16, height: 16, scaleFactor: 1),
          physicalWidth: 16,
          physicalHeight: 16,
          context: OpenglContextDescriptor(
            ownership: OpenglContextOwnership.shared,
            data: OpenglContextDescriptorDataEgl(EglContextDescriptor()),
          ),
          texture: 0,
          target: 0,
        ),
        renderSessionAttachOptionsDefault(),
      ),
      throwsA(isA<MaplibreException>()),
    );
    expect(
      () => map.openglSurfaceAttach(
        const OpenglSurfaceDescriptor(
          extent: RenderTargetExtent(width: 16, height: 16, scaleFactor: 1),
          context: OpenglContextDescriptor(
            ownership: OpenglContextOwnership.shared,
            data: OpenglContextDescriptorDataEgl(EglContextDescriptor()),
          ),
          surface: NativePointer.nullPointer,
        ),
        renderSessionAttachOptionsDefault(),
      ),
      throwsA(isA<MaplibreException>()),
    );

    await map.close();
    await runtime.close();
  });

  test(
    'style sources, layers, and images round-trip through the native C ABI',
    () async {
      final runtime = runtimeCreate(runtimeOptionsDefault());
      final map = await runtime.createMap();
      await _expectCommandCommitted(
        map.setStyleJson(_jsonBytes(_emptyStyleJson)),
      );

      // Style image metadata answers existence, and a removal is a command that
      // commits once and then fails with not-found.
      map.setStyleImage(
        'dart-image',
        PremultipliedRgba8Image(
          width: 1,
          height: 1,
          stride: 4,
          pixels: Uint8List.fromList([255, 0, 0, 255]),
        ),
        options: StyleImageOptions(pixelRatio: 2, sdf: true),
      );
      final styleImageInfo = await map.getStyleImageInfo('dart-image');
      expect(styleImageInfo, isNotNull);
      expect(styleImageInfo!.info.width, 1);
      expect(styleImageInfo.info.height, 1);
      expect(styleImageInfo.info.pixelRatio, closeTo(2, 0.0001));
      expect(styleImageInfo.info.sdf, isTrue);
      expect(await map.copyStyleImagePremultipliedRgba8('dart-image'), [
        255,
        0,
        0,
        255,
      ]);
      await _expectCommandCommitted(map.removeStyleImage('dart-image'));
      expect(await map.getStyleImageInfo('dart-image'), isNull);
      await _expectCommandFailure(
        map.removeStyleImage('dart-image'),
        MaplibreStatus.notFound,
      );

      final sourceIds = await map.listStyleSourceIds();
      expect(sourceIds, isEmpty);
      expect(await map.listStyleLayerIds(), isEmpty);
      // Existence is answered by the info getters' found flag, and removing a
      // missing object fails with not-found.
      expect(await map.getStyleSourceInfo('missing-source'), isNull);
      expect(await map.getStyleLayerInfo('missing-layer'), isNull);
      await _expectCommandFailure(
        map.removeStyleSource('missing-source'),
        MaplibreStatus.notFound,
      );
      await _expectCommandFailure(
        map.removeStyleLayer('missing-layer'),
        MaplibreStatus.notFound,
      );

      map.addGeojsonSourceUrl(
        'dart-geojson-url-source',
        'https://example.com/a.geojson',
      );
      expect(
        (await map.getStyleSourceInfo('dart-geojson-url-source'))!.info.type,
        StyleSourceType.geojson,
      );
      map.setGeojsonSourceUrl(
        'dart-geojson-url-source',
        'https://example.com/b.geojson',
      );
      await _expectCommandCommitted(
        map.removeStyleSource('dart-geojson-url-source'),
      );
      expect(
        () => geojsonSourceDataCreate(
          _jsonBytes('{"type":"FeatureCollection","features":[]}'),
          options: GeojsonSourceOptions(tileSize: 4294967296),
        ),
        throwsA(isA<InvalidArgumentException>()),
      );
      // Cluster validation runs at preparation: clustering rejects a bare
      // geometry because it accepts only point-feature collections.
      expect(
        () => geojsonSourceDataCreate(
          _jsonBytes('{"type":"Point","coordinates":[0,0]}'),
          options: GeojsonSourceOptions(cluster: true),
        ),
        throwsA(isA<InvalidArgumentException>()),
      );
      final clusteredData = geojsonSourceDataCreate(
        _jsonBytes(
          '{"type":"FeatureCollection","features":[{"type":"Feature",'
          '"geometry":{"type":"Point","coordinates":[0,0]},"properties":{}}]}',
        ),
        options: GeojsonSourceOptions(cluster: true, clusterRadius: 60),
      );
      await _expectCommandCommitted(
        map.addGeojsonSourceData(
          'dart-clustered-geojson-source',
          clusteredData,
        ),
      );
      // The map thread rejects data whose baked-in options differ from the
      // source's, reported through the command's terminal event.
      final plainData = geojsonSourceDataCreate(
        _jsonBytes('{"type":"Point","coordinates":[0,0]}'),
      );
      final mismatchedInstallCommand = map.setGeojsonSourceData(
        'dart-clustered-geojson-source',
        plainData,
      );
      // A prepared handle may close as soon as the install command is submitted.
      plainData.close();
      await _expectCommandFailure(
        mismatchedInstallCommand,
        MaplibreStatus.invalidArgument,
      );
      await _expectCommandCommitted(
        map.setGeojsonSourceSynchronousTiling(
          'dart-clustered-geojson-source',
          true,
        ),
      );
      await _expectCommandCommitted(
        map.setGeojsonSourceSynchronousTiling(
          'dart-clustered-geojson-source',
          false,
        ),
      );
      await _expectCommandFailure(
        map.setGeojsonSourceSynchronousTiling('missing-source', true),
        MaplibreStatus.notFound,
      );
      // One prepared handle installs on any number of sources.
      await _expectCommandCommitted(
        map.addGeojsonSourceData('dart-clustered-geojson-copy', clusteredData),
      );
      clusteredData.close();
      // Closing the handle never invalidates a source it was installed on.
      expect(
        await map.getStyleSourceInfo('dart-clustered-geojson-source'),
        isNotNull,
      );
      await _expectCommandCommitted(
        map.removeStyleSource('dart-clustered-geojson-copy'),
      );
      await _expectCommandCommitted(
        map.removeStyleSource('dart-clustered-geojson-source'),
      );
      clusteredData.close();
      expect(clusteredData.isClosed, isTrue);
      expect(
        () => map.addGeojsonSourceData('dart-closed-data', clusteredData),
        throwsA(isA<InvalidArgumentException>()),
      );
      expect(
        () => map.addVectorSourceTiles(
          'dart-vector-invalid-tiles-source',
          const ['https://example.com/{z}/{x}/{y}.mvt'],
          options: const StyleTileSourceOptions(tileSize: 4294967297),
        ),
        throwsA(isA<InvalidArgumentException>()),
      );
      map.addRasterDemSourceTiles(
        'dart-raster-dem-tiles-source',
        const ['https://example.com/{z}/{x}/{y}.png'],
        options: const StyleTileSourceOptions(
          tileSize: 256,
          rasterEncoding: StyleRasterDemEncoding.terrarium,
        ),
      );
      map.addHillshadeLayer(
        'dart-hillshade-layer',
        'dart-raster-dem-tiles-source',
      );
      expect(
        (await map.getStyleLayerInfo('dart-hillshade-layer'))!.info.type,
        'hillshade',
      );
      map.addColorReliefLayer(
        'dart-color-relief-layer',
        'dart-raster-dem-tiles-source',
      );
      expect(
        (await map.getStyleLayerInfo('dart-color-relief-layer'))!.info.type,
        'color-relief',
      );
      map.moveStyleLayer(
        'dart-color-relief-layer',
        beforeLayerId: 'dart-hillshade-layer',
      );
      await _expectCommandCommitted(
        map.removeStyleLayer('dart-color-relief-layer'),
      );
      await _expectCommandCommitted(
        map.removeStyleLayer('dart-hillshade-layer'),
      );
      await _expectCommandCommitted(
        map.removeStyleSource('dart-raster-dem-tiles-source'),
      );
      map.addLocationIndicatorLayer('dart-location-layer');
      expect(
        (await map.getStyleLayerInfo('dart-location-layer'))!.info.type,
        'location-indicator',
      );
      map.setLocationIndicatorLocation(
        'dart-location-layer',
        const LatLng(37.7749, -122.4194),
        0,
      );
      final location =
          jsonDecode(
                utf8.decode(
                  (await map.getLayerProperty(
                    'dart-location-layer',
                    'location',
                  ))!,
                ),
              )
              as List<dynamic>;
      expect(location.cast<num>(), [
        closeTo(37.7749, 1e-6),
        closeTo(-122.4194, 1e-6),
        closeTo(0, 1e-6),
      ]);
      map.setLocationIndicatorBearing('dart-location-layer', 0);
      map.setLocationIndicatorAccuracyRadius('dart-location-layer', 1);
      map.setLocationIndicatorImageName(
        'dart-location-layer',
        LocationIndicatorImageKind.top,
        'dart-location-image',
      );
      await _expectCommandCommitted(
        map.removeStyleLayer('dart-location-layer'),
      );
      const imageSourceCoordinates = [
        LatLng(1, -1),
        LatLng(1, 1),
        LatLng(-1, 1),
        LatLng(-1, -1),
      ];
      map.addImageSourceImage(
        'dart-image-source',
        imageSourceCoordinates,
        PremultipliedRgba8Image(
          width: 1,
          height: 1,
          stride: 4,
          pixels: Uint8List.fromList([0, 255, 0, 255]),
        ),
      );
      expect(
        (await map.getStyleSourceInfo('dart-image-source'))!.info.type,
        StyleSourceType.image,
      );
      expect(
        await map.getImageSourceCoordinates('dart-image-source'),
        imageSourceCoordinates,
      );
      map.setImageSourceUrl(
        'dart-image-source',
        'https://example.com/image.png',
      );
      map.setImageSourceCoordinates(
        'dart-image-source',
        imageSourceCoordinates.reversed.toList(),
      );
      expect(
        await map.getImageSourceCoordinates('dart-image-source'),
        imageSourceCoordinates.reversed.toList(),
      );
      await _expectCommandCommitted(map.removeStyleSource('dart-image-source'));

      expect(
        () => map.addCustomGeometrySource(
          'dart-custom-invalid-source',
          CustomGeometrySourceOptions(fetchTile: (_) {}, tileSize: 4294967297),
        ),
        throwsA(isA<InvalidArgumentException>()),
      );
      expect(
        () => map.addCustomGeometrySource(
          'dart-custom-invalid-buffer-source',
          CustomGeometrySourceOptions(fetchTile: (_) {}, buffer: 4294967297),
        ),
        throwsA(isA<InvalidArgumentException>()),
      );
      expect(
        () => map.addCustomGeometrySource(
          'dart-custom-negative-tile-size-source',
          CustomGeometrySourceOptions(fetchTile: (_) {}, tileSize: -1),
        ),
        throwsA(isA<InvalidArgumentException>()),
      );
      expect(
        () => map.addCustomGeometrySource(
          'dart-custom-negative-buffer-source',
          CustomGeometrySourceOptions(fetchTile: (_) {}, buffer: -1),
        ),
        throwsA(isA<InvalidArgumentException>()),
      );
      map.addCustomGeometrySource(
        'dart-custom-source',
        CustomGeometrySourceOptions(fetchTile: (_) {}),
      );
      expect(
        (await map.getStyleSourceInfo('dart-custom-source'))!.info.type,
        StyleSourceType.customVector,
      );
      map.setCustomGeometrySourceTileData(
        'dart-custom-source',
        const CanonicalTileId(z: 0, x: 0, y: 0),
        _jsonBytes('{"type":"FeatureCollection","features":[]}'),
      );
      map.invalidateCustomGeometrySourceTile(
        'dart-custom-source',
        const CanonicalTileId(z: 0, x: 0, y: 0),
      );
      expect(
        () => map.invalidateCustomGeometrySourceTile(
          'dart-custom-source',
          const CanonicalTileId(z: -1, x: 0, y: 0),
        ),
        throwsA(isA<InvalidArgumentException>()),
      );
      map.invalidateCustomGeometrySourceRegion(
        'dart-custom-source',
        const LatLngBounds(southwest: LatLng(-1, -1), northeast: LatLng(1, 1)),
      );
      await _expectCommandCommitted(
        map.removeStyleSource('dart-custom-source'),
      );
      map.addCustomGeometrySource(
        'dart-custom-source',
        CustomGeometrySourceOptions(fetchTile: (_) {}),
      );
      await _expectCommandCommitted(
        map.removeStyleSource('dart-custom-source'),
      );
      map.addCustomMvtVectorSource(
        'dart-custom-mvt-source',
        CustomMvtVectorSourceOptions(fetchTile: (_) {}),
      );
      expect(
        (await map.getStyleSourceInfo('dart-custom-mvt-source'))!.info.type,
        StyleSourceType.customMvtVector,
      );
      map.setCustomMvtVectorSourceTileData(
        'dart-custom-mvt-source',
        const CanonicalTileId(z: 0, x: 0, y: 0),
        Uint8List(0),
      );
      map.setCustomMvtVectorSourceTileError(
        'dart-custom-mvt-source',
        const CanonicalTileId(z: 0, x: 0, y: 0),
        'tile missing',
      );
      map.invalidateCustomMvtVectorSourceTile(
        'dart-custom-mvt-source',
        const CanonicalTileId(z: 0, x: 0, y: 0),
      );
      await _expectCommandCommitted(
        map.removeStyleSource('dart-custom-mvt-source'),
      );

      final geoJsonData = geojsonSourceDataCreate(
        _jsonBytes(
          '{"type":"Feature","geometry":{"type":"Point","coordinates":[0,0]},'
          '"properties":{"kind":"dart"}}',
        ),
      );
      await _expectCommandCommitted(
        map.addGeojsonSourceData('dart-geojson-source', geoJsonData),
      );
      geoJsonData.close();
      final info = await map.getStyleSourceInfo('dart-geojson-source');
      expect(info, isNotNull);
      expect(info!.info.type, StyleSourceType.geojson);

      expect(info.attribution, isNull);
      expect(await map.listStyleSourceIds(), contains('dart-geojson-source'));

      final updatedGeoJsonData = geojsonSourceDataCreate(
        _jsonBytes('{"type":"Point","coordinates":[2,1]}'),
      );
      map.setGeojsonSourceData('dart-geojson-source', updatedGeoJsonData);
      updatedGeoJsonData.close();
      map.addStyleLayerJson(
        _jsonBytes(
          '{"id":"dart-circle-layer","type":"circle","source":"dart-geojson-source"}',
        ),
      );
      final circleInfo = await map.getStyleLayerInfo('dart-circle-layer');
      expect(circleInfo, isNotNull);
      expect(circleInfo!.info.type, 'circle');
      expect(circleInfo.sourceId, 'dart-geojson-source');
      expect(await map.listStyleLayerIds(), contains('dart-circle-layer'));
      final layerJson = await map.getStyleLayerJson('dart-circle-layer');
      expect(
        jsonDecode(utf8.decode(layerJson!)),
        containsPair('id', 'dart-circle-layer'),
      );

      map.setLayerProperty(
        'dart-circle-layer',
        'circle-radius',
        _jsonBytes('6.5'),
      );
      expect(
        await map.getLayerProperty('dart-circle-layer', 'circle-radius'),
        _jsonBytes('6.5'),
      );
      map.setLayerFilter(
        'dart-circle-layer',
        filter: _jsonBytes('["==",["get","kind"],"dart"]'),
      );
      expect(
        await map.getLayerFilter('dart-circle-layer'),
        _jsonBytes('["==",["get","kind"],"dart"]'),
      );
      map.setLayerFilter('dart-circle-layer', filter: null);
      expect(await map.getLayerFilter('dart-circle-layer'), isNull);

      await _expectCommandCommitted(map.removeStyleLayer('dart-circle-layer'));
      await _expectCommandCommitted(
        map.removeStyleSource('dart-geojson-source'),
      );

      await map.close();
      await runtime.close();
    },
  );
  test('native pointer preserves address value semantics', () {
    const pointer = NativePointer(0x1234);

    expect(pointer.address, 0x1234);
    expect(pointer.isNull, isFalse);
    expect(pointer, equals(const NativePointer(0x1234)));
    expect(pointer.hashCode, equals(const NativePointer(0x1234).hashCode));
    expect({pointer}, contains(const NativePointer(0x1234)));
    expect(NativePointer.nullPointer.isNull, isTrue);
  });

  test('source inspection returns independent copied metadata', () async {
    final runtime = runtimeCreate(runtimeOptionsDefault());
    final map = await runtime.createMap();
    map.setStyleJson(_jsonBytes(_emptyStyleJson));

    const tileUrls = [
      'https://a.example.com/{z}/{x}/{y}.mvt',
      'https://b.example.com/{z}/{x}/{y}.mvt',
    ];
    const bounds = LatLngBounds(
      southwest: LatLng(-12, -34),
      northeast: LatLng(56, 78),
    );
    map.addVectorSourceTiles(
      'inline-vector',
      tileUrls,
      options: const StyleTileSourceOptions(
        minZoom: 0,
        maxZoom: 12,
        attribution: 'Inline attribution',
        scheme: StyleTileScheme.tms,
        bounds: bounds,
        tileSize: 512,
        vectorEncoding: StyleVectorTileEncoding.mlt,
      ),
    );

    final inline = (await map.getStyleSourceInfo('inline-vector'))!;
    expect(inline.info.type, StyleSourceType.vector);
    expect(inline.url, isNull);
    expect(inline.attribution, 'Inline attribution');
    expect(inline.info.tileSize, 512);
    expect(inline.info.vectorEncoding, StyleVectorTileEncoding.mlt);
    expect(inline.info.rasterEncoding, isNull);
    expect(inline.info.tilejson, isNotNull);
    expect(inline.tileUrls!, tileUrls);
    expect(inline.info.tilejson!.minZoom, 0);
    expect(inline.info.tilejson!.maxZoom, 12);
    expect(inline.info.tilejson!.scheme, StyleTileScheme.tms);
    expect(inline.info.bounds, bounds);
    expect(
      () => inline.tileUrls!.add('https://example.com/extra'),
      throwsUnsupportedError,
    );

    map.addVectorSourceUrl(
      'url-vector',
      'https://example.com/vector-tilejson.json',
    );
    final urlBacked = (await map.getStyleSourceInfo('url-vector'))!;
    expect(urlBacked.url, 'https://example.com/vector-tilejson.json');
    expect(urlBacked.info.tilejson, isNull);

    expect(
      await map.copyStyleSourceAttribution('inline-vector'),
      'Inline attribution',
    );
    expect(await map.copyStyleSourceUrl('inline-vector'), isNull);
    expect(
      (await map.getStyleSourceTileUrls('inline-vector'))!.tileUrls,
      tileUrls,
    );
    expect(await map.copyStyleSourceAttribution('url-vector'), isNull);
    expect(await map.copyStyleSourceUrl('url-vector'), urlBacked.url);
    expect((await map.getStyleSourceTileUrls('url-vector'))!.tileUrls, isEmpty);
    expect(await map.copyStyleSourceAttribution('missing-source'), isNull);
    expect(await map.copyStyleSourceUrl('missing-source'), isNull);
    expect(await map.getStyleSourceTileUrls('missing-source'), isNull);

    map.addRasterDemSourceTiles(
      'inline-dem',
      const ['https://example.com/{z}/{x}/{y}.png'],
      options: const StyleTileSourceOptions(
        tileSize: 256,
        rasterEncoding: StyleRasterDemEncoding.terrarium,
      ),
    );
    final rasterDem = (await map.getStyleSourceInfo('inline-dem'))!;
    expect(rasterDem.info.tileSize, 256);
    expect(rasterDem.info.rasterEncoding, StyleRasterDemEncoding.terrarium);
    expect(rasterDem.info.vectorEncoding, isNull);

    await _expectCommandCommitted(map.removeStyleSource('inline-vector'));
    await _expectCommandCommitted(map.removeStyleSource('url-vector'));
    await _expectCommandCommitted(map.removeStyleSource('inline-dem'));
    await map.close();
    await runtime.close();

    expect(inline.tileUrls!, tileUrls);
    expect(urlBacked.url, 'https://example.com/vector-tilejson.json');
  });

  test('style source volatility round-trips through the public API', () async {
    final runtime = runtimeCreate(runtimeOptionsDefault());
    final map = await runtime.createMap();
    map.setStyleJson(_jsonBytes(_emptyStyleJson));
    map.addVectorSourceTiles('dart-volatile-source', const [
      'https://example.com/{z}/{x}/{y}.mvt',
    ]);

    expect(
      (await map.getStyleSourceInfo('dart-volatile-source'))!.info.isVolatile,
      isFalse,
    );
    await _expectCommandCommitted(
      map.setStyleSourceVolatile('dart-volatile-source', true),
    );
    expect(
      (await map.getStyleSourceInfo('dart-volatile-source'))!.info.isVolatile,
      isTrue,
    );
    await _expectCommandCommitted(
      map.setStyleSourceVolatile('dart-volatile-source', false),
    );
    expect(
      (await map.getStyleSourceInfo('dart-volatile-source'))!.info.isVolatile,
      isFalse,
    );
    await _expectCommandFailure(
      map.setStyleSourceVolatile('missing-source', true),
      MaplibreStatus.notFound,
    );

    await map.close();
    await runtime.close();
  });

  test('scoped native values validate before exposing borrowed values', () {
    var live = true;
    void checkLive() {
      if (!live) {
        throw StateError('scope closed');
      }
    }

    final pointer = ScopedNativePointer(
      0x1234,
      checkValid: checkLive,
      debugName: 'test pointer',
    );
    final value = ScopedNativeInt(
      7,
      checkValid: checkLive,
      debugName: 'test value',
    );

    expect(pointer.address, 0x1234);
    expect(pointer.toNativePointer(), const NativePointer(0x1234));
    expect(value.value, 7);

    live = false;
    expect(() => pointer.address, throwsStateError);
    expect(() => value.value, throwsStateError);
  });

  test(
    'runtime and map survive isolate execution and await resumption',
    () async {
      final runtime = runtimeCreate(
        const RuntimeOptions(cachePath: ':memory:'),
      );
      final map = await runtime.createMap(
        options: const MapOptions(
          initialExtent: LogicalExtent(width: 64, height: 64, scaleFactor: 1),
          eventMask: RuntimeEventMask.all,
        ),
      );

      await Isolate.run(() {});

      final before = map.snapshotGet();
      final completion = map.requestRepaint();
      final camera = await map.cameraQuery();
      expect((await completion).disposition, CommandDisposition.committed);
      expect(camera.generation, greaterThanOrEqualTo(before.generation));

      await map.close();
      await runtime.close();
      expect(map.isClosed, isTrue);
      expect(runtime.isClosed, isTrue);
    },
  );

  test('native execution progresses without blocking the isolate', () async {
    final runtime = runtimeCreate(runtimeOptionsDefault());
    final map = await runtime.createMap();

    // An ordered query hands the isolate back to its event loop while native
    // code works, so a zero-delay timer runs before the query answers.
    var timerRan = false;
    unawaited(Future<void>.delayed(Duration.zero, () => timerRan = true));
    await map.cameraQuery();
    expect(timerRan, isTrue);

    map.setStyleUrl('unsupported://autonomous-progress.json');
    final event = await _waitUntilEvent(
      runtime,
      (candidate) => candidate.type == RuntimeEventType.mapLoadingFailed,
    );
    expect(event.type, RuntimeEventType.mapLoadingFailed);

    await map.close();
    await runtime.close();
  });

  test('runtime close waits for native teardown', () async {
    final runtime = runtimeCreate(runtimeOptionsDefault());
    final map = await runtime.createMap();
    map.setStyleUrl('unsupported://runtime-teardown.json');

    // The live map rejects the release before native teardown starts.
    expect(runtime.close, throwsA(isA<InvalidStateException>()));
    expect(runtime.isClosed, isFalse);

    await map.close();
    // This future resolves only after the runtime's threads and its released
    // map's teardown are gone.
    await runtime.close();
    expect(runtime.isClosed, isTrue);

    // A second close no-ops rather than releasing again.
    await runtime.close();
    expect(runtime.isClosed, isTrue);
  });
}

void _clearLogCallback() {
  logClearCallback();
}

Future<void> _expectCommandCommitted(
  Future<CommandCompletion> completion,
) async {
  expect((await completion).disposition, CommandDisposition.committed);
}

Future<RuntimeEvent> _waitUntilEvent(
  RuntimeHandle runtime,
  bool Function(RuntimeEvent event) predicate,
) async {
  RuntimeEvent? matched;
  await _waitUntil(() {
    for (final event in runtime.drainCopiedEvents()) {
      if (predicate(event)) {
        matched = event;
      }
    }
    return matched != null;
  });
  return matched!;
}

Future<void> _waitUntil(
  bool Function() condition, {
  Duration timeout = const Duration(seconds: 5),
}) async {
  final deadline = DateTime.now().add(timeout);
  while (!condition()) {
    await Future<void>.delayed(const Duration(milliseconds: 1));
    if (DateTime.now().isAfter(deadline)) {
      fail('condition was not met within $timeout');
    }
  }
}
