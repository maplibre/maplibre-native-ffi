package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNotEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenBox
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.style.GeoJsonSourceDataHandle
import org.maplibre.nativeffi.style.GeoJsonSourceOptions

class RenderClusterQueryTest {
  // BND-107: an unsigned cluster_id survives the query round trip, and an
  // unsigned leaves limit bounds the returned features.

  @Test
  fun clusterFeatureExtensionQueriesResolveUnsignedClusterIdAndLimit() {
    Maplibre.setLogCallback(LogCallback { true })
    Maplibre.setAsyncLogSeverities(emptySet())
    try {
      withOwnedTextureSession(width = 64, height = 64) { runtime, map, owned ->
        val session = owned.session
        map.jumpTo(
          CameraOptions().apply {
            center = LatLng(0.0, 0.0)
            zoom = 0.0
          }
        )
        map.setStyleJson(CLUSTER_STYLE_JSON.encodeToByteArray())
        GeoJsonSourceDataHandle.create(clusterPoints(), clusterSourceOptions()).use { clusterData ->
          map.addGeoJsonSourceData("cluster-source", clusterData)
        }
        map.addStyleLayerJson(clusterCircleLayer(), "")
        assertTrue(waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE))
        assertEquals(RenderResult.RENDERED, session.renderUpdate().result)

        val queryPoint = map.pixelForLatLng(LatLng(0.0, 0.0))
        val queryGeometry =
          RenderedQueryGeometry.Box(
            ScreenBox(
              ScreenPoint(queryPoint.x - 30.0, queryPoint.y - 30.0),
              ScreenPoint(queryPoint.x + 30.0, queryPoint.y + 30.0),
            )
          )
        val cluster =
          waitForQueriedFeature(runtime, map, session) {
            session.queryRenderedFeatures(
              queryGeometry,
              RenderedFeatureQueryOptions().apply { layerIds = listOf("cluster-circle") },
            )
          }
        val clusterProperties =
          rawMember(cluster.feature, "properties") ?: error("feature has no properties")
        // The serialized feature must keep cluster_id as an integral value so
        // MapLibre can resolve it when the bytes are passed back in.
        assertTrue(numberMember(clusterProperties, "cluster_id") != null)

        // The rendered cluster exists because GeoJsonSourceOptions enables
        // clustering, and weightSum comes from the byte-encoded aggregation.
        assertEquals(3.0, numberMember(clusterProperties, "point_count"))
        assertEquals(6.0, numberMember(clusterProperties, "weightSum"))

        val children =
          session.queryFeatureExtension(
            "cluster-source",
            cluster.feature,
            "supercluster",
            "children",
            null,
          )
        assertTrue(firstFeature(children) != null)

        val expansionZoom =
          session.queryFeatureExtension(
            "cluster-source",
            cluster.feature,
            "supercluster",
            "expansion-zoom",
            null,
          )
        assertTrue(expansionZoom.decodeToString().toULongOrNull() != null)

        // An unsigned limit bounds the collection, and an unsigned offset
        // selects a later leaf. Native ignores arguments of another type and
        // falls back to ten leaves at offset zero, so both bounds must move
        // the observed result.
        val feature = cluster.feature
        val first = singleClusterLeaf(session, feature, 0)
        val second = singleClusterLeaf(session, feature, 1)
        assertNotEquals(featureStringProperty(first, "name"), featureStringProperty(second, "name"))
      }
    } finally {
      Maplibre.clearLogCallback()
      Maplibre.restoreDefaultAsyncLogSeverities()
    }
  }

  /** Returns the one leaf at [offset] through a bounded supercluster query. */
  private fun singleClusterLeaf(
    session: RenderSessionHandle,
    feature: ByteArray,
    offset: Long,
  ): ByteArray {
    val leaves =
      session.queryFeatureExtension(
        "cluster-source",
        feature,
        "supercluster",
        "leaves",
        jsonBytes("""{"limit":1,"offset":$offset}"""),
      )
    return firstFeature(leaves) ?: error("expected one leaf")
  }

  /** Point features close enough together to collapse into one cluster at zoom 0. */
  private fun clusterPoints(): ByteArray =
    jsonBytes(
      """
      {
        "type": "FeatureCollection",
        "features": [
          ${clusterPoint("one", 0.0)},
          ${clusterPoint("two", 0.001)},
          ${clusterPoint("three", 0.002)}
        ]
      }
      """
    )

  private fun clusterPoint(name: String, offset: Double): String =
    """{"type":"Feature","geometry":{"type":"Point","coordinates":[$offset,$offset]},"properties":{"name":"$name","weight":2}}"""

  private fun clusterSourceOptions(): GeoJsonSourceOptions =
    GeoJsonSourceOptions().apply {
      cluster = true
      clusterRadius = 50
      clusterMaxZoom = 14.0
      clusterMinPoints = 2
      clusterProperties = jsonBytes("""{"weightSum":["+",["get","weight"]]}""")
    }

  private fun clusterCircleLayer(): ByteArray =
    jsonBytes(
      """
      {
        "id": "cluster-circle",
        "type": "circle",
        "source": "cluster-source",
        "filter": ["has", "point_count"],
        "paint": {"circle-color": "#2563eb", "circle-radius": 20}
      }
      """
    )

  private companion object {
    /**
     * The clustered source and its layer are added afterwards through the typed GeoJSON adder, so
     * clustering comes from [GeoJsonSourceOptions] rather than from style JSON.
     */
    private const val CLUSTER_STYLE_JSON =
      """
      {
        "version": 8,
        "name": "kotlin-cluster-query-test",
        "sources": {},
        "layers": [
          {"id": "background", "type": "background", "paint": {"background-color": "#ffffff"}}
        ]
      }
      """
  }
}
