import 'dart:typed_data';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:maplibre_native_ffi/src/internal/c/maplibre_native_c.g.dart'
    as raw;
import 'package:maplibre_native_ffi/src/internal/struct/struct.dart';
import 'package:maplibre_native_ffi/src/internal/value/uint64.dart';
import 'package:test/test.dart';

void main() {
  test('geographic values preserve fields', () {
    const coordinate = LatLng(45, -122);
    final native = latLngToNative(coordinate);

    expect(native.latitude, 45);
    expect(native.longitude, -122);
    expect(latLngFromNative(native), coordinate);
    expect(
      latLngBoundsToNative(
        const LatLngBounds(southwest: LatLng(1, 2), northeast: LatLng(3, 4)),
      ).northeast.longitude,
      4,
    );
  });

  test('full-range uint64 conversion preserves native bit patterns', () {
    final maximum = (BigInt.one << 64) - BigInt.one;

    expect(uint64ToNative(maximum, 'value'), -1);
    expect(uint64FromNative(-1), maximum);
    expect(
      () => uint64ToNative(maximum + BigInt.one, 'value'),
      throwsA(isA<InvalidArgumentException>()),
    );
  });

  test('camera options materialize field masks and semantic fields', () {
    final native = cameraOptionsToNative(
      const CameraOptions(
        center: LatLng(1, 2),
        zoom: 3,
        padding: EdgeInsets(top: 4, left: 5, bottom: 6, right: 7),
        fieldOfView: 8,
      ),
      raw.mln_camera_options_default(),
    );

    expect(
      native.fields &
          raw.mln_camera_option_field.MLN_CAMERA_OPTION_CENTER.value,
      isNonZero,
    );
    expect(
      native.fields & raw.mln_camera_option_field.MLN_CAMERA_OPTION_ZOOM.value,
      isNonZero,
    );
    expect(
      native.fields &
          raw.mln_camera_option_field.MLN_CAMERA_OPTION_PADDING.value,
      isNonZero,
    );
    expect(native.latitude, 1);
    expect(native.longitude, 2);
    expect(native.zoom, 3);
    expect(native.padding.bottom, 6);
    expect(native.field_of_view, 8);
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
        const BoundOptions(
          bounds: BoundsConstraint.unbounded(),
          minZoom: 1,
          maxZoom: 10,
        ),
        const BoundOptions(
          bounds: BoundsConstraint.unbounded(),
          minZoom: 1,
          maxZoom: 10,
        ),
      ),
      (
        const FreeCameraOptions(position: Vec3(1, 2, 3)),
        const FreeCameraOptions(position: Vec3(1, 2, 3)),
      ),
      (
        const ProjectionModeOptions(axonometric: true, xSkew: 0.5),
        const ProjectionModeOptions(axonometric: true, xSkew: 0.5),
      ),
    ];

    for (final (left, right) in equalPairs) {
      expect(left, right);
      expect(left.hashCode, right.hashCode);
    }
    expect(const CameraOptions(zoom: 3), isNot(const CameraOptions(zoom: 4)));
    expect(
      GeoJsonSourceOptions(cluster: true, clusterRadius: 50),
      GeoJsonSourceOptions(cluster: true, clusterRadius: 50),
    );
  });

  test('north orientation preserves unknown native values', () {
    final unknown = NorthOrientation.fromRawValue(100);
    final native = mapViewportOptionsToNative(
      MapViewportOptions(northOrientation: unknown),
      raw.mln_map_viewport_options_default(),
    );
    final copied = mapViewportOptionsFromNative(native);

    expect(unknown.name, 'unknown(100)');
    expect(native.north_orientation, 100);
    expect(copied.northOrientation, unknown);
  });

  test('animation options materialize field masks', () {
    final maximum = (BigInt.one << 64) - BigInt.one;
    final native = animationOptionsToNative(
      AnimationOptions(
        durationMs: 100,
        easing: const UnitBezier(0, 0.25, 0.75, 1),
        transitionId: maximum,
      ),
      raw.mln_animation_options_default(),
    );

    expect(
      native.fields &
          raw.mln_animation_option_field.MLN_ANIMATION_OPTION_DURATION.value,
      isNonZero,
    );
    expect(
      native.fields &
          raw.mln_animation_option_field.MLN_ANIMATION_OPTION_EASING.value,
      isNonZero,
    );
    expect(
      native.fields &
          raw
              .mln_animation_option_field
              .MLN_ANIMATION_OPTION_TRANSITION_ID
              .value,
      isNonZero,
    );
    expect(native.duration_ms, 100);
    expect(native.easing.y2, 1);
    expect(native.transition_id, -1);
  });

  test('bounds constraints distinguish bounded and unbounded states', () {
    final bounded = boundOptionsToNative(
      const BoundOptions(
        bounds: BoundsConstraint.bounded(
          LatLngBounds(southwest: LatLng(-1, -2), northeast: LatLng(3, 4)),
        ),
      ),
      raw.mln_bound_options_default(),
    );
    final unbounded = boundOptionsToNative(
      const BoundOptions(bounds: BoundsConstraint.unbounded()),
      raw.mln_bound_options_default(),
    );

    expect(
      bounded.fields & raw.mln_bound_option_field.MLN_BOUND_OPTION_BOUNDS.value,
      isNonZero,
    );
    expect(
      unbounded.fields &
          raw.mln_bound_option_field.MLN_BOUND_OPTION_UNBOUNDED.value,
      isNonZero,
    );
    expect(
      boundOptionsFromNative(unbounded).bounds,
      const BoundsConstraint.unbounded(),
    );
  });

  test('query descriptors preserve public semantic fields', () {
    final geometry = RenderedQueryLineString([
      ScreenPoint(1, 2),
      ScreenPoint(3, 4),
    ]);
    final renderedOptions = RenderedFeatureQueryOptions(
      layerIds: ['roads'],
      filter: Uint8List.fromList('["==","class","primary"]'.codeUnits),
    );
    final sourceOptions = SourceFeatureQueryOptions(
      sourceLayerIds: ['transportation'],
    );

    expect(geometry.points.length, 2);
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
    final geoJsonOptions = GeoJsonSourceOptions(
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
      GeoJsonSourceOptions(clusterProperties: Uint8List.fromList([1, 2, 3])),
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

  test('public enum-like values preserve native raw values', () {
    expect(ResourceKind.tile.rawValue, 3);
    expect(ResourceLoadingMethod.all.rawValue, 0);
    expect(ResourceStoragePolicy.permanent.rawValue, 0);
    expect(ResourceResponseStatus.error.rawValue, 1);
    expect(ResourceErrorReason.rateLimit.rawValue, 4);
    expect(SourceType.customVector.rawValue, 8);
    expect(TileScheme.tms.rawValue, 1);
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
