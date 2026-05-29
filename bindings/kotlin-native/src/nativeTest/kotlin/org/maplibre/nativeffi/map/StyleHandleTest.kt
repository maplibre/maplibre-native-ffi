package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import kotlinx.cinterop.BooleanVar
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.value
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.Feature
import org.maplibre.nativeffi.geo.GeoJson
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.internal.c.mln_map_remove_style_source
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.struct.CoreStructs
import org.maplibre.nativeffi.json.JsonValue
import org.maplibre.nativeffi.render.PremultipliedRgba8Image
import org.maplibre.nativeffi.runtime.RuntimeEventSourceType
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.style.CustomGeometrySourceCallback
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions
import org.maplibre.nativeffi.style.LocationIndicatorImageKind
import org.maplibre.nativeffi.style.RasterDemEncoding
import org.maplibre.nativeffi.style.SourceType
import org.maplibre.nativeffi.style.StyleImageOptions
import org.maplibre.nativeffi.style.TileScheme
import org.maplibre.nativeffi.style.TileSourceOptions
import org.maplibre.nativeffi.style.VectorTileEncoding

@OptIn(ExperimentalForeignApi::class)
class StyleHandleTest {
  @Test
  fun customGeometrySourceApisKeepCallbackStateMapScoped() {
    val runtime = RuntimeHandle.create()
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
        GeoJson.featureCollection(emptyList()),
      )
      map.invalidateCustomGeometrySourceTile("custom", CanonicalTileId(0, 0, 0))
      map.invalidateCustomGeometrySourceRegion(
        "custom",
        LatLngBounds(LatLng(0.0, 0.0), LatLng(1.0, 1.0)),
      )
      assertTrue(map.removeStyleSource("custom"))
      assertEquals(0, map.customGeometrySourceCountForTesting())
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun mapStyleLoadedEventDropsCallbacksAfterNativeSourceDetaches() {
    val runtime = RuntimeHandle.create()
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
            map.nativeHandle(),
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
          map.nativeAddress(),
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
    val runtime = RuntimeHandle.create()
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
            map.nativeHandle(),
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
  fun styleSourceAndLayerJsonApisCallNativeAndCopyDescriptors() {
    val runtime = RuntimeHandle.create()
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
        JsonValue.`object`(
          listOf(
            JsonValue.Member("type", JsonValue.of("geojson")),
            JsonValue.Member(
              "data",
              JsonValue.`object`(
                listOf(
                  JsonValue.Member("type", JsonValue.of("FeatureCollection")),
                  JsonValue.Member("features", JsonValue.array(emptyList())),
                )
              ),
            ),
          )
        ),
      )
      assertTrue(map.styleSourceExists("parks"))
      assertEquals(SourceType.GEOJSON, map.styleSourceType("parks"))
      assertEquals(SourceType.GEOJSON, map.styleSourceInfo("parks")?.type)
      assertTrue(map.styleSourceIds().contains("parks"))

      map.addStyleLayerJson(
        JsonValue.`object`(
          listOf(
            JsonValue.Member("id", JsonValue.of("park-circles")),
            JsonValue.Member("type", JsonValue.of("circle")),
            JsonValue.Member("source", JsonValue.of("parks")),
          )
        )
      )
      assertTrue(map.styleLayerExists("park-circles"))
      assertEquals("circle", map.styleLayerType("park-circles"))
      assertTrue(map.styleLayerIds().contains("park-circles"))
      assertTrue(map.styleLayerJson("park-circles") is JsonValue.ObjectValue)
      map.moveStyleLayer("park-circles")
      map.setLayerProperty("park-circles", "circle-radius", JsonValue.of(5.0))
      assertTrue(map.layerProperty("park-circles", "circle-radius") != null)
      map.setLayerFilter(
        "park-circles",
        JsonValue.array(listOf(JsonValue.of("has"), JsonValue.of("kind"))),
      )
      assertTrue(map.layerFilter("park-circles") != null)
      map.clearLayerFilter("park-circles")
      assertTrue(map.removeStyleLayer("park-circles"))
      assertFalse(map.styleLayerExists("park-circles"))
      assertTrue(map.removeStyleSource("parks"))
      assertFalse(map.styleSourceExists("parks"))
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun styleImageApisCopyPixelsAndMetadata() {
    val runtime = RuntimeHandle.create()
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
      map.addLocationIndicatorLayer("location")
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
    val runtime = RuntimeHandle.create()
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
    val runtime = RuntimeHandle.create()
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
      map.addHillshadeLayer("hillshade", "dem")
      assertEquals("hillshade", map.styleLayerType("hillshade"))
      map.addColorReliefLayer("relief", "dem")
      assertEquals("color-relief", map.styleLayerType("relief"))
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun geoJsonSourceApisMaterializeGeoJsonDescriptors() {
    val runtime = RuntimeHandle.create()
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
        GeoJson.featureCollection(
          listOf(
            Feature(
              Geometry.point(LatLng(0.0, 0.0)),
              listOf(JsonValue.Member("kind", JsonValue.of("point"))),
            )
          )
        ),
      )
      assertEquals(SourceType.GEOJSON, map.styleSourceType("points"))
      map.setGeoJsonSourceData("points", GeoJson.geometry(Geometry.point(LatLng(1.0, 1.0))))
      assertTrue(map.removeStyleSource("points"))
    } finally {
      map.close()
      runtime.close()
    }
  }
}
