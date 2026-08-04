package org.maplibre.nativeffi.map

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.browserTest
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.UnsupportedFeatureException
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.Feature
import org.maplibre.nativeffi.geo.FeatureIdentifier
import org.maplibre.nativeffi.geo.GeoJson
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.internal.wasm.Heap
import org.maplibre.nativeffi.internal.wasm.HeapArena
import org.maplibre.nativeffi.internal.wasm.JsonMarshal
import org.maplibre.nativeffi.json.JsonValue
import org.maplibre.nativeffi.maplibreScope
import org.maplibre.nativeffi.render.PremultipliedRgba8Image
import org.maplibre.nativeffi.style.CustomGeometrySourceCallback
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions
import org.maplibre.nativeffi.style.GeoJsonSourceOptions
import org.maplibre.nativeffi.style.LocationIndicatorImageKind
import org.maplibre.nativeffi.style.RasterDemEncoding
import org.maplibre.nativeffi.style.SourceType
import org.maplibre.nativeffi.style.StyleImageOptions
import org.maplibre.nativeffi.style.StyleLayerVisibility
import org.maplibre.nativeffi.style.StyleTransitionOptions
import org.maplibre.nativeffi.style.TileScheme
import org.maplibre.nativeffi.style.TileSourceOptions
import org.maplibre.nativeffi.style.VectorTileEncoding
import org.maplibre.nativeffi.withMap

/**
 * The style a map holds, written and read back through public values.
 *
 * Everything here crosses the boundary twice: a descriptor written into the module's heap at
 * generated offsets, and a result read back out of storage the runtime reuses. The assertions are
 * on the round trip rather than on the call succeeding, because a descriptor written at the wrong
 * offset is accepted and produces a different answer rather than an error.
 */
class StyleBrowserTest {
  // Spec coverage: BND-060, BND-061, BND-062, BND-063, BND-064, BND-066, BND-067, BND-069,
  // BND-101, BND-105.

  @Test
  fun sourcesAndLayersAreAddedQueriedAndRemovedThroughCopiedValues(): Promise<JsAny?> =
    browserTest {
      maplibreScope {
        withMap { _, map ->
          map.setStyleJson(EMPTY_STYLE_JSON)

          map.addStyleSourceJson(
            "parks",
            JsonValue.ObjectValue(
              listOf(
                JsonValue.Member("type", JsonValue.StringValue("geojson")),
                JsonValue.Member(
                  "data",
                  JsonValue.ObjectValue(
                    listOf(
                      JsonValue.Member("type", JsonValue.StringValue("FeatureCollection")),
                      JsonValue.Member("features", JsonValue.Array(emptyList())),
                    )
                  ),
                ),
              )
            ),
          )

          assertTrue(map.styleSourceExists("parks"))
          assertEquals(SourceType.GEOJSON, map.styleSourceType("parks"))
          assertEquals(SourceType.GEOJSON, map.styleSourceInfo("parks")?.type)
          val copiedSourceIds = map.styleSourceIds()
          assertTrue(copiedSourceIds.contains("parks"))

          map.addStyleLayerJson(
            JsonValue.ObjectValue(
              listOf(
                JsonValue.Member("id", JsonValue.StringValue("park-circles")),
                JsonValue.Member("type", JsonValue.StringValue("circle")),
                JsonValue.Member("source", JsonValue.StringValue("parks")),
              )
            ),
            "",
          )
          assertTrue(map.styleLayerExists("park-circles"))
          assertEquals("circle", map.styleLayerType("park-circles"))
          val copiedLayerIds = map.styleLayerIds()
          val copiedLayerJson = map.styleLayerJson("park-circles")
          assertTrue(copiedLayerJson is JsonValue.ObjectValue)

          map.moveStyleLayer("park-circles", "")
          map.setLayerProperty("park-circles", "circle-radius", JsonValue.DoubleValue(5.0))
          assertNotNull(map.layerProperty("park-circles", "circle-radius"))
          map.setLayerFilter(
            "park-circles",
            JsonValue.Array(listOf(JsonValue.StringValue("has"), JsonValue.StringValue("kind"))),
          )
          assertNotNull(map.layerFilter("park-circles"))
          map.clearLayerFilter("park-circles")

          assertTrue(map.removeStyleLayer("park-circles"))
          assertFalse(map.styleLayerExists("park-circles"))
          assertTrue(map.removeStyleSource("parks"))
          assertFalse(map.styleSourceExists("parks"))

          // The lists and the layer document were read out of runtime-owned storage that the
          // removals
          // above have since released, so a view rather than a copy would no longer read back.
          assertTrue(copiedSourceIds.contains("parks"))
          assertTrue(copiedLayerIds.contains("park-circles"))
          assertEquals(
            JsonValue.StringValue("park-circles"),
            (copiedLayerJson as JsonValue.ObjectValue).members.firstOrNull { it.key == "id" }?.value,
          )
        }
      }
    }

  @Test
  fun layerBaseAccessorsRoundTripAndRejectWhatTheStyleCannotHold(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap { _, map ->
        map.setStyleJson(FILL_STYLE_JSON)

        assertEquals("", map.layerSourceLayer("fill"))
        map.setLayerSourceLayer("fill", "roads")
        assertEquals("roads", map.layerSourceLayer("fill"))
        assertEquals("geo", map.layerSourceId("fill"))

        // A layer type that takes no source is rejected rather than silently ignored.
        assertFailsWith<InvalidArgumentException> { map.setLayerSourceLayer("bg", "roads") }
        assertEquals("", map.layerSourceId("bg"))

        // An unset zoom range crosses the boundary as infinities, which is a distinct value from
        // any zoom a caller could set.
        assertEquals(Double.NEGATIVE_INFINITY, map.layerMinZoom("fill"))
        assertEquals(Double.POSITIVE_INFINITY, map.layerMaxZoom("fill"))
        map.setLayerMinZoom("fill", 4.0)
        map.setLayerMaxZoom("fill", 12.5)
        assertEquals(4.0, map.layerMinZoom("fill"))
        assertEquals(12.5, map.layerMaxZoom("fill"))

        assertEquals(StyleLayerVisibility.VISIBLE, map.layerVisibility("fill"))
        map.setLayerVisibility("fill", StyleLayerVisibility.NONE)
        assertEquals(StyleLayerVisibility.NONE, map.layerVisibility("fill"))

        // An unknown raw enum keeps its value and is passed to C, which is what rejects it.
        assertEquals(900, StyleLayerVisibility(900).nativeValue)
        assertFailsWith<InvalidArgumentException> {
          map.setLayerVisibility("fill", StyleLayerVisibility(900))
        }
        assertFailsWith<InvalidArgumentException> { map.layerMinZoom("missing") }
      }
    }
  }

  @Test
  fun styleTransitionOptionsSeparateAnAbsentFieldFromAPresentZero(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap { _, map ->
        // A map with no style yet reports no duration or delay. The placement flag always reports,
        // because MapLibre Native always holds a value for it.
        val empty = map.styleTransitionOptions()
        assertNull(empty.durationMs)
        assertNull(empty.delayMs)
        assertEquals(true, empty.enablePlacementTransitions)

        // The style parser fills in its own 300ms for a style that declares no transition.
        map.setStyleJson(EMPTY_STYLE_JSON)
        assertEquals(300.0, map.styleTransitionOptions().durationMs)
        assertNull(map.styleTransitionOptions().delayMs)

        map.setStyleJson(TRANSITION_STYLE_JSON)
        val declared = map.styleTransitionOptions()
        assertEquals(750.0, declared.durationMs)
        assertEquals(100.0, declared.delayMs)

        // A present zero stays distinguishable from an absent field, and an absent field clears
        // what the style declared rather than merging into it.
        val options =
          StyleTransitionOptions().apply {
            durationMs = 0.0
            enablePlacementTransitions = false
          }
        map.setStyleTransitionOptions(options)
        assertEquals(options, map.styleTransitionOptions())

        // Omitting the flag leaves the cross-fade on rather than clearing it.
        map.setStyleTransitionOptions(StyleTransitionOptions().apply { durationMs = 250.0 })
        assertEquals(true, map.styleTransitionOptions().enablePlacementTransitions)

        // Loading a style replaces the override with what that style declares.
        map.setStyleJson(TRANSITION_STYLE_JSON)
        assertEquals(declared, map.styleTransitionOptions())

        assertFailsWith<InvalidArgumentException> {
          map.setStyleTransitionOptions(StyleTransitionOptions().apply { delayMs = -1.0 })
        }
      }
    }
  }

  /**
   * A structured value written into the module's heap and read back out of it.
   *
   * The descriptor rather than a style: MapLibre stores a parsed style in containers of its own,
   * which order members and discard repeats before the binding ever sees them again. What the C API
   * requires a binding to preserve is what the `mln_value` descriptor carries, so that is what is
   * written and read here — the same seam the other bindings use for this.
   */
  @Test
  fun aStructuredValueKeepsMemberOrderRepeatedNamesAndIntegerWidth(): Promise<JsAny?> =
    browserTest {
      val structured =
        JsonValue.ObjectValue(
          listOf(
            JsonValue.Member("zeta", JsonValue.StringValue("first")),
            JsonValue.Member("alpha", JsonValue.UInt(-1L)),
            JsonValue.Member("zeta", JsonValue.StringValue("second")),
            JsonValue.Member("signed", JsonValue.Int(-9_007_199_254_740_993L)),
            JsonValue.Member(
              "nested",
              JsonValue.Array(
                listOf(JsonValue.Bool(true), JsonValue.Null, JsonValue.DoubleValue(0.5))
              ),
            ),
          )
        )

      val size = JsonMarshal.measure(structured)
      val copied =
        Heap.withScratch(size) { block ->
          val arena = HeapArena(block, size)
          JsonMarshal.read(JsonMarshal.write(arena, structured))
        }

      val members = (copied as JsonValue.ObjectValue).members
      assertEquals(listOf("zeta", "alpha", "zeta", "signed", "nested"), members.map { it.key })
      assertEquals(JsonValue.StringValue("first"), members[0].value)
      assertEquals(JsonValue.StringValue("second"), members[2].value)
      // An unsigned value whose bit pattern is all ones and a signed value past the range a double
      // represents exactly both come back at their own width rather than through a double.
      assertEquals(JsonValue.UInt(-1L), members[1].value)
      assertEquals(JsonValue.Int(-9_007_199_254_740_993L), members[3].value)
      assertEquals(
        JsonValue.Array(listOf(JsonValue.Bool(true), JsonValue.Null, JsonValue.DoubleValue(0.5))),
        members[4].value,
      )
      assertEquals(structured, copied)
    }

  @Test
  fun aLayerPropertyExpressionRoundTripsThroughTheStyle(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap { _, map ->
        map.setStyleJson(EMPTY_STYLE_JSON)
        map.addStyleLayerJson(
          JsonValue.ObjectValue(
            listOf(
              JsonValue.Member("id", JsonValue.StringValue("bg")),
              JsonValue.Member("type", JsonValue.StringValue("background")),
            )
          ),
          "",
        )

        // A nested expression, so the value that comes back is a tree rather than a scalar.
        val expression =
          JsonValue.Array(
            listOf(
              JsonValue.StringValue("interpolate"),
              JsonValue.Array(listOf(JsonValue.StringValue("linear"))),
              JsonValue.Array(listOf(JsonValue.StringValue("zoom"))),
              JsonValue.DoubleValue(0.0),
              JsonValue.DoubleValue(0.25),
              JsonValue.DoubleValue(10.0),
              JsonValue.DoubleValue(0.75),
            )
          )
        map.setLayerProperty("bg", "background-opacity", expression)

        val readBack = assertNotNull(map.layerProperty("bg", "background-opacity"))
        val values = (readBack as JsonValue.Array).values
        assertEquals(JsonValue.StringValue("interpolate"), values[0])
        assertEquals(JsonValue.Array(listOf(JsonValue.StringValue("zoom"))), values[2])
        assertEquals(JsonValue.DoubleValue(0.75), values.last())

        // A scalar goes through the same path, so both shapes of the value union are covered.
        map.setLayerProperty("bg", "background-opacity", JsonValue.DoubleValue(0.5))
        assertEquals(
          JsonValue.DoubleValue(0.5),
          assertNotNull(map.layerProperty("bg", "background-opacity")),
        )
      }
    }
  }

  @Test
  fun styleImagesCopyPixelsAndMetadataInBothDirections(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap { _, map ->
        map.setStyleJson(EMPTY_STYLE_JSON)

        // Caller-owned storage: the array is mutated straight after the call, so the descriptor has
        // to have snapshotted it.
        val pixels = byteArrayOf(1, 2, 3, 4)
        val image = PremultipliedRgba8Image(1, 1, 4, pixels)
        pixels[0] = 9

        map.setStyleImage(
          "dot",
          image,
          StyleImageOptions().apply {
            pixelRatio = 2.0f
            sdf = true
          },
        )
        assertTrue(map.styleImageExists("dot"))
        assertEquals(2.0f, map.styleImageInfo("dot")?.pixelRatio)
        assertEquals(true, map.styleImageInfo("dot")?.sdf)
        // Read twice, because a readback that released the wrong handle would fail the second time.
        assertEquals(image, map.copyStyleImagePremultipliedRgba8("dot")?.image)
        assertEquals(image, map.copyStyleImagePremultipliedRgba8("dot")?.image)

        map.addLocationIndicatorLayer("location", "")
        assertEquals("location-indicator", map.styleLayerType("location"))
        map.setLocationIndicatorLocation("location", LatLng(0.0, 0.0), 0.0)
        map.setLocationIndicatorBearing("location", 45.0)
        map.setLocationIndicatorAccuracyRadius("location", 10.0)
        map.setLocationIndicatorImageName("location", LocationIndicatorImageKind.TOP, "dot")

        assertTrue(map.removeStyleImage("dot"))
        assertFalse(map.styleImageExists("dot"))
        assertNull(map.styleImageInfo("dot"))
      }
    }
  }

  @Test
  fun tileAndGeoJsonSourceOptionsReachNativeAsWrittenDescriptors(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap { _, map ->
        map.setStyleJson(EMPTY_STYLE_JSON)

        map.addVectorSourceTiles(
          "vector",
          listOf("https://example.com/vector/{z}/{x}/{y}.pbf"),
          TileSourceOptions().apply {
            minZoom = 0.0
            maxZoom = 14.0
            attribution = "vector attribution"
            scheme = TileScheme.XYZ
            tileSize = 512
            vectorEncoding = VectorTileEncoding.MVT
          },
        )
        assertEquals(SourceType.VECTOR, map.styleSourceType("vector"))
        assertEquals("vector attribution", map.styleSourceInfo("vector")?.attribution)

        map.addRasterSourceTiles(
          "raster",
          listOf("https://example.com/raster/{z}/{x}/{y}.png"),
          TileSourceOptions().apply {
            tileSize = 256
            scheme = TileScheme.TMS
          },
        )
        assertEquals(SourceType.RASTER, map.styleSourceType("raster"))

        map.addRasterDemSourceTiles(
          "dem",
          listOf("https://example.com/dem/{z}/{x}/{y}.png"),
          TileSourceOptions().apply {
            tileSize = 512
            rasterDemEncoding = RasterDemEncoding.TERRARIUM
          },
        )
        assertEquals(SourceType.RASTER_DEM, map.styleSourceType("dem"))
        map.addHillshadeLayer("hillshade", "dem", "")
        assertEquals("hillshade", map.styleLayerType("hillshade"))
        map.addColorReliefLayer("relief", "dem", "")
        assertEquals("color-relief", map.styleLayerType("relief"))

        // A nested descriptor tree: options carrying a structured value, over data carrying a
        // geometry, properties and an identifier.
        map.addGeoJsonSourceData(
          "points",
          GeoJson.FeatureCollection(
            listOf(
              Feature(
                Geometry.Point(LatLng(0.0, 0.0)),
                listOf(JsonValue.Member("weight", JsonValue.DoubleValue(2.0))),
                FeatureIdentifier.StringValue("first"),
              )
            )
          ),
          GeoJsonSourceOptions().apply {
            minZoom = 0.0
            maxZoom = 14.0
            tolerance = 0.5
            tileSize = 256
            buffer = 64
            lineMetrics = true
            cluster = true
            clusterRadius = 40
            clusterMaxZoom = 13.0
            clusterMinPoints = 2
            clusterProperties =
              JsonValue.ObjectValue(
                listOf(
                  JsonValue.Member(
                    "total",
                    JsonValue.Array(
                      listOf(
                        JsonValue.StringValue("+"),
                        JsonValue.Array(
                          listOf(JsonValue.StringValue("get"), JsonValue.StringValue("weight"))
                        ),
                      )
                    ),
                  )
                )
              )
          },
        )
        assertEquals(SourceType.GEOJSON, map.styleSourceType("points"))

        // A clustered source indexes every feature as a point, so native refuses replacement data
        // that is not a feature collection. The refusal is native's, not the binding's.
        assertFailsWith<InvalidArgumentException> {
          map.setGeoJsonSourceData(
            "points",
            GeoJson.GeometryValue(Geometry.Point(LatLng(1.0, 1.0))),
          )
        }

        // An option value native rejects is not swallowed by the binding either.
        assertFailsWith<InvalidArgumentException> {
          map.addGeoJsonSourceUrl(
            "invalid-zooms",
            "https://example.com/places.geojson",
            GeoJsonSourceOptions().apply {
              minZoom = 12.0
              maxZoom = 4.0
            },
          )
        }
        assertFalse(map.styleSourceExists("invalid-zooms"))
      }
    }
  }

  @Test
  fun imageSourcesCopyCoordinatesAndPixels(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap { _, map ->
        map.setStyleJson(EMPTY_STYLE_JSON)
        val coordinates =
          listOf(LatLng(1.0, 1.0), LatLng(1.0, 2.0), LatLng(0.0, 2.0), LatLng(0.0, 1.0))

        map.addImageSourceImage(
          "overlay",
          coordinates,
          PremultipliedRgba8Image(1, 1, 4, byteArrayOf(4, 3, 2, 1)),
        )
        assertEquals(SourceType.IMAGE, map.styleSourceType("overlay"))
        assertEquals(coordinates, map.imageSourceCoordinates("overlay"))

        val moved = coordinates.reversed()
        map.setImageSourceCoordinates("overlay", moved)
        assertEquals(moved, map.imageSourceCoordinates("overlay"))
        map.setImageSourceImage(
          "overlay",
          PremultipliedRgba8Image(1, 1, 4, byteArrayOf(1, 1, 1, 1)),
        )

        // The list handed back is the caller's own, so mutating it cannot reach the source.
        assertEquals(moved, map.imageSourceCoordinates("overlay"))
      }
    }
  }

  /**
   * The one style workflow a browser cannot carry.
   *
   * MapLibre invokes a custom geometry source's tile callbacks synchronously on worker threads, and
   * a worker cannot enter the page's WebAssembly instance. Unlike the resource provider, whose
   * request the module's thunk can forward to the page and wait for, this callback runs while the
   * map is blocked on it, so the page turn that would answer never comes. It is refused rather than
   * left half-working, and asserted here so it is not quietly "fixed" into something that
   * deadlocks.
   */
  @Test
  fun aCustomGeometrySourceIsRefusedWithTheReasonItCannotWork(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap { _, map ->
        map.setStyleJson(EMPTY_STYLE_JSON)

        val callback =
          object : CustomGeometrySourceCallback {
            override fun fetchTile(tileId: CanonicalTileId) = Unit
          }
        assertFailsWith<UnsupportedFeatureException> {
          map.addCustomGeometrySource("custom", CustomGeometrySourceOptions(callback))
        }
        assertFalse(map.styleSourceExists("custom"))

        // The rest of the family is not refused by the binding: with no source to name, native is
        // what rejects them, and it does so as an invalid argument rather than as unsupported.
        assertFailsWith<InvalidArgumentException> {
          map.setCustomGeometrySourceTileData(
            "custom",
            CanonicalTileId(0, 0, 0),
            GeoJson.FeatureCollection(emptyList()),
          )
        }
      }
    }
  }

  private companion object {
    const val FILL_STYLE_JSON =
      """{"version":8,"sources":{"geo":{"type":"geojson","data":""" +
        """{"type":"FeatureCollection","features":[]}}},"layers":[""" +
        """{"id":"bg","type":"background"},{"id":"fill","type":"fill","source":"geo"}]}"""

    const val TRANSITION_STYLE_JSON =
      """{"version":8,"transition":{"duration":750,"delay":100},"sources":{},"layers":[]}"""
  }
}
