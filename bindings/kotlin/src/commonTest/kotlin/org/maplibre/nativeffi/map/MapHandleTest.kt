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
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.render.PremultipliedRgba8Image
import org.maplibre.nativeffi.runtime.CommandCompletion
import org.maplibre.nativeffi.runtime.CommandDisposition
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions
import org.maplibre.nativeffi.runtime.use
import org.maplibre.nativeffi.style.CustomGeometrySourceCallback
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions
import org.maplibre.nativeffi.style.CustomMvtVectorSourceCallback
import org.maplibre.nativeffi.style.CustomMvtVectorSourceOptions
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
              map
                .setStyleJson(
                  ("{\"version\":8,\"sources\":{\"geo\":{\"type\":\"geojson\",\"data\":" +
                      "{\"type\":\"FeatureCollection\",\"features\":[]}}},\"layers\":[" +
                      "{\"id\":\"bg\",\"type\":\"background\"}," +
                      "{\"id\":\"fill\",\"type\":\"fill\",\"source\":\"geo\"}]}")
                    .encodeToByteArray()
                )
                .await(),
            )

            assertEquals("", map.layerSourceLayer("fill").await())
            assertCommandCommitted(runtime, map.setLayerSourceLayer("fill", "roads").await())
            assertEquals("roads", map.layerSourceLayer("fill").await())
            assertEquals("geo", map.layerSourceId("fill").await())

            // A background layer takes no source.
            assertCommandFailed(
              map.setLayerSourceLayer("bg", "roads").await(),
              MaplibreStatus.INVALID_ARGUMENT,
            )
            assertEquals("", map.layerSourceLayer("bg").await())

            // An unset zoom range crosses the boundary as infinities.
            val unbounded = assertNotNull(map.styleLayerInfo("fill").await())
            assertEquals("fill", unbounded.type)
            assertEquals(Double.NEGATIVE_INFINITY, unbounded.minZoom)
            assertEquals(Double.POSITIVE_INFINITY, unbounded.maxZoom)
            assertEquals(StyleLayerVisibility.VISIBLE, unbounded.visibility)
            // The info's source flags feed the copy operations.
            assertEquals("geo", unbounded.sourceId)
            assertEquals("roads", unbounded.sourceLayer)

            assertCommandCommitted(runtime, map.setLayerMinZoom("fill", 4.0).await())
            assertCommandCommitted(runtime, map.setLayerMaxZoom("fill", 12.5).await())
            assertCommandCommitted(
              runtime,
              map.setLayerVisibility("fill", StyleLayerVisibility.NONE).await(),
            )
            val bounded = assertNotNull(map.styleLayerInfo("fill").await())
            assertEquals(4.0, bounded.minZoom)
            assertEquals(12.5, bounded.maxZoom)
            assertEquals(StyleLayerVisibility.NONE, bounded.visibility)

            // A sourceless layer reports no source fields.
            val background = assertNotNull(map.styleLayerInfo("bg").await())
            assertEquals("background", background.type)
            assertNull(background.sourceId)
            assertNull(background.sourceLayer)

            // No layer carries this ID.
            assertNull(map.styleLayerInfo("missing").await())

            assertCommandFailed(
              map.setLayerVisibility("fill", StyleLayerVisibility(900)).await(),
              MaplibreStatus.INVALID_ARGUMENT,
            )
            assertEquals(StyleLayerVisibility.NONE, map.styleLayerInfo("fill").await()?.visibility)
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
            val empty = map.styleTransitionOptions().await()
            assertNull(empty.durationMs)
            assertNull(empty.delayMs)
            assertEquals(true, empty.enablePlacementTransitions)

            // The style parser supplies a 300ms default duration.
            assertCommandCommitted(
              runtime,
              map
                .setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}".encodeToByteArray())
                .await(),
            )
            val parsed = map.styleTransitionOptions().await()
            assertEquals(300.0, parsed.durationMs)
            assertNull(parsed.delayMs)

            assertCommandCommitted(
              runtime,
              map.setStyleJson(transitionStyleJson.encodeToByteArray()).await(),
            )
            val declared = map.styleTransitionOptions().await()
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
            assertCommandCommitted(runtime, map.setStyleTransitionOptions(options).await())
            assertEquals(options, map.styleTransitionOptions().await())

            // Omitting the flag leaves the cross-fade on.
            assertCommandCommitted(
              runtime,
              map
                .setStyleTransitionOptions(StyleTransitionOptions().apply { durationMs = 250.0 })
                .await(),
            )
            assertEquals(true, map.styleTransitionOptions().await().enablePlacementTransitions)

            // Loading a style replaces the override with what that style declares.
            assertCommandCommitted(
              runtime,
              map.setStyleJson(transitionStyleJson.encodeToByteArray()).await(),
            )
            assertEquals(declared, map.styleTransitionOptions().await())

            assertCommandFailed(
              map
                .setStyleTransitionOptions(StyleTransitionOptions().apply { delayMs = -1.0 })
                .await(),
              MaplibreStatus.INVALID_ARGUMENT,
            )
            assertEquals(declared, map.styleTransitionOptions().await())
          }
      }
    }

  @Test
  fun canonicalTileIdRejectsOutOfRangeInputs() {
    assertFailsWith<InvalidArgumentException> { CanonicalTileId(0, UInt.MAX_VALUE.toLong() + 1, 0) }
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
          .await()

      assertFalse(map.isClosed)
      assertSame(runtime, map.runtime())
      assertFailsWith<InvalidStateException> {
        org.maplibre.nativeffi.runtime.runSuspendTest { runtime.close() }
      }

      map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()).await()
      map.setStyleUrl("https://example.com/style.json").await()
      map.close()
      map.close()

      assertTrue(map.isClosed)
      assertFailsWith<InvalidStateException> {
        map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()).await()
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
          .await()

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
          .await()

      try {
        map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()).await()
        map.addStyleSourceJson("places", geoJsonSource()).await()

        assertEquals(SourceType.GEOJSON, map.styleSourceInfo("places").await()?.type)
        assertTrue(map.styleSourceIds().await().contains("places"))
        assertCommandCommitted(runtime, map.removeStyleSource("places").await())
        assertNull(map.styleSourceInfo("places").await())
        assertCommandFailed(map.removeStyleSource("places").await(), MaplibreStatus.NOT_FOUND)
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
          .await()
      lateinit var retainedInfo: SourceInfo
      val tileUrls =
        listOf("https://a.example.com/{z}/{x}/{y}.pbf", "https://b.example.com/{z}/{x}/{y}.pbf")
      val bounds = LatLngBounds(LatLng(-5.0, -10.0), LatLng(15.0, 20.0))
      try {
        assertCommandCommitted(
          runtime,
          map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()).await(),
        )
        assertCommandCommitted(
          runtime,
          map.addVectorSourceUrl("remote", "https://example.com/vector.json", null).await(),
        )
        val remote = assertNotNull(map.styleSourceInfo("remote").await())
        assertEquals("https://example.com/vector.json", remote.url)
        assertNull(remote.tileJson)

        assertCommandCommitted(
          runtime,
          map
            .addVectorSourceTiles(
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
            .await(),
        )

        retainedInfo = assertNotNull(map.styleSourceInfo("inline").await())
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
        assertCommandCommitted(runtime, map.removeStyleSource("inline").await())
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
          .await()

      try {
        assertCommandCommitted(
          runtime,
          map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()).await(),
        )
        assertCommandCommitted(
          runtime,
          map
            .addGeoJsonSourceUrl("remote-places", "https://example.com/places.geojson", null)
            .await(),
        )
        assertEquals(SourceType.GEOJSON, map.styleSourceInfo("remote-places").await()?.type)
        assertCommandCommitted(
          runtime,
          map.setGeoJsonSourceUrl("remote-places", "https://example.com/updated.geojson").await(),
        )

        val inlineOptions =
          GeoJsonSourceOptions().apply {
            minZoom = 0.0
            maxZoom = 14.0
            tolerance = 0.5
            tileSize = 256
            buffer = 64
            lineMetrics = true
          }
        GeoJsonSourceDataHandle.create(geoJsonData(), inlineOptions).use { data ->
          assertCommandCommitted(runtime, map.addGeoJsonSourceData("inline-places", data).await())
        }
        assertEquals(SourceType.GEOJSON, map.styleSourceInfo("inline-places").await()?.type)
        GeoJsonSourceDataHandle.create(
            ("{\"type\":\"Feature\",\"geometry\":{\"type\":\"LineString\"," +
                "\"coordinates\":[[0,0],[1,1]]},\"properties\":{}}")
              .encodeToByteArray(),
            inlineOptions,
          )
          .use { update ->
            assertCommandCommitted(
              runtime,
              map.setGeoJsonSourceData("inline-places", update).await(),
            )
          }

        GeoJsonSourceDataHandle.create(nearbyPoints(), clusterOptions()).use { clustered ->
          assertCommandCommitted(
            runtime,
            map.addGeoJsonSourceData("clustered-places", clustered).await(),
          )
        }
        assertEquals(SourceType.GEOJSON, map.styleSourceInfo("clustered-places").await()?.type)

        assertFailsWith<InvalidArgumentException> {
          map
            .addGeoJsonSourceUrl(
              "invalid-zooms",
              "https://example.com/places.geojson",
              GeoJsonSourceOptions().apply {
                minZoom = 12.0
                maxZoom = 4.0
              },
            )
            .await()
        }
        assertNull(map.styleSourceInfo("invalid-zooms").await())
        assertCommandFailed(
          map
            .addGeoJsonSourceUrl(
              "invalid-cluster-properties",
              "https://example.com/places.geojson",
              GeoJsonSourceOptions().apply {
                clusterProperties = "\"not an object\"".encodeToByteArray()
              },
            )
            .await(),
          MaplibreStatus.INVALID_ARGUMENT,
        )
        assertNull(map.styleSourceInfo("invalid-cluster-properties").await())
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
          .await()

      try {
        map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()).await()
        map
          .addCustomGeometrySource(
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
          .await()

        assertEquals(SourceType.CUSTOM_VECTOR, map.styleSourceInfo("custom-places").await()?.type)

        val tileId = CanonicalTileId(0, 0, 0)
        map.setCustomGeometrySourceTileData("custom-places", tileId, geoJsonData())
        map.invalidateCustomGeometrySourceTile("custom-places", tileId).await()
        map
          .invalidateCustomGeometrySourceRegion(
            "custom-places",
            LatLngBounds(LatLng(-1.0, -1.0), LatLng(1.0, 1.0)),
          )
          .await()

        assertCommandCommitted(runtime, map.removeStyleSource("custom-places").await())
        assertNull(map.styleSourceInfo("custom-places").await())
      } finally {
        map.close()
        runtime.close()
      }
    }

  @Test
  fun customMvtVectorSourcesCanBeManaged(): Unit =
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
          .await()

      try {
        map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()).await()
        map
          .addCustomMvtVectorSource(
            "custom-mvt",
            CustomMvtVectorSourceOptions(
                object : CustomMvtVectorSourceCallback {
                  override fun fetchTile(tileId: CanonicalTileId) {}
                }
              )
              .apply {
                minZoom = 0.0
                maxZoom = 14.0
              },
          )
          .await()

        assertEquals(SourceType.CUSTOM_MVT_VECTOR, map.styleSourceInfo("custom-mvt").await()?.type)

        val tileId = CanonicalTileId(0, 0, 0)
        map.setCustomMvtVectorSourceTileData("custom-mvt", tileId, ByteArray(0)).await()
        map.setCustomMvtVectorSourceTileError("custom-mvt", tileId, "tile missing").await()
        map.invalidateCustomMvtVectorSourceTile("custom-mvt", tileId).await()

        // A second source under the same ID is rejected, and the first one stays installed.
        assertCommandFailed(
          map
            .addCustomMvtVectorSource(
              "custom-mvt",
              CustomMvtVectorSourceOptions(
                object : CustomMvtVectorSourceCallback {
                  override fun fetchTile(tileId: CanonicalTileId) {}
                }
              ),
            )
            .await(),
          MaplibreStatus.INVALID_ARGUMENT,
        )

        assertCommandCommitted(runtime, map.removeStyleSource("custom-mvt").await())
        assertNull(map.styleSourceInfo("custom-mvt").await())
      } finally {
        map.close()
        runtime.close()
      }
    }

  // Every live custom source keeps its own callback state, past the ten-slot JavaCPP
  // function-pointer pool that per-source thunks would exhaust on Android.

  @Test
  fun elevenLiveCustomSourcesStayRegistered(): Unit =
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
          .await()

      try {
        map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()).await()
        val geometryOptions =
          CustomGeometrySourceOptions(
            object : CustomGeometrySourceCallback {
              override fun fetchTile(tileId: CanonicalTileId) {}
            }
          )
        val mvtOptions =
          CustomMvtVectorSourceOptions(
            object : CustomMvtVectorSourceCallback {
              override fun fetchTile(tileId: CanonicalTileId) {}
            }
          )
        val geometryIds = (1..11).map { "custom-$it" }
        val mvtIds = (1..11).map { "custom-mvt-$it" }
        geometryIds.forEach { map.addCustomGeometrySource(it, geometryOptions).await() }
        mvtIds.forEach { map.addCustomMvtVectorSource(it, mvtOptions).await() }
        (geometryIds + mvtIds).forEach { id -> assertNotNull(map.styleSourceInfo(id).await(), id) }

        assertCommandCommitted(runtime, map.removeStyleSource("custom-1").await())
        assertCommandCommitted(runtime, map.removeStyleSource("custom-mvt-1").await())
        map.addCustomGeometrySource("custom-12", geometryOptions).await()
        map.addCustomMvtVectorSource("custom-mvt-12", mvtOptions).await()
        assertNotNull(map.styleSourceInfo("custom-12").await())
        assertNotNull(map.styleSourceInfo("custom-mvt-12").await())
        assertNull(map.styleSourceInfo("custom-1").await())
        assertNull(map.styleSourceInfo("custom-mvt-1").await())
      } finally {
        map.close()
        runtime.close()
      }
    }

  @Test
  fun featureStateRoundTripsThroughTheMapStore(): Unit =
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
          .await()

      try {
        val selector = FeatureStateSelector("point").apply { featureId = "feature-1" }

        // The store answers before any source loads, and missing state reads as an empty object.
        assertEquals("{}", map.getFeatureState(selector).await().decodeToString())

        assertCommandCommitted(
          runtime,
          map
            .setFeatureState(selector, """{"hover":true,"radius":20}""".encodeToByteArray())
            .await(),
        )
        val stored = map.getFeatureState(selector).await().decodeToString()
        assertTrue(stored.contains("\"hover\":true"), stored)
        assertTrue(stored.contains("\"radius\":20"), stored)

        // State must be one JSON object.
        assertFailsWith<InvalidArgumentException> {
          map.setFeatureState(selector, "[]".encodeToByteArray()).await()
        }

        // A state key narrows the removal to that one member.
        assertCommandCommitted(
          runtime,
          map
            .removeFeatureState(
              FeatureStateSelector("point").apply {
                featureId = "feature-1"
                stateKey = "hover"
              }
            )
            .await(),
        )
        val afterRemove = map.getFeatureState(selector).await().decodeToString()
        assertFalse(afterRemove.contains("hover"), afterRemove)
        assertTrue(afterRemove.contains("\"radius\":20"), afterRemove)

        assertCommandCommitted(runtime, map.removeFeatureState(selector).await())
        assertEquals("{}", map.getFeatureState(selector).await().decodeToString())
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
          .await()

      try {
        assertCommandCommitted(
          runtime,
          map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()).await(),
        )
        assertCommandCommitted(
          runtime,
          map
            .addVectorSourceUrl(
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
            .await(),
        )
        assertCommandCommitted(
          runtime,
          map
            .addRasterSourceTiles(
              "satellite",
              listOf("https://example.com/raster/{z}/{x}/{y}.png"),
              TileSourceOptions().apply { tileSize = 256 },
            )
            .await(),
        )
        assertCommandCommitted(
          runtime,
          map
            .addRasterDemSourceTiles(
              "terrain",
              listOf("https://example.com/terrain/{z}/{x}/{y}.png"),
              TileSourceOptions().apply {
                tileSize = 512
                rasterDemEncoding = RasterDemEncoding.TERRARIUM
              },
            )
            .await(),
        )

        assertEquals(SourceType.VECTOR, map.styleSourceInfo("roads").await()?.type)
        val rasterInfo = assertNotNull(map.styleSourceInfo("satellite").await())
        assertEquals(SourceType.RASTER, rasterInfo.type)
        assertEquals(256, rasterInfo.tileSize)
        assertEquals(SourceType.RASTER_DEM, map.styleSourceInfo("terrain").await()?.type)
        assertEquals(
          RasterDemEncoding.TERRARIUM,
          map.styleSourceInfo("terrain").await()?.rasterDemEncoding,
        )
        assertTrue(
          map.styleSourceIds().await().containsAll(listOf("roads", "satellite", "terrain"))
        )
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
          .await()

      try {
        assertCommandCommitted(
          runtime,
          map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()).await(),
        )
        assertCommandCommitted(runtime, map.addStyleLayerJson(backgroundLayer(), "").await())
        assertCommandCommitted(runtime, map.addLocationIndicatorLayer("puck", "").await())
        assertCommandCommitted(
          runtime,
          map.setLocationIndicatorLocation("puck", LatLng(12.0, 34.0), 56.0).await(),
        )
        assertCommandCommitted(runtime, map.setLocationIndicatorBearing("puck", 78.0).await())
        assertCommandCommitted(runtime, map.setLocationIndicatorAccuracyRadius("puck", 9.0).await())
        assertCommandCommitted(runtime, map.moveStyleLayer("puck", "background").await())

        assertEquals("background", map.styleLayerInfo("background").await()?.type)
        assertNotNull(map.styleLayerInfo("puck").await())
        assertTrue(map.styleLayerIds().await().contains("background"))
        assertTrue(map.styleLayerIds().await().contains("puck"))
        assertTrue(
          map
            .styleLayerJson("background")
            .await()!!
            .decodeToString()
            .contains("\"type\":\"background\"")
        )
        assertCommandCommitted(
          runtime,
          map
            .setLayerProperty("background", "background-opacity", "0.5".encodeToByteArray())
            .await(),
        )
        assertEquals(
          "0.5",
          map.layerProperty("background", "background-opacity").await()?.decodeToString(),
        )
        assertCommandCommitted(
          runtime,
          map.setStyleLightProperty("anchor", "\"viewport\"".encodeToByteArray()).await(),
        )
        assertEquals("\"viewport\"", map.styleLightProperty("anchor").await()?.decodeToString())
        assertCommandCommitted(runtime, map.removeStyleLayer("background").await())
        assertCommandCommitted(runtime, map.removeStyleLayer("puck").await())
        assertNull(map.styleLayerInfo("background").await())
        assertCommandFailed(map.removeStyleLayer("background").await(), MaplibreStatus.NOT_FOUND)
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
          .await()

      try {
        val image = PremultipliedRgba8Image(1, 1, 4, byteArrayOf(1, 2, 3, 4))
        val options =
          StyleImageOptions().apply {
            pixelRatio = 2.0f
            sdf = true
          }

        map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()).await()
        map.setStyleImage("dot", image, options)

        val info = map.styleImageInfo("dot").await()
        assertEquals(1, info?.width)
        assertEquals(1, info?.height)
        assertEquals(4, info?.stride)
        assertEquals(4, info?.byteLength)
        assertEquals(2.0f, info?.pixelRatio)
        assertEquals(true, info?.sdf)
        assertEquals(image, map.copyStyleImagePremultipliedRgba8("dot").await()?.image)
        assertEquals(2.0f, map.copyStyleImagePremultipliedRgba8("dot").await()?.pixelRatio)
        assertEquals(true, map.copyStyleImagePremultipliedRgba8("dot").await()?.sdf)
        assertCommandCommitted(runtime, map.removeStyleImage("dot").await())
        assertNull(map.styleImageInfo("dot").await())
        assertCommandFailed(map.removeStyleImage("dot").await(), MaplibreStatus.NOT_FOUND)
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
          .await()

      try {
        val image = PremultipliedRgba8Image(1, 1, 4, byteArrayOf(1, 2, 3, 4))
        val coordinates = imageCoordinates()
        val moved = listOf(LatLng(1.0, 0.0), LatLng(1.0, 1.0), LatLng(0.0, 1.0), LatLng(0.0, 0.0))

        map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()).await()
        map.addImageSourceUrl("overlay", coordinates, "https://example.com/image.png").await()

        assertEquals(SourceType.IMAGE, map.styleSourceInfo("overlay").await()?.type)
        assertEquals(coordinates, map.imageSourceCoordinates("overlay").await())
        map.setImageSourceUrl("overlay", "https://example.com/updated-image.png").await()
        map.setImageSourceImage("overlay", image).await()
        map.setImageSourceCoordinates("overlay", moved).await()
        assertEquals(moved, map.imageSourceCoordinates("overlay").await())
        assertEquals(null, map.imageSourceCoordinates("missing-overlay").await())

        map.addImageSourceImage("inline-overlay", coordinates, image)
        assertEquals(SourceType.IMAGE, map.styleSourceInfo("inline-overlay").await()?.type)
      } finally {
        map.close()
        runtime.close()
      }
    }

  // The command completion generation fences a later snapshot: a snapshot at or past it
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
          .await()
          .use { map ->
            val debug = setOf(DebugOption.TILE_BORDERS, DebugOption.OVERDRAW)
            val generation = map.setDebugOptions(debug).await().generation.toLong()
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
          .await()
          .use { map ->
            assertEquals(emptySet<DebugOption>(), map.snapshot().debugOptions)
            assertFalse(map.snapshot().renderingStatsViewEnabled)

            assertCommandCommitted(runtime, map.setRenderingStatsViewEnabled(true).await())
            assertTrue(map.snapshot().renderingStatsViewEnabled)

            val viewport = ViewportOptions().apply { northOrientation = NorthOrientation.DOWN }
            assertCommandCommitted(runtime, map.setViewportOptions(viewport).await())
            assertEquals(NorthOrientation.DOWN, map.snapshot().viewportOptions.northOrientation)

            val tile =
              TileOptions().apply {
                prefetchZoomDelta = 3
                lodScale = 1.5
              }
            assertCommandCommitted(runtime, map.setTileOptions(tile).await())
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
            assertCommandCommitted(runtime, map.setBounds(bounds).await())
            val boundsSnapshot = map.snapshot().bounds
            assertEquals(2.0, boundsSnapshot.minZoom)
            assertEquals(15.0, boundsSnapshot.maxZoom)
            assertEquals(bounds.bounds, boundsSnapshot.bounds)

            val freeCamera =
              FreeCameraOptions().apply {
                position = Vec3(0.5, 0.5, 0.5)
                orientation = Quaternion(0.0, 0.0, 0.0, 1.0)
              }
            assertCommandCommitted(runtime, map.setFreeCameraOptions(freeCamera).await())
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

  private fun imageCoordinates(): List<LatLng> =
    listOf(LatLng(0.0, 0.0), LatLng(0.0, 1.0), LatLng(1.0, 1.0), LatLng(1.0, 0.0))

  private fun assertCommandCommitted(
    @Suppress("UNUSED_PARAMETER") runtime: RuntimeHandle,
    completion: CommandCompletion,
  ) {
    assertEquals(CommandDisposition.COMMITTED, completion.disposition)
    assertEquals(MaplibreStatus.OK, completion.status)
  }

  private fun assertCommandFailed(completion: CommandCompletion, status: MaplibreStatus) {
    assertEquals(CommandDisposition.FAILED, completion.disposition)
    assertEquals(status, completion.status)
    assertTrue(completion.diagnostic.isNotEmpty())
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
          .await()
          .use { map ->
            assertTrue(map.loadedStyleJson().await().isEmpty())
            assertEquals("", map.styleUrl().await())

            // The document reads back byte-for-byte.
            map.setStyleJson(styleJson.encodeToByteArray()).await()
            assertEquals(styleJson, map.loadedStyleJson().await().decodeToString())
            // Inline JSON clears the URL.
            assertEquals("", map.styleUrl().await())

            // setStyleUrl records request state before the load can succeed; the document
            // still reports the style that last parsed.
            map.setStyleUrl("https://example.com/style.json").await()
            assertEquals("https://example.com/style.json", map.styleUrl().await())
            assertEquals(styleJson, map.loadedStyleJson().await().decodeToString())
          }
      }
    }
}
