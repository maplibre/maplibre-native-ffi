// Snippets for the docs site's Style page. They compile with the tests, which
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
private func style(map: MapHandle, pixels: Data) throws {
  // #region load
  try map.discarding.setStyleUrl("https://demotiles.maplibre.org/style.json")
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
  let data = try GeoJsonSourceDataHandle.create(
    data: #"{"type": "Point", "coordinates": [-122.4, 37.8]}"#
  )
  try map.discarding.addGeoJsonSourceData("points", data: data)
  // The source keeps its own reference to the data.
  data.close()

  // Later, prepare new data with the same options, and set it.
  let next = try GeoJsonSourceDataHandle.create(
    data: #"{"type": "Point", "coordinates": [2.35, 48.86]}"#
  )
  try map.discarding.setGeoJsonSourceData("points", data: next)
  next.close()
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
  """, beforeLayerId: "")
  // #endregion image
}

private func styleEvents(runtime: RuntimeHandle, map: MapHandle) throws {
  // #region missing-image
  // Runs on a library thread.
  try runtime.discarding.setEventHandler(RuntimeEventHandler { events in
    for event in events
      where event.source == map.id && event.type == .mapStyleImageMissing
    {
      // The message holds the image's ID.
      try? map.discarding.setStyleImage(
        event.message,
        image: drawImage(event.message)
      )
    }
  })
  // #endregion missing-image
}
