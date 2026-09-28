package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import org.maplibre.nativeffi.generated.CameraOptions
import org.maplibre.nativeffi.generated.CameraUpdate
import org.maplibre.nativeffi.generated.FeatureStateSelector
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.LatLng
import org.maplibre.nativeffi.generated.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.generated.ScreenBox
import org.maplibre.nativeffi.generated.ScreenPoint
import org.maplibre.nativeffi.generated.SourceFeatureQueryOptions
import org.maplibre.nativeffi.runtime.runSuspendTest

class RenderQueryTest {
  // rendered and source queries copy GeoJSON, source IDs, and feature state.

  @Test
  fun renderedAndSourceQueriesReturnCopiedQueriedFeatures(): Unit = runSuspendTest {
    withOwnedTextureSession { runtime, map, owned ->
      val session = owned.session
      val featureCoordinate = LatLng(37.7749, -122.4194)
      session.completeOnDriver(
        map.updateCamera(CameraUpdate(camera = CameraOptions().copy(center = featureCoordinate)))
      )
      session.completeOnDriver(map.setStyleJson(QUERY_STYLE_JSON.encodeToByteArray()))
      session.completeOnDriver(runtime.barrier())

      val projection = session.completeOnDriver(map.projectionCreate())
      val queryPoint =
        try {
          projection.pixelForLatLng(featureCoordinate)
        } finally {
          projection.close()
        }
      val queryGeometry =
        GeneratedApi.renderedQueryGeometryBox(
          ScreenBox(
            ScreenPoint(queryPoint.x - 20.0, queryPoint.y - 20.0),
            ScreenPoint(queryPoint.x + 20.0, queryPoint.y + 20.0),
          )
        )
      val filter = jsonBytes("""["==",["get","kind"],"capital"]""")
      val rendered =
        waitForQueriedFeature(session) {
          session.queryRenderedFeatures(
            queryGeometry,
            RenderedFeatureQueryOptions().copy(layerIds = listOf("point-circle"), filter = filter),
          )
        }
      assertEquals("point", rendered.sourceId)
      assertEquals("capital", featureStringProperty(rendered.feature, "kind"))

      val source =
        waitForQueriedFeature(session) {
          session.querySourceFeatures("point", SourceFeatureQueryOptions().copy(filter = filter))
        }
      assertEquals("point", source.sourceId)
      assertEquals("capital", featureStringProperty(source.feature, "kind"))

      // Feature state belongs to the map; the renderer observes it on the next update.
      session.completeOnDriver(map.setFeatureState(featureStateSelector(), featureState()))
      var renderedState: ByteArray? = null
      for (attempt in 0 until 500) {
        session.renderOneFrame()
        renderedState =
          session
            .completeOnDriver(
              session.queryRenderedFeatures(
                queryGeometry,
                RenderedFeatureQueryOptions()
                  .copy(layerIds = listOf("point-circle"), filter = filter),
              )
            )
            .firstOrNull()
            ?.state
        if (renderedState != null) break
      }
      renderedState = renderedState ?: error("missing state")
      assertEquals("true", rawMember(renderedState, "hover")?.decodeToString())
      assertEquals(20.0, numberMember(renderedState, "radius"))

      session.completeOnDriver(session.detach())
    }
  }

  private fun featureStateSelector(): FeatureStateSelector =
    FeatureStateSelector(sourceId = "point").copy(featureId = "feature-1")

  private fun featureState(): ByteArray = jsonBytes("""{"hover":true,"radius":20}""")
}
