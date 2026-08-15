package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertSame
import kotlin.test.assertTrue
import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.BoundsConstraint
import org.maplibre.nativeffi.camera.FreeCameraOptions
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.geo.Quaternion
import org.maplibre.nativeffi.geo.Vec3
import org.maplibre.nativeffi.render.PremultipliedRgba8Image
import org.maplibre.nativeffi.runtime.CommandDisposition
import org.maplibre.nativeffi.runtime.RuntimeEvent
import org.maplibre.nativeffi.runtime.RuntimeEventPayload
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions
import org.maplibre.nativeffi.runtime.use
import org.maplibre.nativeffi.style.CustomGeometrySourceCallback
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions
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
  fun layerBaseAccessorsReachNativeThroughDowncalls(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      RuntimeHandle.create(RuntimeOptions()).use { runtime ->
        MapHandle.create(
            runtime,
            MapOptions().apply {
              width = 64
              height = 64
            },
          )
          .use { map ->
            assertCommandCommitted(
              runtime,
              map.setStyleJson(
                ("{\"version\":8,\"sources\":{\"geo\":{\"type\":\"geojson\",\"data\":" +
                    "{\"type\":\"FeatureCollection\",\"features\":[]}}},\"layers\":[" +
                    "{\"id\":\"bg\",\"type\":\"background\"}," +
                    "{\"id\":\"fill\",\"type\":\"fill\",\"source\":\"geo\"}]}")
                  .encodeToByteArray()
              ),
            )

            assertEquals("", map.layerSourceLayer("fill"))
            assertCommandCommitted(runtime, map.setLayerSourceLayer("fill", "roads"))
            assertEquals("roads", map.layerSourceLayer("fill"))
            assertEquals("geo", map.layerSourceId("fill"))

            // A background layer takes no source.
            assertCommandFinished(
              runtime,
              map.setLayerSourceLayer("bg", "roads"),
              CommandDisposition.FAILED,
            )
            assertEquals("", map.layerSourceLayer("bg"))

            // An unset zoom range crosses the boundary as infinities.
            val unbounded = assertNotNull(map.styleLayerInfo("fill"))
            assertEquals("fill", unbounded.type)
            assertEquals(Double.NEGATIVE_INFINITY, unbounded.minZoom)
            assertEquals(Double.POSITIVE_INFINITY, unbounded.maxZoom)
            assertEquals(StyleLayerVisibility.VISIBLE, unbounded.visibility)
            // The info's source flags feed the copy operations.
            assertEquals("geo", unbounded.sourceId)
            assertEquals("roads", unbounded.sourceLayer)

            assertCommandCommitted(runtime, map.setLayerMinZoom("fill", 4.0))
            assertCommandCommitted(runtime, map.setLayerMaxZoom("fill", 12.5))
            assertCommandCommitted(
              runtime,
              map.setLayerVisibility("fill", StyleLayerVisibility.NONE),
            )
            val bounded = assertNotNull(map.styleLayerInfo("fill"))
            assertEquals(4.0, bounded.minZoom)
            assertEquals(12.5, bounded.maxZoom)
            assertEquals(StyleLayerVisibility.NONE, bounded.visibility)

            // A sourceless layer reports no source fields.
            val background = assertNotNull(map.styleLayerInfo("bg"))
            assertEquals("background", background.type)
            assertNull(background.sourceId)
            assertNull(background.sourceLayer)

            // No layer carries this ID.
            assertNull(map.styleLayerInfo("missing"))

            assertCommandFinished(
              runtime,
              map.setLayerVisibility("fill", StyleLayerVisibility(900)),
              CommandDisposition.FAILED,
            )
            assertEquals(StyleLayerVisibility.NONE, map.styleLayerInfo("fill")?.visibility)
          }
      }
    }

  @Test
  fun styleTransitionOptionsRoundTripThroughDowncalls(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
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
            assertCommandCommitted(
              runtime,
              map.setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}".encodeToByteArray()),
            )
            val parsed = map.styleTransitionOptions()
            assertEquals(300.0, parsed.durationMs)
            assertNull(parsed.delayMs)

            assertCommandCommitted(
              runtime,
              map.setStyleJson(transitionStyleJson.encodeToByteArray()),
            )
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
            assertCommandCommitted(runtime, map.setStyleTransitionOptions(options))
            assertEquals(options, map.styleTransitionOptions())

            // Omitting the flag leaves the cross-fade on.
            assertCommandCommitted(
              runtime,
              map.setStyleTransitionOptions(StyleTransitionOptions().apply { durationMs = 250.0 }),
            )
            assertEquals(true, map.styleTransitionOptions().enablePlacementTransitions)

            // Loading a style replaces the override with what that style declares.
            assertCommandCommitted(
              runtime,
              map.setStyleJson(transitionStyleJson.encodeToByteArray()),
            )
            assertEquals(declared, map.styleTransitionOptions())

            assertCommandFinished(
              runtime,
              map.setStyleTransitionOptions(StyleTransitionOptions().apply { delayMs = -1.0 }),
              CommandDisposition.FAILED,
            )
            assertEquals(declared, map.styleTransitionOptions())
          }
      }
    }

  @Test
  fun canonicalTileIdRejectsOutOfRangeInputs(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      assertFailsWith<InvalidArgumentException> {
        CanonicalTileId(0, UInt.MAX_VALUE.toLong() + 1, 0)
      }
    }

  @Test
  fun mapCreateStyleAndCloseRetainsRuntime(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
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
      assertFailsWith<InvalidStateException> {
        org.maplibre.nativeffi.runtime.runSuspendTest { runtime.close() }
      }

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
  fun mapSizeReportsCreationExtentAndPixelRatio(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
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

      val snapshot = map.snapshot()
      val size = snapshot.size
      assertEquals(512, size.width)
      assertEquals(256, size.height)
      assertEquals(2.0, size.scaleFactor)
      assertEquals(MapSize(512, 256, 2.0), size)
      assertEquals(MapSize(512, 256, 2.0).hashCode(), size.hashCode())

      map.close()
      runtime.close()
    }

  @Test
  fun styleSourceJsonCanBeAddedInspectedListedAndRemoved(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
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

        assertEquals(SourceType.GEOJSON, map.styleSourceInfo("places")?.type)
        assertTrue(map.styleSourceIds().contains("places"))
        assertCommandCommitted(runtime, map.removeStyleSource("places"))
        assertNull(map.styleSourceInfo("places"))
        assertCommandNotFound(runtime, map.removeStyleSource("places"))
      } finally {
        map.close()
        runtime.close()
      }
    }

  // BND-109.

  @Test
  fun styleSourceInfoCopiesUrlAndInlineTileMetadataPastNativeLifetime(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
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
        assertCommandCommitted(
          runtime,
          map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()),
        )
        assertCommandCommitted(
          runtime,
          map.addVectorSourceUrl("remote", "https://example.com/vector.json", null),
        )
        val remote = assertNotNull(map.styleSourceInfo("remote"))
        assertEquals("https://example.com/vector.json", remote.url)
        assertNull(remote.tileJson)

        assertCommandCommitted(
          runtime,
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
          ),
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
        assertCommandCommitted(runtime, map.removeStyleSource("inline"))
      } finally {
        map.close()
        runtime.close()
      }

      assertEquals(tileUrls, retainedInfo.tileJson?.tileUrls)
      assertEquals(bounds, retainedInfo.tileJson?.bounds)
    }

  @Test
  fun geoJsonSourcesCanBeAddedAndUpdated(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
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
        assertCommandCommitted(
          runtime,
          map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()),
        )
        assertCommandCommitted(
          runtime,
          map.addGeoJsonSourceUrl("remote-places", "https://example.com/places.geojson", null),
        )
        assertEquals(SourceType.GEOJSON, map.styleSourceInfo("remote-places")?.type)
        assertCommandCommitted(
          runtime,
          map.setGeoJsonSourceUrl("remote-places", "https://example.com/updated.geojson"),
        )

        assertCommandCommitted(
          runtime,
          map.addGeoJsonSourceData(
            "inline-places",
            geoJsonData(),
            GeoJsonSourceOptions().apply {
              minZoom = 0.0
              maxZoom = 14.0
              tolerance = 0.5
              tileSize = 256
              buffer = 64
              lineMetrics = true
            },
          ),
        )
        assertEquals(SourceType.GEOJSON, map.styleSourceInfo("inline-places")?.type)
        assertCommandCommitted(
          runtime,
          map.setGeoJsonSourceData(
            "inline-places",
            "{\"type\":\"LineString\",\"coordinates\":[[0,0],[1,1]]}".encodeToByteArray(),
          ),
        )

        assertCommandCommitted(
          runtime,
          map.addGeoJsonSourceData("clustered-places", nearbyPoints(), clusterOptions()),
        )
        assertEquals(SourceType.GEOJSON, map.styleSourceInfo("clustered-places")?.type)

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
        assertNull(map.styleSourceInfo("invalid-zooms"))
        assertCommandFinished(
          runtime,
          map.addGeoJsonSourceUrl(
            "invalid-cluster-properties",
            "https://example.com/places.geojson",
            GeoJsonSourceOptions().apply {
              clusterProperties = "\"not an object\"".encodeToByteArray()
            },
          ),
          CommandDisposition.FAILED,
        )
        assertNull(map.styleSourceInfo("invalid-cluster-properties"))
      } finally {
        map.close()
        runtime.close()
      }
    }

  @Test
  fun customGeometrySourcesCanBeManaged(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
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

        assertEquals(SourceType.CUSTOM_VECTOR, map.styleSourceInfo("custom-places")?.type)

        val tileId = CanonicalTileId(0, 0, 0)
        map.setCustomGeometrySourceTileData("custom-places", tileId, geoJsonData())
        map.invalidateCustomGeometrySourceTile("custom-places", tileId)
        map.invalidateCustomGeometrySourceRegion(
          "custom-places",
          LatLngBounds(LatLng(-1.0, -1.0), LatLng(1.0, 1.0)),
        )

        assertCommandCommitted(runtime, map.removeStyleSource("custom-places"))
        assertNull(map.styleSourceInfo("custom-places"))
      } finally {
        map.close()
        runtime.close()
      }
    }

  @Test
  fun tileSourcesCanBeAddedAndInspected(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
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
        assertCommandCommitted(
          runtime,
          map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()),
        )
        assertCommandCommitted(
          runtime,
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
          ),
        )
        assertCommandCommitted(
          runtime,
          map.addRasterSourceTiles(
            "satellite",
            listOf("https://example.com/raster/{z}/{x}/{y}.png"),
            TileSourceOptions().apply { tileSize = 256 },
          ),
        )
        assertCommandCommitted(
          runtime,
          map.addRasterDemSourceTiles(
            "terrain",
            listOf("https://example.com/terrain/{z}/{x}/{y}.png"),
            TileSourceOptions().apply {
              tileSize = 512
              rasterDemEncoding = RasterDemEncoding.TERRARIUM
            },
          ),
        )

        assertEquals(SourceType.VECTOR, map.styleSourceInfo("roads")?.type)
        val rasterInfo = assertNotNull(map.styleSourceInfo("satellite"))
        assertEquals(SourceType.RASTER, rasterInfo.type)
        assertEquals(256, rasterInfo.tileSize)
        assertEquals(SourceType.RASTER_DEM, map.styleSourceInfo("terrain")?.type)
        assertEquals(RasterDemEncoding.TERRARIUM, map.styleSourceInfo("terrain")?.rasterDemEncoding)
        assertTrue(map.styleSourceIds().containsAll(listOf("roads", "satellite", "terrain")))
      } finally {
        map.close()
        runtime.close()
      }
    }

  @Test
  fun styleLayerJsonCanBeAddedInspectedListedAndRemoved(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
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
        assertCommandCommitted(
          runtime,
          map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()),
        )
        assertCommandCommitted(runtime, map.addStyleLayerJson(backgroundLayer(), ""))
        assertCommandCommitted(runtime, map.addLocationIndicatorLayer("puck", ""))
        assertCommandCommitted(
          runtime,
          map.setLocationIndicatorLocation("puck", LatLng(12.0, 34.0), 56.0),
        )
        assertCommandCommitted(runtime, map.setLocationIndicatorBearing("puck", 78.0))
        assertCommandCommitted(runtime, map.setLocationIndicatorAccuracyRadius("puck", 9.0))
        assertCommandCommitted(runtime, map.moveStyleLayer("puck", "background"))

        assertEquals("background", map.styleLayerInfo("background")?.type)
        assertNotNull(map.styleLayerInfo("puck"))
        assertTrue(map.styleLayerIds().contains("background"))
        assertTrue(map.styleLayerIds().contains("puck"))
        assertTrue(
          map.styleLayerJson("background")!!.decodeToString().contains("\"type\":\"background\"")
        )
        assertCommandCommitted(
          runtime,
          map.setLayerProperty("background", "background-opacity", "0.5".encodeToByteArray()),
        )
        assertEquals("0.5", map.layerProperty("background", "background-opacity")?.decodeToString())
        assertCommandCommitted(
          runtime,
          map.setStyleLightProperty("anchor", "\"viewport\"".encodeToByteArray()),
        )
        assertEquals("\"viewport\"", map.styleLightProperty("anchor")?.decodeToString())
        assertCommandCommitted(runtime, map.removeStyleLayer("background"))
        assertCommandCommitted(runtime, map.removeStyleLayer("puck"))
        assertNull(map.styleLayerInfo("background"))
        assertCommandNotFound(runtime, map.removeStyleLayer("background"))
      } finally {
        map.close()
        runtime.close()
      }
    }

  @Test
  fun styleImageCanBeSetCopiedInspectedAndRemoved(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
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
        assertCommandCommitted(runtime, map.removeStyleImage("dot"))
        assertNull(map.styleImageInfo("dot"))
        assertCommandNotFound(runtime, map.removeStyleImage("dot"))
      } finally {
        map.close()
        runtime.close()
      }
    }

  @Test
  fun imageSourcesCanBeAddedUpdatedAndInspected(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
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

        assertEquals(SourceType.IMAGE, map.styleSourceInfo("overlay")?.type)
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
  fun mapProjectionCoordinateConversionsCanBeRoundTrippedAndClosed(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
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

        assertFailsWith<InvalidStateException> {
          org.maplibre.nativeffi.runtime.runSuspendTest { map.close() }
        }
        projection.close()
        assertTrue(projection.isClosed)
        projection.close()
        assertFailsWith<InvalidStateException> { projection.pixelForLatLng(coordinate) }
      } finally {
        map.close()
        runtime.close()
      }
    }

  // The COMMAND_FINISHED generation fences a later snapshot: a snapshot at or past it
  // observes the commit.
  @Test
  fun committedCommandGenerationFencesTheSnapshot(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      RuntimeHandle.create(RuntimeOptions()).use { runtime ->
        MapHandle.create(
            runtime,
            MapOptions().apply {
              width = 64
              height = 64
            },
          )
          .use { map ->
            val debug = setOf(DebugOption.TILE_BORDERS, DebugOption.OVERDRAW)
            val event =
              assertCommandFinished(
                runtime,
                map.setDebugOptions(debug).toULong(),
                CommandDisposition.COMMITTED,
              )
            val generation =
              (event.payload as RuntimeEventPayload.CommandFinished).generation.toLong()
            assertTrue(generation > 0L, "committed command must publish a generation")
            val snapshot = map.snapshot()
            assertTrue(snapshot.generation >= generation)
            assertEquals(debug, snapshot.debugOptions)
          }
      }
    }

  @Test
  fun snapshotFieldsRoundTripThroughTheirSetCommands(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      RuntimeHandle.create(RuntimeOptions()).use { runtime ->
        MapHandle.create(
            runtime,
            MapOptions().apply {
              width = 64
              height = 64
            },
          )
          .use { map ->
            assertEquals(emptySet<DebugOption>(), map.snapshot().debugOptions)
            assertFalse(map.snapshot().renderingStatsViewEnabled)

            assertCommandCommitted(runtime, map.setRenderingStatsViewEnabled(true).toULong())
            assertTrue(map.snapshot().renderingStatsViewEnabled)

            val viewport = ViewportOptions().apply { northOrientation = NorthOrientation.DOWN }
            assertCommandCommitted(runtime, map.setViewportOptions(viewport).toULong())
            assertEquals(NorthOrientation.DOWN, map.snapshot().viewportOptions.northOrientation)

            val tile =
              TileOptions().apply {
                prefetchZoomDelta = 3
                lodScale = 1.5
              }
            assertCommandCommitted(runtime, map.setTileOptions(tile).toULong())
            val tileSnapshot = map.snapshot().tileOptions
            assertEquals(3, tileSnapshot.prefetchZoomDelta)
            assertEquals(1.5, tileSnapshot.lodScale)

            val bounds =
              BoundOptions().apply {
                minZoom = 2.0
                maxZoom = 15.0
                this.bounds =
                  BoundsConstraint.Bounded(LatLngBounds(LatLng(-10.0, -10.0), LatLng(10.0, 10.0)))
              }
            assertCommandCommitted(runtime, map.setBounds(bounds).toULong())
            val boundsSnapshot = map.snapshot().bounds
            assertEquals(2.0, boundsSnapshot.minZoom)
            assertEquals(15.0, boundsSnapshot.maxZoom)
            assertEquals(bounds.bounds, boundsSnapshot.bounds)

            val freeCamera =
              FreeCameraOptions().apply {
                position = Vec3(0.5, 0.5, 0.5)
                orientation = Quaternion(0.0, 0.0, 0.0, 1.0)
              }
            assertCommandCommitted(runtime, map.setFreeCameraOptions(freeCamera).toULong())
            val freeCameraSnapshot = map.snapshot().freeCameraOptions
            kotlin.test.assertNotNull(freeCameraSnapshot.position)
            kotlin.test.assertNotNull(freeCameraSnapshot.orientation)
          }
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

  private suspend fun assertCommandCommitted(runtime: RuntimeHandle, commandId: ULong) {
    assertCommandFinished(runtime, commandId, CommandDisposition.COMMITTED)
  }

  /** Asserts a FAILED terminal outcome whose status detail is NOT_FOUND. */
  private suspend fun assertCommandNotFound(runtime: RuntimeHandle, commandId: ULong) {
    val event = assertCommandFinished(runtime, commandId, CommandDisposition.FAILED)
    assertEquals(MaplibreStatus.NOT_FOUND.nativeCode, event.code)
  }

  private suspend fun assertCommandFinished(
    runtime: RuntimeHandle,
    commandId: ULong,
    expectedDisposition: CommandDisposition,
  ): RuntimeEvent {
    runtime.barrier()
    val matches =
      runtime.drainEvents().events.filter {
        (it.payload as? RuntimeEventPayload.CommandFinished)?.commandId == commandId
      }
    assertEquals(1, matches.size, "terminal outcome count for command $commandId")
    val event = matches.single()
    assertEquals(
      expectedDisposition,
      (event.payload as RuntimeEventPayload.CommandFinished).disposition,
    )
    return event
  }

  private fun assertNear(expected: LatLng, actual: LatLng) {
    assertEquals(expected.latitude, actual.latitude, 1e-6)
    assertEquals(expected.longitude, actual.longitude, 1e-6)
  }

  @Test
  fun loadedStyleDocumentAndUrlReadBackWhatWasLoaded(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
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
