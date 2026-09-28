import 'dart:typed_data';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:maplibre_native_ffi/src/internal/value/uint64.dart';
import 'package:test/test.dart';

void main() {
  test('full-range uint64 conversion preserves native bit patterns', () {
    final maximum = (BigInt.one << 64) - BigInt.one;

    expect(uint64ToNative(maximum, 'value'), -1);
    expect(uint64FromNative(-1), maximum);
    expect(
      () => uint64ToNative(maximum + BigInt.one, 'value'),
      throwsA(isA<InvalidArgumentException>()),
    );
  });

  test('camera descriptor values compare and hash by every field', () {
    final equalPairs = <(Object, Object)>[
      (
        const CameraOptions(center: LatLng(1, 2), zoom: 3),
        const CameraOptions(center: LatLng(1, 2), zoom: 3),
      ),
      (
        AnimationOptions(
          durationMs: 4,
          easing: const UnitBezier(0, 0, 1, 1),
          transitionId: BigInt.from(5),
        ),
        AnimationOptions(
          durationMs: 4,
          easing: const UnitBezier(0, 0, 1, 1),
          transitionId: BigInt.from(5),
        ),
      ),
      (
        const CameraFitOptions(bearing: 5, pitch: 6),
        const CameraFitOptions(bearing: 5, pitch: 6),
      ),
      (NorthOrientation.fromRawValue(100), NorthOrientation.fromRawValue(100)),
      (
        const MapViewportOptions(
          northOrientation: NorthOrientation.right,
          frustumOffset: EdgeInsets(top: 1),
        ),
        const MapViewportOptions(
          northOrientation: NorthOrientation.right,
          frustumOffset: EdgeInsets(top: 1),
        ),
      ),
      (
        const MapTileOptions(lodScale: 2, lodMode: TileLodMode.distance),
        const MapTileOptions(lodScale: 2, lodMode: TileLodMode.distance),
      ),
      (
        const BoundOptions(unbounded: true, minZoom: 1, maxZoom: 10),
        const BoundOptions(unbounded: true, minZoom: 1, maxZoom: 10),
      ),
      (
        const FreeCameraOptions(position: Vec3(1, 2, 3)),
        const FreeCameraOptions(position: Vec3(1, 2, 3)),
      ),
      (
        const ProjectionMode(axonometric: true, xSkew: 0.5),
        const ProjectionMode(axonometric: true, xSkew: 0.5),
      ),
      (
        const StyleLayerEntry(
          id: 'roads',
          type: 'line',
          sourceId: 'streets',
          sourceLayer: 'transportation',
        ),
        const StyleLayerEntry(
          id: 'roads',
          type: 'line',
          sourceId: 'streets',
          sourceLayer: 'transportation',
        ),
      ),
    ];

    for (final (left, right) in equalPairs) {
      expect(left, right);
      expect(left.hashCode, right.hashCode);
    }
    expect(const CameraOptions(zoom: 3), isNot(const CameraOptions(zoom: 4)));
    expect(
      const StyleLayerEntry(id: 'roads', type: 'line', sourceId: 'streets'),
      isNot(const StyleLayerEntry(id: 'roads', type: 'line')),
    );
    expect(
      GeojsonSourceOptions(cluster: true, clusterRadius: 50),
      GeojsonSourceOptions(cluster: true, clusterRadius: 50),
    );
  });

  test('query descriptors preserve public semantic fields', () {
    final geometry = RenderedQueryGeometryLineString(
      ScreenLineString(points: [ScreenPoint(1, 2), ScreenPoint(3, 4)]),
    );
    final renderedOptions = RenderedFeatureQueryOptions(
      layerIds: ['roads'],
      filter: Uint8List.fromList('["==","class","primary"]'.codeUnits),
    );
    final sourceOptions = SourceFeatureQueryOptions(
      sourceLayerIds: ['transportation'],
    );

    expect(geometry.value.points.length, 2);
    expect(renderedOptions.layerIds, ['roads']);
    expect(renderedOptions.filter, '["==","class","primary"]'.codeUnits);
    expect(sourceOptions.sourceLayerIds, ['transportation']);

    final hit = QueriedFeature(
      feature: Uint8List.fromList('{"type":"Feature"}'.codeUnits),
      sourceId: 'point',
      state: Uint8List.fromList('{"selected":true}'.codeUnits),
    );
    expect(hit.sourceId, 'point');
    expect(hit.sourceLayerId, isNull);
    expect(hit.feature, '{"type":"Feature"}'.codeUnits);
    expect(hit.state, '{"selected":true}'.codeUnits);
  });

  test('byte-backed values own storage and compare by content', () {
    final clusterProperties = Uint8List.fromList([1, 2, 3]);
    final filter = Uint8List.fromList([4, 5, 6]);
    final geometry = Uint8List.fromList([7, 8, 9]);
    final feature = Uint8List.fromList([10, 11, 12]);
    final state = Uint8List.fromList([13, 14, 15]);
    final geoJsonOptions = GeojsonSourceOptions(
      clusterProperties: clusterProperties,
    );
    final queryOptions = RenderedFeatureQueryOptions(filter: filter);
    final offlineDefinition = OfflineGeometryRegionDefinition(
      styleUrl: 'https://example.invalid/style.json',
      geometry: geometry,
      minZoom: 0,
      maxZoom: 10,
      pixelRatio: 1,
    );
    final queriedFeature = QueriedFeature(
      feature: feature,
      sourceId: 'point',
      state: state,
    );

    clusterProperties[0] = 9;
    filter[0] = 9;
    geometry[0] = 9;
    feature[0] = 9;
    state[0] = 9;

    expect(
      geoJsonOptions,
      GeojsonSourceOptions(clusterProperties: Uint8List.fromList([1, 2, 3])),
    );
    expect(
      queryOptions,
      RenderedFeatureQueryOptions(filter: Uint8List.fromList([4, 5, 6])),
    );
    expect(
      offlineDefinition,
      OfflineGeometryRegionDefinition(
        styleUrl: 'https://example.invalid/style.json',
        geometry: Uint8List.fromList([7, 8, 9]),
        minZoom: 0,
        maxZoom: 10,
        pixelRatio: 1,
      ),
    );
    expect(
      queriedFeature,
      QueriedFeature(
        feature: Uint8List.fromList([10, 11, 12]),
        sourceId: 'point',
        state: Uint8List.fromList([13, 14, 15]),
      ),
    );
    expect(
      () => geoJsonOptions.clusterProperties![0] = 9,
      throwsUnsupportedError,
    );
    expect(() => queryOptions.filter![0] = 9, throwsUnsupportedError);
    expect(() => offlineDefinition.geometry[0] = 9, throwsUnsupportedError);
    expect(() => queriedFeature.feature[0] = 9, throwsUnsupportedError);
    expect(() => queriedFeature.state![0] = 9, throwsUnsupportedError);
  });

  test('resource responses preserve public semantic fields', () {
    final response = ResourceResponse(
      status: ResourceResponseStatus.ok,
      bytes: Uint8List.fromList([1, 2, 3]),
      etag: 'abc',
      modifiedUnixMs: 42,
    );

    expect(response.status, ResourceResponseStatus.ok);
    expect(response.bytes, [1, 2, 3]);
    expect(response.etag, 'abc');
    expect(response.modifiedUnixMs, 42);
  });
}
