// Snippets for the docs site's Style page. They are analyzed with the tests,
// which never run them.

import 'dart:typed_data';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

PremultipliedRgba8Image drawImage(String imageId) => PremultipliedRgba8Image(
  width: 1,
  height: 1,
  stride: 4,
  pixels: Uint8List.fromList([255, 0, 0, 255]),
);

// pixels holds 16 x 16 premultiplied RGBA pixels.
void style(MapHandle map, Uint8List pixels) {
  // #region load
  map.discarding.setStyleUrl('https://demotiles.maplibre.org/style.json');
  // #endregion load

  // #region add
  map.discarding.addStyleSourceJson('satellite', {
    'type': 'raster',
    'tileSize': 256,
    'tiles': ['https://tiles.example.com/{z}/{x}/{y}.png'],
  });

  // Draw the layer under the "countries-label" layer.
  map.discarding.addStyleLayerJson({
    'id': 'satellite',
    'type': 'raster',
    'source': 'satellite',
  }, 'countries-label');
  // #endregion add

  // #region geojson
  final data = GeoJsonSourceDataHandle.create({
    'type': 'Point',
    'coordinates': [-122.4, 37.8],
  });
  map.discarding.addGeoJsonSourceData('points', data);
  // The source keeps its own reference to the data.
  data.close();

  // Later, prepare new data with the same options, and set it.
  final next = GeoJsonSourceDataHandle.create({
    'type': 'Point',
    'coordinates': [2.35, 48.86],
  });
  map.discarding.setGeoJsonSourceData('points', next);
  next.close();
  // #endregion geojson

  // #region set-property
  // Show both changes in the same frame.
  map.discarding.beginCommandGroup();
  map.discarding.setStyleLayerProperty('satellite', 'visibility', 'none');
  map.discarding.setStyleLayerProperty('countries-boundary', 'line-color', [
    'interpolate',
    ['linear'],
    ['zoom'],
    5,
    '#9cf',
    12,
    '#06c',
  ]);
  map.discarding.endCommandGroup();
  // #endregion set-property

  // #region image
  map.discarding.setStyleImage(
    'marker',
    PremultipliedRgba8Image(
      width: 16,
      height: 16,
      stride: 16 * 4,
      pixels: pixels,
    ),
  );

  // An empty layer ID draws the layer on top.
  map.discarding.addStyleLayerJson({
    'id': 'markers',
    'type': 'symbol',
    'source': 'points',
    'layout': {'icon-image': 'marker'},
  }, '');
  // #endregion image
}

void styleEvents(RuntimeHandle runtime, MapHandle map) {
  // #region missing-image
  // The handler runs in this isolate. The binding queues each call.
  runtime.discarding.setEventHandler((events) {
    for (final event in events) {
      if (event.source != map.id ||
          event.type != RuntimeEventType.mapStyleImageMissing) {
        continue;
      }
      // The message holds the image's ID.
      map.discarding.setStyleImage(event.message, drawImage(event.message));
    }
  });
  // #endregion missing-image
}
