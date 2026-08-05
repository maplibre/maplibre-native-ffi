package org.maplibre.nativeffi.render

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertIs
import kotlin.test.assertNotNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.assertResultHandleDestroyed
import org.maplibre.nativeffi.browserTest
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.geo.Feature
import org.maplibre.nativeffi.geo.FeatureIdentifier
import org.maplibre.nativeffi.geo.GeoJson
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenBox
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.wasm.InjectedFaults
import org.maplibre.nativeffi.json.JsonValue
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.maplibreScope
import org.maplibre.nativeffi.query.FeatureExtensionResult
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.QueriedFeature
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.style.GeoJsonSourceOptions
import org.maplibre.nativeffi.withMap

/**
 * Feature queries, which only a live render session can answer.
 *
 * A query reads the tiles the session last rendered, so everything here needs a real frame first.
 * What comes back is a copied tree — geometry, properties, identifier, feature state — read out of
 * a native result handle that the call releases before it returns, so anything held by reference
 * rather than copied would be reading freed storage by the time it is asserted on.
 */
class QueryBrowserTest {
  // Spec coverage: BND-065, BND-066, BND-071, BND-105, BND-106, BND-107.

  @Test
  fun aQueriedFeatureCarriesACopiedGeometryTreeAndItsIdentifiers(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap(WIDTH, HEIGHT) { runtime, map ->
        withSession(map) { session ->
          map.setStyleJson(POINTS_STYLE_JSON)
          map.jumpTo(
            CameraOptions().apply {
              center = LatLng(0.0, 0.0)
              zoom = 3.0
            }
          )
          render(runtime, session)

          val sourceFeatures = querySourceUntilFound(runtime, session, "points", null)
          assertTrue(sourceFeatures.isNotEmpty(), "the source query returned nothing")

          val queried =
            assertNotNull(
              sourceFeatures.firstOrNull { it.feature.identifier is FeatureIdentifier.StringValue }
            )
          assertEquals("points", queried.sourceId)
          // A GeoJSON source has no source layer, and the C API reports that as absent rather than
          // as an empty string.
          assertEquals(null, queried.sourceLayerId)

          val geometry = assertIs<Geometry.Point>(queried.feature.geometry)
          assertTrue(geometry.coordinate.latitude.isFinite())
          assertEquals(FeatureIdentifier.StringValue("origin"), queried.feature.identifier)
          assertEquals(
            JsonValue.StringValue("origin"),
            queried.feature.properties.firstOrNull { it.key == "name" }?.value,
          )
          // A nested property comes back as a tree rather than flattened.
          val nested =
            assertIs<JsonValue.ObjectValue>(
              assertNotNull(queried.feature.properties.firstOrNull { it.key == "detail" }).value
            )
          assertEquals(
            // The two integers keep the unsigned width the C API gave them rather than arriving
            // as doubles.
            JsonValue.Array(listOf(JsonValue.UInt(1L), JsonValue.UInt(2L))),
            nested.members.firstOrNull { it.key == "pair" }?.value,
          )

          // Built from a distinct list holding equal contents, so the comparison is by value.
          val rebuilt =
            Feature(
              Geometry.Point(LatLng(geometry.coordinate.latitude, geometry.coordinate.longitude)),
              queried.feature.properties.toList(),
              FeatureIdentifier.StringValue("origin"),
            )
          assertEquals(rebuilt, queried.feature)

          // A rendered query reads the same features through screen space instead, which needs the
          // frame that placed them rather than only the tile that holds them.
          val viewport =
            RenderedQueryGeometry.Box(
              ScreenBox(ScreenPoint(0.0, 0.0), ScreenPoint(WIDTH.toDouble(), HEIGHT.toDouble()))
            )
          var rendered: List<QueriedFeature> = emptyList()
          repeat(ATTEMPTS) {
            rendered = session.queryRenderedFeatures(viewport, null)
            if (rendered.isNotEmpty()) return@repeat
            session.renderUpdate()
            runtime.pump(PUMP_MILLIS)
            while (runtime.pollEvent() != null) {}
          }
          assertTrue(rendered.isNotEmpty(), "the rendered query returned nothing")
          assertTrue(
            rendered.any { it.feature.identifier == FeatureIdentifier.StringValue("origin") }
          )

          // Feature state is set on the session and read back through the query result.
          val selector = FeatureStateSelector("points").apply { featureId = "origin" }
          session.setFeatureState(
            selector,
            JsonValue.ObjectValue(listOf(JsonValue.Member("hovered", JsonValue.Bool(true)))),
          )
          val state = assertIs<JsonValue.ObjectValue>(session.getFeatureState(selector))
          assertEquals(
            JsonValue.Bool(true),
            state.members.firstOrNull { it.key == "hovered" }?.value,
          )

          // Removal is applied with the next update rather than in place, so the read that checks
          // it has to follow one.
          session.removeFeatureState(selector)
          var cleared: JsonValue = session.getFeatureState(selector)
          repeat(ATTEMPTS) {
            if (
              cleared == JsonValue.Null ||
                (cleared is JsonValue.ObjectValue &&
                  (cleared as JsonValue.ObjectValue).members.isEmpty())
            ) {
              return@repeat
            }
            session.renderUpdate()
            runtime.pump(PUMP_MILLIS)
            while (runtime.pollEvent() != null) {}
            cleared = session.getFeatureState(selector)
          }
          assertTrue(
            cleared == JsonValue.Null ||
              (cleared is JsonValue.ObjectValue &&
                (cleared as JsonValue.ObjectValue).members.isEmpty()),
            "feature state was $cleared after removal",
          )
        }
      }
    }
  }

  @Test
  fun aClusterFeatureResolvesItsUnsignedIdAndBoundsItsLeaves(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap(WIDTH, HEIGHT) { runtime, map ->
        withSession(map) { session ->
          map.setStyleJson(CLUSTER_STYLE_JSON)
          map.addGeoJsonSourceData(
            "clustered",
            GeoJson.FeatureCollection(
              (0 until LEAF_COUNT).map { index ->
                Feature(
                  Geometry.Point(LatLng(index * 0.0001, index * 0.0001)),
                  listOf(JsonValue.Member("index", JsonValue.Int(index.toLong()))),
                  FeatureIdentifier.Int(index.toLong()),
                )
              }
            ),
            GeoJsonSourceOptions().apply {
              cluster = true
              clusterRadius = 200
              clusterMaxZoom = 20.0
            },
          )
          map.addStyleLayerJson(
            JsonValue.ObjectValue(
              listOf(
                JsonValue.Member("id", JsonValue.StringValue("clusters")),
                JsonValue.Member("type", JsonValue.StringValue("circle")),
                JsonValue.Member("source", JsonValue.StringValue("clustered")),
              )
            ),
            "",
          )
          map.jumpTo(
            CameraOptions().apply {
              center = LatLng(0.0, 0.0)
              zoom = 1.0
            }
          )
          render(runtime, session)

          // The cluster feature is whichever queried feature carries a cluster_id, and the C API
          // requires that property to keep its unsigned width so it can be handed straight back.
          var cluster: QueriedFeature? = null
          querySourceUntilFound(runtime, session, "clustered", null).forEach { candidate ->
            if (candidate.feature.properties.any { it.key == "cluster_id" }) cluster = candidate
          }
          val found = assertNotNull(cluster, "no cluster feature was queried")
          val clusterId =
            assertNotNull(found.feature.properties.firstOrNull { it.key == "cluster_id" }).value
          assertIs<JsonValue.UInt>(clusterId)

          // Handed back unmodified, the extension resolves the cluster and returns its leaves.
          val all =
            session.queryFeatureExtension(
              "clustered",
              found.feature,
              "supercluster",
              "leaves",
              JsonValue.ObjectValue(
                listOf(JsonValue.Member("limit", JsonValue.UInt(LEAF_COUNT.toLong())))
              ),
            )
          val allLeaves = assertIs<FeatureExtensionResult.FeatureCollection>(all).features
          assertTrue(allLeaves.isNotEmpty(), "the extension returned no leaves")

          // An unsigned limit bounds the result, and an unsigned offset shifts it.
          val bounded =
            session.queryFeatureExtension(
              "clustered",
              found.feature,
              "supercluster",
              "leaves",
              JsonValue.ObjectValue(listOf(JsonValue.Member("limit", JsonValue.UInt(2L)))),
            )
          val boundedLeaves = assertIs<FeatureExtensionResult.FeatureCollection>(bounded).features
          assertEquals(2, boundedLeaves.size)

          val shifted =
            session.queryFeatureExtension(
              "clustered",
              found.feature,
              "supercluster",
              "leaves",
              JsonValue.ObjectValue(
                listOf(
                  JsonValue.Member("limit", JsonValue.UInt(2L)),
                  JsonValue.Member("offset", JsonValue.UInt(1L)),
                )
              ),
            )
          val shiftedLeaves = assertIs<FeatureExtensionResult.FeatureCollection>(shifted).features
          assertEquals(2, shiftedLeaves.size)
          assertEquals(boundedLeaves[1], shiftedLeaves[0])
        }
      }
    }
  }

  /**
   * A query result belongs to the call that made it, so a copy that fails still has to end it.
   *
   * The failure injected is the one the copy really has: everything a result is read through goes
   * via a block the page allocates first, and the module's allocator can refuse it. What matters is
   * not the error — a caller sees an allocation failure either way — but whether the result handle
   * native had already produced went with it. A leaked one is silent, so it is replayed against
   * native, which is the only party that can say whether it is still there.
   */
  // Spec coverage: BND-066.
  @Test
  fun aFailedResultCopyDestroysTheNativeResultRatherThanLeakingIt(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap(WIDTH, HEIGHT) { runtime, map ->
        withSession(map) { session ->
          map.setStyleJson(POINTS_STYLE_JSON)
          map.jumpTo(
            CameraOptions().apply {
              center = LatLng(0.0, 0.0)
              zoom = 3.0
            }
          )
          render(runtime, session)
          // The same query first, so what the injected failure changes is this call rather than a
          // query that was never going to answer.
          assertTrue(querySourceUntilFound(runtime, session, "points", null).isNotEmpty())

          val acquired: Long
          try {
            InjectedFaults.failResultCopies()
            val error =
              assertFailsWith<InvalidStateException> { session.querySourceFeatures("points", null) }
            assertEquals(MaplibreStatus.INVALID_STATE, error.status)
            assertTrue(error.diagnostic.contains("could not allocate"), error.diagnostic)
            acquired =
              assertNotNull(
                InjectedFaults.takeCopiedResults().singleOrNull(),
                "the query did not reach the copy, so nothing proves what it did with the result",
              )
          } finally {
            InjectedFaults.reset()
          }
          assertResultHandleDestroyed(
            acquired,
            "mln_feature_query_result_count",
            "mln_feature_query_result",
          )

          // And the session is unharmed: the next query answers as the first one did.
          assertTrue(querySourceUntilFound(runtime, session, "points", null).isNotEmpty())
        }
      }
    }
  }

  /** Queries until the source has tiles to answer from, pumping and rendering in between. */
  private fun querySourceUntilFound(
    runtime: RuntimeHandle,
    session: RenderSessionHandle,
    sourceId: String,
    options: SourceFeatureQueryOptions?,
  ): List<QueriedFeature> {
    var result: List<QueriedFeature> = emptyList()
    repeat(ATTEMPTS) {
      result = session.querySourceFeatures(sourceId, options)
      if (result.isNotEmpty()) return result
      session.renderUpdate()
      runtime.pump(PUMP_MILLIS)
      while (runtime.pollEvent() != null) {}
    }
    return result
  }

  private fun render(runtime: RuntimeHandle, session: RenderSessionHandle) {
    repeat(ATTEMPTS) {
      if (session.renderUpdate()) return
      runtime.pump(PUMP_MILLIS)
      while (runtime.pollEvent() != null) {}
    }
  }

  private fun <T> withSession(map: MapHandle, body: (RenderSessionHandle) -> T): T {
    val context = WebglContext.createOffscreen(WIDTH, HEIGHT)
    try {
      val session =
        map.attachOpenGLOwnedTexture(
          OpenGLOwnedTextureDescriptor(RenderTargetExtent(WIDTH, HEIGHT, 1.0), context.descriptor())
        )
      try {
        return body(session)
      } finally {
        session.close()
      }
    } finally {
      context.close()
    }
  }

  private companion object {
    const val WIDTH = 128
    const val HEIGHT = 128
    const val ATTEMPTS = 400
    const val PUMP_MILLIS = 2L
    const val LEAF_COUNT = 8

    const val POINTS_STYLE_JSON =
      """{"version":8,"sources":{"points":{"type":"geojson","data":{"type":"FeatureCollection",""" +
        """"features":[{"type":"Feature","id":"origin","geometry":{"type":"Point",""" +
        """"coordinates":[0,0]},"properties":{"name":"origin","detail":{"pair":[1,2]}}}]}}},""" +
        """"layers":[{"id":"dots","type":"circle","source":"points",""" +
        """"paint":{"circle-radius":10}}]}"""

    const val CLUSTER_STYLE_JSON = """{"version":8,"sources":{},"layers":[]}"""
  }
}
