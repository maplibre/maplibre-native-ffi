// Snippets for the docs site's Style page. They compile with the tests, which never run them.
package org.maplibre.nativeffi.docsnippets

import org.maplibre.nativeffi.GeoJsonSourceDataHandle
import org.maplibre.nativeffi.MapHandle
import org.maplibre.nativeffi.PremultipliedRgba8Image
import org.maplibre.nativeffi.RuntimeEventHandler
import org.maplibre.nativeffi.RuntimeEventType
import org.maplibre.nativeffi.RuntimeHandle

private fun drawImage(imageId: String): PremultipliedRgba8Image =
  PremultipliedRgba8Image(width = 1u, height = 1u, stride = 4u, pixels = byteArrayOf(-1, 0, 0, -1))

// pixels holds 16 x 16 premultiplied RGBA pixels.
private fun style(map: MapHandle, pixels: ByteArray) {
  // #region load
  map.discarding.setStyleUrl("https://demotiles.maplibre.org/style.json")
  // #endregion load

  // #region add
  map.discarding.addStyleSourceJson(
    "satellite",
    """
    {
      "type": "raster",
      "tileSize": 256,
      "tiles": ["https://tiles.example.com/{z}/{x}/{y}.png"]
    }
    """,
  )

  // Draw the layer under the "countries-label" layer.
  map.discarding.addStyleLayerJson(
    """{"id": "satellite", "type": "raster", "source": "satellite"}""",
    "countries-label",
  )
  // #endregion add

  // #region geojson
  val data = GeoJsonSourceDataHandle.create("""{"type": "Point", "coordinates": [-122.4, 37.8]}""")
  map.discarding.addGeoJsonSourceData("points", data)
  // The source keeps its own reference to the data.
  data.close()

  // Later, prepare new data with the same options, and set it.
  val next = GeoJsonSourceDataHandle.create("""{"type": "Point", "coordinates": [2.35, 48.86]}""")
  map.discarding.setGeoJsonSourceData("points", next)
  next.close()
  // #endregion geojson

  // #region set-property
  // Show both changes in the same frame.
  map.discarding.beginCommandGroup()
  map.discarding.setStyleLayerProperty("satellite", "visibility", "\"none\"")
  map.discarding.setStyleLayerProperty(
    "countries-boundary",
    "line-color",
    """["interpolate", ["linear"], ["zoom"], 5, "#9cf", 12, "#06c"]""",
  )
  map.discarding.endCommandGroup()
  // #endregion set-property

  // #region image
  map.discarding.setStyleImage(
    "marker",
    PremultipliedRgba8Image(width = 16u, height = 16u, stride = 16u * 4u, pixels = pixels),
  )

  // An empty layer ID draws the layer on top.
  map.discarding.addStyleLayerJson(
    """
    {
      "id": "markers",
      "type": "symbol",
      "source": "points",
      "layout": {"icon-image": "marker"}
    }
    """,
    "",
  )
  // #endregion image
}

private fun styleEvents(runtime: RuntimeHandle, map: MapHandle) {
  // #region missing-image
  runtime.discarding.setEventHandler(
    RuntimeEventHandler { events ->
      for (event in events) {
        if (event.source != map.id || event.type != RuntimeEventType.MAP_STYLE_IMAGE_MISSING) {
          continue
        }
        // The message holds the image's ID.
        map.discarding.setStyleImage(event.message, drawImage(event.message))
      }
    },
  )
  // #endregion missing-image
}
