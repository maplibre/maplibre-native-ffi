package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenBox
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions

class RenderQueryTest {
  // BND-106: rendered and source queries copy GeoJSON, source IDs, and feature state.

  @Test
  fun renderedAndSourceQueriesReturnCopiedQueriedFeatures() {
    Maplibre.setLogCallback(LogCallback { true })
    Maplibre.setAsyncLogSeverities(emptySet())
    try {
      val runtime = RuntimeHandle.create(RuntimeOptions())
      try {
        val map =
          MapHandle.create(
            runtime,
            MapOptions().apply {
              width = 64
              height = 64
            },
          )
        try {
          val owned = OwnedTextureTestSupport.attach(map, 32, 16) ?: return
          owned.use {
            val session = owned.session
            val featureCoordinate = LatLng(37.7749, -122.4194)
            map.jumpTo(CameraOptions().apply { center = featureCoordinate })
            map.setStyleJson(QUERY_STYLE_JSON.encodeToByteArray())
            assertTrue(waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE))
            assertEquals(RenderResult.RENDERED, session.renderUpdate().result)

            val queryPoint = map.pixelForLatLng(featureCoordinate)
            val queryGeometry =
              RenderedQueryGeometry.Box(
                ScreenBox(
                  ScreenPoint(queryPoint.x - 20.0, queryPoint.y - 20.0),
                  ScreenPoint(queryPoint.x + 20.0, queryPoint.y + 20.0),
                )
              )
            val filter = jsonBytes("""["==",["get","kind"],"capital"]""")
            val rendered =
              waitForQueriedFeature(runtime, map, session) {
                session.queryRenderedFeatures(
                  queryGeometry,
                  RenderedFeatureQueryOptions().apply {
                    layerIds = listOf("point-circle")
                    this.filter = filter
                  },
                )
              }
            assertEquals("point", rendered.sourceId)
            assertEquals("capital", featureStringProperty(rendered.feature, "kind"))

            val source =
              waitForQueriedFeature(runtime, map, session) {
                session.querySourceFeatures(
                  "point",
                  SourceFeatureQueryOptions().apply { this.filter = filter },
                )
              }
            assertEquals("point", source.sourceId)
            assertEquals("capital", featureStringProperty(source.feature, "kind"))

            session.setFeatureState(featureStateSelector(), featureState())
            renderIfAvailable(runtime, map, session)
            val renderedWithState =
              waitForQueriedFeature(runtime, map, session) {
                session.queryRenderedFeatures(
                  queryGeometry,
                  RenderedFeatureQueryOptions().apply {
                    layerIds = listOf("point-circle")
                    this.filter = filter
                  },
                )
              }
            val renderedState = renderedWithState.state ?: error("missing state")
            assertEquals("true", rawMember(renderedState, "hover")?.decodeToString())
            assertEquals(20.0, numberMember(renderedState, "radius"))
          }
        } finally {
          map.close()
        }
      } finally {
        runtime.close()
      }
    } finally {
      Maplibre.clearLogCallback()
      Maplibre.restoreDefaultAsyncLogSeverities()
    }
  }

  private fun featureStateSelector(): FeatureStateSelector =
    FeatureStateSelector("point").apply { featureId = "feature-1" }

  private fun featureState(): ByteArray = jsonBytes("""{"hover":true,"radius":20}""")
}
