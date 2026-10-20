// One representative per generated value shape, against the real library.
import 'dart:async';
import 'dart:typed_data';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:maplibre_native_ffi/src/internal/value/uint64.dart';
import 'package:test/test.dart';

import 'support/fixture.dart';

const _featureCollection = '{"type":"FeatureCollection","features":[]}';

void main() {
  test('host strings use views and native-lent strings decode', () async {
    // Native rejects a NUL in a path view before a C-string consumer reads it.
    expect(
      () => runtimeCreate(const RuntimeOptions(cachePath: 'cache\u0000tail')),
      throwsA(isA<InvalidArgumentException>()),
    );

    // A layer ID crosses as a view with an explicit length, so an ID with an
    // embedded NUL names that layer and not the prefix before the NUL.
    const url = 'custom://native-lent.json';
    final requested = Completer<String?>();
    final fixture = await openRuntime(
      provider: routedProvider([styleRoute(url)], (request, handle) {
        requested.complete(request.requestedUrl);
        handle.complete(
          ResourceResponse(
            status: ResourceResponseStatus.ok,
            bytes: jsonBytes(emptyStyleJson),
          ),
        );
        handle.close();
      }),
    );
    final map = await fixture.openStyledMap(
      '{"version":8,"sources":{},"layers":['
      '{"id":"a\\u0000b","type":"background"}]}',
    );
    expect((await map.getStyleLayer('a\u0000b'))?.type, 'background');
    expect(await map.getStyleLayer('a'), isNull);
    expect((await map.listStyleLayers()).map((layer) => layer.id), [
      'a\u0000b',
    ]);
    await expectCommitted(map.setStyleUrl(url));
    expect(await within(requested.future, 'the native-lent URL'), url);
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
        options: GeojsonSourceOptions(buffer: 4294967296),
      ),
      throwsA(isA<InvalidArgumentException>()),
    );
    final fixture = await openRuntime();

    // Cancelling the largest identity ends the ease that carries it, so the
    // identity crossed whole both times.
    final map = await fixture.openStyledMap();
    final ended = Completer<CameraTransitionEnd>();
    await expectCommitted(
      map.updateCamera(
        CameraUpdate(
          camera: const CameraOptions(zoom: 2),
          mode: CameraUpdateMode.ease,
          animation: AnimationOptions(
            durationMs: 60000,
            transitionId: maximum,
            endHandler: CameraTransitionHandler(callback: ended.complete),
          ),
        ),
      ),
    );
    await expectCommitted(map.cancelCameraTransition(maximum));
    final end = await within(ended.future, 'the end handler');
    expect(end.outcome, CameraTransitionOutcome.cancelled);
  });

  test('an array input is copied when the call submits it', () async {
    final fixture = await openRuntime();
    final map = await fixture.openStyledMap();
    final coordinates = [const LatLng(10, 20), const LatLng(12, 21)];
    final expected = await map.cameraForLatLngs(List.of(coordinates));
    final pending = map.cameraForLatLngs(coordinates);
    coordinates.clear();
    expect(await pending, expected);
  });

  // A constructor's defaults come from the header's field annotations, so
  // the record they build matches what the native default function returns.
  test(
    'a record built from its constructor defaults equals the native default',
    () {
      expect(const MapOptions(), mapOptionsDefault());
    },
  );

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
