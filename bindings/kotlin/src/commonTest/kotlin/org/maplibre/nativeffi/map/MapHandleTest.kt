package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertSame
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.render.PremultipliedRgba8Image
import org.maplibre.nativeffi.runtime.CommandDisposition
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
            assertEquals(Double.NEGATIVE_INFINITY, map.layerMinZoom("fill"))
            assertEquals(Double.POSITIVE_INFINITY, map.layerMaxZoom("fill"))
            assertCommandCommitted(runtime, map.setLayerMinZoom("fill", 4.0))
            assertCommandCommitted(runtime, map.setLayerMaxZoom("fill", 12.5))
            assertEquals(4.0, map.layerMinZoom("fill"))
            assertEquals(12.5, map.layerMaxZoom("fill"))

            assertEquals(StyleLayerVisibility.VISIBLE, map.layerVisibility("fill"))
            assertCommandCommitted(
              runtime,
              map.setLayerVisibility("fill", StyleLayerVisibility.NONE),
            )
            assertEquals(StyleLayerVisibility.NONE, map.layerVisibility("fill"))

            assertCommandFinished(
              runtime,
              map.setLayerVisibility("fill", StyleLayerVisibility(900)),
              CommandDisposition.FAILED,
            )
            assertEquals(StyleLayerVisibility.NONE, map.layerVisibility("fill"))
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
        assertTrue(map.removeStyleSource("inline"))
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
        assertEquals(SourceType.GEOJSON, map.styleSourceType("remote-places"))
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
        assertEquals(SourceType.GEOJSON, map.styleSourceType("inline-places"))
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
        assertEquals(SourceType.GEOJSON, map.styleSourceType("clustered-places"))

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
        assertFalse(map.styleSourceExists("invalid-cluster-properties"))
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

        assertTrue(map.styleLayerExists("background"))
        assertEquals("background", map.styleLayerType("background"))
        assertTrue(map.styleLayerExists("puck"))
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

  private suspend fun assertCommandFinished(
    runtime: RuntimeHandle,
    commandId: ULong,
    expectedDisposition: CommandDisposition,
  ) {
    runtime.barrier()
    val matches =
      runtime
        .drainEvents()
        .events
        .mapNotNull { it.payload as? RuntimeEventPayload.CommandFinished }
        .filter { it.commandId == commandId }
    assertEquals(1, matches.size, "terminal outcome count for command $commandId")
    assertEquals(expectedDisposition, matches.single().disposition)
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
