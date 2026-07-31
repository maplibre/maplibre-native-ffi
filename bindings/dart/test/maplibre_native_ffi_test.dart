import 'dart:ffi';
import 'dart:io';
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
    show customGeometryCallbackProbeForTesting;
import 'package:test/test.dart';

const _emptyStyleJson = '{"version":8,"sources":{},"layers":[]}';

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
    return c.raw.mln_adapter_log_callback(
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
  test('map options carry FastPFOR decoding to native', () {
    expect(const MapOptions().fastPforEnabled, isFalse);
    expect(const MapOptions(fastPforEnabled: true), isNot(const MapOptions()));

    final runtime = RuntimeHandle.create();
    final map = runtime.createMap(
      options: const MapOptions(width: 64, height: 64, fastPforEnabled: true),
    );
    expect(map.size(), const MapSize(width: 64, height: 64, scaleFactor: 1));
    map.close();
    runtime.close();
  });
  test('process-global APIs cross the native C ABI', () {
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

  test('render target extents report their physical size through native', () {
    final size = const RenderTargetExtent(
      width: 65,
      height: 33,
      scaleFactor: 1.5,
    ).physicalSize();
    expect(size.width, 98);
    expect(size.height, 50);
  });

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
          url: styleUrl,
          response: ResourceResponse(
            status: ResourceResponseStatus.ok,
            bytes: Uint8List.fromList(_emptyStyleJson.codeUnits),
          ),
        ),
      ]);
      final map = runtime.createMap();

      map.setStyleUrl(styleUrl);
      final event = await _pumpUntilEvent(
        runtime,
        (candidate) =>
            candidate.eventType == RuntimeEventType.mapStyleLoaded &&
            candidate.source is MapRuntimeEventSource,
      );
      expect((event.source as MapRuntimeEventSource).map, same(map));

      map.close();
      runtime.close();
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
    final map = runtime.createMap();

    map.setStyleUrl(unmatchedUrl);
    final event = await _pumpUntilEvent(
      runtime,
      (candidate) => candidate.eventType == RuntimeEventType.mapLoadingFailed,
    );
    expect(event.source, isA<MapRuntimeEventSource>());
    expect(providerCalls, 0);

    map.close();
    runtime.close();
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
          expect(request.url, styleUrl);
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

    final map = runtime.createMap();
    map.setStyleUrl(styleUrl);
    await _pumpUntil(runtime, () => requests.isNotEmpty);

    ownerToken.waitUntilRetired();
    expect(
      () => ownerToken.cancelled(),
      throwsA(isA<InvalidArgumentException>()),
    );
    ownerToken.close();
    map.close();
    runtime.close();
    expect(await completion, isTrue);
  });

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

    final map = runtime.createMap();
    map.setStyleUrl(styleUrl);
    await _pumpUntil(runtime, () => token != null);
    final liveToken = token!;
    final waiter = Isolate.run(() {
      liveToken.waitUntilRetired();
      return true;
    });

    map.close();
    runtime.close();
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
    final map = runtime.createMap();
    map.setStyleUrl(styleUrl);
    await _pumpUntil(runtime, () => token != null);

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

    map.close();
    runtime.close();
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
    final map = runtime.createMap();
    map.setStyleUrl(styleUrl);
    await _pumpUntil(runtime, () => token != null);

    final liveToken = token!;
    final waiter = Isolate.run(() {
      liveToken.waitUntilRetired();
      return true;
    });
    // Completion is the one-shot operation; release is idempotent by design,
    // so racing two completions is what can have a single winner. Teardown
    // starts first because closing a map or runtime must not follow an await
    // on this isolate: the Dart VM may resume it on another native thread and
    // the owner-thread check would reject the close.
    final race = Future.wait([
      Isolate.run(() => _completeTokenAlias(liveToken)),
      Isolate.run(() => _completeTokenAlias(liveToken)),
    ]);
    map.close();
    runtime.close();
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

    final map = runtime.createMap();
    map.setStyleUrl(styleUrl);
    await _pumpUntil(runtime, () => calls > 0);

    map.close();
    runtime.close();
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
    final map = runtime.createMap();
    map.setStyleUrl(styleUrl);
    await _pumpUntil(runtime, () => callbackFinished);

    expect(cancelledRejected, isTrue);
    expect(completionRejected, isTrue);
    expect(repeatedCloseSucceeded, isTrue);

    map.close();
    runtime.close();
  });

  test('projection remains usable after its source map closes', () {
    final runtime = RuntimeHandle.create();
    final map = runtime.createMap();
    final projection = map.createProjection();

    map.close();
    expect(projection.pixelForLatLng(const LatLng(0, 0)).x.isFinite, isTrue);
    projection.setCamera(const CameraOptions(center: LatLng(1, 1), zoom: 2));
    expect(projection.camera().zoom, closeTo(2, 0.0001));

    projection.close();
    runtime.close();
  });

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

  test(
    'custom geometry callback roots retire at lifecycle boundaries',
    () async {
      final runtime = RuntimeHandle.create();
      final map = runtime.createMap();
      map.setStyleJson(_emptyStyleJson);

      map.addCustomGeometrySource(
        'dart-lifecycle-source',
        CustomGeometrySourceOptions(fetchTile: (_) {}),
      );
      final removedProbe = customGeometryCallbackProbeForTesting(
        map,
        'dart-lifecycle-source',
      )!;
      expect(map.removeStyleSource('dart-lifecycle-source'), isTrue);
      expect(removedProbe.retirementQueued, isTrue);

      map.addCustomGeometrySource(
        'dart-lifecycle-source',
        CustomGeometrySourceOptions(fetchTile: (_) {}),
      );
      final reloadProbe = customGeometryCallbackProbeForTesting(
        map,
        'dart-lifecycle-source',
      )!;
      map.setStyleJson(_emptyStyleJson);
      expect(reloadProbe.retirementQueued, isTrue);

      map.addCustomGeometrySource(
        'dart-lifecycle-source',
        CustomGeometrySourceOptions(fetchTile: (_) {}),
      );
      final closeProbe = customGeometryCallbackProbeForTesting(
        map,
        'dart-lifecycle-source',
      )!;
      map.close();
      runtime.close();
      expect(closeProbe.retirementQueued, isTrue);
      await _waitUntil(
        () => removedProbe.closed && reloadProbe.closed && closeProbe.closed,
      );
    },
  );

  test('nine-patch style images round-trip through the native C ABI', () {
    final runtime = RuntimeHandle.create();
    final map = runtime.createMap();
    try {
      map.setStyleJson('{"version":8,"sources":{},"layers":[]}');
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

      final info = map.getStyleImageInfo('patch');
      expect(info, isNotNull);
      expect(info!.stretchXCount, 1);
      expect(info.stretchYCount, 2);
      expect(info.content?.right, 1.5);
      // An absent text fit stays distinguishable from a present default.
      expect(info.textFitWidth, isNull);
      expect(info.textFitHeight, StyleImageTextFit.proportional);

      final stretches = map.getStyleImageStretches('patch');
      expect(stretches, isNotNull);
      expect(stretches!.stretchX, [const ImageStretch(0, 1)]);
      expect(stretches.stretchY, [
        const ImageStretch(0, 1),
        const ImageStretch(1, 2),
      ]);
      expect(map.getStyleImageStretches('missing'), isNull);

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
      map.close();
      runtime.close();
    }
  });

  test('layer base accessors round-trip through the native C ABI', () {
    final runtime = RuntimeHandle.create();
    final map = runtime.createMap();
    try {
      map.setStyleJson(
        '{"version":8,"sources":{"geo":{"type":"geojson","data":'
        '{"type":"FeatureCollection","features":[]}}},"layers":['
        '{"id":"bg","type":"background"},'
        '{"id":"fill","type":"fill","source":"geo"}]}',
      );

      expect(map.getLayerSourceLayer('fill'), '');
      map.setLayerSourceLayer('fill', 'roads');
      expect(map.getLayerSourceLayer('fill'), 'roads');
      expect(map.getLayerSourceId('fill'), 'geo');

      // A layer type that takes no source is rejected, not silently ignored.
      expect(
        () => map.setLayerSourceLayer('bg', 'roads'),
        throwsA(isA<InvalidArgumentException>()),
      );
      expect(map.getLayerSourceId('bg'), '');

      // An unset zoom range crosses the boundary as infinities.
      expect(map.getLayerMinZoom('fill'), double.negativeInfinity);
      expect(map.getLayerMaxZoom('fill'), double.infinity);
      map.setLayerMinZoom('fill', 4);
      map.setLayerMaxZoom('fill', 12.5);
      expect(map.getLayerMinZoom('fill'), 4);
      expect(map.getLayerMaxZoom('fill'), 12.5);

      expect(map.getLayerVisibility('fill'), StyleLayerVisibility.visible);
      map.setLayerVisibility('fill', StyleLayerVisibility.none);
      expect(map.getLayerVisibility('fill'), StyleLayerVisibility.none);

      expect(
        () => map.getLayerMinZoom('missing'),
        throwsA(isA<InvalidArgumentException>()),
      );
    } finally {
      map.close();
      runtime.close();
    }
  });

  test('style transition options round-trip through the native C ABI', () {
    const transitionStyleJson =
        '{"version":8,"transition":{"duration":750,"delay":100},'
        '"sources":{},"layers":[]}';
    final runtime = RuntimeHandle.create();
    final map = runtime.createMap();
    try {
      // A map with no style yet reports no duration or delay. The placement
      // flag always reports, because native always holds a value for it.
      final empty = map.getStyleTransitionOptions();
      expect(empty.durationMs, isNull);
      expect(empty.delayMs, isNull);
      expect(empty.enablePlacementTransitions, isTrue);

      // The style parser fills in its own 300ms duration for a style that
      // declares no transition.
      map.setStyleJson(_emptyStyleJson);
      final parsed = map.getStyleTransitionOptions();
      expect(parsed.durationMs, 300);
      expect(parsed.delayMs, isNull);

      map.setStyleJson(transitionStyleJson);
      final declared = map.getStyleTransitionOptions();
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
      expect(map.getStyleTransitionOptions(), options);
      expect(map.getStyleTransitionOptions().hashCode, options.hashCode);

      // Omitting the flag leaves the cross-fade on rather than clearing it.
      map.setStyleTransitionOptions(
        const StyleTransitionOptions(durationMs: 250),
      );
      expect(
        map.getStyleTransitionOptions().enablePlacementTransitions,
        isTrue,
      );

      // Loading a style replaces the override with what that style declares.
      map.setStyleJson(transitionStyleJson);
      expect(map.getStyleTransitionOptions(), declared);

      expect(
        () => map.setStyleTransitionOptions(
          const StyleTransitionOptions(delayMs: -1),
        ),
        throwsA(isA<InvalidArgumentException>()),
      );
    } finally {
      map.close();
      runtime.close();
    }
  });

  test('runtime and map handles use the native C ABI', () async {
    final cacheSizeRuntime = RuntimeHandle.create(
      options: const RuntimeOptions(cachePath: ':memory:'),
    );
    // An out-of-domain unsigned value is rejected before crossing into C.
    expect(
      () => cacheSizeRuntime.setMaximumAmbientCacheSize(BigInt.from(-1)),
      throwsA(isA<InvalidArgumentException>()),
    );
    final cacheSizeOperation = cacheSizeRuntime.setMaximumAmbientCacheSize(
      BigInt.zero,
    );
    expect(cacheSizeOperation.isDiscarded, isFalse);
    cacheSizeOperation.discard();
    expect(cacheSizeOperation.isDiscarded, isTrue);
    cacheSizeRuntime.close();

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
      () => runtime.setResourceProviderRules([
        ResourceProviderRule(
          url: 'https://example.com/provider\u0000truncated',
          response: ResourceResponse(status: ResourceResponseStatus.ok),
        ),
      ]),
      throwsA(isA<InvalidArgumentException>()),
    );
    expect(
      () => runtime.setResourceProviderRules([
        ResourceProviderRule(
          url: 'https://example.com/provider-error-message',
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
          url: 'https://example.com/provider-etag',
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
    expect(
      () => runtime.createMap(options: const MapOptions(width: -1)),
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
        url: 'https://example.com/provider-style.json',
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
    final offlineOperation = runtime.runAmbientCacheOperation(
      AmbientCacheOperation.clear,
    );
    expect(offlineOperation.isDiscarded, isFalse);
    expect(() => runtime.close(), throwsA(isA<InvalidStateException>()));
    RuntimeEventOfflineOperationCompleted? offlineCompletion;
    await _waitUntil(() {
      runtime.pump();
      RuntimeEvent? event;
      while ((event = runtime.pollEvent()) != null) {
        final payload = event!.payload;
        if (payload is RuntimeEventOfflineOperationCompleted &&
            identical(payload.operation, offlineOperation)) {
          offlineCompletion = payload;
        }
      }
      return offlineCompletion != null;
    });
    expect(offlineCompletion!.operation, same(offlineOperation));
    expect(offlineCompletion!.resultStatus, MaplibreStatus.ok);
    expect(runtime.pollEvent(), isNull);
    offlineOperation.discard();
    expect(offlineOperation.isDiscarded, isTrue);
    final offlineListOperation = runtime.listOfflineRegions();
    expect(offlineListOperation.isDiscarded, isFalse);
    expect(
      () => offlineListOperation.takeRegionStatus(),
      throwsA(isA<InvalidStateException>()),
    );
    offlineListOperation.discard();
    expect(
      () => offlineListOperation.takeRegionList(),
      throwsA(isA<InvalidStateException>()),
    );
    final offlineCreateOperation = runtime.createOfflineRegion(
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
    expect(offlineCreateOperation.isDiscarded, isFalse);
    offlineCreateOperation.discard();

    final map = runtime.createMap();
    expect(map.isClosed, isFalse);
    expect(map.size(), const MapSize(width: 256, height: 256, scaleFactor: 1));
    runtime.setResourceProviderRules(const []);
    runtime.setResourceProvider(
      ResourceProvider(
        routes: const [],
        callback: (_, handle) => handle.close(),
      ),
    );
    runtime.clearResourceProvider();
    map.setStyleJson(_emptyStyleJson);
    map.requestRepaint();
    expect(() => map.requestStillImage(), throwsA(isA<MaplibreException>()));
    map.setDebugOptions(MapDebugOptions.tileBorders);
    expect(map.debugOptions().contains(MapDebugOptions.tileBorders), isTrue);
    map.setDebugOptions(MapDebugOptions.none);
    var throwingLogCalls = 0;
    Maplibre.setLogCallback((_) {
      throwingLogCalls += 1;
      throw StateError('log callback failure');
    });
    map.dumpDebugLogs();
    await _waitUntil(() => throwingLogCalls > 0);
    Maplibre.clearLogCallback();
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
    expect(map.styleImageExists('dart-image'), isTrue);
    final styleImageInfo = map.getStyleImageInfo('dart-image');
    expect(styleImageInfo, isNotNull);
    expect(styleImageInfo!.width, 1);
    expect(styleImageInfo.height, 1);
    expect(styleImageInfo.pixelRatio, closeTo(2, 0.0001));
    expect(styleImageInfo.sdf, isTrue);
    final styleImage = map.copyStyleImagePremultipliedRgba8('dart-image');
    expect(styleImage, isNotNull);
    expect(styleImage!.bytes, [255, 0, 0, 255]);
    expect(map.removeStyleImage('dart-image'), isTrue);
    expect(map.styleImageExists('dart-image'), isFalse);
    runtime.pump();
    final copiedEvents = <RuntimeEvent>[];
    RuntimeEvent? copiedEvent;
    while ((copiedEvent = runtime.pollEvent()) != null) {
      copiedEvents.add(copiedEvent!);
    }
    final styleLoadedEvent = copiedEvents.firstWhere(
      (event) => event.eventType == RuntimeEventType.mapStyleLoaded,
    );
    expect(styleLoadedEvent.source, isA<MapRuntimeEventSource>());
    expect((styleLoadedEvent.source as MapRuntimeEventSource).map, same(map));
    expect(runtime.pollEvent(), isNull);
    map.jumpTo(const CameraOptions(center: LatLng(0, 0), zoom: 1));
    final camera = map.camera();
    expect(camera.center, const LatLng(0, 0));
    expect(camera.zoom, closeTo(1, 0.0001));
    runtime.drainEvents();
    final transitionId = (BigInt.one << 64) - BigInt.one;
    map.easeTo(
      const CameraOptions(zoom: 2),
      animation: AnimationOptions(durationMs: 0, transitionId: transitionId),
    );
    final cameraEvents = <RuntimeEvent>[];
    RuntimeEvent? cameraEvent;
    while ((cameraEvent = runtime.pollEvent()) != null) {
      cameraEvents.add(cameraEvent!);
    }
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
    map.setRenderingStatsViewEnabled(true);
    expect(map.renderingStatsViewEnabled(), isTrue);
    map.setRenderingStatsViewEnabled(false);
    expect(map.isFullyLoaded(), isA<bool>());
    map.setViewportOptions(
      const MapViewportOptions(viewportMode: ViewportMode.defaultMode),
    );
    expect(map.viewportOptions().viewportMode, isNotNull);
    map.setTileOptions(const MapTileOptions(prefetchZoomDelta: 0));
    expect(map.tileOptions().prefetchZoomDelta, isNotNull);
    const cameraBounds = LatLngBounds(
      southwest: LatLng(-10, -20),
      northeast: LatLng(10, 20),
    );
    map.setBounds(
      const BoundOptions(
        bounds: BoundsConstraint.bounded(cameraBounds),
        minZoom: 0,
        maxZoom: 24,
      ),
    );
    expect(map.bounds().bounds, const BoundsConstraint.bounded(cameraBounds));
    map.setBounds(const BoundOptions(bounds: BoundsConstraint.unbounded()));
    expect(map.bounds().bounds, const BoundsConstraint.unbounded());
    final projectionMode = map.projectionMode();
    expect(projectionMode.axonometric, isNotNull);
    map.setProjectionMode(const ProjectionModeOptions(axonometric: false));
    expect(map.freeCameraOptions(), isA<FreeCameraOptions>());
    expect(
      map
          .cameraForLatLngBounds(
            const LatLngBounds(
              southwest: LatLng(-1, -1),
              northeast: LatLng(1, 1),
            ),
          )
          .zoom,
      isNotNull,
    );
    expect(
      map.cameraForLatLngs(const [LatLng(-1, -1), LatLng(1, 1)]).zoom,
      isNotNull,
    );
    expect(
      map
          .latLngBoundsForCamera(const CameraOptions(center: LatLng(0, 0)))
          .southwest
          .latitude
          .isFinite,
      isTrue,
    );
    map.moveBy(1, 1);
    map.scaleBy(1.01, anchor: const ScreenPoint(128, 128));
    map.rotateBy(const ScreenPoint(0, 0), const ScreenPoint(1, 1));
    map.pitchBy(0);
    map.cancelTransitions();
    expect(map.isGestureInProgress(), isFalse);
    map.setGestureInProgress(true);
    map.moveBy(8, -4);
    expect(map.isGestureInProgress(), isTrue);
    map.setGestureInProgress(false);
    expect(map.isGestureInProgress(), isFalse);
    expect(() => map.scaleBy(-1), throwsA(isA<InvalidArgumentException>()));
    final centerPixel = map.pixelForLatLng(const LatLng(0, 0));
    expect(centerPixel.x.isFinite, isTrue);
    expect(map.latLngForPixel(centerPixel).latitude.isFinite, isTrue);
    expect(map.pixelsForLatLngs(const [LatLng(0, 0)]), hasLength(1));
    expect(map.latLngsForPixels([centerPixel]), hasLength(1));
    final projection = map.createProjection();
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
      () => map.attachRef().attachMetalSurface(
        const MetalSurfaceDescriptor(
          extent: RenderTargetExtent(width: -1, height: 16),
          context: MetalContextDescriptor(device: NativePointer.nullPointer),
          layer: NativePointer.nullPointer,
        ),
      ),
      throwsA(isA<InvalidArgumentException>()),
    );
    expect(
      () => map.attachRef().attachMetalSurface(
        const MetalSurfaceDescriptor(
          extent: RenderTargetExtent(width: 16, height: 16),
          context: MetalContextDescriptor(device: NativePointer.nullPointer),
          layer: NativePointer.nullPointer,
        ),
      ),
      throwsA(isA<MaplibreException>()),
    );
    expect(
      () => map.attachRef().attachMetalOwnedTexture(
        const MetalOwnedTextureDescriptor(
          extent: RenderTargetExtent(width: 16, height: 16),
          context: MetalContextDescriptor(device: NativePointer.nullPointer),
        ),
      ),
      throwsA(isA<MaplibreException>()),
    );
    expect(
      () => map.attachRef().attachOpenGLOwnedTexture(
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
      () => map.attachRef().attachOpenGLOwnedTexture(
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
      () => map.attachRef().attachOpenGLBorrowedTexture(
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
      () => map.attachRef().attachOpenGLSurface(
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

    final sourceIds = map.listStyleSourceIds();
    expect(sourceIds, contains('org.maplibre.annotations'));
    expect(
      map.listStyleLayerIds(),
      contains('org.maplibre.annotations.points'),
    );
    expect(map.styleSourceExists('missing-source'), isFalse);
    expect(map.styleLayerExists('missing-layer'), isFalse);
    expect(map.removeStyleSource('missing-source'), isFalse);
    expect(map.removeStyleLayer('missing-layer'), isFalse);

    map.addGeoJsonSourceUrl(
      'dart-geojson-url-source',
      'https://example.com/a.geojson',
    );
    expect(
      map.getStyleSourceInfo('dart-geojson-url-source')!.type,
      SourceType.geoJson,
    );
    map.setGeoJsonSourceUrl(
      'dart-geojson-url-source',
      'https://example.com/b.geojson',
    );
    expect(map.removeStyleSource('dart-geojson-url-source'), isTrue);
    expect(
      () => map.addGeoJsonSourceData(
        'dart-invalid-geojson-options',
        FeatureCollectionGeoJson([]),
        options: const GeoJsonSourceOptions(tileSize: 4294967296),
      ),
      throwsA(isA<InvalidArgumentException>()),
    );
    map.addGeoJsonSourceData(
      'dart-clustered-geojson-source',
      FeatureCollectionGeoJson([
        FeatureGeoJson(geometry: const PointGeometry(LatLng(0, 0))),
      ]),
      options: const GeoJsonSourceOptions(cluster: true, clusterRadius: 60),
    );
    expect(
      () => map.setGeoJsonSourceData(
        'dart-clustered-geojson-source',
        const GeometryGeoJson(PointGeometry(LatLng(0, 0))),
      ),
      throwsA(isA<InvalidArgumentException>()),
    );
    expect(map.removeStyleSource('dart-clustered-geojson-source'), isTrue);
    map.addVectorSourceUrl(
      'dart-vector-source',
      'https://example.com/vector.json',
    );
    expect(
      map.getStyleSourceInfo('dart-vector-source')!.type,
      SourceType.vector,
    );
    expect(map.removeStyleSource('dart-vector-source'), isTrue);
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
      map.getStyleSourceInfo('dart-vector-tiles-source')!.type,
      SourceType.vector,
    );
    expect(map.removeStyleSource('dart-vector-tiles-source'), isTrue);
    map.addRasterSourceTiles('dart-raster-tiles-source', const [
      'https://example.com/{z}/{x}/{y}.png',
    ], options: const TileSourceOptions(tileSize: 256));
    expect(
      map.getStyleSourceInfo('dart-raster-tiles-source')!.type,
      SourceType.raster,
    );
    expect(map.removeStyleSource('dart-raster-tiles-source'), isTrue);
    map.addRasterDemSourceTiles(
      'dart-raster-dem-tiles-source',
      const ['https://example.com/{z}/{x}/{y}.png'],
      options: const TileSourceOptions(
        tileSize: 256,
        rasterDemEncoding: RasterDemEncoding.terrarium,
      ),
    );
    expect(
      map.getStyleSourceInfo('dart-raster-dem-tiles-source')!.type,
      SourceType.rasterDem,
    );
    map.addHillshadeLayer(
      'dart-hillshade-layer',
      'dart-raster-dem-tiles-source',
    );
    expect(map.getStyleLayerType('dart-hillshade-layer'), 'hillshade');
    map.addColorReliefLayer(
      'dart-color-relief-layer',
      'dart-raster-dem-tiles-source',
    );
    expect(map.getStyleLayerType('dart-color-relief-layer'), 'color-relief');
    map.moveStyleLayer(
      'dart-color-relief-layer',
      beforeLayerId: 'dart-hillshade-layer',
    );
    expect(map.removeStyleLayer('dart-color-relief-layer'), isTrue);
    expect(map.removeStyleLayer('dart-hillshade-layer'), isTrue);
    expect(map.removeStyleSource('dart-raster-dem-tiles-source'), isTrue);
    map.addLocationIndicatorLayer('dart-location-layer');
    expect(map.getStyleLayerType('dart-location-layer'), 'location-indicator');
    map.setLocationIndicatorLocation(
      'dart-location-layer',
      const LatLng(37.7749, -122.4194),
    );
    final location =
        map.getLayerProperty('dart-location-layer', 'location') as JsonArray;
    expect(location.values.map((value) => (value as JsonDouble).value), [
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
    expect(map.removeStyleLayer('dart-location-layer'), isTrue);
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
    expect(map.getStyleSourceInfo('dart-image-source')!.type, SourceType.image);
    expect(
      map.getImageSourceCoordinates('dart-image-source'),
      imageSourceCoordinates,
    );
    map.setImageSourceUrl('dart-image-source', 'https://example.com/image.png');
    map.setImageSourceCoordinates(
      'dart-image-source',
      imageSourceCoordinates.reversed.toList(),
    );
    expect(
      map.getImageSourceCoordinates('dart-image-source'),
      imageSourceCoordinates.reversed.toList(),
    );
    expect(map.removeStyleSource('dart-image-source'), isTrue);

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
      map.getStyleSourceInfo('dart-custom-source')!.type,
      SourceType.customVector,
    );
    map.setCustomGeometrySourceTileData(
      'dart-custom-source',
      const CanonicalTileId(z: 0, x: 0, y: 0),
      FeatureCollectionGeoJson([]),
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
    expect(map.removeStyleSource('dart-custom-source'), isTrue);
    map.addCustomGeometrySource(
      'dart-custom-source',
      CustomGeometrySourceOptions(fetchTile: fetchedTiles.add),
    );
    expect(map.removeStyleSource('dart-custom-source'), isTrue);

    map.addGeoJsonSourceData(
      'dart-geojson-source',
      FeatureGeoJson(
        geometry: PointGeometry(LatLng(0, 0)),
        properties: [JsonMember('kind', JsonString('dart'))],
      ),
    );
    expect(map.styleSourceExists('dart-geojson-source'), isTrue);
    final info = map.getStyleSourceInfo('dart-geojson-source');
    expect(info, isNotNull);
    expect(info!.type, SourceType.geoJson);
    expect(info.id, 'dart-geojson-source');
    expect(info.attribution, isNull);
    expect(map.listStyleSourceIds(), contains('dart-geojson-source'));

    map.setGeoJsonSourceData(
      'dart-geojson-source',
      const GeometryGeoJson(PointGeometry(LatLng(1, 2))),
    );
    map.addStyleLayerJson(
      JsonObject([
        JsonMember('id', JsonString('dart-circle-layer')),
        JsonMember('type', JsonString('circle')),
        JsonMember('source', JsonString('dart-geojson-source')),
      ]),
    );
    expect(map.styleLayerExists('dart-circle-layer'), isTrue);
    expect(map.getStyleLayerType('dart-circle-layer'), 'circle');
    expect(map.listStyleLayerIds(), contains('dart-circle-layer'));
    final layerJson = map.getStyleLayerJson('dart-circle-layer');
    expect(layerJson, isA<JsonObject>());
    expect(
      (layerJson! as JsonObject).members.map((member) => member.key),
      contains('id'),
    );

    map.setLayerProperty(
      'dart-circle-layer',
      'circle-radius',
      const JsonDouble(6.5),
    );
    expect(
      map.getLayerProperty('dart-circle-layer', 'circle-radius'),
      isA<JsonDouble>(),
    );
    map.setLayerFilter(
      'dart-circle-layer',
      JsonArray([
        JsonString('=='),
        JsonArray([const JsonString('get'), const JsonString('kind')]),
        JsonString('dart'),
      ]),
    );
    expect(map.getLayerFilter('dart-circle-layer'), isA<JsonArray>());
    map.setLayerFilter('dart-circle-layer', null);
    expect(
      map.getLayerFilter('dart-circle-layer'),
      anyOf(isNull, isA<JsonNull>()),
    );

    expect(map.removeStyleLayer('dart-circle-layer'), isTrue);
    expect(map.removeStyleSource('dart-geojson-source'), isTrue);

    map.close();
    expect(map.isClosed, isTrue);
    runtime.close();
    expect(runtime.isClosed, isTrue);
  });

  test('native pointer preserves address value semantics', () {
    const pointer = NativePointer(0x1234);

    expect(pointer.address, 0x1234);
    expect(pointer.isNull, isFalse);
    expect(pointer, equals(const NativePointer(0x1234)));
    expect(pointer.hashCode, equals(const NativePointer(0x1234).hashCode));
    expect({pointer}, contains(const NativePointer(0x1234)));
    expect(NativePointer.nullPointer.isNull, isTrue);
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

  test('native buffer owns reusable native byte storage', () {
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

  test('runtime value wrappers preserve unknown raw values', () {
    final eventType = RuntimeEventType.fromRawValue(0xfeed);
    final sourceType = RuntimeEventSourceType.fromRawValue(0xbeef);
    final renderMode = RenderMode.fromRawValue(42);
    final operationKind = OfflineOperationKind.fromRawValue(99);
    final resultKind = OfflineOperationResultKind.fromRawValue(100);
    const unknownDefinition = UnknownOfflineRegionDefinition(101);

    expect(eventType.rawValue, 0xfeed);
    expect(eventType, RuntimeEventType.fromRawValue(0xfeed));
    expect(sourceType.rawValue, 0xbeef);
    expect(renderMode.name, 'unknown(42)');
    expect(operationKind.rawValue, 99);
    expect(resultKind.rawValue, 100);
    expect(unknownDefinition.rawType, 101);
  });

  test(
    'a parked owner isolate wakes for native work and for a signal',
    () async {
      final ready = ReceivePort();
      final signalled = ReceivePort();
      await Isolate.spawn(_signalWakeSource, [
        ready.sendPort,
        signalled.sendPort,
      ]);
      final worker = await ready.first as SendPort;

      // Every await below this line would be a hazard: the VM may resume an
      // isolate on another OS thread after an asynchronous suspension, and
      // runtime calls are owner-thread affine. The worker handshake is finished
      // above so this isolate owns the runtime from creation through close.
      final runtime = RuntimeHandle.create(
        options: const RuntimeOptions(cachePath: ':memory:'),
      );
      final map = runtime.createMap();
      _quiesce(runtime);

      // The scheme is unsupported, so native reports the failure from its own
      // threads and that failure reaches the parked owner isolate.
      map.setStyleUrl('unsupported://style.json');
      var loadingFailed = false;
      final loadStarted = Stopwatch()..start();
      for (var attempt = 0; attempt < 20 && !loadingFailed; attempt += 1) {
        runtime.pump(timeout: _parkTimeout);
        expect(
          loadStarted.elapsed,
          lessThan(_promptReturn),
          reason: 'parks sat out their timeouts while loading was pending',
        );
        RuntimeEvent? event;
        while ((event = runtime.pollEvent()) != null) {
          if (event!.eventType == RuntimeEventType.mapLoadingFailed) {
            loadingFailed = true;
          }
        }
      }
      expect(loadingFailed, isTrue);

      // A source signalled from another isolate matches a host's submission
      // path, and the park it releases has no other work to end it.
      final source = runtime.acquireWakeSource();
      _quiesce(runtime);
      worker.send(source);

      final parkStarted = Stopwatch()..start();
      runtime.pump(timeout: _parkTimeout);
      expect(
        parkStarted.elapsed,
        lessThan(_promptReturn),
        reason:
            'the parked owner isolate timed out instead of taking the signal',
      );

      // A wake source stays usable after its runtime closes, so hosts tear the
      // two down in either order.
      map.close();
      runtime.close();
      source.signal();
      source.close();
      expect(source.isClosed, isTrue);
      expect(source.close, returnsNormally);
      expect(source.signal, throwsA(isA<InvalidArgumentException>()));

      await signalled.first;
      ready.close();
      signalled.close();
    },
  );

  test('a pump clears the wake flag it returns on', () {
    final runtime = RuntimeHandle.create(
      options: const RuntimeOptions(cachePath: ':memory:'),
    );
    final source = runtime.acquireWakeSource();
    _quiesce(runtime);

    source.signal();
    final signalledStarted = Stopwatch()..start();
    runtime.pump(timeout: _parkTimeout);
    expect(
      signalledStarted.elapsed,
      lessThan(_promptReturn),
      reason: 'a pump waited even though the wake flag was set',
    );

    // The pump above cleared the wake flag, so this one waits its full timeout.
    final idleStarted = Stopwatch()..start();
    runtime.pump(timeout: const Duration(milliseconds: 200));
    expect(
      idleStarted.elapsed,
      greaterThanOrEqualTo(const Duration(milliseconds: 100)),
      reason: 'the first pump left the wake flag set',
    );

    source.close();
    runtime.close();
  });
  test('an attach reference reaches native from another isolate', () async {
    final runtime = RuntimeHandle.create();
    final map = runtime.createMap(
      options: const MapOptions(width: 64, height: 64),
    );
    // A MapHandle cannot cross isolates, so the reference carries the address.
    final attachRef = map.attachRef();

    // Close before awaiting. The isolate may resume on a different OS thread
    // after an await, which would make these handles unusable; see the README's
    // draft deviation. The reference outliving them is the point here.
    map.close();
    runtime.close();

    // Attaching from another isolate must reach the C API. The extent is valid
    // so the binding's own checks pass and native answers: it validates the map
    // before the descriptor, and this one has been retired. Before the session
    // was decoupled from the map's isolate, this threw wrongThread from the
    // binding without ever calling native.
    final diagnostic = await Isolate.run(() {
      try {
        attachRef.attachMetalSurface(
          const MetalSurfaceDescriptor(
            extent: RenderTargetExtent(width: 16, height: 16),
            context: MetalContextDescriptor(device: NativePointer.nullPointer),
            layer: NativePointer.nullPointer,
          ),
        );
        return 'attached';
      } on MaplibreException catch (error) {
        return error.diagnostic;
      }
    });
    // BND-196: the C API rejects the released id as stale rather than binding
    // the session to whatever map is created next.
    expect(diagnostic, contains('stale'));
  });
}

/// Long enough that a park only ends early because something woke it.
const _parkTimeout = Duration(seconds: 10);

/// Well below [_parkTimeout], and far above the scheduling noise a loaded CI
/// machine adds to a wake.
const _promptReturn = Duration(seconds: 5);

/// Pumps until the runtime is idle, so a park that follows is released by the
/// signal the test raises rather than by leftover work.
void _quiesce(RuntimeHandle runtime) {
  for (var attempt = 0; attempt < 100; attempt += 1) {
    runtime.pump();
    var drained = false;
    while (runtime.pollEvent() != null) {
      drained = true;
    }
    if (!drained) {
      return;
    }
  }
  fail('the runtime kept producing events while idle');
}

/// Signals a wake source transferred from the owner isolate, once that isolate
/// has had time to enter its park.
Future<void> _signalWakeSource(List<SendPort> ports) async {
  final inbox = ReceivePort();
  ports[0].send(inbox.sendPort);
  final source = await inbox.first as WakeSource;
  sleep(const Duration(milliseconds: 20));
  source.signal();
  ports[1].send(null);
  inbox.close();
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

Future<void> _pumpUntil(
  RuntimeHandle runtime,
  bool Function() condition, {
  Duration timeout = const Duration(seconds: 5),
}) async {
  await _waitUntil(() {
    runtime.pump();
    runtime.drainEvents();
    return condition();
  }, timeout: timeout);
}

Future<RuntimeEvent> _pumpUntilEvent(
  RuntimeHandle runtime,
  bool Function(RuntimeEvent event) predicate,
) async {
  RuntimeEvent? matched;
  await _waitUntil(() {
    runtime.pump();
    RuntimeEvent? event;
    while ((event = runtime.pollEvent()) != null) {
      if (predicate(event!)) {
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
