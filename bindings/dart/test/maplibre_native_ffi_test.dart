import 'dart:ffi';
import 'dart:isolate';
import 'dart:typed_data';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.dart';
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.g.dart'
    as raw;
import 'package:test/test.dart';

const _emptyStyleJson = '{"version":8,"sources":{},"layers":[]}';

void main() {
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

  test('process-global log callbacks retire across isolates', () async {
    Maplibre.setLogCallback((_) {});
    await Isolate.run(_clearLogCallback);
    await Future<void>.delayed(Duration.zero);

    Maplibre.setLogCallback((_) {});
    Maplibre.clearLogCallback();
  });

  test('queued resource provider callbacks cross the native C ABI', () async {
    const styleUrl = 'custom://dart-provider-style.json';
    final runtime = RuntimeHandle.create();
    final requests = <ResourceRequest>[];
    late Future<bool> completion;
    late ResourceRequestToken ownerToken;

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
          expect(handle.isReleased, isFalse);
          final token = handle.transfer();
          expect(handle.isReleased, isTrue);
          ownerToken = token;
          completion = _completeTransferredRequest(token);
        },
      ),
    );

    final map = runtime.createMap();
    map.setStyleUrl(styleUrl);
    await _pumpUntil(runtime, () => requests.isNotEmpty);

    ownerToken.waitUntilReleased();
    map.close();
    runtime.close();
    expect(await completion, isTrue);
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

  test('custom geometry tile callbacks cross the native shim', () async {
    final c = MaplibreNativeCApi.open();
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

    c.dartTestInvokeCustomGeometryTileCallback(
      callback.nativeFunction,
      nullptr,
      tileId,
    );
    await _waitUntil(() => deliveredTiles.isNotEmpty);

    expect(deliveredTiles.single.z, 3);
    expect(deliveredTiles.single.x, 4);
    expect(deliveredTiles.single.y, 5);
    callback.close();
  });

  test('runtime and map handles use the native C ABI', () async {
    expect(
      () => RuntimeHandle.create(
        options: RuntimeOptions(maximumCacheSize: BigInt.from(-1)),
      ),
      throwsA(isA<InvalidArgumentException>()),
    );
    final defaultedRuntime = RuntimeHandle.create(
      options: RuntimeOptions(maximumCacheSize: BigInt.zero),
    );
    defaultedRuntime.close();

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
      runtime.runOnce();
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
        bounds: LatLngBounds(LatLng(-1, -1), LatLng(1, 1)),
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
      options: const StyleImageOptions(pixelRatio: 2, sdf: true),
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
    runtime.runOnce();
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
    map.setBounds(const BoundOptions(minZoom: 0, maxZoom: 24));
    expect(map.bounds().minZoom, isNotNull);
    final projectionMode = map.projectionMode();
    expect(projectionMode.axonometric, isNotNull);
    map.setProjectionMode(const ProjectionModeOptions(axonometric: false));
    expect(map.freeCameraOptions(), isA<FreeCameraOptions>());
    expect(
      map
          .cameraForLatLngBounds(
            const LatLngBounds(LatLng(-1, -1), LatLng(1, 1)),
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
    map.setLocationIndicatorLocation('dart-location-layer', const LatLng(0, 0));
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
      const LatLngBounds(LatLng(-1, -1), LatLng(1, 1)),
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
}

Future<bool> _completeTransferredRequest(ResourceRequestToken token) {
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

void _clearLogCallback() {
  Maplibre.clearLogCallback();
}

Future<void> _pumpUntil(
  RuntimeHandle runtime,
  bool Function() condition, {
  Duration timeout = const Duration(seconds: 5),
}) async {
  await _waitUntil(() {
    runtime.runOnce();
    runtime.drainEvents();
    return condition();
  }, timeout: timeout);
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
