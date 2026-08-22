import 'dart:ffi';
import 'dart:convert';
import 'dart:isolate';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';
import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.dart';
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.g.dart'
    as raw;
import 'package:maplibre_native_ffi/src/maplibre.dart'
    show logCallbackStateForTesting;
import 'package:maplibre_native_ffi/src/runtime/runtime.dart'
    show
        customGeometryCallbackProbeForTesting,
        customMvtVectorCallbackProbeForTesting;
import 'package:test/test.dart';

const _emptyStyleJson = '{"version":8,"sources":{},"layers":[]}';

/// Every event type but style-loaded, the type a host most plausibly clears
/// while still expecting custom-geometry callback state to be released.
final _maskWithoutStyleLoaded = RuntimeEventMask(
  RuntimeEventMask.all.value & ~RuntimeEventMask.mapStyleLoaded.value,
);

Uint8List _jsonBytes(String value) => Uint8List.fromList(utf8.encode(value));

Future<CommandCompletion> _expectCommandFailure(
  Future<CommandCompletion> future,
  MaplibreStatus status,
) async {
  final completion = await future;
  expect(completion.disposition, CommandDisposition.failed);
  expect(completion.status, status);
  expect(completion.diagnostic, isNotEmpty);
  return completion;
}

/// Dispatches one record through the registered adapter log callback, the way
/// MapLibre's logging threads do, and reports the consume value native code
/// sees.
int _dispatchLogRecord(
  MaplibreNativeCApi c, {
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
  test('map options carry FastPFOR decoding to native', () async {
    expect(const MapOptions().fastPforEnabled, isFalse);
    expect(const MapOptions(fastPforEnabled: true), isNot(const MapOptions()));

    final runtime = RuntimeHandle.create();
    final map = await runtime.createMap(
      options: const MapOptions(width: 64, height: 64, fastPforEnabled: true),
    );
    expect(
      map.snapshot().size,
      const MapSize(width: 64, height: 64, scaleFactor: 1),
    );
    await map.close();
    await runtime.close();
  });
  test('one drain takes every event a style load queued', () async {
    final runtime = RuntimeHandle.create();
    addTearDown(runtime.close);

    // A fresh runtime has nothing queued.
    final empty = runtime.drainEvents();
    expect(empty.events, isEmpty);

    final map = await runtime.createMap();
    addTearDown(map.close);
    // Start the style load with an empty queue.

    map.setStyleJson(_jsonBytes(_emptyStyleJson));
    late RuntimeEventBatch batch;
    await _waitUntil(() {
      batch = runtime.drainEvents();
      return batch.events.any(
        (event) => event.eventType == RuntimeEventType.mapStyleLoaded,
      );
    });

    // A style load reports several event types, and one drain takes them all.
    expect(batch.events.length, greaterThan(1));
    expect(
      batch.events.map((event) => event.eventType),
      contains(RuntimeEventType.mapRenderUpdateAvailable),
    );
  });

  test('both handles report and narrow their event masks', () async {
    final runtime = RuntimeHandle.create();
    addTearDown(runtime.close);
    final map = await runtime.createMap();
    addTearDown(map.close);

    // The default options select every event type.
    expect(runtime.eventMask, RuntimeEventMask.all);
    expect(map.eventMask, RuntimeEventMask.all);

    map.setEventMask(RuntimeEventMask.all);
    runtime.setEventMask(RuntimeEventMask.all);
    expect(map.eventMask, RuntimeEventMask.all);
    expect(runtime.eventMask, RuntimeEventMask.all);

    // A host reads the mask, clears one bit, and writes it back; every other
    // bit survives.
    final withoutIdle = RuntimeEventMask(
      map.eventMask.value & ~RuntimeEventMask.mapIdle.value,
    );
    map.setEventMask(withoutIdle);
    expect(map.eventMask, withoutIdle);
    expect(map.eventMask.contains(RuntimeEventType.mapIdle), isFalse);
    expect(map.eventMask.contains(RuntimeEventType.mapStyleLoaded), isTrue);

    const outsideAll = RuntimeEventMask(1 << 40);
    expect(
      () => map.setEventMask(outsideAll),
      throwsA(isA<InvalidArgumentException>()),
    );
    expect(
      () => runtime.setEventMask(outsideAll),
      throwsA(isA<InvalidArgumentException>()),
    );
    // A rejected mask leaves the installed one in place.
    expect(map.eventMask, withoutIdle);
  });

  test(
    'a narrowed map mask keeps cleared event types out of a batch',
    () async {
      final runtime = RuntimeHandle.create();
      addTearDown(runtime.close);
      final map = await runtime.createMap();
      addTearDown(map.close);
      map.setEventMask(
        RuntimeEventMask.mapStyleLoaded | RuntimeEventMask.mapLoadingFailed,
      );

      map.setStyleJson(_jsonBytes(_emptyStyleJson));
      final types = <RuntimeEventType>{};
      await _waitUntil(() {
        types.addAll(
          runtime.drainEvents().events.map((event) => event.eventType),
        );
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
      final runtime = RuntimeHandle.create();
      addTearDown(runtime.close);
      final map = await runtime.createMap(
        options: MapOptions(eventMask: _maskWithoutStyleLoaded),
      );
      addTearDown(map.close);
      await map.setStyleJson(_jsonBytes(_emptyStyleJson));
      await map.addCustomGeometrySource(
        sourceId,
        CustomGeometrySourceOptions(fetchTile: (_) {}),
      );
      final probe = customGeometryCallbackProbeForTesting(map, sourceId)!;

      // The mask reads back as the host set it, because the binding selects
      // nothing of its own.
      expect(map.eventMask, _maskWithoutStyleLoaded);

      await map.setStyleJson(_jsonBytes(_emptyStyleJson));
      final types = <RuntimeEventType>{};
      await _waitUntil(() {
        types.addAll(
          runtime.drainEvents().events.map((event) => event.eventType),
        );
        return probe.retirementQueued;
      });
      expect(types, isNot(contains(RuntimeEventType.mapStyleLoaded)));
      expect(customGeometryCallbackProbeForTesting(map, sourceId), isNull);
      await _waitUntil(() => probe.closed);
    },
  );

  test(
    'a style replacement releases a custom MVT vector source with style loads '
    'unselected',
    () async {
      const sourceId = 'dart-released-mvt-source';
      final runtime = RuntimeHandle.create();
      addTearDown(runtime.close);
      final map = await runtime.createMap(
        options: MapOptions(eventMask: _maskWithoutStyleLoaded),
      );
      addTearDown(map.close);
      await map.setStyleJson(_jsonBytes(_emptyStyleJson));
      await map.addCustomMvtVectorSource(
        sourceId,
        CustomMvtVectorSourceOptions(fetchTile: (_) {}),
      );
      final probe = customMvtVectorCallbackProbeForTesting(map, sourceId)!;

      expect(map.eventMask, _maskWithoutStyleLoaded);

      await map.setStyleJson(_jsonBytes(_emptyStyleJson));
      final types = <RuntimeEventType>{};
      await _waitUntil(() {
        types.addAll(
          runtime.drainEvents().events.map((event) => event.eventType),
        );
        return probe.retirementQueued;
      });
      expect(types, isNot(contains(RuntimeEventType.mapStyleLoaded)));
      expect(customMvtVectorCallbackProbeForTesting(map, sourceId), isNull);
      await _waitUntil(() => probe.closed);
    },
  );

  test('process-global APIs cross the native C ABI', () async {
    expect(Maplibre.cVersion(), greaterThanOrEqualTo(0));
    final backends = Maplibre.supportedRenderBackends();
    expect(backends.bits, greaterThanOrEqualTo(0));
    expect(backends.contains(const RenderBackendMask(0)), isTrue);
    expect(RenderBackendMask.opengl.bits, 1 << 2);
    final openGLProviders = Maplibre.supportedOpenGLContextProviders();
    expect(openGLProviders.bits, greaterThanOrEqualTo(0));
    expect(
      const OpenGLContextProviderMask(
        3,
      ).contains(OpenGLContextProviderMask.wgl),
      isTrue,
    );
    expect(
      const OpenGLContextProviderMask(
        3,
      ).contains(OpenGLContextProviderMask.egl),
      isTrue,
    );

    final meters = Maplibre.projectedMetersForLatLng(const LatLng(0, 0));
    expect(meters.northing.isFinite, isTrue);
    expect(meters.easting.isFinite, isTrue);
    expect(
      Maplibre.latLngForProjectedMeters(meters).latitude,
      closeTo(0, 0.0001),
    );

    final status = Maplibre.networkStatus();
    expect(
      status.rawValue,
      isIn([NetworkStatus.online.rawValue, NetworkStatus.offline.rawValue]),
    );
    Maplibre.setNetworkStatus(status);
    final logRecords = <LogRecord>[];
    Maplibre.setLogCallback(logRecords.add);
    Maplibre.setAsyncLogSeverityMask(LogSeverityMask.defaultMask);
    Maplibre.restoreDefaultAsyncLogSeverityMask();
    Maplibre.clearLogCallback();
  });

  test(
    'render target extents report their physical size through native',
    () async {
      final size = const RenderTargetExtent(
        width: 65,
        height: 33,
        scaleFactor: 1.5,
      ).physicalSize();
      expect(size.width, 98);
      expect(size.height, 50);
    },
  );

  test('process-global log callbacks retire across isolates', () async {
    Maplibre.setLogCallback((_) {});
    await Isolate.run(_clearLogCallback);
    await Future<void>.delayed(Duration.zero);

    Maplibre.setLogCallback((_) {});
    Maplibre.clearLogCallback();
  });

  test('log callback replacement and clear change native delivery', () async {
    final c = MaplibreNativeCApi.open();
    final first = <LogRecord>[];
    final replacement = <LogRecord>[];

    Maplibre.setLogCallback(first.add, consume: true);
    expect(
      _dispatchLogRecord(
        c,
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

    Maplibre.setLogCallback(replacement.add);
    expect(
      _dispatchLogRecord(
        c,
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

    Maplibre.clearLogCallback();
    expect(
      _dispatchLogRecord(
        c,
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
      final runtime = RuntimeHandle.create();
      runtime.setResourceProviderRules([
        ResourceProviderRule(
          kind: ResourceKind.style,
          requestedUrl: styleUrl,
          response: ResourceResponse(
            status: ResourceResponseStatus.ok,
            bytes: Uint8List.fromList(_emptyStyleJson.codeUnits),
          ),
        ),
      ]);
      final map = await runtime.createMap();

      map.setStyleUrl(styleUrl);
      final event = await _waitUntilEvent(
        runtime,
        (candidate) =>
            candidate.eventType == RuntimeEventType.mapStyleLoaded &&
            candidate.source is MapRuntimeEventSource,
      );
      expect((event.source as MapRuntimeEventSource).map, same(map));

      await map.close();
      await runtime.close();
    },
  );

  test('unmatched provider routes pass through to native loading', () async {
    const unmatchedUrl = 'custom://dart-provider-pass-through.json';
    final runtime = RuntimeHandle.create();
    var providerCalls = 0;
    runtime.setResourceProvider(
      ResourceProvider(
        routes: const [
          ResourceProviderRoute(
            kind: ResourceKind.style,
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
      (candidate) => candidate.eventType == RuntimeEventType.mapLoadingFailed,
    );
    expect(event.source, isA<MapRuntimeEventSource>());
    expect(providerCalls, 0);

    await map.close();
    await runtime.close();
  });

  test('queued resource provider callbacks cross the native C ABI', () async {
    const styleUrl = 'custom://dart-provider-style.json';
    final runtime = RuntimeHandle.create();
    final requests = <ResourceRequest>[];
    late Future<bool> completion;
    late ResourceRequestHandle ownerToken;

    runtime.setResourceProvider(
      ResourceProvider(
        routes: const [
          ResourceProviderRoute(kind: ResourceKind.style, url: styleUrl),
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
          completion = _completeTransferredRequest(handle);
        },
      ),
    );

    final map = await runtime.createMap();
    map.setStyleUrl(styleUrl);
    await _waitUntilCondition(runtime, () => requests.isNotEmpty);

    ownerToken.waitUntilRetired();
    expect(
      () => ownerToken.cancelled(),
      throwsA(isA<InvalidArgumentException>()),
    );
    ownerToken.close();
    await map.close();
    await runtime.close();
    expect(await completion, isTrue);
  });

  // BND-155: a configured URI-scheme alias reaches the provider as the alias,
  // alongside the URL the built-in network path would have fetched.
  test('queued resource provider sees scheme alias and resolved URL', () async {
    const aliasUrl = 'maplibre://maps/style';
    final runtime = RuntimeHandle.create();
    final requests = <ResourceRequest>[];

    runtime.setResourceProvider(
      ResourceProvider(
        routes: const [
          ResourceProviderRoute(
            kind: ResourceKind.style,
            url: aliasUrl,
            useRequestedUrl: true,
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
    await _waitUntilCondition(runtime, () => requests.isNotEmpty);

    expect(requests.first.requestedUrl, aliasUrl);
    expect(
      requests.first.resolvedUrl,
      'https://demotiles.maplibre.org/style.json',
    );

    await map.close();
    await runtime.close();
  });

  // BND-156: a glob route claims a URL family whose members are known only
  // when they are requested.
  test('queued resource provider glob routes claim a URL family', () async {
    const origin = 'custom://dart-provider-glob/';
    final runtime = RuntimeHandle.create();
    final claimed = <String>[];

    runtime.setResourceProvider(
      ResourceProvider(
        routes: const [
          ResourceProviderRoute(url: '$origin**', matchGlob: true),
        ],
        callback: (request, handle) {
          claimed.add(request.resolvedUrl);
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
      (candidate) => candidate.eventType == RuntimeEventType.mapStyleLoaded,
    );

    // A pattern matches the complete URL, so a URL that merely contains the
    // route's origin stays with native loading.
    map.setStyleUrl('custom://elsewhere/${origin}style.json');
    await _waitUntilEvent(
      runtime,
      (candidate) => candidate.eventType == RuntimeEventType.mapLoadingFailed,
    );

    expect(claimed, ['${origin}unenumerated/style.json']);

    await map.close();
    await runtime.close();
  });

  // BND-157: a route picks which of the request's two URLs it compares, so a
  // configured URI-scheme alias is reachable by the alias and by the URL the
  // built-in network path would have fetched.
  test(
    'queued resource provider routes pick requested or resolved URL',
    () async {
      const aliasUrl = 'maplibre://maps/style';
      const normalizedUrl = 'https://demotiles.maplibre.org/style.json';

      Future<ResourceRequest> claimedBy(ResourceProviderRoute route) async {
        final runtime = RuntimeHandle.create();
        final requests = <ResourceRequest>[];
        runtime.setResourceProvider(
          ResourceProvider(
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
        await _waitUntilCondition(runtime, () => requests.isNotEmpty);
        await map.close();
        await runtime.close();
        return requests.single;
      }

      final byResolved = await claimedBy(
        const ResourceProviderRoute(
          kind: ResourceKind.style,
          url: normalizedUrl,
        ),
      );
      expect(byResolved.requestedUrl, aliasUrl);

      final byRequested = await claimedBy(
        const ResourceProviderRoute(
          kind: ResourceKind.style,
          url: aliasUrl,
          useRequestedUrl: true,
        ),
      );
      expect(byRequested.resolvedUrl, normalizedUrl);
    },
  );

  test('cancelled transferred requests complete terminally', () async {
    const styleUrl = 'custom://dart-provider-cancelled.json';
    final runtime = RuntimeHandle.create();
    ResourceRequestHandle? token;

    runtime.setResourceProvider(
      ResourceProvider(
        routes: const [
          ResourceProviderRoute(kind: ResourceKind.style, url: styleUrl),
        ],
        callback: (_, handle) {
          token = handle;
        },
      ),
    );

    final map = await runtime.createMap();
    map.setStyleUrl(styleUrl);
    await _waitUntilCondition(runtime, () => token != null);
    final liveToken = token!;
    final waiter = Isolate.run(() {
      liveToken.waitUntilRetired();
      return true;
    });

    await map.close();
    await runtime.close();
    await _waitUntil(liveToken.cancelled);
    expect(
      () => liveToken.complete(
        ResourceResponse(status: ResourceResponseStatus.noContent),
      ),
      throwsA(isA<InvalidStateException>()),
    );
    expect(await waiter, isTrue);
  });

  test('transferred response validation preserves the live token', () async {
    const styleUrl = 'custom://dart-provider-token-validation.json';
    final runtime = RuntimeHandle.create();
    ResourceRequestHandle? token;

    runtime.setResourceProvider(
      ResourceProvider(
        routes: const [
          ResourceProviderRoute(kind: ResourceKind.style, url: styleUrl),
        ],
        callback: (_, handle) {
          token = handle;
        },
      ),
    );
    final map = await runtime.createMap();
    map.setStyleUrl(styleUrl);
    await _waitUntilCondition(runtime, () => token != null);

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
      throwsA(isA<InvalidArgumentException>()),
    );

    await map.close();
    await runtime.close();
  });

  test('transferred token aliases have one terminal winner', () async {
    const styleUrl = 'custom://dart-provider-token-race.json';
    final runtime = RuntimeHandle.create();
    ResourceRequestHandle? token;

    runtime.setResourceProvider(
      ResourceProvider(
        routes: const [
          ResourceProviderRoute(kind: ResourceKind.style, url: styleUrl),
        ],
        callback: (_, handle) {
          token = handle;
        },
      ),
    );
    final map = await runtime.createMap();
    map.setStyleUrl(styleUrl);
    await _waitUntilCondition(runtime, () => token != null);

    final liveToken = token!;
    final waiter = Isolate.run(() {
      liveToken.waitUntilRetired();
      return true;
    });
    // Completion is one-shot, so racing two of them can have a single winner.
    // Teardown starts first because closing a map or runtime must not follow
    // an await on this isolate.
    final race = Future.wait([
      Isolate.run(() => _completeTokenAlias(liveToken)),
      Isolate.run(() => _completeTokenAlias(liveToken)),
    ]);
    await map.close();
    await runtime.close();
    final results = await race;

    // Teardown may retire the request before either alias reaches it, so the
    // guarantee is that completion never succeeds twice.
    expect(results.where((result) => result).length, lessThanOrEqualTo(1));
    expect(await waiter, isTrue);
    // The id is retired either way, so every later use is rejected.
    expect(
      () => liveToken.cancelled(),
      throwsA(isA<InvalidArgumentException>()),
    );
  });

  test('queued resource provider callback exceptions are contained', () async {
    const styleUrl = 'custom://dart-provider-throws.json';
    final runtime = RuntimeHandle.create();
    var calls = 0;

    runtime.setResourceProvider(
      ResourceProvider(
        routes: const [
          ResourceProviderRoute(kind: ResourceKind.style, url: styleUrl),
        ],
        callback: (_, _) {
          calls += 1;
          throw StateError('provider failed');
        },
      ),
    );

    final map = await runtime.createMap();
    map.setStyleUrl(styleUrl);
    await _waitUntilCondition(runtime, () => calls > 0);

    await map.close();
    await runtime.close();
  });

  test('closed resource request handles reject further use', () async {
    const styleUrl = 'custom://dart-provider-closed-handle.json';
    final runtime = RuntimeHandle.create();
    var callbackFinished = false;
    var cancelledRejected = false;
    var completionRejected = false;
    var repeatedCloseSucceeded = false;

    runtime.setResourceProvider(
      ResourceProvider(
        routes: const [
          ResourceProviderRoute(kind: ResourceKind.style, url: styleUrl),
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
    await _waitUntilCondition(runtime, () => callbackFinished);

    expect(cancelledRejected, isTrue);
    expect(completionRejected, isTrue);
    expect(repeatedCloseSucceeded, isTrue);

    await map.close();
    await runtime.close();
  });

  test(
    'projection is synchronous and observes only earlier commands',
    () async {
      final runtime = RuntimeHandle.create();
      final map = await runtime.createMap(
        options: const MapOptions(width: 256, height: 256),
      );
      // A projection created after a camera command observes that command.
      map.updateCamera(const CameraOptions(center: LatLng(10, 20), zoom: 3));
      final projection = await map.createProjection();
      try {
        final created = projection.camera();
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
        expect(projection.camera().zoom, closeTo(2, 0.0001));
        final after = projection.pixelForLatLng(const LatLng(0, 0));
        expect(after == before, isFalse);
        expect((await map.queryCamera()).camera.zoom, closeTo(3, 0.0001));

        // A later map command never reaches an existing projection.
        map.updateCamera(const CameraOptions(zoom: 9));
        await map.queryCamera();
        expect(projection.camera().zoom, closeTo(2, 0.0001));

        // Projection calls remain valid when Dart resumes the isolate on
        // another native thread.
        await Isolate.run(() {});
        projection.setVisibleCoordinates(const [LatLng(-1, -1), LatLng(1, 1)]);
        projection.setVisibleGeometry(
          _jsonBytes('{"type":"Point","coordinates":[0,0]}'),
        );
        await map.close();
        await runtime.close();
        expect(projection.camera().center, isNotNull);
      } finally {
        projection.close();
        expect(projection.isClosed, isTrue);
        await map.close();
        await runtime.close();
      }
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
    final runtime = RuntimeHandle.create();
    final map = await runtime.createMap();
    try {
      // Nothing parsed and nothing requested yet.
      expect(await map.getLoadedStyleJson(), isEmpty);
      expect(await map.getStyleUrl(), '');

      // The document reads back byte-for-byte, so it can be reloaded unchanged.
      map.setStyleJson(_jsonBytes(_emptyStyleJson));
      expect(await map.getLoadedStyleJson(), _jsonBytes(_emptyStyleJson));
      // Inline JSON clears the URL.
      expect(await map.getStyleUrl(), '');

      // The URL is request state, recorded before the load can succeed, while
      // the document still reports the style that last parsed.
      map.setStyleUrl('https://example.com/style.json');
      expect(await map.getStyleUrl(), 'https://example.com/style.json');
      expect(await map.getLoadedStyleJson(), _jsonBytes(_emptyStyleJson));
    } finally {
      await map.close();
      await runtime.close();
    }
  });

  test('a removal and a map close each release a callback root', () async {
    const sourceId = 'dart-lifecycle-source';
    final runtime = RuntimeHandle.create();
    final map = await runtime.createMap();
    await map.setStyleJson(_jsonBytes(_emptyStyleJson));

    await map.addCustomGeometrySource(
      sourceId,
      CustomGeometrySourceOptions(fetchTile: (_) {}),
    );
    final removedProbe = customGeometryCallbackProbeForTesting(map, sourceId)!;
    expect(
      await _waitForCommandDisposition(
        runtime,
        map.removeStyleSource(sourceId),
      ),
      CommandDisposition.committed,
    );
    await _waitUntil(() => removedProbe.retirementQueued);
    expect(customGeometryCallbackProbeForTesting(map, sourceId), isNull);

    await map.addCustomGeometrySource(
      sourceId,
      CustomGeometrySourceOptions(fetchTile: (_) {}),
    );
    final closeProbe = customGeometryCallbackProbeForTesting(map, sourceId)!;
    await map.close();
    await runtime.close();
    expect(closeProbe.retirementQueued, isTrue);
    await _waitUntil(() => removedProbe.closed && closeProbe.closed);
  });

  test('a custom MVT vector source removal and map close each release a '
      'callback root', () async {
    const sourceId = 'dart-mvt-lifecycle-source';
    final runtime = RuntimeHandle.create();
    final map = await runtime.createMap();
    await map.setStyleJson(_jsonBytes(_emptyStyleJson));

    await map.addCustomMvtVectorSource(
      sourceId,
      CustomMvtVectorSourceOptions(fetchTile: (_) {}),
    );
    final removedProbe = customMvtVectorCallbackProbeForTesting(map, sourceId)!;
    expect(
      await _waitForCommandDisposition(
        runtime,
        map.removeStyleSource(sourceId),
      ),
      CommandDisposition.committed,
    );
    await _waitUntil(() => removedProbe.retirementQueued);
    expect(customMvtVectorCallbackProbeForTesting(map, sourceId), isNull);

    await map.addCustomMvtVectorSource(
      sourceId,
      CustomMvtVectorSourceOptions(fetchTile: (_) {}),
    );
    final closeProbe = customMvtVectorCallbackProbeForTesting(map, sourceId)!;
    await map.close();
    await runtime.close();
    expect(closeProbe.retirementQueued, isTrue);
    await _waitUntil(() => removedProbe.closed && closeProbe.closed);
  });

  test('feature state round-trips through the map store', () async {
    final runtime = RuntimeHandle.create();
    final map = await runtime.createMap();
    try {
      await map.setStyleJson(_jsonBytes(_emptyStyleJson));
      const selector = FeatureStateSelector(
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
        runtime,
        map.setFeatureState(selector, _jsonBytes('{"hover":true,"rank":2}')),
      );
      expect(jsonDecode(utf8.decode(await map.getFeatureState(selector))), {
        'hover': true,
        'rank': 2,
      });

      // Removing one key leaves the rest of the feature's state.
      await _expectCommandCommitted(
        runtime,
        map.removeFeatureState(
          const FeatureStateSelector(
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
        runtime,
        map.removeFeatureState(
          const FeatureStateSelector(sourceId: 'dart-feature-state-source'),
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
          const FeatureStateSelector(sourceId: 'dart-feature-state-source'),
          _jsonBytes('{"hover":true}'),
        ),
        throwsA(isA<InvalidArgumentException>()),
      );
      expect(
        () => map.removeFeatureState(
          const FeatureStateSelector(
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
    final runtime = RuntimeHandle.create();
    final map = await runtime.createMap();
    try {
      map.setStyleJson(_jsonBytes(_emptyStyleJson));
      final image = PremultipliedRgba8Image(
        width: 2,
        height: 2,
        stride: 8,
        bytes: Uint8List(16),
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
      expect(info!.stretchXCount, 1);
      expect(info.stretchYCount, 2);
      expect(info.content?.right, 1.5);
      // An absent text fit stays distinguishable from a present default.
      expect(info.textFitWidth, isNull);
      expect(info.textFitHeight, StyleImageTextFit.proportional);

      final stretches = await map.getStyleImageStretches('patch');
      expect(stretches, isNotNull);
      expect(stretches!.stretchX, [const ImageStretch(0, 1)]);
      expect(stretches.stretchY, [
        const ImageStretch(0, 1),
        const ImageStretch(1, 2),
      ]);
      expect(await map.getStyleImageStretches('missing'), isNull);

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
    final runtime = RuntimeHandle.create();
    final map = await runtime.createMap();
    try {
      await map.setStyleJson(
        _jsonBytes(
          '{"version":8,"sources":{"geo":{"type":"geojson","data":'
          '{"type":"FeatureCollection","features":[]}}},"layers":['
          '{"id":"bg","type":"background"},'
          '{"id":"fill","type":"fill","source":"geo"}]}',
        ),
      );

      expect(await map.getLayerSourceLayer('fill'), '');
      await map.setLayerSourceLayer('fill', 'roads');
      expect(await map.getLayerSourceLayer('fill'), 'roads');
      expect(await map.getLayerSourceId('fill'), 'geo');

      // A layer type that takes no source rejects a source-layer command and
      // preserves its previous value.
      await _expectCommandFailure(
        map.setLayerSourceLayer('bg', 'roads'),
        MaplibreStatus.invalidArgument,
      );
      expect(await map.getLayerSourceId('bg'), '');

      // An unset zoom range crosses the boundary as infinities, and the
      // reported source ID and source-layer sizes feed the copy operations.
      var fillInfo = (await map.getStyleLayerInfo('fill'))!;
      expect(fillInfo.type, 'fill');
      expect(fillInfo.minZoom, double.negativeInfinity);
      expect(fillInfo.maxZoom, double.infinity);
      expect(fillInfo.visibility, StyleLayerVisibility.visible);
      expect(fillInfo.sourceId, 'geo');
      expect(fillInfo.sourceLayer, 'roads');

      map.setLayerMinZoom('fill', 4);
      map.setLayerMaxZoom('fill', 12.5);
      map.setLayerVisibility('fill', StyleLayerVisibility.none);
      fillInfo = (await map.getStyleLayerInfo('fill'))!;
      expect(fillInfo.minZoom, 4);
      expect(fillInfo.maxZoom, 12.5);
      expect(fillInfo.visibility, StyleLayerVisibility.none);

      // A layer that carries no source reports absent source fields.
      final bgInfo = (await map.getStyleLayerInfo('bg'))!;
      expect(bgInfo.type, 'background');
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
      final runtime = RuntimeHandle.create();
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

  test('runtime and map handles use the native C ABI', () async {
    final cacheSizeRuntime = RuntimeHandle.create(
      options: const RuntimeOptions(cachePath: ':memory:'),
    );
    // An out-of-domain unsigned value is rejected before crossing into C.
    expect(
      () => cacheSizeRuntime.setMaximumAmbientCacheSize(BigInt.from(-1)),
      throwsA(isA<InvalidArgumentException>()),
    );
    await cacheSizeRuntime.setMaximumAmbientCacheSize(BigInt.zero);
    await cacheSizeRuntime.close();

    final runtime = RuntimeHandle.create();
    expect(runtime.isClosed, isFalse);
    expect(
      () => runtime.setResourceUrlRewriteRules([
        const ResourceUrlRewriteRule(
          url: 'https://example.com/original\u0000truncated',
          replacementUrl: 'https://example.com/replacement',
        ),
      ]),
      throwsA(isA<InvalidArgumentException>()),
    );
    expect(
      () => runtime.setResourceUrlRewriteRules([
        const ResourceUrlRewriteRule(
          url: 'https://example.com/original',
          replacementUrl: 'https://example.com/replacement\u0000truncated',
        ),
      ]),
      throwsA(isA<InvalidArgumentException>()),
    );
    expect(
      () => runtime.setHttpHeaderTransformRules([
        const HttpHeaderTransformRule(
          url: 'https://example.com/',
          headers: [HttpHeader(name: 'Bad Name', value: 'secret')],
        ),
      ]),
      throwsA(isA<InvalidArgumentException>()),
    );
    expect(
      () => runtime.setHttpHeaderTransformRules([
        const HttpHeaderTransformRule(
          url: 'https://example.com/',
          headers: [HttpHeader(name: 'Range', value: 'secret')],
        ),
      ]),
      throwsA(isA<InvalidArgumentException>()),
    );
    runtime.setHttpHeaderTransformRules([
      const HttpHeaderTransformRule(
        url: 'https://example.com/**',
        matchGlob: true,
        headers: [HttpHeader(name: 'X-Test', value: 'café')],
      ),
    ]);
    runtime.clearHttpHeaderTransform();
    expect(
      () => runtime.setResourceProviderRules([
        ResourceProviderRule(
          requestedUrl: 'https://example.com/provider\u0000truncated',
          response: ResourceResponse(status: ResourceResponseStatus.ok),
        ),
      ]),
      throwsA(isA<InvalidArgumentException>()),
    );
    expect(
      () => runtime.setResourceProviderRules([
        ResourceProviderRule(
          requestedUrl: 'https://example.com/provider-error-message',
          response: ResourceResponse(
            status: ResourceResponseStatus.error,
            errorMessage: 'bad\u0000message',
          ),
        ),
      ]),
      throwsA(isA<InvalidArgumentException>()),
    );
    expect(
      () => runtime.setResourceProviderRules([
        ResourceProviderRule(
          requestedUrl: 'https://example.com/provider-etag',
          response: ResourceResponse(
            status: ResourceResponseStatus.ok,
            etag: 'etag\u0000tail',
          ),
        ),
      ]),
      throwsA(isA<InvalidArgumentException>()),
    );
    expect(
      () => runtime.setResourceProvider(
        ResourceProvider(
          routes: const [
            ResourceProviderRoute(url: 'https://example.com/provider\u0000x'),
          ],
          callback: (_, _) {},
        ),
      ),
      throwsA(isA<InvalidArgumentException>()),
    );
    await expectLater(
      runtime.createMap(options: const MapOptions(width: -1)),
      throwsA(isA<InvalidArgumentException>()),
    );
    runtime.setResourceUrlRewriteRules([
      const ResourceUrlRewriteRule(
        kind: ResourceKind.unknown,
        url: 'https://example.com/style.json',
        replacementUrl: 'https://example.com/rewritten-style.json',
      ),
    ]);
    runtime.clearResourceTransform();
    runtime.setResourceProviderRules([
      ResourceProviderRule(
        kind: ResourceKind.style,
        requestedUrl: 'https://example.com/provider-style.json',
        response: ResourceResponse(
          status: ResourceResponseStatus.ok,
          bytes: Uint8List.fromList([123]),
        ),
      ),
    ]);
    runtime.setResourceProvider(
      ResourceProvider(
        routes: const [
          ResourceProviderRoute(
            kind: ResourceKind.style,
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
    expect(await runtime.listOfflineRegions(), isEmpty);
    final offlineRegion = await runtime.createOfflineRegion(
      const OfflineTilePyramidRegionDefinition(
        styleUrl: 'https://example.com/style.json',
        bounds: LatLngBounds(
          southwest: LatLng(-1, -1),
          northeast: LatLng(1, 1),
        ),
        minZoom: 0,
        maxZoom: 1,
        pixelRatio: 1,
      ),
      metadata: Uint8List.fromList([1, 2, 3]),
    );
    expect(offlineRegion.metadata, [1, 2, 3]);

    final map = await runtime.createMap(
      options: const MapOptions(mapMode: MapMode.staticMap),
    );
    expect(map.isClosed, isFalse);
    expect(
      map.snapshot().size,
      const MapSize(width: 256, height: 256, scaleFactor: 1),
    );
    runtime.setResourceProviderRules(const []);
    runtime.setResourceProvider(
      ResourceProvider(
        routes: const [],
        callback: (_, handle) => handle.close(),
      ),
    );
    runtime.clearResourceProvider();
    await map.setStyleJson(_jsonBytes(_emptyStyleJson));
    await _expectCommandFailure(
      map.requestRepaint(),
      MaplibreStatus.invalidState,
    );
    var throwingLogCalls = 0;
    Maplibre.setLogCallback((_) {
      throwingLogCalls += 1;
      throw StateError('log callback failure');
    });
    map.dumpDebugLogs();
    await _waitUntil(() => throwingLogCalls > 0);
    Maplibre.clearLogCallback();
    final copiedEvents = runtime.drainEvents().events;
    final styleLoadedEvent = copiedEvents.firstWhere(
      (event) => event.eventType == RuntimeEventType.mapStyleLoaded,
    );
    expect(styleLoadedEvent.source, isA<MapRuntimeEventSource>());
    expect((styleLoadedEvent.source as MapRuntimeEventSource).map, same(map));
    expect(runtime.drainEvents().events, isEmpty);

    // A committed command reports the published snapshot generation, and a
    // snapshot at or past that generation observes the commit.
    final debugCommand = map.setDebugOptions(MapDebugOptions.tileBorders);
    final debugFinished = await _waitForCommandCompletion(
      runtime,
      debugCommand,
    );
    expect(debugFinished.disposition, CommandDisposition.committed);
    expect(debugFinished.generation, greaterThan(BigInt.zero));
    final debugSnapshot = map.snapshot();
    expect(
      debugSnapshot.generation,
      greaterThanOrEqualTo(debugFinished.generation),
    );
    expect(
      debugSnapshot.debugOptions.contains(MapDebugOptions.tileBorders),
      isTrue,
    );
    map.setDebugOptions(MapDebugOptions.none);

    // Style image metadata answers existence, and a removal is a command that
    // commits once and then fails with not-found.
    map.setStyleImage(
      'dart-image',
      PremultipliedRgba8Image(
        width: 1,
        height: 1,
        stride: 4,
        bytes: Uint8List.fromList([255, 0, 0, 255]),
      ),
      options: StyleImageOptions(pixelRatio: 2, sdf: true),
    );
    final styleImageInfo = await map.getStyleImageInfo('dart-image');
    expect(styleImageInfo, isNotNull);
    expect(styleImageInfo!.width, 1);
    expect(styleImageInfo.height, 1);
    expect(styleImageInfo.pixelRatio, closeTo(2, 0.0001));
    expect(styleImageInfo.sdf, isTrue);
    final styleImage = await map.copyStyleImagePremultipliedRgba8('dart-image');
    expect(styleImage, isNotNull);
    expect(styleImage!.bytes, [255, 0, 0, 255]);
    expect(
      await _waitForCommandDisposition(
        runtime,
        map.removeStyleImage('dart-image'),
      ),
      CommandDisposition.committed,
    );
    expect(await map.getStyleImageInfo('dart-image'), isNull);
    await _expectCommandFailure(
      map.removeStyleImage('dart-image'),
      MaplibreStatus.notFound,
    );

    final jumpCommand = map.updateCamera(
      const CameraOptions(center: LatLng(0, 0), zoom: 1),
    );
    final camera = await map.queryCamera();
    expect((await jumpCommand).disposition, CommandDisposition.committed);
    expect(camera.camera.center, const LatLng(0, 0));
    expect(camera.camera.zoom, closeTo(1, 0.0001));
    runtime.drainEvents();
    final transitionId = (BigInt.one << 63) - BigInt.one;
    map.updateCamera(
      const CameraOptions(zoom: 2),
      mode: CameraUpdateMode.ease,
      animation: AnimationOptions(durationMs: 0, transitionId: transitionId),
    );
    await runtime.barrier();
    final cameraEvents = runtime.drainEvents().events;
    final transitionEvent = cameraEvents.firstWhere(
      (event) =>
          event.eventType == RuntimeEventType.mapCameraTransitionFinished,
    );
    expect(
      (transitionEvent.payload as RuntimeEventCameraTransitionFinished)
          .transitionId,
      transitionId,
    );
    expect(
      cameraEvents
          .where(
            (event) => event.eventType == RuntimeEventType.mapCameraDidChange,
          )
          .map((event) => CameraChangeMode.fromRawValue(event.code)),
      contains(CameraChangeMode.immediate),
    );
    // Each new snapshot field round-trips through its set command.
    final statsFinished = await _waitForCommandCompletion(
      runtime,
      map.setRenderingStatsViewEnabled(true),
    );
    final statsSnapshot = map.snapshot();
    expect(statsSnapshot.renderingStatsViewEnabled, isTrue);
    expect(
      statsSnapshot.generation,
      greaterThanOrEqualTo(statsFinished.generation),
    );
    map.setRenderingStatsViewEnabled(false);
    await _waitForCommandCompletion(
      runtime,
      map.setViewportOptions(
        const MapViewportOptions(viewportMode: ViewportMode.flippedY),
      ),
    );
    expect(map.snapshot().viewportOptions.viewportMode, ViewportMode.flippedY);
    map.setViewportOptions(
      const MapViewportOptions(viewportMode: ViewportMode.defaultMode),
    );
    await _waitForCommandCompletion(
      runtime,
      map.setTileOptions(const MapTileOptions(prefetchZoomDelta: 0)),
    );
    expect(map.snapshot().tileOptions.prefetchZoomDelta, 0);
    const cameraBounds = LatLngBounds(
      southwest: LatLng(-10, -20),
      northeast: LatLng(10, 20),
    );
    await _waitForCommandCompletion(
      runtime,
      map.setBounds(
        const BoundOptions(
          bounds: BoundsConstraint.bounded(cameraBounds),
          minZoom: 0,
          maxZoom: 24,
        ),
      ),
    );
    expect(
      map.snapshot().bounds.bounds,
      const BoundsConstraint.bounded(cameraBounds),
    );
    await _waitForCommandCompletion(
      runtime,
      map.setBounds(const BoundOptions(bounds: BoundsConstraint.unbounded())),
    );
    expect(map.snapshot().bounds.bounds, const BoundsConstraint.unbounded());
    final projectionMode = map.projectionMode();
    expect(projectionMode.axonometric, isNotNull);
    map.setProjectionMode(const ProjectionModeOptions(axonometric: false));
    await _waitForCommandCompletion(
      runtime,
      map.setFreeCameraOptions(
        const FreeCameraOptions(orientation: Quaternion(0, 0, 0, 1)),
      ),
    );
    expect(map.snapshot().freeCameraOptions.orientation, isNotNull);
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
    final projection = await map.createProjection();
    final projectionCamera = projection.camera();
    expect(projectionCamera.center, isNotNull);
    expect(projection.pixelForLatLng(const LatLng(0, 0)).x.isFinite, isTrue);
    expect(
      projection.latLngForPixel(const ScreenPoint(0, 0)).latitude.isFinite,
      isTrue,
    );
    projection.setCamera(const CameraOptions(center: LatLng(1, 1), zoom: 2));
    expect(projection.camera().zoom, closeTo(2, 0.0001));
    projection.close();
    expect(projection.isClosed, isTrue);
    expect(
      () => map.attachMetalSurface(
        const MetalSurfaceDescriptor(
          extent: RenderTargetExtent(width: -1, height: 16),
          context: MetalContextDescriptor(device: NativePointer.nullPointer),
          layer: NativePointer.nullPointer,
        ),
      ),
      throwsA(isA<InvalidArgumentException>()),
    );
    expect(
      () => map.attachMetalSurface(
        const MetalSurfaceDescriptor(
          extent: RenderTargetExtent(width: 16, height: 16),
          context: MetalContextDescriptor(device: NativePointer.nullPointer),
          layer: NativePointer.nullPointer,
        ),
      ),
      throwsA(isA<MaplibreException>()),
    );
    expect(
      () => map.attachMetalOwnedTexture(
        const MetalOwnedTextureDescriptor(
          extent: RenderTargetExtent(width: 16, height: 16),
          context: MetalContextDescriptor(device: NativePointer.nullPointer),
        ),
      ),
      throwsA(isA<MaplibreException>()),
    );
    expect(
      () => map.attachOpenGLOwnedTexture(
        const OpenGLOwnedTextureDescriptor(
          extent: RenderTargetExtent(width: -1, height: 16),
          context: EglContextDescriptor(
            display: NativePointer.nullPointer,
            config: NativePointer.nullPointer,
            shareContext: NativePointer.nullPointer,
          ),
        ),
      ),
      throwsA(isA<InvalidArgumentException>()),
    );
    expect(
      () => map.attachOpenGLOwnedTexture(
        const OpenGLOwnedTextureDescriptor(
          extent: RenderTargetExtent(width: 16, height: 16),
          context: EglContextDescriptor(
            display: NativePointer.nullPointer,
            config: NativePointer.nullPointer,
            shareContext: NativePointer.nullPointer,
          ),
        ),
      ),
      throwsA(isA<MaplibreException>()),
    );
    expect(
      () => map.attachOpenGLBorrowedTexture(
        const OpenGLBorrowedTextureDescriptor(
          extent: RenderTargetExtent(width: 16, height: 16),
          physicalWidth: 16,
          physicalHeight: 16,
          context: EglContextDescriptor(
            display: NativePointer.nullPointer,
            config: NativePointer.nullPointer,
            shareContext: NativePointer.nullPointer,
          ),
          texture: 0,
          target: 0,
        ),
      ),
      throwsA(isA<MaplibreException>()),
    );
    expect(
      () => map.attachOpenGLSurface(
        const OpenGLSurfaceDescriptor(
          extent: RenderTargetExtent(width: 16, height: 16),
          context: EglContextDescriptor(
            display: NativePointer.nullPointer,
            config: NativePointer.nullPointer,
            shareContext: NativePointer.nullPointer,
          ),
          surface: NativePointer.nullPointer,
        ),
      ),
      throwsA(isA<MaplibreException>()),
    );

    final sourceIds = await map.listStyleSourceIds();
    expect(sourceIds, contains('org.maplibre.annotations'));
    expect(
      await map.listStyleLayerIds(),
      contains('org.maplibre.annotations.points'),
    );
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

    map.addGeoJsonSourceUrl(
      'dart-geojson-url-source',
      'https://example.com/a.geojson',
    );
    expect(
      (await map.getStyleSourceInfo('dart-geojson-url-source'))!.type,
      SourceType.geoJson,
    );
    map.setGeoJsonSourceUrl(
      'dart-geojson-url-source',
      'https://example.com/b.geojson',
    );
    await _expectCommandCommitted(
      runtime,
      map.removeStyleSource('dart-geojson-url-source'),
    );
    expect(
      () => GeoJsonSourceDataHandle.prepare(
        _jsonBytes('{"type":"FeatureCollection","features":[]}'),
        options: GeoJsonSourceOptions(tileSize: 4294967296),
      ),
      throwsA(isA<InvalidArgumentException>()),
    );
    // Cluster validation runs at preparation: clustering rejects a bare
    // geometry because it accepts only point-feature collections.
    expect(
      () => GeoJsonSourceDataHandle.prepare(
        _jsonBytes('{"type":"Point","coordinates":[0,0]}'),
        options: GeoJsonSourceOptions(cluster: true),
      ),
      throwsA(isA<InvalidArgumentException>()),
    );
    final clusteredData = GeoJsonSourceDataHandle.prepare(
      _jsonBytes(
        '{"type":"FeatureCollection","features":[{"type":"Feature",'
        '"geometry":{"type":"Point","coordinates":[0,0]},"properties":{}}]}',
      ),
      options: GeoJsonSourceOptions(cluster: true, clusterRadius: 60),
    );
    await _expectCommandCommitted(
      runtime,
      map.addGeoJsonSourceData('dart-clustered-geojson-source', clusteredData),
    );
    // The map thread rejects data whose baked-in options differ from the
    // source's, reported through the command's terminal event.
    final plainData = GeoJsonSourceDataHandle.prepare(
      _jsonBytes('{"type":"Point","coordinates":[0,0]}'),
    );
    final mismatchedInstallCommand = map.setGeoJsonSourceData(
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
      runtime,
      map.setGeoJsonSourceSynchronousTiling(
        'dart-clustered-geojson-source',
        true,
      ),
    );
    await _expectCommandCommitted(
      runtime,
      map.setGeoJsonSourceSynchronousTiling(
        'dart-clustered-geojson-source',
        false,
      ),
    );
    await _expectCommandFailure(
      map.setGeoJsonSourceSynchronousTiling('missing-source', true),
      MaplibreStatus.invalidArgument,
    );
    // One prepared handle installs on any number of sources.
    await _expectCommandCommitted(
      runtime,
      map.addGeoJsonSourceData('dart-clustered-geojson-copy', clusteredData),
    );
    clusteredData.close();
    // Closing the handle never invalidates a source it was installed on.
    expect(
      await map.getStyleSourceInfo('dart-clustered-geojson-source'),
      isNotNull,
    );
    await _expectCommandCommitted(
      runtime,
      map.removeStyleSource('dart-clustered-geojson-copy'),
    );
    await _expectCommandCommitted(
      runtime,
      map.removeStyleSource('dart-clustered-geojson-source'),
    );
    clusteredData.close();
    expect(clusteredData.isClosed, isTrue);
    expect(
      () => map.addGeoJsonSourceData('dart-closed-data', clusteredData),
      throwsA(isA<InvalidArgumentException>()),
    );
    map.addVectorSourceUrl(
      'dart-vector-source',
      'https://example.com/vector.json',
    );
    expect(
      (await map.getStyleSourceInfo('dart-vector-source'))!.type,
      SourceType.vector,
    );
    await _expectCommandCommitted(
      runtime,
      map.removeStyleSource('dart-vector-source'),
    );
    expect(
      () => map.addVectorSourceTiles(
        'dart-vector-invalid-tiles-source',
        const ['https://example.com/{z}/{x}/{y}.mvt'],
        options: const TileSourceOptions(tileSize: 4294967297),
      ),
      throwsA(isA<InvalidArgumentException>()),
    );
    map.addVectorSourceTiles(
      'dart-vector-tiles-source',
      const ['https://example.com/{z}/{x}/{y}.mvt'],
      options: const TileSourceOptions(minZoom: 0, maxZoom: 14),
    );
    expect(
      (await map.getStyleSourceInfo('dart-vector-tiles-source'))!.type,
      SourceType.vector,
    );
    await _expectCommandCommitted(
      runtime,
      map.removeStyleSource('dart-vector-tiles-source'),
    );
    map.addRasterSourceTiles('dart-raster-tiles-source', const [
      'https://example.com/{z}/{x}/{y}.png',
    ], options: const TileSourceOptions(tileSize: 256));
    expect(
      (await map.getStyleSourceInfo('dart-raster-tiles-source'))!.type,
      SourceType.raster,
    );
    await _expectCommandCommitted(
      runtime,
      map.removeStyleSource('dart-raster-tiles-source'),
    );
    map.addRasterDemSourceTiles(
      'dart-raster-dem-tiles-source',
      const ['https://example.com/{z}/{x}/{y}.png'],
      options: const TileSourceOptions(
        tileSize: 256,
        rasterDemEncoding: RasterDemEncoding.terrarium,
      ),
    );
    expect(
      (await map.getStyleSourceInfo('dart-raster-dem-tiles-source'))!.type,
      SourceType.rasterDem,
    );
    map.addHillshadeLayer(
      'dart-hillshade-layer',
      'dart-raster-dem-tiles-source',
    );
    expect(
      (await map.getStyleLayerInfo('dart-hillshade-layer'))!.type,
      'hillshade',
    );
    map.addColorReliefLayer(
      'dart-color-relief-layer',
      'dart-raster-dem-tiles-source',
    );
    expect(
      (await map.getStyleLayerInfo('dart-color-relief-layer'))!.type,
      'color-relief',
    );
    map.moveStyleLayer(
      'dart-color-relief-layer',
      beforeLayerId: 'dart-hillshade-layer',
    );
    await _expectCommandCommitted(
      runtime,
      map.removeStyleLayer('dart-color-relief-layer'),
    );
    await _expectCommandCommitted(
      runtime,
      map.removeStyleLayer('dart-hillshade-layer'),
    );
    await _expectCommandCommitted(
      runtime,
      map.removeStyleSource('dart-raster-dem-tiles-source'),
    );
    map.addLocationIndicatorLayer('dart-location-layer');
    expect(
      (await map.getStyleLayerInfo('dart-location-layer'))!.type,
      'location-indicator',
    );
    map.setLocationIndicatorLocation(
      'dart-location-layer',
      const LatLng(37.7749, -122.4194),
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
      runtime,
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
        bytes: Uint8List.fromList([0, 255, 0, 255]),
      ),
    );
    expect(
      (await map.getStyleSourceInfo('dart-image-source'))!.type,
      SourceType.image,
    );
    expect(
      await map.getImageSourceCoordinates('dart-image-source'),
      imageSourceCoordinates,
    );
    map.setImageSourceUrl('dart-image-source', 'https://example.com/image.png');
    map.setImageSourceCoordinates(
      'dart-image-source',
      imageSourceCoordinates.reversed.toList(),
    );
    expect(
      await map.getImageSourceCoordinates('dart-image-source'),
      imageSourceCoordinates.reversed.toList(),
    );
    await _expectCommandCommitted(
      runtime,
      map.removeStyleSource('dart-image-source'),
    );

    final fetchedTiles = <CanonicalTileId>[];
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
      CustomGeometrySourceOptions(fetchTile: fetchedTiles.add),
    );
    expect(
      (await map.getStyleSourceInfo('dart-custom-source'))!.type,
      SourceType.customVector,
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
      runtime,
      map.removeStyleSource('dart-custom-source'),
    );
    map.addCustomGeometrySource(
      'dart-custom-source',
      CustomGeometrySourceOptions(fetchTile: fetchedTiles.add),
    );
    await _expectCommandCommitted(
      runtime,
      map.removeStyleSource('dart-custom-source'),
    );
    map.addCustomMvtVectorSource(
      'dart-custom-mvt-source',
      CustomMvtVectorSourceOptions(fetchTile: fetchedTiles.add),
    );
    expect(
      (await map.getStyleSourceInfo('dart-custom-mvt-source'))!.type,
      SourceType.customMvtVector,
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
      runtime,
      map.removeStyleSource('dart-custom-mvt-source'),
    );

    final geoJsonData = GeoJsonSourceDataHandle.prepare(
      _jsonBytes(
        '{"type":"Feature","geometry":{"type":"Point","coordinates":[0,0]},'
        '"properties":{"kind":"dart"}}',
      ),
    );
    await _expectCommandCommitted(
      runtime,
      map.addGeoJsonSourceData('dart-geojson-source', geoJsonData),
    );
    geoJsonData.close();
    final info = await map.getStyleSourceInfo('dart-geojson-source');
    expect(info, isNotNull);
    expect(info!.type, SourceType.geoJson);
    expect(info.id, 'dart-geojson-source');
    expect(info.attribution, isNull);
    expect(await map.listStyleSourceIds(), contains('dart-geojson-source'));

    final updatedGeoJsonData = GeoJsonSourceDataHandle.prepare(
      _jsonBytes('{"type":"Point","coordinates":[2,1]}'),
    );
    map.setGeoJsonSourceData('dart-geojson-source', updatedGeoJsonData);
    updatedGeoJsonData.close();
    map.addStyleLayerJson(
      _jsonBytes(
        '{"id":"dart-circle-layer","type":"circle","source":"dart-geojson-source"}',
      ),
    );
    final circleInfo = await map.getStyleLayerInfo('dart-circle-layer');
    expect(circleInfo, isNotNull);
    expect(circleInfo!.type, 'circle');
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
      _jsonBytes('["==",["get","kind"],"dart"]'),
    );
    expect(
      await map.getLayerFilter('dart-circle-layer'),
      _jsonBytes('["==",["get","kind"],"dart"]'),
    );
    map.setLayerFilter('dart-circle-layer', null);
    expect(await map.getLayerFilter('dart-circle-layer'), isNull);

    await _expectCommandCommitted(
      runtime,
      map.removeStyleLayer('dart-circle-layer'),
    );
    await _expectCommandCommitted(
      runtime,
      map.removeStyleSource('dart-geojson-source'),
    );

    await map.close();
    expect(map.isClosed, isTrue);
    final operationAfterClose = runtime.runAmbientCacheOperation(
      AmbientCacheOperation.clear,
    );
    await runtime.close();
    expect(runtime.isClosed, isTrue);
    await operationAfterClose;
  });

  test('native pointer preserves address value semantics', () async {
    const pointer = NativePointer(0x1234);

    expect(pointer.address, 0x1234);
    expect(pointer.isNull, isFalse);
    expect(pointer, equals(const NativePointer(0x1234)));
    expect(pointer.hashCode, equals(const NativePointer(0x1234).hashCode));
    expect({pointer}, contains(const NativePointer(0x1234)));
    expect(NativePointer.nullPointer.isNull, isTrue);
  });

  test(
    'BND-109 source inspection returns independent copied metadata',
    () async {
      final runtime = RuntimeHandle.create();
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
        options: const TileSourceOptions(
          minZoom: 0,
          maxZoom: 12,
          attribution: 'Inline attribution',
          scheme: TileScheme.tms,
          bounds: bounds,
          tileSize: 512,
          vectorEncoding: VectorTileEncoding.mlt,
        ),
      );

      final inline = (await map.getStyleSourceInfo('inline-vector'))!;
      expect(inline.type, SourceType.vector);
      expect(inline.url, isNull);
      expect(inline.attribution, 'Inline attribution');
      expect(inline.tileSize, 512);
      expect(inline.vectorEncoding, VectorTileEncoding.mlt);
      expect(inline.rasterDemEncoding, isNull);
      expect(inline.tileJson, isNotNull);
      expect(inline.tileJson!.tileUrls, tileUrls);
      expect(inline.tileJson!.minZoom, 0);
      expect(inline.tileJson!.maxZoom, 12);
      expect(inline.tileJson!.scheme, TileScheme.tms);
      expect(inline.tileJson!.bounds, bounds);
      expect(
        () => inline.tileJson!.tileUrls.add('https://example.com/extra'),
        throwsUnsupportedError,
      );

      map.addVectorSourceUrl(
        'url-vector',
        'https://example.com/vector-tilejson.json',
      );
      final urlBacked = (await map.getStyleSourceInfo('url-vector'))!;
      expect(urlBacked.url, 'https://example.com/vector-tilejson.json');
      expect(urlBacked.tileJson, isNull);

      map.addRasterDemSourceTiles(
        'inline-dem',
        const ['https://example.com/{z}/{x}/{y}.png'],
        options: const TileSourceOptions(
          tileSize: 256,
          rasterDemEncoding: RasterDemEncoding.terrarium,
        ),
      );
      final rasterDem = (await map.getStyleSourceInfo('inline-dem'))!;
      expect(rasterDem.tileSize, 256);
      expect(rasterDem.rasterDemEncoding, RasterDemEncoding.terrarium);
      expect(rasterDem.vectorEncoding, isNull);

      await _expectCommandCommitted(
        runtime,
        map.removeStyleSource('inline-vector'),
      );
      await _expectCommandCommitted(
        runtime,
        map.removeStyleSource('url-vector'),
      );
      await _expectCommandCommitted(
        runtime,
        map.removeStyleSource('inline-dem'),
      );
      await map.close();
      await runtime.close();

      expect(inline.id, 'inline-vector');
      expect(inline.tileJson!.tileUrls, tileUrls);
      expect(urlBacked.url, 'https://example.com/vector-tilejson.json');
      expect(TileScheme.fromRaw(91).rawValue, 91);
      expect(VectorTileEncoding.fromRaw(92).rawValue, 92);
      expect(RasterDemEncoding.fromRaw(93).rawValue, 93);
    },
  );

  test(
    'scoped native values validate before exposing borrowed values',
    () async {
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
    },
  );

  test('native buffer owns reusable native byte storage', () async {
    final buffer = NativeBuffer(4);
    try {
      buffer.writeBytes(Uint8List.fromList([42]));

      expect(buffer.byteLength, 4);
      expect(buffer.isClosed, isFalse);
      expect(buffer.copyBytes(length: 1).single, 42);
      expect(() => buffer.copyBytes(length: 5), throwsRangeError);
      expect(() => buffer.writeBytes(Uint8List(5)), throwsRangeError);
    } finally {
      buffer.close();
    }

    expect(buffer.isClosed, isTrue);
    expect(() => buffer.copyBytes(), throwsStateError);
    expect(() => NativeBuffer(0), throwsArgumentError);
  });

  test('runtime value wrappers preserve unknown raw values', () async {
    final eventType = RuntimeEventType.fromRawValue(0xfeed);
    final sourceType = RuntimeEventSourceType.fromRawValue(0xbeef);
    final renderMode = RenderMode.fromRawValue(42);
    final renderResult = RenderResult.fromRawValue(42);

    const unknownDefinition = UnknownOfflineRegionDefinition(101);
    final ownership = OpenGLContextOwnership.fromRawValue(102);
    final clientApi = OpenGLClientApi.fromRawValue(103);

    expect(eventType.rawValue, 0xfeed);
    expect(eventType, RuntimeEventType.fromRawValue(0xfeed));
    expect(sourceType.rawValue, 0xbeef);
    expect(renderMode.name, 'unknown(42)');
    expect(renderResult, RenderResult.fromRawValue(42));
    expect(renderResult.name, 'unknown(42)');

    expect(unknownDefinition.rawType, 101);
    expect(ownership, OpenGLContextOwnership.fromRawValue(102));
    expect(ownership.name, 'unknown(102)');
    expect(clientApi.rawValue, 103);
  });

  test(
    'runtime and map survive isolate execution and await resumption',
    () async {
      final runtime = RuntimeHandle.create(
        options: const RuntimeOptions(cachePath: ':memory:'),
      );
      final map = await runtime.createMap(
        options: const MapOptions(width: 64, height: 64),
      );

      await Isolate.run(() {});

      final before = map.snapshot();
      final completion = map.requestRepaint();
      final camera = await map.queryCamera();
      expect((await completion).disposition, CommandDisposition.committed);
      expect(camera.generation, greaterThanOrEqualTo(before.generation));

      await map.close();
      await runtime.close();
      expect(map.isClosed, isTrue);
      expect(runtime.isClosed, isTrue);
    },
  );

  test('native execution progresses without blocking the isolate', () async {
    final runtime = RuntimeHandle.create();
    final map = await runtime.createMap();
    map.setStyleUrl('unsupported://autonomous-progress.json');

    final event = await _waitUntilEvent(
      runtime,
      (candidate) => candidate.eventType == RuntimeEventType.mapLoadingFailed,
    );
    expect(event.eventType, RuntimeEventType.mapLoadingFailed);

    await map.close();
    await runtime.close();
  });
}

Future<bool> _completeTransferredRequest(ResourceRequestHandle token) {
  return Isolate.run(() {
    token.cancelled();
    token.complete(
      ResourceResponse(
        status: ResourceResponseStatus.ok,
        bytes: Uint8List.fromList(_emptyStyleJson.codeUnits),
      ),
    );
    try {
      token.complete(
        ResourceResponse(status: ResourceResponseStatus.noContent),
      );
      return false;
    } on InvalidArgumentException {
      return true;
    }
  });
}

/// Whether this alias was the one that completed the request.
///
/// The loser sees invalid-state when the request is still live and already
/// completed, and invalid-argument once the winner's release has retired the
/// id; both mean it lost.
bool _completeTokenAlias(ResourceRequestHandle token) {
  try {
    token.complete(
      ResourceResponse(
        status: ResourceResponseStatus.ok,
        bytes: Uint8List.fromList(_emptyStyleJson.codeUnits),
      ),
    );
    return true;
  } on InvalidStateException {
    return false;
  } on InvalidArgumentException {
    return false;
  }
}

void _clearLogCallback() {
  Maplibre.clearLogCallback();
}

Future<void> _waitUntilCondition(
  RuntimeHandle runtime,
  bool Function() condition, {
  Duration timeout = const Duration(seconds: 5),
}) async {
  await _waitUntil(() {
    runtime.drainEvents();
    return condition();
  }, timeout: timeout);
}

Future<CommandCompletion> _waitForCommandCompletion(
  RuntimeHandle runtime,
  Future<CommandCompletion> completion,
) => completion;

Future<CommandDisposition> _waitForCommandDisposition(
  RuntimeHandle runtime,
  Future<CommandCompletion> completion,
) async => (await completion).disposition;

Future<void> _expectCommandCommitted(
  RuntimeHandle runtime,
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
    for (final event in runtime.drainEvents().events) {
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
