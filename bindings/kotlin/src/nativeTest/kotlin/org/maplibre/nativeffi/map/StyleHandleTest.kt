package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import kotlinx.cinterop.BooleanVar
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.value
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.Feature
import org.maplibre.nativeffi.geo.FeatureIdentifier
import org.maplibre.nativeffi.geo.GeoJson
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.internal.c.mln_map_remove_style_source
import org.maplibre.nativeffi.internal.lifecycle.rawHandleValue
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.struct.CoreStructs
import org.maplibre.nativeffi.json.JsonValue
import org.maplibre.nativeffi.render.PremultipliedRgba8Image
import org.maplibre.nativeffi.runtime.RuntimeEventSourceType
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.style.CustomGeometrySourceCallback
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions
import org.maplibre.nativeffi.style.GeoJsonSourceOptions
import org.maplibre.nativeffi.style.LocationIndicatorImageKind
import org.maplibre.nativeffi.style.RasterDemEncoding
import org.maplibre.nativeffi.style.SourceType
import org.maplibre.nativeffi.style.StyleImageOptions
import org.maplibre.nativeffi.style.StyleLayerVisibility
import org.maplibre.nativeffi.style.TileScheme
import org.maplibre.nativeffi.style.TileSourceOptions
import org.maplibre.nativeffi.style.VectorTileEncoding

@OptIn(ExperimentalForeignApi::class)
class StyleHandleTest : org.maplibre.nativeffi.NativeTestBase() {
  // BND-062: unknown output discriminators keep raw native values.

  @Test
  fun sourceTypePreservesUnknownRawNativeValues() {
    val value = SourceType(999)

    assertEquals(999, value.nativeValue)
  }

  // BND-124: style-scoped callback state follows native source lifetime and reload events.

  @Test
  fun customGeometrySourceApisKeepCallbackStateMapScoped() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 128
          height = 128
        },
      )
    try {
      map.setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}")
      map.addCustomGeometrySource(
        "custom",
        CustomGeometrySourceOptions(
            object : CustomGeometrySourceCallback {
              override fun fetchTile(tileId: CanonicalTileId) = Unit
            }
          )
          .apply {
            minZoom = 0.0
            maxZoom = 4.0
            tileSize = 256
            buffer = 8
            clip = true
            wrap = false
          },
      )
      assertEquals(1, map.customGeometrySourceCountForTesting())
      assertEquals(SourceType.CUSTOM_VECTOR, map.styleSourceType("custom"))
      map.setCustomGeometrySourceTileData(
        "custom",
        CanonicalTileId(0, 0, 0),
        GeoJson.FeatureCollection(emptyList()),
      )
      map.invalidateCustomGeometrySourceTile("custom", CanonicalTileId(0, 0, 0))
      map.invalidateCustomGeometrySourceRegion(
        "custom",
        LatLngBounds(LatLng(0.0, 0.0), LatLng(1.0, 1.0)),
      )
      assertTrue(map.removeStyleSource("custom"))
      assertEquals(0, map.customGeometrySourceCountForTesting())
      map.addCustomGeometrySource(
        "custom",
        CustomGeometrySourceOptions(
          object : CustomGeometrySourceCallback {
            override fun fetchTile(tileId: CanonicalTileId) = Unit
          }
        ),
      )
      assertEquals(1, map.customGeometrySourceCountForTesting())
      assertEquals(SourceType.CUSTOM_VECTOR, map.styleSourceType("custom"))
    } finally {
      map.close()
      assertEquals(0, map.customGeometrySourceCountForTesting())
      runtime.close()
    }
  }

  @Test
  fun failedCustomGeometrySourceReplacementKeepsExistingCallbackState() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 128
          height = 128
        },
      )
    try {
      map.setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}")
      map.addCustomGeometrySource(
        "custom",
        CustomGeometrySourceOptions(
          object : CustomGeometrySourceCallback {
            override fun fetchTile(tileId: CanonicalTileId) = Unit
          }
        ),
      )

      assertFailsWith<InvalidArgumentException> {
        map.addCustomGeometrySource(
          "custom",
          CustomGeometrySourceOptions(
            object : CustomGeometrySourceCallback {
              override fun fetchTile(tileId: CanonicalTileId) = Unit
            }
          ),
        )
      }

      assertEquals(1, map.customGeometrySourceCountForTesting())
      assertEquals(SourceType.CUSTOM_VECTOR, map.styleSourceType("custom"))
      assertTrue(map.removeStyleSource("custom"))
      assertEquals(0, map.customGeometrySourceCountForTesting())
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun mapStyleLoadedEventDropsCallbacksAfterNativeSourceDetaches() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 128
          height = 128
        },
      )
    try {
      map.setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}")
      map.addCustomGeometrySource(
        "custom",
        CustomGeometrySourceOptions(
          object : CustomGeometrySourceCallback {
            override fun fetchTile(tileId: CanonicalTileId) = Unit
          }
        ),
      )
      assertEquals(1, map.customGeometrySourceCountForTesting())

      memScoped {
        val outRemoved = alloc<BooleanVar>()
        Status.check(
          mln_map_remove_style_source(
            map.nativeHandle().rawHandleValue,
            CoreStructs.stringView("custom", this),
            outRemoved.ptr,
          )
        )
        assertTrue(outRemoved.value)
      }

      val mapSource =
        runtime.applyEventSideEffectsForTesting(
          RuntimeEventType.MAP_STYLE_LOADED,
          RuntimeEventSourceType.MAP,
          map.nativeHandleId(),
        )

      assertEquals(map, mapSource)
      assertEquals(0, map.customGeometrySourceCountForTesting())
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun releaseDetachedCustomGeometrySourcesDropsCallbacksAfterNativeSourceDetaches() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 128
          height = 128
        },
      )
    try {
      map.setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}")
      map.addCustomGeometrySource(
        "custom",
        CustomGeometrySourceOptions(
          object : CustomGeometrySourceCallback {
            override fun fetchTile(tileId: CanonicalTileId) = Unit
          }
        ),
      )
      assertEquals(1, map.customGeometrySourceCountForTesting())

      memScoped {
        val outRemoved = alloc<BooleanVar>()
        Status.check(
          mln_map_remove_style_source(
            map.nativeHandle().rawHandleValue,
            CoreStructs.stringView("custom", this),
            outRemoved.ptr,
          )
        )
        assertTrue(outRemoved.value)
      }
      map.releaseDetachedCustomGeometrySources()

      assertEquals(0, map.customGeometrySourceCountForTesting())
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun styleReloadAndStaleEventsDoNotDropReusedCustomGeometrySourceIds() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 128
          height = 128
        },
      )
    try {
      map.setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}")
      map.addCustomGeometrySource(
        "custom",
        CustomGeometrySourceOptions(
          object : CustomGeometrySourceCallback {
            override fun fetchTile(tileId: CanonicalTileId) = Unit
          }
        ),
      )
      assertEquals(1, map.customGeometrySourceCountForTesting())

      map.setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}")
      assertEquals(0, map.customGeometrySourceCountForTesting())

      map.addCustomGeometrySource(
        "custom",
        CustomGeometrySourceOptions(
          object : CustomGeometrySourceCallback {
            override fun fetchTile(tileId: CanonicalTileId) = Unit
          }
        ),
      )
      assertEquals(1, map.customGeometrySourceCountForTesting())

      val mapSource =
        runtime.applyEventSideEffectsForTesting(
          RuntimeEventType.MAP_STYLE_LOADED,
          RuntimeEventSourceType.MAP,
          map.nativeHandleId(),
        )

      assertEquals(map, mapSource)
      assertEquals(1, map.customGeometrySourceCountForTesting())
    } finally {
      map.close()
      assertEquals(0, map.customGeometrySourceCountForTesting())
      runtime.close()
    }
  }

  // BND-105: typed layer base accessors round-trip through native style state.

  @Test
  fun layerBaseAccessorsRoundTripThroughNativeStyle() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 128
          height = 128
        },
      )
    try {
      map.setStyleJson(
        "{\"version\":8,\"sources\":{\"geo\":{\"type\":\"geojson\",\"data\":" +
          "{\"type\":\"FeatureCollection\",\"features\":[]}}},\"layers\":[" +
          "{\"id\":\"bg\",\"type\":\"background\"}," +
          "{\"id\":\"fill\",\"type\":\"fill\",\"source\":\"geo\"}]}"
      )

      assertEquals("", map.layerSourceLayer("fill"))
      map.setLayerSourceLayer("fill", "roads")
      assertEquals("roads", map.layerSourceLayer("fill"))
      assertEquals("geo", map.layerSourceId("fill"))

      // A layer type that takes no source is rejected rather than silently ignored.
      assertFailsWith<InvalidArgumentException> { map.setLayerSourceLayer("bg", "roads") }
      assertEquals("", map.layerSourceId("bg"))

      // An unset zoom range crosses the boundary as infinities.
      assertEquals(Double.NEGATIVE_INFINITY, map.layerMinZoom("fill"))
      assertEquals(Double.POSITIVE_INFINITY, map.layerMaxZoom("fill"))
      map.setLayerMinZoom("fill", 4.0)
      map.setLayerMaxZoom("fill", 12.5)
      assertEquals(4.0, map.layerMinZoom("fill"))
      assertEquals(12.5, map.layerMaxZoom("fill"))

      assertEquals(StyleLayerVisibility.VISIBLE, map.layerVisibility("fill"))
      map.setLayerVisibility("fill", StyleLayerVisibility.NONE)
      assertEquals(StyleLayerVisibility.NONE, map.layerVisibility("fill"))

      // An unknown raw visibility passes through to C, which rejects it.
      assertFailsWith<InvalidArgumentException> {
        map.setLayerVisibility("fill", StyleLayerVisibility(900))
      }
      assertFailsWith<InvalidArgumentException> { map.layerMinZoom("missing") }
    } finally {
      map.close()
      runtime.close()
    }
  }

  // BND-101, BND-105, BND-069: style loading and workflows use copied public values.

  @Test
  fun styleSourceAndLayerJsonApisCallNativeAndCopyDescriptors() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 128
          height = 128
        },
      )
    try {
      map.setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}")
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
      assertTrue(copiedLayerIds.contains("park-circles"))
      val copiedLayerJson = map.styleLayerJson("park-circles")
      assertTrue(copiedLayerJson is JsonValue.ObjectValue)
      map.moveStyleLayer("park-circles", "")
      map.setLayerProperty("park-circles", "circle-radius", JsonValue.DoubleValue(5.0))
      assertTrue(map.layerProperty("park-circles", "circle-radius") != null)
      map.setLayerFilter(
        "park-circles",
        JsonValue.Array(listOf(JsonValue.StringValue("has"), JsonValue.StringValue("kind"))),
      )
      assertTrue(map.layerFilter("park-circles") != null)
      map.clearLayerFilter("park-circles")
      assertTrue(map.removeStyleLayer("park-circles"))
      assertFalse(map.styleLayerExists("park-circles"))
      assertTrue(map.removeStyleSource("parks"))
      assertFalse(map.styleSourceExists("parks"))
      assertTrue(copiedSourceIds.contains("parks"))
      assertTrue(copiedLayerIds.contains("park-circles"))
      assertEquals(
        JsonValue.StringValue("park-circles"),
        copiedLayerJson.members.firstOrNull { it.key == "id" }?.value,
      )
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun styleImageApisCopyPixelsAndMetadata() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 128
          height = 128
        },
      )
    try {
      map.setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}")
      val image = PremultipliedRgba8Image(1, 1, 4, byteArrayOf(1, 2, 3, 4))
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
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun imageSourceApisCopyCoordinatesAndPixels() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 128
          height = 128
        },
      )
    val coordinates = listOf(LatLng(1.0, 1.0), LatLng(1.0, 2.0), LatLng(0.0, 2.0), LatLng(0.0, 1.0))
    try {
      map.setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}")
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
      map.setImageSourceImage("overlay", PremultipliedRgba8Image(1, 1, 4, byteArrayOf(1, 1, 1, 1)))
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun tileSourceApisMaterializeOptionsAndTileUrlLists() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 128
          height = 128
        },
      )
    try {
      map.setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}")
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
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun geoJsonSourceApisMaterializeGeoJsonDescriptors() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 128
          height = 128
        },
      )
    try {
      map.setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}")
      map.addGeoJsonSourceData(
        "points",
        GeoJson.FeatureCollection(
          listOf(
            Feature(
              Geometry.Point(LatLng(0.0, 0.0)),
              listOf(JsonValue.Member("kind", JsonValue.StringValue("point"))),
              FeatureIdentifier.Null,
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

      // A clustered source indexes every feature as a point, so native rejects
      // replacement data that is not a feature collection.
      assertFailsWith<InvalidArgumentException> {
        map.setGeoJsonSourceData("points", GeoJson.GeometryValue(Geometry.Point(LatLng(1.0, 1.0))))
      }
      map.setGeoJsonSourceData(
        "points",
        GeoJson.FeatureCollection(
          listOf(Feature(Geometry.Point(LatLng(1.0, 1.0)), emptyList(), FeatureIdentifier.Null))
        ),
      )

      // An unclustered source still materializes a bare geometry descriptor.
      map.addGeoJsonSourceData(
        "bare",
        GeoJson.GeometryValue(Geometry.Point(LatLng(2.0, 2.0))),
        null,
      )
      map.setGeoJsonSourceData("bare", GeoJson.GeometryValue(Geometry.Point(LatLng(3.0, 3.0))))
      assertTrue(map.removeStyleSource("bare"))

      // Option values reach native validation rather than being dropped by the binding.
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

      assertTrue(map.removeStyleSource("points"))
    } finally {
      map.close()
      runtime.close()
    }
  }
}
