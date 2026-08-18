package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertIs
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertSame
import kotlin.test.assertTrue
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.BoundsConstraint
import org.maplibre.nativeffi.camera.CameraFitOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.camera.UnitBezier
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.geo.Quaternion
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.geo.Vec3
import org.maplibre.nativeffi.render.PremultipliedRgba8Image
import org.maplibre.nativeffi.runtime.CameraChangeMode
import org.maplibre.nativeffi.runtime.RuntimeEventPayload
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions
import org.maplibre.nativeffi.style.CustomGeometrySourceCallback
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions
import org.maplibre.nativeffi.style.GeoJsonSourceDataHandle
import org.maplibre.nativeffi.style.GeoJsonSourceOptions
import org.maplibre.nativeffi.style.RasterDemEncoding
import org.maplibre.nativeffi.style.SourceInfo
import org.maplibre.nativeffi.style.SourceType
import org.maplibre.nativeffi.style.StyleImageOptions
import org.maplibre.nativeffi.style.StyleLayerVisibility
import org.maplibre.nativeffi.style.StyleTransitionOptions
import org.maplibre.nativeffi.style.TileScheme
import org.maplibre.nativeffi.style.TileSourceOptions
import org.maplibre.nativeffi.style.VectorTileEncoding

class MapHandleTest {

  @Test
  fun layerBaseAccessorsReachNativeThroughDowncalls() {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 64
            height = 64
          },
        )
        .use { map ->
          map.setStyleJson(
            ("{\"version\":8,\"sources\":{\"geo\":{\"type\":\"geojson\",\"data\":" +
                "{\"type\":\"FeatureCollection\",\"features\":[]}}},\"layers\":[" +
                "{\"id\":\"bg\",\"type\":\"background\"}," +
                "{\"id\":\"fill\",\"type\":\"fill\",\"source\":\"geo\"}]}")
              .encodeToByteArray()
          )

          assertEquals("", map.layerSourceLayer("fill"))
          map.setLayerSourceLayer("fill", "roads")
          assertEquals("roads", map.layerSourceLayer("fill"))
          assertEquals("geo", map.layerSourceId("fill"))

          // A background layer takes no source.
          assertFailsWith<InvalidArgumentException> { map.setLayerSourceLayer("bg", "roads") }

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

          assertFailsWith<InvalidArgumentException> {
            map.setLayerVisibility("fill", StyleLayerVisibility(900))
          }
        }
    }
  }

  @Test
  fun styleTransitionOptionsRoundTripThroughDowncalls() {
    val transitionStyleJson =
      "{\"version\":8,\"transition\":{\"duration\":750,\"delay\":100}," +
        "\"sources\":{},\"layers\":[]}"
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 64
            height = 64
          },
        )
        .use { map ->
          // Duration and delay are absent until a style loads; the placement flag always
          // holds a value.
          val empty = map.styleTransitionOptions()
          assertNull(empty.durationMs)
          assertNull(empty.delayMs)
          assertEquals(true, empty.enablePlacementTransitions)

          // The style parser supplies a 300ms default duration.
          map.setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}".encodeToByteArray())
          val parsed = map.styleTransitionOptions()
          assertEquals(300.0, parsed.durationMs)
          assertNull(parsed.delayMs)

          map.setStyleJson(transitionStyleJson.encodeToByteArray())
          val declared = map.styleTransitionOptions()
          assertEquals(750.0, declared.durationMs)
          assertEquals(100.0, declared.delayMs)
          assertEquals(true, declared.enablePlacementTransitions)

          // A present zero stays distinguishable from an absent field, and an absent field
          // clears what the style declared.
          val options =
            StyleTransitionOptions().apply {
              durationMs = 0.0
              enablePlacementTransitions = false
            }
          map.setStyleTransitionOptions(options)
          assertEquals(options, map.styleTransitionOptions())

          // Omitting the flag leaves the cross-fade on.
          map.setStyleTransitionOptions(StyleTransitionOptions().apply { durationMs = 250.0 })
          assertEquals(true, map.styleTransitionOptions().enablePlacementTransitions)

          // Loading a style replaces the override with what that style declares.
          map.setStyleJson(transitionStyleJson.encodeToByteArray())
          assertEquals(declared, map.styleTransitionOptions())

          assertFailsWith<InvalidArgumentException> {
            map.setStyleTransitionOptions(StyleTransitionOptions().apply { delayMs = -1.0 })
          }
        }
    }
  }

  @Test
  fun canonicalTileIdRejectsOutOfRangeInputs() {
    assertFailsWith<InvalidArgumentException> { CanonicalTileId(0, UInt.MAX_VALUE.toLong() + 1, 0) }
  }

  @Test
  fun mapCreateStyleAndCloseRetainsRuntime() {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 64
          height = 64
          scaleFactor = 1.0
          mapMode = MapMode.STATIC
        },
      )

    assertFalse(map.isClosed)
    assertSame(runtime, map.runtime())
    assertFailsWith<InvalidStateException> { runtime.close() }

    map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray())
    map.setStyleUrl("https://example.com/style.json")
    map.close()
    map.close()

    assertTrue(map.isClosed)
    assertFailsWith<InvalidStateException> {
      map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray())
    }
    runtime.close()
    assertTrue(runtime.isClosed)
  }

  @Test
  fun mapSizeReportsCreationExtentAndPixelRatio() {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 512
          height = 256
          scaleFactor = 2.0
        },
      )

    val size = map.size
    assertEquals(512, size.width)
    assertEquals(256, size.height)
    assertEquals(2.0, size.scaleFactor)
    assertEquals(MapSize(512, 256, 2.0), size)
    assertEquals(MapSize(512, 256, 2.0).hashCode(), size.hashCode())

    map.close()
    runtime.close()
  }

  @Test
  fun styleSourceJsonCanBeAddedInspectedListedAndRemoved() {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 64
          height = 64
          mapMode = MapMode.STATIC
        },
      )

    try {
      map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray())
      map.addStyleSourceJson("places", geoJsonSource())

      assertTrue(map.styleSourceExists("places"))
      assertEquals(SourceType.GEOJSON, map.styleSourceType("places"))
      assertEquals(SourceType.GEOJSON, map.styleSourceInfo("places")?.type)
      assertTrue(map.styleSourceIds().contains("places"))
      assertTrue(map.removeStyleSource("places"))
      assertFalse(map.styleSourceExists("places"))
      assertFalse(map.removeStyleSource("places"))
    } finally {
      map.close()
      runtime.close()
    }
  }

  // BND-109.

  @Test
  fun styleSourceInfoCopiesUrlAndInlineTileMetadataPastNativeLifetime() {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 64
          height = 64
          mapMode = MapMode.STATIC
        },
      )
    lateinit var retainedInfo: SourceInfo
    val tileUrls =
      listOf("https://a.example.com/{z}/{x}/{y}.pbf", "https://b.example.com/{z}/{x}/{y}.pbf")
    val bounds = LatLngBounds(LatLng(-5.0, -10.0), LatLng(15.0, 20.0))
    try {
      map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray())
      map.addVectorSourceUrl("remote", "https://example.com/vector.json", null)
      val remote = assertNotNull(map.styleSourceInfo("remote"))
      assertEquals("https://example.com/vector.json", remote.url)
      assertNull(remote.tileJson)

      map.addVectorSourceTiles(
        "inline",
        tileUrls,
        TileSourceOptions().apply {
          minZoom = 0.0
          maxZoom = 12.0
          attribution = "inline attribution"
          scheme = TileScheme.TMS
          this.bounds = bounds
          tileSize = 256
          vectorEncoding = VectorTileEncoding.MLT
        },
      )

      retainedInfo = assertNotNull(map.styleSourceInfo("inline"))
      assertNull(retainedInfo.url)
      assertEquals("inline attribution", retainedInfo.attribution)
      assertEquals(tileUrls, retainedInfo.tileJson?.tileUrls)
      assertEquals(0.0, retainedInfo.tileJson?.minZoom)
      assertEquals(12.0, retainedInfo.tileJson?.maxZoom)
      assertEquals(TileScheme.TMS, retainedInfo.tileJson?.scheme)
      assertEquals(bounds, retainedInfo.tileJson?.bounds)
      assertEquals(512, retainedInfo.tileSize)
      assertEquals(VectorTileEncoding.MLT, retainedInfo.vectorEncoding)
      assertNull(retainedInfo.rasterDemEncoding)
      assertTrue(map.removeStyleSource("inline"))
    } finally {
      map.close()
      runtime.close()
    }

    assertEquals(tileUrls, retainedInfo.tileJson?.tileUrls)
    assertEquals(bounds, retainedInfo.tileJson?.bounds)
  }

  @Test
  fun geoJsonSourcesCanBeAddedAndUpdated() {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 64
          height = 64
          mapMode = MapMode.STATIC
        },
      )

    try {
      map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray())
      map.addGeoJsonSourceUrl("remote-places", "https://example.com/places.geojson", null)
      assertEquals(SourceType.GEOJSON, map.styleSourceType("remote-places"))
      map.setGeoJsonSourceUrl("remote-places", "https://example.com/updated.geojson")

      val options =
        GeoJsonSourceOptions().apply {
          minZoom = 0.0
          maxZoom = 14.0
          tolerance = 0.5
          tileSize = 256
          buffer = 64
          lineMetrics = true
        }
      GeoJsonSourceDataHandle.create(geoJsonData(), options).use { initialData ->
        map.addGeoJsonSourceData("inline-places", initialData)
      }
      assertEquals(SourceType.GEOJSON, map.styleSourceType("inline-places"))
      GeoJsonSourceDataHandle.create(
          "{\"type\":\"LineString\",\"coordinates\":[[0,0],[1,1]]}".encodeToByteArray(),
          options,
        )
        .use { updatedData -> map.setGeoJsonSourceData("inline-places", updatedData) }
      map.setGeoJsonSourceSynchronousTiling("inline-places", true)
      map.setGeoJsonSourceSynchronousTiling("inline-places", false)

      GeoJsonSourceDataHandle.create(nearbyPoints(), clusterOptions()).use { clusteredData ->
        map.addGeoJsonSourceData("clustered-places", clusteredData)
      }
      assertEquals(SourceType.GEOJSON, map.styleSourceType("clustered-places"))
      assertFailsWith<InvalidArgumentException> {
        map.setGeoJsonSourceSynchronousTiling("no-such-source", true)
      }

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
      assertFailsWith<InvalidArgumentException> {
        map.addGeoJsonSourceUrl(
          "invalid-cluster-properties",
          "https://example.com/places.geojson",
          GeoJsonSourceOptions().apply {
            clusterProperties = "\"not an object\"".encodeToByteArray()
          },
        )
      }
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun customGeometrySourcesCanBeManaged() {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 64
          height = 64
          mapMode = MapMode.STATIC
        },
      )

    try {
      map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray())
      map.addCustomGeometrySource(
        "custom-places",
        CustomGeometrySourceOptions(
            object : CustomGeometrySourceCallback {
              override fun fetchTile(tileId: CanonicalTileId) {}
            }
          )
          .apply {
            minZoom = 0.0
            maxZoom = 14.0
            tolerance = 0.375
            tileSize = 512
            buffer = 64
            clip = true
            wrap = false
          },
      )

      assertTrue(map.styleSourceExists("custom-places"))
      assertEquals(SourceType.CUSTOM_VECTOR, map.styleSourceType("custom-places"))

      val tileId = CanonicalTileId(0, 0, 0)
      map.setCustomGeometrySourceTileData("custom-places", tileId, geoJsonData())
      map.invalidateCustomGeometrySourceTile("custom-places", tileId)
      map.invalidateCustomGeometrySourceRegion(
        "custom-places",
        LatLngBounds(LatLng(-1.0, -1.0), LatLng(1.0, 1.0)),
      )

      assertTrue(map.removeStyleSource("custom-places"))
      assertFalse(map.styleSourceExists("custom-places"))
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun sixLiveCustomGeometrySourcesStayRegistered() {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 64
            height = 64
            mapMode = MapMode.STATIC
          },
        )
        .use { map ->
          map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray())
          val options =
            CustomGeometrySourceOptions(
              object : CustomGeometrySourceCallback {
                override fun fetchTile(tileId: CanonicalTileId) {}
              }
            )
          val ids = (1..6).map { "custom-$it" }
          ids.forEach { map.addCustomGeometrySource(it, options) }
          ids.forEach { id -> assertTrue(map.styleSourceExists(id), id) }

          assertTrue(map.removeStyleSource("custom-1"))
          map.addCustomGeometrySource("custom-7", options)
          assertTrue(map.styleSourceExists("custom-7"))
          assertFalse(map.styleSourceExists("custom-1"))
        }
    }
  }

  @Test
  fun tileSourcesCanBeAddedAndInspected() {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 64
          height = 64
          mapMode = MapMode.STATIC
        },
      )

    try {
      map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray())
      map.addVectorSourceUrl(
        "roads",
        "https://example.com/vector.json",
        TileSourceOptions().apply {
          minZoom = 1.0
          maxZoom = 12.0
          attribution = "vector attribution"
          scheme = TileScheme.XYZ
          vectorEncoding = VectorTileEncoding.MVT
        },
      )
      map.addRasterSourceTiles(
        "satellite",
        listOf("https://example.com/raster/{z}/{x}/{y}.png"),
        TileSourceOptions().apply { tileSize = 256 },
      )
      map.addRasterDemSourceTiles(
        "terrain",
        listOf("https://example.com/terrain/{z}/{x}/{y}.png"),
        TileSourceOptions().apply {
          tileSize = 512
          rasterDemEncoding = RasterDemEncoding.TERRARIUM
        },
      )

      assertEquals(SourceType.VECTOR, map.styleSourceType("roads"))
      val rasterInfo = assertNotNull(map.styleSourceInfo("satellite"))
      assertEquals(SourceType.RASTER, rasterInfo.type)
      assertEquals(256, rasterInfo.tileSize)
      assertEquals(SourceType.RASTER_DEM, map.styleSourceType("terrain"))
      assertEquals(RasterDemEncoding.TERRARIUM, map.styleSourceInfo("terrain")?.rasterDemEncoding)
      assertTrue(map.styleSourceIds().containsAll(listOf("roads", "satellite", "terrain")))
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun styleLayerJsonCanBeAddedInspectedListedAndRemoved() {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 64
          height = 64
          mapMode = MapMode.STATIC
        },
      )

    try {
      map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray())
      map.addStyleLayerJson(backgroundLayer(), "")
      map.addLocationIndicatorLayer("puck", "")
      map.setLocationIndicatorLocation("puck", LatLng(12.0, 34.0), 56.0)
      map.setLocationIndicatorBearing("puck", 78.0)
      map.setLocationIndicatorAccuracyRadius("puck", 9.0)
      map.moveStyleLayer("puck", "background")

      assertTrue(map.styleLayerExists("background"))
      assertEquals("background", map.styleLayerType("background"))
      assertTrue(map.styleLayerExists("puck"))
      assertTrue(map.styleLayerIds().contains("background"))
      assertTrue(map.styleLayerIds().contains("puck"))
      assertTrue(
        map.styleLayerJson("background")!!.decodeToString().contains("\"type\":\"background\"")
      )
      map.setLayerProperty("background", "background-opacity", "0.5".encodeToByteArray())
      assertEquals("0.5", map.layerProperty("background", "background-opacity")?.decodeToString())
      map.setStyleLightProperty("anchor", "\"viewport\"".encodeToByteArray())
      assertEquals("\"viewport\"", map.styleLightProperty("anchor")?.decodeToString())
      assertTrue(map.removeStyleLayer("background"))
      assertTrue(map.removeStyleLayer("puck"))
      assertFalse(map.styleLayerExists("background"))
      assertFalse(map.removeStyleLayer("background"))
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun styleImageCanBeSetCopiedInspectedAndRemoved() {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 64
          height = 64
          mapMode = MapMode.STATIC
        },
      )

    try {
      val image = PremultipliedRgba8Image(1, 1, 4, byteArrayOf(1, 2, 3, 4))
      val options =
        StyleImageOptions().apply {
          pixelRatio = 2.0f
          sdf = true
        }

      map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray())
      map.setStyleImage("dot", image, options)

      assertTrue(map.styleImageExists("dot"))
      val info = map.styleImageInfo("dot")
      assertEquals(1, info?.width)
      assertEquals(1, info?.height)
      assertEquals(4, info?.stride)
      assertEquals(4, info?.byteLength)
      assertEquals(2.0f, info?.pixelRatio)
      assertEquals(true, info?.sdf)
      assertEquals(image, map.copyStyleImagePremultipliedRgba8("dot")?.image)
      assertEquals(2.0f, map.copyStyleImagePremultipliedRgba8("dot")?.pixelRatio)
      assertEquals(true, map.copyStyleImagePremultipliedRgba8("dot")?.sdf)
      assertTrue(map.removeStyleImage("dot"))
      assertFalse(map.styleImageExists("dot"))
      assertFalse(map.removeStyleImage("dot"))
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun imageSourcesCanBeAddedUpdatedAndInspected() {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 64
          height = 64
          mapMode = MapMode.STATIC
        },
      )

    try {
      val image = PremultipliedRgba8Image(1, 1, 4, byteArrayOf(1, 2, 3, 4))
      val coordinates = imageCoordinates()
      val moved = listOf(LatLng(1.0, 0.0), LatLng(1.0, 1.0), LatLng(0.0, 1.0), LatLng(0.0, 0.0))

      map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray())
      map.addImageSourceUrl("overlay", coordinates, "https://example.com/image.png")

      assertEquals(SourceType.IMAGE, map.styleSourceType("overlay"))
      assertEquals(coordinates, map.imageSourceCoordinates("overlay"))
      map.setImageSourceUrl("overlay", "https://example.com/updated-image.png")
      map.setImageSourceImage("overlay", image)
      map.setImageSourceCoordinates("overlay", moved)
      assertEquals(moved, map.imageSourceCoordinates("overlay"))
      assertEquals(null, map.imageSourceCoordinates("missing-overlay"))

      map.addImageSourceImage("inline-overlay", coordinates, image)
      assertEquals(SourceType.IMAGE, map.styleSourceInfo("inline-overlay")?.type)
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun mapDebugControlsCanBeReadAndWritten() {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 64
          height = 64
          mapMode = MapMode.CONTINUOUS
        },
      )

    try {
      assertTrue(map.debugOptions.isEmpty())
      map.debugOptions = setOf(DebugOption.TILE_BORDERS, DebugOption.TIMESTAMPS)
      assertEquals(setOf(DebugOption.TILE_BORDERS, DebugOption.TIMESTAMPS), map.debugOptions)
      map.debugOptions = emptySet()
      assertTrue(map.debugOptions.isEmpty())

      assertFalse(map.isRenderingStatsViewEnabled)
      map.isRenderingStatsViewEnabled = true
      assertTrue(map.isRenderingStatsViewEnabled)
      map.isRenderingStatsViewEnabled = false
      assertFalse(map.isRenderingStatsViewEnabled)

      map.requestRepaint()
      map.isFullyLoaded
      map.dumpDebugLogs()
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun mapOptionPropertiesCanBeRoundTripped() {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 128
          height = 128
          mapMode = MapMode.STATIC
        },
      )

    try {
      map.viewportOptions =
        ViewportOptions().apply {
          northOrientation = NorthOrientation.UP
          constrainMode = ConstrainMode.HEIGHT_ONLY
          viewportMode = ViewportMode.DEFAULT
          frustumOffset = EdgeInsets.ZERO
        }
      val viewport = map.viewportOptions
      assertEquals(NorthOrientation.UP, viewport.northOrientation)
      assertEquals(ConstrainMode.HEIGHT_ONLY, viewport.constrainMode)
      assertEquals(ViewportMode.DEFAULT, viewport.viewportMode)
      assertEquals(EdgeInsets.ZERO, viewport.frustumOffset)

      map.tileOptions =
        TileOptions().apply {
          prefetchZoomDelta = 2
          lodMinRadius = 1.5
          lodScale = 2.5
          lodPitchThreshold = 30.0
          lodZoomShift = 1.0
          lodMode = TileLodMode.DEFAULT
        }
      val tile = map.tileOptions
      assertEquals(2, tile.prefetchZoomDelta)
      assertEquals(1.5, tile.lodMinRadius)
      assertEquals(2.5, tile.lodScale)
      assertEquals(30.0, tile.lodPitchThreshold)
      assertEquals(1.0, tile.lodZoomShift)
      assertEquals(TileLodMode.DEFAULT, tile.lodMode)

      map.projectionMode =
        ProjectionModeOptions().apply {
          axonometric = false
          xSkew = 0.0
          ySkew = 0.0
        }
      val projectionMode = map.projectionMode
      assertEquals(false, projectionMode.axonometric)
      assertEquals(0.0, projectionMode.xSkew)
      assertEquals(0.0, projectionMode.ySkew)
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun cameraCommandsAndProjectionCameraCanBeUsed() {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 128
          height = 128
          mapMode = MapMode.STATIC
        },
      )

    try {
      val camera =
        CameraOptions().apply {
          center = LatLng(10.0, 20.0)
          zoom = 3.0
          bearing = 15.0
          pitch = 20.0
          padding = EdgeInsets.ZERO
          anchor = ScreenPoint(64.0, 64.0)
        }
      map.jumpTo(camera)
      assertNear(requireNotNull(camera.center), requireNotNull(map.camera.center))
      assertEquals(3.0, map.camera.zoom ?: 0.0, 1e-6)
      // BND-070: successive snapshots of an unchanged camera compare equal.
      assertEquals(map.camera, map.camera)

      val animation =
        AnimationOptions().apply {
          durationMs = 0.0
          minZoom = 2.0
          easing = UnitBezier(0.0, 0.0, 1.0, 1.0)
        }
      map.easeTo(CameraOptions().apply { zoom = 3.25 }, animation)
      map.flyTo(CameraOptions().apply { zoom = 3.5 }, null)
      map.moveBy(0.0, 0.0)
      map.moveByAnimated(0.0, 0.0, null)
      map.scaleBy(1.0, null)
      map.scaleByAnimated(1.0, ScreenPoint(64.0, 64.0), animation)
      map.rotateBy(ScreenPoint(0.0, 0.0), ScreenPoint(1.0, 1.0))
      map.rotateByAnimated(ScreenPoint(0.0, 0.0), ScreenPoint(1.0, 1.0), null)
      map.pitchBy(0.0)
      map.pitchByAnimated(0.0, null)
      map.cancelTransitions()

      assertFalse(map.isGestureInProgress)
      map.isGestureInProgress = true
      map.moveBy(8.0, -4.0)
      assertTrue(map.isGestureInProgress)
      map.isGestureInProgress = false
      assertFalse(map.isGestureInProgress)

      val fit =
        CameraFitOptions().apply {
          padding = EdgeInsets.ZERO
          bearing = 0.0
          pitch = 0.0
        }
      val bounds = LatLngBounds(LatLng(-1.0, -1.0), LatLng(1.0, 1.0))
      assertTrue(map.cameraForLatLngBounds(bounds, fit).center != null)
      assertTrue(
        map.cameraForLatLngs(listOf(bounds.southwest, bounds.northeast), null).center != null
      )
      assertTrue(map.cameraForGeometry(pointGeometry(), fit).center != null)
      assertTrue(map.latLngBoundsForCamera(camera).southwest.latitude.isFinite())
      assertTrue(map.latLngBoundsForCameraUnwrapped(camera).southwest.latitude.isFinite())

      val projection = map.createProjection()
      try {
        projection.setCamera(camera)
        assertNear(requireNotNull(camera.center), requireNotNull(projection.camera.center))
        projection.setVisibleCoordinates(
          listOf(bounds.southwest, bounds.northeast),
          EdgeInsets.ZERO,
        )
        assertTrue(projection.camera.center != null)
        projection.setVisibleGeometry(pointGeometry(), EdgeInsets.ZERO)
        assertTrue(projection.camera.center != null)
      } finally {
        projection.close()
      }

      map.bounds =
        BoundOptions().apply {
          this.bounds = BoundsConstraint.Bounded(bounds)
          minZoom = 1.0
          maxZoom = 10.0
          minPitch = 0.0
          maxPitch = 45.0
        }
      val boundOptions = map.bounds
      assertEquals(BoundsConstraint.Bounded(bounds), boundOptions.bounds)
      assertEquals(1.0, boundOptions.minZoom ?: 0.0, 1e-6)
      assertEquals(10.0, boundOptions.maxZoom ?: 0.0, 1e-6)
      assertEquals(0.0, boundOptions.minPitch ?: -1.0, 1e-6)
      assertEquals(45.0, boundOptions.maxPitch ?: 0.0, 1e-6)

      map.freeCameraOptions =
        org.maplibre.nativeffi.camera.FreeCameraOptions().apply {
          position = Vec3(0.0, 0.0, 1.0)
          orientation = Quaternion(0.0, 0.0, 0.0, 1.0)
        }
      assertTrue(map.freeCameraOptions.position != null)
      assertTrue(map.freeCameraOptions.orientation != null)
    } finally {
      map.close()
      runtime.close()
    }
  }

  // BND-087, BND-102.

  @Test
  fun cameraTransitionIdsReportEveryTerminalOutcomeOnce() {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 64
          height = 64
        },
      )

    try {
      // A zero-duration ease resolves inside the call. An id above Long.MAX_VALUE
      // round-trips as the unsigned bit pattern the caller passed in.
      val instantId = (Long.MAX_VALUE.toULong() + 1UL).toLong()
      map.easeTo(CameraOptions().apply { zoom = 2.0 }, transitionAnimation(instantId, 0.0))
      val instant = drainCameraEvents(runtime)
      assertEquals(listOf(instantId), instant.finishedTransitionIds)
      assertEquals(CameraChangeMode.IMMEDIATE, instant.lastChangeMode)

      map.easeTo(CameraOptions().apply { zoom = 12.0 }, transitionAnimation(11L, 5_000.0))
      assertEquals(emptyList(), drainCameraEvents(runtime).finishedTransitionIds)

      // A later camera command supersedes the running transition, ending it.
      map.easeTo(CameraOptions().apply { zoom = 13.0 }, transitionAnimation(12L, 5_000.0))
      val superseded = drainCameraEvents(runtime)
      assertEquals(listOf(11L), superseded.finishedTransitionIds)
      assertEquals(CameraChangeMode.ANIMATED, superseded.lastChangeMode)

      map.cancelTransitions()
      assertEquals(listOf(12L), drainCameraEvents(runtime).finishedTransitionIds)

      // Omitting the id leaves the transition silent.
      map.easeTo(
        CameraOptions().apply { zoom = 14.0 },
        AnimationOptions().apply { durationMs = 0.0 },
      )
      assertEquals(emptyList(), drainCameraEvents(runtime).finishedTransitionIds)
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun completedCameraTransitionReportsItsIdOnceAndReachesTheRequestedCamera() {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 64
          height = 64
          mapMode = MapMode.STATIC
        },
      )

    try {
      map.easeTo(CameraOptions().apply { zoom = 5.0 }, transitionAnimation(21L, 5_000.0))
      // A still-image request runs a static map's pending transitions to their end.
      map.requestStillImage()

      val finished = mutableListOf<Long>()
      val cameraEventTypes = mutableListOf<RuntimeEventType>()
      var rounds = 0
      while (finished.isEmpty() && rounds < 10_000) {
        runtime.pump(0)
        val events = drainCameraEvents(runtime)
        finished += events.finishedTransitionIds
        cameraEventTypes += events.cameraEventTypes
        rounds++
        runtime.pump(1)
      }
      assertEquals(listOf(21L), finished)
      assertEquals(5.0, map.camera.zoom ?: 0.0, 1e-6)

      // The transition reports its end once; later pumping adds nothing.
      repeat(100) {
        runtime.pump(0)
        val events = drainCameraEvents(runtime)
        finished += events.finishedTransitionIds
        cameraEventTypes += events.cameraEventTypes
      }
      assertEquals(listOf(21L), finished)

      // The transition reports its end ahead of the camera change that settles it.
      val finishedIndex = cameraEventTypes.indexOf(RuntimeEventType.MAP_CAMERA_TRANSITION_FINISHED)
      assertTrue(
        cameraEventTypes.drop(finishedIndex + 1).contains(RuntimeEventType.MAP_CAMERA_DID_CHANGE),
        "camera-did-change did not follow the transition: $cameraEventTypes",
      )
    } finally {
      map.close()
      runtime.close()
    }
  }

  private fun transitionAnimation(transitionId: Long, durationMs: Double): AnimationOptions =
    AnimationOptions().apply {
      this.transitionId = transitionId
      this.durationMs = durationMs
    }

  private class CameraEvents(
    val finishedTransitionIds: List<Long>,
    val lastChangeMode: CameraChangeMode?,
    /** Camera event types in queue order, so a test can assert their order. */
    val cameraEventTypes: List<RuntimeEventType>,
  )

  private fun drainCameraEvents(runtime: RuntimeHandle): CameraEvents {
    val finished = mutableListOf<Long>()
    val types = mutableListOf<RuntimeEventType>()
    var lastChangeMode: CameraChangeMode? = null
    for (event in runtime.drainEvents().events) {
      when (event.type) {
        RuntimeEventType.MAP_CAMERA_TRANSITION_FINISHED -> {
          finished +=
            assertIs<RuntimeEventPayload.CameraTransitionFinished>(event.payload).transitionId
          types += event.type
        }
        RuntimeEventType.MAP_CAMERA_DID_CHANGE -> {
          lastChangeMode = CameraChangeMode(event.code)
          types += event.type
        }
      }
    }
    return CameraEvents(finished, lastChangeMode, types)
  }

  @Test
  fun boundsConstraintDistinguishesUnboundedFromWorldBounds() {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 128
          height = 128
          mapMode = MapMode.STATIC
        },
      )

    fun jumpedLongitude(longitude: Double): Double {
      map.jumpTo(
        CameraOptions().apply {
          center = LatLng(0.0, longitude)
          zoom = 2.0
        }
      )
      return requireNotNull(map.camera.center).longitude
    }

    try {
      // An unbounded map wraps across the antimeridian.
      assertEquals(BoundsConstraint.Unbounded, map.bounds.bounds)
      assertEquals(-160.0, jumpedLongitude(200.0), 1e-6)

      val world = LatLngBounds(LatLng(-90.0, -180.0), LatLng(90.0, 180.0))
      map.bounds = BoundOptions().apply { bounds = BoundsConstraint.Bounded(world) }
      assertEquals(BoundsConstraint.Bounded(world), map.bounds.bounds)
      // World bounds clamp at the antimeridian instead of wrapping.
      assertEquals(180.0, jumpedLongitude(200.0), 1e-6)

      map.bounds = BoundOptions().apply { bounds = BoundsConstraint.Unbounded }
      assertEquals(BoundsConstraint.Unbounded, map.bounds.bounds)
      // Releasing the constraint restores antimeridian wrapping.
      assertEquals(-160.0, jumpedLongitude(200.0), 1e-6)
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun mapCoordinateConversionsCanBeRoundTripped() {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 128
          height = 128
          mapMode = MapMode.STATIC
        },
      )

    try {
      val coordinate = LatLng(0.0, 0.0)
      val point = map.pixelForLatLng(coordinate)
      val roundTrip = map.latLngForPixel(point)
      assertNear(coordinate, roundTrip)

      val coordinates = listOf(LatLng(0.0, 0.0), LatLng(10.0, 20.0))
      val points = map.pixelsForLatLngs(coordinates)
      assertEquals(coordinates.size, points.size)
      val coordinateRoundTrips = map.latLngsForPixels(points)
      assertEquals(coordinates.size, coordinateRoundTrips.size)
      coordinates.zip(coordinateRoundTrips).forEach { (expected, actual) ->
        assertNear(expected, actual)
      }

      assertEquals(emptyList(), map.pixelsForLatLngs(emptyList()))
      assertEquals(emptyList(), map.latLngsForPixels(emptyList()))
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun mapProjectionCoordinateConversionsCanBeRoundTrippedAndClosed() {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 128
          height = 128
          mapMode = MapMode.STATIC
        },
      )

    try {
      val projection = map.createProjection()
      assertFalse(projection.isClosed)
      val coordinate = LatLng(0.0, 0.0)
      val point = projection.pixelForLatLng(coordinate)
      val roundTrip = projection.latLngForPixel(point)
      assertNear(coordinate, roundTrip)

      map.close()
      val detachedPoint = projection.pixelForLatLng(coordinate)
      assertNear(coordinate, projection.latLngForPixel(detachedPoint))

      projection.close()
      projection.close()
      assertTrue(projection.isClosed)
      assertFailsWith<InvalidStateException> { projection.pixelForLatLng(coordinate) }
    } finally {
      map.close()
      runtime.close()
    }
  }

  private fun geoJsonSource(): ByteArray =
    "{\"type\":\"geojson\",\"data\":{\"type\":\"FeatureCollection\",\"features\":[]}}"
      .encodeToByteArray()

  /** Point features close enough together to collapse into one cluster at low zoom. */
  private fun nearbyPoints(): ByteArray =
    ("{\"type\":\"FeatureCollection\",\"features\":[" +
        (0..3).joinToString(",") { index ->
          "{\"type\":\"Feature\",\"id\":$index,\"geometry\":{\"type\":\"Point\"," +
            "\"coordinates\":[${index * 0.001},${index * 0.001}]},\"properties\":{\"weight\":1}}"
        } +
        "]}")
      .encodeToByteArray()

  private fun clusterOptions(): GeoJsonSourceOptions =
    GeoJsonSourceOptions().apply {
      cluster = true
      clusterRadius = 50
      clusterMaxZoom = 14.0
      clusterMinPoints = 2
      clusterProperties = "{\"total\":[\"+\",[\"get\",\"weight\"]]}".encodeToByteArray()
    }

  private fun geoJsonData(): ByteArray =
    ("{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\",\"id\":1," +
        "\"geometry\":{\"type\":\"GeometryCollection\",\"geometries\":[" +
        "{\"type\":\"Point\",\"coordinates\":[0,0]}," +
        "{\"type\":\"MultiLineString\",\"coordinates\":[[[0,0],[1,1]]]}]}," +
        "\"properties\":{\"name\":\"Null Island\",\"rank\":1}}]}")
      .encodeToByteArray()

  private fun backgroundLayer(): ByteArray =
    "{\"id\":\"background\",\"type\":\"background\"}".encodeToByteArray()

  private fun pointGeometry(): ByteArray =
    "{\"type\":\"Point\",\"coordinates\":[0,0]}".encodeToByteArray()

  private fun imageCoordinates(): List<LatLng> =
    listOf(LatLng(0.0, 0.0), LatLng(0.0, 1.0), LatLng(1.0, 1.0), LatLng(1.0, 0.0))

  private fun assertNear(expected: LatLng, actual: LatLng) {
    assertEquals(expected.latitude, actual.latitude, 1e-6)
    assertEquals(expected.longitude, actual.longitude, 1e-6)
  }

  @Test
  fun loadedStyleDocumentAndUrlReadBackWhatWasLoaded() {
    val styleJson = "{\"version\":8,\"sources\":{},\"layers\":[]}"
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 64
            height = 64
          },
        )
        .use { map ->
          assertTrue(map.loadedStyleJson().isEmpty())
          assertEquals("", map.styleUrl())

          // The document reads back byte-for-byte.
          map.setStyleJson(styleJson.encodeToByteArray())
          assertEquals(styleJson, map.loadedStyleJson().decodeToString())
          // Inline JSON clears the URL.
          assertEquals("", map.styleUrl())

          // setStyleUrl records request state before the load can succeed; the document
          // still reports the style that last parsed.
          map.setStyleUrl("https://example.com/style.json")
          assertEquals("https://example.com/style.json", map.styleUrl())
          assertEquals(styleJson, map.loadedStyleJson().decodeToString())
        }
    }
  }
}
