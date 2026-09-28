package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNotEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.generated.CameraOptions
import org.maplibre.nativeffi.generated.CameraUpdate
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.GeojsonSourceOptions
import org.maplibre.nativeffi.generated.LatLng
import org.maplibre.nativeffi.generated.RenderSessionHandle
import org.maplibre.nativeffi.generated.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.generated.ScreenBox
import org.maplibre.nativeffi.generated.ScreenPoint
import org.maplibre.nativeffi.runtime.runSuspendTest

class RenderClusterQueryTest {
  // an unsigned cluster_id survives the query round trip, and an
  // unsigned leaves limit bounds the returned features.

  @Test
  fun clusterFeatureExtensionQueriesResolveUnsignedClusterIdAndLimit(): Unit = runSuspendTest {
    withOwnedTextureSession(width = 64, height = 64) { runtime, map, owned ->
      val session = owned.session
      session.completeOnDriver(
        map.updateCamera(
          CameraUpdate(camera = CameraOptions().copy(center = LatLng(0.0, 0.0), zoom = 0.0))
        )
      )
      session.completeOnDriver(map.setStyleJson(CLUSTER_STYLE_JSON.encodeToByteArray()))
      GeneratedApi.geojsonSourceDataCreate(clusterPoints(), clusterSourceOptions()).use {
        clusterData ->
        session.completeOnDriver(map.addGeojsonSourceData("cluster-source", clusterData))
      }
      session.completeOnDriver(map.addStyleLayerJson(clusterCircleLayer(), ""))
      session.completeOnDriver(runtime.barrier())

      val projection = session.completeOnDriver(map.projectionCreate())
      val queryPoint =
        try {
          projection.pixelForLatLng(LatLng(0.0, 0.0))
        } finally {
          projection.close()
        }
      val queryGeometry =
        GeneratedApi.renderedQueryGeometryBox(
          ScreenBox(
            ScreenPoint(queryPoint.x - 30.0, queryPoint.y - 30.0),
            ScreenPoint(queryPoint.x + 30.0, queryPoint.y + 30.0),
          )
        )
      val cluster =
        waitForQueriedFeature(session) {
          session.queryRenderedFeatures(
            queryGeometry,
            RenderedFeatureQueryOptions().copy(layerIds = listOf("cluster-circle")),
          )
        }
      val clusterProperties =
        rawMember(cluster.feature, "properties") ?: error("feature has no properties")
      // The serialized feature must keep cluster_id as an integral value so
      // MapLibre can resolve it when the bytes are passed back in.
      assertTrue(numberMember(clusterProperties, "cluster_id") != null)

      // The rendered cluster exists because GeojsonSourceOptions enables
      // clustering, and weightSum comes from the byte-encoded aggregation.
      assertEquals(3.0, numberMember(clusterProperties, "point_count"))
      assertEquals(6.0, numberMember(clusterProperties, "weightSum"))

      val children =
        session.completeOnDriver(
          session.queryFeatureExtensions(
            "cluster-source",
            cluster.feature,
            "supercluster",
            "children",
            null,
          )
        )
      assertTrue(firstFeature(children) != null)

      val expansionZoom =
        session.completeOnDriver(
          session.queryFeatureExtensions(
            "cluster-source",
            cluster.feature,
            "supercluster",
            "expansion-zoom",
            null,
          )
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

      session.completeOnDriver(session.detach())
    }
  }

  /** Returns the one leaf at [offset] through a bounded supercluster query. */
  private suspend fun singleClusterLeaf(
    session: RenderSessionHandle,
    feature: ByteArray,
    offset: Long,
  ): ByteArray {
    val leaves =
      session.completeOnDriver(
        session.queryFeatureExtensions(
          "cluster-source",
          feature,
          "supercluster",
          "leaves",
          jsonBytes("""{"limit":1,"offset":$offset}"""),
        )
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

  private fun clusterSourceOptions(): GeojsonSourceOptions =
    GeojsonSourceOptions(
      cluster = true,
      clusterRadius = 50u,
      clusterMaxZoom = 14.0,
      clusterMinPoints = 2u,
      clusterProperties = jsonBytes("""{"weightSum":["+",["get","weight"]]}"""),
    )

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
     * clustering comes from [GeojsonSourceOptions] rather than from style JSON.
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
