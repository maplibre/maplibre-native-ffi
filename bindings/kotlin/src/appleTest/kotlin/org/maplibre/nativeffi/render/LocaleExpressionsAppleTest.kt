package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import kotlin.time.Duration.Companion.seconds
import kotlin.time.TimeSource
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenBox
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions

class LocaleExpressionsAppleTest {
  @Test
  fun formattingRetainsDefaultsAndHonorsExplicitFractionLimits() {
    val cases =
      listOf(
        "" to "12.3",
        "\"currency\":\"USD\"" to "$12.30",
        "\"currency\":\"USD\",\"min-fraction-digits\":0,\"max-fraction-digits\":3" to "$12.3",
        "\"currency\":\"USD\",\"min-fraction-digits\":3,\"max-fraction-digits\":3" to "$12.300",
        "\"currency\":\"USD\",\"min-fraction-digits\":4" to "$12.3000",
        "\"currency\":\"USD\",\"max-fraction-digits\":0" to "$12",
        "\"min-fraction-digits\":4" to "12.3000",
        "\"max-fraction-digits\":0" to "12",
        "\"min-fraction-digits\":2,\"max-fraction-digits\":4" to "12.30",
      )
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      MapHandle.create(runtime, MapOptions()).use { map ->
        map.setStyleJson(
          jsonBytes(
            """{"version":8,"sources":{},"layers":[{
          "id":"background","type":"background"}]}"""
          )
        )
        for ((options, expected) in cases) {
          map.setLayerProperty(
            "background",
            "background-opacity",
            jsonBytes(
              """["case",["==",["number-format",12.3,{"locale":"en-US"${if (options.isEmpty()) "" else ",$options"}}],"$expected"],0.25,0.75]"""
            ),
          )
          assertEquals(
            "0.25",
            map.layerProperty("background", "background-opacity")?.decodeToString(),
            options,
          )
        }
      }
    }
  }

  @Test
  fun collationRejectsMalformedUTF8AndPreservesEmbeddedNulls() {
    // Feature queries use the existing Metal fixture; other Apple backends have no fixture here.
    if (RenderBackend.METAL !in Maplibre.supportedRenderBackends()) return
    val collator =
      """["collator",{"locale":"en-US","case-sensitive":false,"diacritic-sensitive":false}]"""
    assertMatches(
      """["==",["get","name"],"cafe",$collator]""",
      listOf("cafe", "café", "__INVALID_UTF8__"),
      setOf("case-0", "case-1"),
    )
    assertMatches(
      """["==",["get","name"],"__INVALID_UTF8__",$collator]""",
      listOf("cafe"),
      emptySet(),
    )
    assertMatches(
      """["==",["get","name"],"a\u0000b",$collator]""",
      listOf("""a\u0000b""", """a\u0000c"""),
      setOf("case-0"),
    )
  }

  private fun assertMatches(filter: String, names: List<String>, expected: Set<String>) {
    withOwnedTextureSession(width = 64, height = 64) { runtime, map, owned ->
      val session = owned.session
      val features =
        names
          .mapIndexed { index, name ->
            """{"type":"Feature","id":"case-$index","geometry":{"type":"Point","coordinates":[0,0]},"properties":{"name":"$name"}}"""
          }
          .joinToString(",")
      map.jumpTo(
        CameraOptions().apply {
          center = LatLng(0.0, 0.0)
          zoom = 2.0
        }
      )
      map.setStyleJson(
        rawJSON(
          """{"version":8,"sources":{"point":{"type":"geojson","data":{
        "type":"FeatureCollection","features":[$features]}}},"layers":[{
        "id":"cases","type":"circle","source":"point","filter":$filter,"paint":{"circle-radius":4}}]}"""
        )
      )
      val started = TimeSource.Monotonic.markNow()
      while (!map.isFullyLoaded && started.elapsedNow() < 10.seconds) {
        runtime.pump(1000)
        val events = runtime.drainEvents().events
        events
          .firstOrNull { it.type == RuntimeEventType.MAP_LOADING_FAILED }
          ?.let { error(it.message) }
        if (events.any { it.type == RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE })
          session.renderUpdate()
      }
      assertTrue(map.isFullyLoaded, "locale fixture did not finish loading")
      val source =
        session.querySourceFeatures(
          "point",
          SourceFeatureQueryOptions().apply { this.filter = rawJSON(filter) },
        )
      assertEquals(
        expected,
        source.map { stringMember(it.feature, "id") }.toSet(),
        "source query: $filter",
      )
      val rendered =
        session.queryRenderedFeatures(
          RenderedQueryGeometry.Box(ScreenBox(ScreenPoint(0.0, 0.0), ScreenPoint(64.0, 64.0))),
          RenderedFeatureQueryOptions().apply { layerIds = listOf("cases") },
        )
      assertEquals(
        expected,
        rendered.map { stringMember(it.feature, "id") }.toSet(),
        "worker filter: $filter",
      )
    }
  }

  private fun rawJSON(value: String): ByteArray {
    val parts = value.trimIndent().split("__INVALID_UTF8__")
    return if (parts.size == 1) parts.single().encodeToByteArray()
    else
      parts.first().encodeToByteArray() +
        byteArrayOf(0xFF.toByte()) +
        parts.last().encodeToByteArray()
  }
}
