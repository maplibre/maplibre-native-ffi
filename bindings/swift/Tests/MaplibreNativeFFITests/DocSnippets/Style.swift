// Snippets for the docs site's Style page. They compile with the
// tests, which
// never run them.

import Foundation
import MaplibreNativeFFI

private func drawImage(_: String) -> PremultipliedRgba8Image {
  PremultipliedRgba8Image(
    width: 1,
    height: 1,
    stride: 4,
    pixels: Data([255, 0, 0, 255])
  )
}

/// pixels holds 16 x 16 premultiplied RGBA pixels.
private func style(map: MapHandle, pixels: Data) async throws {
  // #region load
  try await map
    .setStyleUrl("https://demotiles.maplibre.org/style.json")
  // #endregion load

  // #region add
  try map.discarding.addStyleSourceJson("satellite", sourceJson: """
  {
    "type": "raster",
    "tileSize": 256,
    "tiles": ["https://tiles.example.com/{z}/{x}/{y}.png"]
  }
  """)

  // Draw the layer under the "countries-label" layer.
  try map.discarding.addStyleLayerJson(
    #"{"id": "satellite", "type": "raster", "source": "satellite"}"#,
    beforeLayerId: "countries-label"
  )
  // #endregion add

  // #region geojson
  try map.discarding.addGeoJsonSource("points")
  try map.discarding.setGeoJsonSourceData(
    "points",
    data: #"{"type": "Point", "coordinates": [-122.4, 37.8]}"#
  )
  // #endregion geojson

  // #region set-property
  // Show both changes in the same frame.
  try map.discarding.beginCommandGroup()
  try map.discarding.setStyleLayerProperty(
    "satellite",
    propertyName: "visibility",
    value: #""none""#
  )
  try map.discarding.setStyleLayerProperty(
    "countries-boundary",
    propertyName: "line-color",
    value: """
    ["interpolate", ["linear"], ["zoom"], 5, "#9cf", 12, "#06c"]
    """
  )
  try map.discarding.endCommandGroup()
  // #endregion set-property

  // #region image
  try map.discarding.setStyleImage(
    "marker",
    image: PremultipliedRgba8Image(
      width: 16,
      height: 16,
      stride: 16 * 4,
      pixels: pixels
    )
  )

  // An empty layer ID draws the layer on top.
  try map.discarding.addStyleLayerJson("""
  {
    "id": "markers",
    "type": "symbol",
    "source": "points",
    "layout": {"icon-image": "marker"}
  }
  """)
  // #endregion image
}

private func styleEvents(
  runtime: RuntimeHandle,
  map: MapHandle
) throws {
  // #region missing-image
  // Runs on a library thread.
  try runtime.discarding
    .setEventHandler(RuntimeEventHandler { events in
      for event in events where event.source == map.id {
        guard case let .styleImage(missing) = event.payload
        else { continue }
        try? map.discarding.setStyleImage(
          missing.imageId,
          image: drawImage(missing.imageId)
        )
      }
    })
  // #endregion missing-image
}
