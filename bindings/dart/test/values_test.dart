// One representative per generated value shape, against the real library.
import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

import 'support/fixture.dart';

const _featureCollection = {
  'type': 'FeatureCollection',
  'features': <Object?>[],
};

void main() {
  test('caller-sized text returns a result longer than its first storage', () {
    final zoom = styleExpressionCreate([
      'interpolate',
      ['linear'],
      ['zoom'],
      0,
      0,
      10,
      100,
    ]);
    addTearDown(zoom.close);
    expect(zoom.evaluate(inputs: StyleExpressionInputs(zoom: 5)), 50.0);

    // The result outgrows the runtime's first storage, so the call runs
    // again.
    final text = 'a' * 1000;
    final long = styleExpressionCreate(text);
    addTearDown(long.close);
    expect(long.evaluate(), text);
  });

  test('JSON crosses as jsonEncode and jsonDecode values', () async {
    final fixture = await openRuntime();
    final map = await fixture.openStyledMap({
      'version': 8,
      'sources': <String, Object?>{},
      'layers': [
        {'id': 'fill', 'type': 'background'},
      ],
    });

    // A value goes in as jsonEncode writes it and comes back as jsonDecode
    // reads it, so a style-spec string needs no inner quotes.
    await expectCommitted(map.setGlobalStateProperty('mode', 'dark'));
    expect(await map.getGlobalState(), {'mode': 'dark'});
    await expectCommitted(
      map.setStyleLayerProperty('fill', 'background-opacity', 0.5),
    );
    expect(await map.getStyleLayerProperty('fill', 'background-opacity'), 0.5);
    // A parameter that is null is JSON null, which restores the default, and
    // a property without a value comes back absent as null.
    await expectCommitted(
      map.setStyleLayerProperty('fill', 'background-opacity', null),
    );
    expect(
      await map.getStyleLayerProperty('fill', 'background-opacity'),
      isNull,
    );

    // A record member that is null is absent, an empty view.
    final expression = styleExpressionCreate(['get', 'name']);
    addTearDown(expression.close);
    final inputs = StyleExpressionInputs(
      feature: {
        'type': 'Feature',
        'properties': {'name': 'a'},
        'geometry': {
          'type': 'Point',
          'coordinates': [0, 0],
        },
      },
    );
    expect(inputs.featureState, isNull);
    expect(expression.evaluate(inputs: inputs), 'a');
    // A value with JSON members compares them by content.
    expect(
      inputs,
      StyleExpressionInputs(
        feature: {
          'type': 'Feature',
          'properties': {'name': 'a'},
          'geometry': {
            'type': 'Point',
            'coordinates': [0, 0],
          },
        },
      ),
    );

    // A value that jsonEncode rejects throws before native code runs.
    expect(
      () => styleExpressionCreate(Object()),
      throwsA(isA<JsonUnsupportedObjectError>()),
    );
  });

  test(
    'a synchronous call fills a caller-sized output array in input order',
    () async {
      final fixture = await openRuntime();
      final map = await fixture.openMap();
      final projection = mapProjectionCreate(map.getSnapshot().view);
      addTearDown(projection.close);
      const coordinates = [
        LatLng(),
        LatLng(latitude: 10, longitude: 20),
        LatLng(latitude: -12, longitude: -30),
      ];
      final points = projection.pixelsForLatLngs(coordinates);
      expect(points, [
        for (final coordinate in coordinates)
          projection.pixelForLatLng(coordinate),
      ]);
      expect(projection.latLngsForPixels(points), [
        for (final point in points) projection.latLngForPixel(point),
      ]);
      expect(projection.latLngsForPixelsUnwrapped(points), [
        for (final point in points) projection.latLngForPixelUnwrapped(point),
      ]);
      expect(projection.pixelsForLatLngs([]), isEmpty);
      expect(projection.latLngsForPixels([]), isEmpty);
      expect(projection.latLngsForPixelsUnwrapped([]), isEmpty);
    },
  );

  test('host strings use views and native-lent strings decode', () async {
    // Native rejects a NUL in a path view before a C-string consumer reads it.
    expect(
      () => runtimeCreate(RuntimeOptions(cachePath: 'cache\u0000tail')),
      throwsA(
        isA<MaplibreException>().having(
          (error) => error.status,
          'status',
          Status.invalidArgument,
        ),
      ),
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
            bytes: jsonBytes(emptyStyle),
          ),
        );
      }),
    );
    final map = await fixture.openStyledMap({
      'version': 8,
      'sources': <String, Object?>{},
      'layers': [
        {'id': 'a\u0000b', 'type': 'background'},
      ],
    });
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

      final options = StyleTransitionOptions(
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
    // A uint64_t crosses as its bit pattern, so -1 is the largest identity.
    const maximum = -1;

    // A value outside a 32-bit field's range is rejected before it crosses,
    // rather than truncated into a valid one.
    expect(
      () => geojsonSourceDataCreate(
        _featureCollection,
        options: GeojsonSourceOptions(buffer: 4294967296),
      ),
      throwsA(
        isA<MaplibreException>().having(
          (error) => error.status,
          'status',
          Status.invalidArgument,
        ),
      ),
    );
    final fixture = await openRuntime();

    // Cancelling the largest identity ends the ease that carries it, so the
    // identity crossed whole both times.
    final map = await fixture.openStyledMap();
    final ended = Completer<CameraTransitionEnd>();
    await expectCommitted(
      map.updateCamera(
        CameraUpdate(
          camera: CameraOptions(zoom: 2),
          mode: CameraUpdateMode.ease,
          animation: AnimationOptions(
            durationMs: 60000,
            transitionId: maximum,
            endHandler: ended.complete,
          ),
        ),
      ),
    );
    await expectCommitted(map.cancelCameraTransition(maximum));
    final end = await within(ended.future, 'the end handler');
    expect(end.outcome, CameraTransitionOutcome.cancelled);
  });

  // Fitting bounds fits their corners, so a fit of the two diagonal corners
  // matches only when each element reaches native in its own slot.
  test('an array input is copied when the call submits it', () async {
    final fixture = await openRuntime();
    final map = await fixture.openStyledMap();
    const southwest = LatLng(latitude: 10, longitude: 20);
    const northeast = LatLng(latitude: 12, longitude: 21);
    final coordinates = [southwest, northeast];
    final expected = await map.cameraForLatLngBounds(
      const LatLngBounds(southwest: southwest, northeast: northeast),
    );
    final pending = map.cameraForLatLngs(coordinates);
    coordinates.clear();
    expect(await pending, expected);
  });

  // Constructors copy the native record defaults.
  test(
    'a record built from its constructor defaults equals the native default',
    () {
      expect(MapOptions(), mapOptionsDefault());
    },
  );

  test('byte-backed values own their storage and compare by content', () {
    final pixels = Uint8List.fromList([1, 2, 3, 4]);
    final image = PremultipliedRgba8Image(
      width: 1,
      height: 1,
      stride: 4,
      pixels: pixels,
    );

    pixels[0] = 9;

    final same = PremultipliedRgba8Image(
      width: 1,
      height: 1,
      stride: 4,
      pixels: Uint8List.fromList([1, 2, 3, 4]),
    );
    expect(image, same);
    expect(image.hashCode, same.hashCode);
    expect(() => image.pixels[0] = 9, throwsUnsupportedError);
  });

  test('copyWith replaces only the members it is given', () {
    final camera = CameraOptions(zoom: 2, bearing: 30);
    expect(camera.copyWith(zoom: 4), CameraOptions(zoom: 4, bearing: 30));
    expect(camera.copyWith(), camera);

    // A JSON member's null is a value of its own: the member is absent.
    final filtered = RenderedFeatureQueryOptions(filter: ['has', 'name']);
    expect(filtered.copyWith().filter, ['has', 'name']);
    expect(filtered.copyWith(filter: null).filter, isNull);

    // So is an optional callback's: it disables the callback.
    void ended(CameraTransitionEnd _) {}
    final animated = AnimationOptions(endHandler: ended);
    expect(animated.copyWith().endHandler, ended);
    expect(animated.copyWith(endHandler: null).endHandler, isNull);
  });

  test('values and flag sets describe their members', () {
    expect(
      const LatLng(latitude: 1, longitude: 2).toString(),
      'LatLng(latitude=1.0, longitude=2.0)',
    );
    expect(
      GeojsonSourceOptions(clusterProperties: Uint8List(3)).toString(),
      contains('clusterProperties=Uint8List(3 bytes)'),
    );
    expect(FrameDemandFlag.none.toString(), 'FrameDemandFlag.none');
    expect(
      (FrameDemandFlag.present |
              FrameDemandFlag.waitForUpdate |
              const FrameDemandFlag.fromRawValue(0x100))
          .toString(),
      'FrameDemandFlag.present | FrameDemandFlag.waitForUpdate | 0x100',
    );
    expect(RuntimeEventFlag.all.toString(), 'RuntimeEventFlag.all');
  });
}
