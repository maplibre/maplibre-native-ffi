package org.maplibre.nativeffi.render

import kotlin.test.assertEquals
import org.maplibre.nativeffi.CameraOptions
import org.maplibre.nativeffi.CameraUpdate
import org.maplibre.nativeffi.LatLng
import org.maplibre.nativeffi.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.RenderedQueryGeometry
import org.maplibre.nativeffi.ScreenBox
import org.maplibre.nativeffi.ScreenPoint
import org.maplibre.nativeffi.SourceFeatureQueryOptions

/**
 * Renders one point feature per entry of [properties], with ids `case-0`, `case-1`, and so on, and
 * asserts that both a source query and the rendered layer keep exactly the [expected] indices under
 * [filter]. The two paths evaluate the filter on different threads.
 */
internal suspend fun assertFilterSelects(
  filter: String,
  properties: List<String>,
  expected: Set<Int> = properties.indices.toSet(),
) {
  val features =
    properties
      .mapIndexed { index, value ->
        """{"type":"Feature","id":"case-$index","geometry":{"type":"Point","coordinates":[0,0]},"properties":$value}"""
      }
      .joinToString(",")
  val style =
    """{"version":8,"sources":{"point":{"type":"geojson","data":{
      "type":"FeatureCollection","features":[$features]}}},"layers":[{
      "id":"cases","type":"circle","source":"point","filter":$filter,"paint":{"circle-radius":4}}]}"""
  withOwnedTexture(width = FILTER_MAP_SIZE, height = FILTER_MAP_SIZE) {
    val map = mapFixture.map
    map
      .updateCamera(CameraUpdate(camera = CameraOptions(center = LatLng(0.0, 0.0), zoom = 2.0)))
      .await()
    complete(map.setStyleJson(style.trimIndent()))
    // The still image waits for the source to load and the layer to render.
    renderStill()

    val ids = expected.map { "case-$it" }.toSet()
    val source =
      complete(
        session.querySourceFeatures(
          "point",
          SourceFeatureQueryOptions(filter = filter.trimIndent()),
        ),
      )
    assertEquals(ids, source.map { it.feature.stringId() }.toSet(), "source query: $filter")
    val rendered =
      complete(
        session.queryRenderedFeatures(
          RenderedQueryGeometry.Box(
            ScreenBox(
              ScreenPoint(0.0, 0.0),
              ScreenPoint(FILTER_MAP_SIZE.toDouble(), FILTER_MAP_SIZE.toDouble()),
            ),
          ),
          RenderedFeatureQueryOptions(layerIds = listOf("cases")),
        ),
      )
    assertEquals(
      ids,
      rendered.map { it.feature.stringId() }.toSet(),
      "rendered layer filter: $filter",
    )
  }
}

private const val FILTER_MAP_SIZE = 64

private val STRING_ID = Regex("\"id\"\\s*:\\s*\"([^\"]*)\"")

/** The feature's string id. The fixture's properties hold no member named id. */
private fun String.stringId(): String? = STRING_ID.find(this)?.groupValues?.get(1)
