// One representative per generated value shape, against the real library.
import 'dart:typed_data';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:maplibre_native_ffi/src/internal/value/uint64.dart';
import 'package:test/test.dart';

import 'support/fixture.dart';

const _featureCollection = '{"type":"FeatureCollection","features":[]}';

void main() {
  test('strings cross as NUL-terminated and explicit-length views', () async {
    // The C API reads a path to its NUL, so a path with an embedded NUL is
    // rejected before it crosses.
    expect(
      () => runtimeCreate(const RuntimeOptions(cachePath: 'cache\u0000tail')),
      throwsA(isA<InvalidArgumentException>()),
    );

    // A layer ID crosses as a view with an explicit length, so an ID with an
    // embedded NUL names that layer and not the prefix before the NUL.
    final fixture = await openRuntime();
    final map = await fixture.openStyledMap(
      '{"version":8,"sources":{},"layers":['
      '{"id":"a\\u0000b","type":"background"}]}',
    );
    expect((await map.getStyleLayerInfo('a\u0000b'))?.info.type, 'background');
    expect(await map.getStyleLayerInfo('a'), isNull);
    expect(await map.listStyleLayerIds(), ['a\u0000b']);
  });

  test(
    'presence fields keep a present zero apart from an absent one',
    () async {
      final fixture = await openRuntime();
      final map = await fixture.openStyledMap();

      const options = StyleTransitionOptions(
        durationMs: 0,
        enablePlacementTransitions: false,
      );
      await expectCommitted(map.setStyleTransitionOptions(options));
      final read = await map.getStyleTransitionOptions();
      expect(read.durationMs, 0);
      expect(read.delayMs, isNull);
      expect(read.enablePlacementTransitions, isFalse);
      expect(read, options);
    },
  );

  test('integer narrowing is checked and 64-bit carriers round-trip', () async {
    final maximum = (BigInt.one << 64) - BigInt.one;
    expect(uint64ToNative(maximum, 'value'), -1);
    expect(uint64FromNative(-1), maximum);
    expect(
      () => uint64ToNative(maximum + BigInt.one, 'value'),
      throwsA(isA<InvalidArgumentException>()),
    );

    // A value outside a 32-bit field's range is rejected before it crosses,
    // rather than truncated into a valid one.
    expect(
      () => geojsonSourceDataCreate(
        jsonBytes(_featureCollection),
        options: GeojsonSourceOptions(tileSize: 4294967296),
      ),
      throwsA(isA<InvalidArgumentException>()),
    );
    final fixture = await openRuntime();
    expect(
      () => fixture.runtime.setMaximumAmbientCacheSize(BigInt.from(-1)),
      throwsA(isA<InvalidArgumentException>()),
    );

    // An identity with the high bit set comes back from native unchanged.
    final map = await fixture.openStyledMap();
    await expectCommitted(
      map.updateCamera(
        CameraUpdate(
          camera: const CameraOptions(zoom: 2),
          mode: CameraUpdateMode.ease,
          animation: AnimationOptions(durationMs: 0, transitionId: maximum),
        ),
      ),
    );
    final finished = await fixture.awaitEventType(
      RuntimeEventType.mapCameraTransitionFinished,
    );
    expect(
      (finished.payload as RuntimeEventPayloadCameraTransitionFinished)
          .value
          .transitionId,
      maximum,
    );
  });

  test('an array input is copied when the call submits it', () async {
    final fixture = await openRuntime();
    final map = await fixture.openStyledMap();
    final points = [const ScreenPoint(0, 0), const ScreenPoint(10, 10)];
    final expected = [
      for (final point in points) await map.latLngForPixel(point),
    ];

    final converted = map.latLngsForPixels(points);
    points
      ..clear()
      ..add(const ScreenPoint(5, 5));
    expect(await converted, expected);
  });

  test('byte-backed values own their storage and compare by content', () {
    final clusterProperties = Uint8List.fromList([1, 2, 3]);
    final geometry = Uint8List.fromList([4, 5, 6]);
    final options = GeojsonSourceOptions(clusterProperties: clusterProperties);
    final definition = OfflineGeometryRegionDefinition(
      styleUrl: 'custom://style.json',
      geometry: geometry,
      minZoom: 0,
      maxZoom: 10,
      pixelRatio: 1,
    );

    clusterProperties[0] = 9;
    geometry[0] = 9;

    expect(
      options,
      GeojsonSourceOptions(clusterProperties: Uint8List.fromList([1, 2, 3])),
    );
    expect(
      definition,
      OfflineGeometryRegionDefinition(
        styleUrl: 'custom://style.json',
        geometry: Uint8List.fromList([4, 5, 6]),
        minZoom: 0,
        maxZoom: 10,
        pixelRatio: 1,
      ),
    );
    expect(
      definition.hashCode,
      OfflineGeometryRegionDefinition(
        styleUrl: 'custom://style.json',
        geometry: Uint8List.fromList([4, 5, 6]),
        minZoom: 0,
        maxZoom: 10,
        pixelRatio: 1,
      ).hashCode,
    );
    expect(() => options.clusterProperties![0] = 9, throwsUnsupportedError);
    expect(() => definition.geometry[0] = 9, throwsUnsupportedError);
  });
}
