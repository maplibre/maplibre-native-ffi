package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertSame
import kotlin.test.assertTrue
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.BoundsConstraint
import org.maplibre.nativeffi.camera.CameraFitOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.CameraUpdate
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.camera.FreeCameraOptions
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.geo.Quaternion
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.geo.Vec3
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.render.PremultipliedRgba8Image
import org.maplibre.nativeffi.runtime.CommandCompletion
import org.maplibre.nativeffi.runtime.CommandDisposition
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions
import org.maplibre.nativeffi.runtime.awaitCommitted
import org.maplibre.nativeffi.runtime.runSuspendTest
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
  fun layerBaseAccessorsReachNativeThroughDowncalls(): Unit = runSuspendTest {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 64
            height = 64
          },
        )
        .use { map ->
          map
            .setStyleJson(
              ("{\"version\":8,\"sources\":{\"geo\":{\"type\":\"geojson\",\"data\":" +
                  "{\"type\":\"FeatureCollection\",\"features\":[]}}},\"layers\":[" +
                  "{\"id\":\"bg\",\"type\":\"background\"}," +
                  "{\"id\":\"fill\",\"type\":\"fill\",\"source\":\"geo\"}]}")
                .encodeToByteArray()
            )
            .awaitCommitted()

          assertEquals("", map.layerSourceLayer("fill").await())
          // An unset source layer reads as absent through the copied layer info.
          assertNull(assertNotNull(map.styleLayerInfo("fill").await()).sourceLayer)
          map.setLayerSourceLayer("fill", "roads").awaitCommitted()
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

          map.setLayerMinZoom("fill", 4.0).awaitCommitted()
          map.setLayerMaxZoom("fill", 12.5).awaitCommitted()
          map.setLayerVisibility("fill", StyleLayerVisibility.NONE).awaitCommitted()
          val bounded = assertNotNull(map.styleLayerInfo("fill").await())
          assertEquals(4.0, bounded.minZoom)
          assertEquals(12.5, bounded.maxZoom)
          assertEquals(StyleLayerVisibility.NONE, bounded.visibility)

          // A sourceless layer reports absent source fields.
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
  fun styleTransitionOptionsRoundTripThroughDowncalls(): Unit = runSuspendTest {
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
          map
            .setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}".encodeToByteArray())
            .awaitCommitted()
          val parsed = map.styleTransitionOptions().await()
          assertEquals(300.0, parsed.durationMs)
          assertNull(parsed.delayMs)

          map.setStyleJson(transitionStyleJson.encodeToByteArray()).awaitCommitted()
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
          map.setStyleTransitionOptions(options).awaitCommitted()
          assertEquals(options, map.styleTransitionOptions().await())

          // Omitting the flag leaves the cross-fade on.
          map
            .setStyleTransitionOptions(StyleTransitionOptions().apply { durationMs = 250.0 })
            .awaitCommitted()
          assertEquals(true, map.styleTransitionOptions().await().enablePlacementTransitions)

          // Loading a style replaces the override with what that style declares.
          map.setStyleJson(transitionStyleJson.encodeToByteArray()).awaitCommitted()
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
  fun mapCreateStyleAndCloseRetainsRuntime(): Unit = runSuspendTest {
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
    assertFailsWith<InvalidStateException> { runtime.close() }

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
  fun mapSizeReportsCreationExtentAndPixelRatio(): Unit = runSuspendTest {
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

    // A resize that keeps the creation scale factor publishes the new extent.
    map.resize(MapSize(320, 200, 2.0)).awaitCommitted()
    assertEquals(MapSize(320, 200, 2.0), map.snapshot().size)

    // A different scale factor is rejected, and the published extent does not move.
    assertFailsWith<InvalidArgumentException> { map.resize(MapSize(320, 200, 1.0)).await() }
    assertEquals(MapSize(320, 200, 2.0), map.snapshot().size)

    map.close()
    runtime.close()
  }

  @Test
  fun styleSourceJsonCanBeAddedInspectedListedAndRemoved(): Unit = runSuspendTest {
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
      map.removeStyleSource("places").awaitCommitted()
      assertNull(map.styleSourceInfo("places").await())
      assertCommandFailed(map.removeStyleSource("places").await(), MaplibreStatus.NOT_FOUND)
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun styleSourceVolatilityCanBeToggled(): Unit = runSuspendTest {
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

      assertFalse(assertNotNull(map.styleSourceInfo("places").await()).volatileSource)
      map.setStyleSourceVolatile("places", true).awaitCommitted()
      assertTrue(assertNotNull(map.styleSourceInfo("places").await()).volatileSource)
      map.setStyleSourceVolatile("places", false).awaitCommitted()
      assertFalse(assertNotNull(map.styleSourceInfo("places").await()).volatileSource)
      assertCommandFailed(
        map.setStyleSourceVolatile("missing", true).await(),
        MaplibreStatus.NOT_FOUND,
      )
    } finally {
      map.close()
      runtime.close()
    }
  }

  // BND-109.

  @Test
  fun styleSourceInfoCopiesUrlAndInlineTileMetadataPastNativeLifetime(): Unit = runSuspendTest {
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
      map
        .setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray())
        .awaitCommitted()
      map.addVectorSourceUrl("remote", "https://example.com/vector.json", null).awaitCommitted()
      val remote = assertNotNull(map.styleSourceInfo("remote").await())
      assertEquals("https://example.com/vector.json", remote.url)
      assertNull(remote.tileJson)

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
        .awaitCommitted()

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
      map.removeStyleSource("inline").awaitCommitted()
    } finally {
      map.close()
      runtime.close()
    }

    assertEquals(tileUrls, retainedInfo.tileJson?.tileUrls)
    assertEquals(bounds, retainedInfo.tileJson?.bounds)
  }

  @Test
  fun styleSourceUrlAttributionAndTileUrlsCopyIndependently(): Unit = runSuspendTest {
    val tileUrls =
      listOf("https://a.example.com/{z}/{x}/{y}.pbf", "https://b.example.com/{z}/{x}/{y}.pbf")
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
          map.setStyleJson(EMPTY_STYLE_JSON.encodeToByteArray()).awaitCommitted()
          map
            .addVectorSourceTiles(
              "inline",
              tileUrls,
              TileSourceOptions().apply { attribution = "inline attribution" },
            )
            .awaitCommitted()
          map.addVectorSourceUrl("remote", "https://example.com/vector.json", null).awaitCommitted()

          // An inline tile source carries its tile URLs and no source URL.
          assertEquals("inline attribution", map.styleSourceAttribution("inline").await())
          assertNull(map.styleSourceUrl("inline").await())
          assertEquals(tileUrls, map.styleSourceTileUrls("inline").await())

          // A URL-backed tile source carries the reverse. Its attribution arrives with the
          // TileJSON the URL resolves to, so it reads as absent until that load finishes.
          assertEquals("https://example.com/vector.json", map.styleSourceUrl("remote").await())
          assertEquals(emptyList(), map.styleSourceTileUrls("remote").await())
          assertNull(map.styleSourceAttribution("remote").await())

          // No source carries this ID.
          assertNull(map.styleSourceAttribution("missing").await())
          assertNull(map.styleSourceUrl("missing").await())
          assertEquals(emptyList(), map.styleSourceTileUrls("missing").await())
        }
    }
  }

  @Test
  fun geoJsonSourcesCanBeAddedAndUpdated(): Unit = runSuspendTest {
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
      map
        .setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray())
        .awaitCommitted()
      map
        .addGeoJsonSourceUrl("remote-places", "https://example.com/places.geojson", null)
        .awaitCommitted()
      assertEquals(SourceType.GEOJSON, map.styleSourceInfo("remote-places").await()?.type)
      map
        .setGeoJsonSourceUrl("remote-places", "https://example.com/updated.geojson")
        .awaitCommitted()

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
        map.addGeoJsonSourceData("inline-places", data).awaitCommitted()
      }
      assertEquals(SourceType.GEOJSON, map.styleSourceInfo("inline-places").await()?.type)
      GeoJsonSourceDataHandle.create(
          ("{\"type\":\"Feature\",\"geometry\":{\"type\":\"LineString\"," +
              "\"coordinates\":[[0,0],[1,1]]},\"properties\":{}}")
            .encodeToByteArray(),
          inlineOptions,
        )
        .use { update -> map.setGeoJsonSourceData("inline-places", update).awaitCommitted() }

      GeoJsonSourceDataHandle.create(nearbyPoints(), clusterOptions()).use { clustered ->
        map.addGeoJsonSourceData("clustered-places", clustered).awaitCommitted()
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
  fun customGeometrySourcesCanBeManaged(): Unit = runSuspendTest {
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

      map.removeStyleSource("custom-places").awaitCommitted()
      assertNull(map.styleSourceInfo("custom-places").await())
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun customMvtVectorSourcesCanBeManaged(): Unit = runSuspendTest {
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

      map.removeStyleSource("custom-mvt").awaitCommitted()
      assertNull(map.styleSourceInfo("custom-mvt").await())
    } finally {
      map.close()
      runtime.close()
    }
  }

  // Every live custom source keeps its own callback state, past the ten-slot JavaCPP
  // function-pointer pool that per-source thunks would exhaust on Android.

  @Test
  fun elevenLiveCustomSourcesStayRegistered(): Unit = runSuspendTest {
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

      map.removeStyleSource("custom-1").awaitCommitted()
      map.removeStyleSource("custom-mvt-1").awaitCommitted()
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
  fun featureStateRoundTripsThroughTheMapStore(): Unit = runSuspendTest {
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

      map
        .setFeatureState(selector, """{"hover":true,"radius":20}""".encodeToByteArray())
        .awaitCommitted()
      val stored = map.getFeatureState(selector).await().decodeToString()
      assertTrue(stored.contains("\"hover\":true"), stored)
      assertTrue(stored.contains("\"radius\":20"), stored)

      // State must be one JSON object.
      assertFailsWith<InvalidArgumentException> {
        map.setFeatureState(selector, "[]".encodeToByteArray()).await()
      }

      // A state key narrows the removal to that one member.
      map
        .removeFeatureState(
          FeatureStateSelector("point").apply {
            featureId = "feature-1"
            stateKey = "hover"
          }
        )
        .awaitCommitted()
      val afterRemove = map.getFeatureState(selector).await().decodeToString()
      assertFalse(afterRemove.contains("hover"), afterRemove)
      assertTrue(afterRemove.contains("\"radius\":20"), afterRemove)

      map.removeFeatureState(selector).awaitCommitted()
      assertEquals("{}", map.getFeatureState(selector).await().decodeToString())
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun tileSourcesCanBeAddedAndInspected(): Unit = runSuspendTest {
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
      map
        .setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray())
        .awaitCommitted()
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
        .awaitCommitted()
      map
        .addRasterSourceTiles(
          "satellite",
          listOf("https://example.com/raster/{z}/{x}/{y}.png"),
          TileSourceOptions().apply { tileSize = 256 },
        )
        .awaitCommitted()
      map
        .addRasterDemSourceTiles(
          "terrain",
          listOf("https://example.com/terrain/{z}/{x}/{y}.png"),
          TileSourceOptions().apply {
            tileSize = 512
            rasterDemEncoding = RasterDemEncoding.TERRARIUM
          },
        )
        .awaitCommitted()

      assertEquals(SourceType.VECTOR, map.styleSourceInfo("roads").await()?.type)
      val rasterInfo = assertNotNull(map.styleSourceInfo("satellite").await())
      assertEquals(SourceType.RASTER, rasterInfo.type)
      assertEquals(256, rasterInfo.tileSize)
      assertEquals(SourceType.RASTER_DEM, map.styleSourceInfo("terrain").await()?.type)
      assertEquals(
        RasterDemEncoding.TERRARIUM,
        map.styleSourceInfo("terrain").await()?.rasterDemEncoding,
      )
      assertTrue(map.styleSourceIds().await().containsAll(listOf("roads", "satellite", "terrain")))
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun styleLayerJsonCanBeAddedInspectedListedAndRemoved(): Unit = runSuspendTest {
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
      map
        .setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray())
        .awaitCommitted()
      map.addStyleLayerJson(backgroundLayer(), "").awaitCommitted()
      map.addLocationIndicatorLayer("puck", "").awaitCommitted()
      map.setLocationIndicatorLocation("puck", LatLng(12.0, 34.0), 56.0).awaitCommitted()
      map.setLocationIndicatorBearing("puck", 78.0).awaitCommitted()
      map.setLocationIndicatorAccuracyRadius("puck", 9.0).awaitCommitted()
      map.moveStyleLayer("puck", "background").awaitCommitted()

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
      map
        .setLayerProperty("background", "background-opacity", "0.5".encodeToByteArray())
        .awaitCommitted()
      assertEquals(
        "0.5",
        map.layerProperty("background", "background-opacity").await()?.decodeToString(),
      )
      map.setStyleLightProperty("anchor", "\"viewport\"".encodeToByteArray()).awaitCommitted()
      assertEquals("\"viewport\"", map.styleLightProperty("anchor").await()?.decodeToString())
      map.removeStyleLayer("background").awaitCommitted()
      map.removeStyleLayer("puck").awaitCommitted()
      assertNull(map.styleLayerInfo("background").await())
      assertCommandFailed(map.removeStyleLayer("background").await(), MaplibreStatus.NOT_FOUND)
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun styleImageCanBeSetCopiedInspectedAndRemoved(): Unit = runSuspendTest {
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
      // The pixel copy carries the bytes alone; the metadata stays in the info query.
      assertContentEquals(image.pixels, map.copyStyleImagePremultipliedRgba8("dot").await())
      assertNull(map.copyStyleImagePremultipliedRgba8("missing").await())
      map.removeStyleImage("dot").awaitCommitted()
      assertNull(map.styleImageInfo("dot").await())
      assertNull(map.copyStyleImagePremultipliedRgba8("dot").await())
      assertCommandFailed(map.removeStyleImage("dot").await(), MaplibreStatus.NOT_FOUND)
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun imageSourcesCanBeAddedUpdatedAndInspected(): Unit = runSuspendTest {
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
  fun committedCommandGenerationFencesTheSnapshot(): Unit = runSuspendTest {
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
          val generation = map.setDebugOptions(debug).awaitCommitted().generation
          assertTrue(generation > 0L, "committed command must publish a generation")
          val snapshot = map.snapshot()
          assertTrue(snapshot.generation >= generation)
          assertEquals(debug, snapshot.debugOptions)
        }
    }
  }

  @Test
  fun snapshotFieldsRoundTripThroughTheirSetCommands(): Unit = runSuspendTest {
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

          map.setRenderingStatsViewEnabled(true).awaitCommitted()
          assertTrue(map.snapshot().renderingStatsViewEnabled)

          val viewport = ViewportOptions().apply { northOrientation = NorthOrientation.DOWN }
          map.setViewportOptions(viewport).awaitCommitted()
          assertEquals(NorthOrientation.DOWN, map.snapshot().viewportOptions.northOrientation)

          val tile =
            TileOptions().apply {
              prefetchZoomDelta = 3
              lodScale = 1.5
            }
          map.setTileOptions(tile).awaitCommitted()
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
          map.setBounds(bounds).awaitCommitted()
          val boundsSnapshot = map.snapshot().bounds
          assertEquals(2.0, boundsSnapshot.minZoom)
          assertEquals(15.0, boundsSnapshot.maxZoom)
          assertEquals(bounds.bounds, boundsSnapshot.bounds)

          val freeCamera =
            FreeCameraOptions().apply {
              position = Vec3(0.5, 0.5, 0.5)
              orientation = Quaternion(0.0, 0.0, 0.0, 1.0)
            }
          map.setFreeCameraOptions(freeCamera).awaitCommitted()
          val freeCameraSnapshot = map.snapshot().freeCameraOptions
          kotlin.test.assertNotNull(freeCameraSnapshot.position)
          kotlin.test.assertNotNull(freeCameraSnapshot.orientation)

          val projectionMode =
            ProjectionModeOptions().apply {
              axonometric = true
              xSkew = 0.5
              ySkew = 0.25
            }
          map.setProjectionMode(projectionMode).awaitCommitted()
          val projectionModeSnapshot = map.snapshot().projectionMode
          assertEquals(true, projectionModeSnapshot.axonometric)
          assertEquals(0.5, projectionModeSnapshot.xSkew)
          assertEquals(0.25, projectionModeSnapshot.ySkew)

          // The debug dump writes to the log; the map keeps serving commands after it.
          map.dumpDebugLogs().awaitCommitted()
          assertTrue(map.snapshot().renderingStatsViewEnabled)
        }
    }
  }

  @Test
  fun repaintIsAcceptedOnlyByAContinuousMap(): Unit = runSuspendTest {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 64
            height = 64
          },
        )
        .await()
        .use { map -> map.requestRepaint().awaitCommitted() }

      MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 64
            height = 64
            mapMode = MapMode.STATIC
          },
        )
        .await()
        .use { map -> assertFailsWith<InvalidStateException> { map.requestRepaint().await() } }
    }
  }

  @Test
  fun cameraFitAndBoundsQueriesResolveBehindEarlierCommands(): Unit = runSuspendTest {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 1024
            height = 512
            scaleFactor = 1.0
            mapMode = MapMode.STATIC
          },
        )
        .await()
        .use { map ->
          val camera =
            CameraOptions().apply {
              center = LatLng(10.0, 20.0)
              zoom = 3.0
            }
          map.updateCamera(CameraUpdate(camera = camera)).awaitCommitted()

          val fit =
            CameraFitOptions().apply {
              padding = EdgeInsets.ZERO
              bearing = 0.0
              pitch = 0.0
            }
          val bounds = LatLngBounds(LatLng(-1.0, -2.0), LatLng(1.0, 2.0))

          // Fitting the bounds and fitting its two corners name the same camera.
          val fromBounds = map.cameraForLatLngBounds(bounds, fit).await()
          val fromCoordinates =
            map.cameraForLatLngs(listOf(bounds.southwest, bounds.northeast), fit).await()
          val boundsCenter = assertNotNull(fromBounds.center)
          assertEquals(0.0, boundsCenter.latitude, 1e-6)
          assertEquals(0.0, boundsCenter.longitude, 1e-6)
          assertEquals(fromBounds.center, fromCoordinates.center)
          assertEquals(fromBounds.zoom, fromCoordinates.zoom)

          // A single point fits at the point itself.
          val fromGeometry = map.cameraForGeometry(pointGeometry(), fit).await()
          val geometryCenter = assertNotNull(fromGeometry.center)
          assertEquals(0.0, geometryCenter.latitude, 1e-6)
          assertEquals(0.0, geometryCenter.longitude, 1e-6)

          // The wrapped bounds cover the camera's own center.
          val covered = map.latLngBoundsForCamera(camera).await()
          assertTrue(covered.southwest.latitude <= 10.0 && covered.northeast.latitude >= 10.0)
          assertTrue(covered.southwest.longitude in -180.0..180.0)
          assertTrue(covered.northeast.longitude in -180.0..180.0)

          // A viewport that straddles the antimeridian reads its east edge past 180.
          val antimeridian =
            CameraOptions().apply {
              center = LatLng(0.0, 179.0)
              zoom = 3.0
            }
          val wrappedAcrossSeam = map.latLngBoundsForCamera(antimeridian).await()
          val unwrappedAcrossSeam = map.latLngBoundsForCameraUnwrapped(antimeridian).await()
          assertTrue(wrappedAcrossSeam.southwest.longitude in -180.0..180.0)
          assertTrue(wrappedAcrossSeam.northeast.longitude in -180.0..180.0)
          // The unwrapped east edge stays in the world copy it was read from, so the span
          // is the 90 degrees the viewport covers rather than the 270 the wrapped hull spans.
          assertTrue(unwrappedAcrossSeam.northeast.longitude > 180.0)
          assertEquals(
            90.0,
            unwrappedAcrossSeam.northeast.longitude - unwrappedAcrossSeam.southwest.longitude,
            1e-3,
          )
        }
    }
  }

  /** A GeoJSON point at Null Island, for the geometry-fitting queries. */
  private fun pointGeometry(): ByteArray =
    "{\"type\":\"Point\",\"coordinates\":[0,0]}".encodeToByteArray()

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

  // BND-103.

  @Test
  fun projectionUnwrappedConversionPreservesVisibleWorldCopy(): Unit = runSuspendTest {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 1024
            height = 512
            scaleFactor = 1.0
            mapMode = MapMode.STATIC
          },
        )
        .await()

    try {
      map
        .updateCamera(
          CameraUpdate(
            camera =
              CameraOptions().apply {
                center = LatLng(0.0, 179.0)
                zoom = 0.0
              }
          )
        )
        .await()

      val projection = map.createProjection().await()
      try {
        // The right edge of the viewport sits in the next world copy at this camera.
        val rightEdge = ScreenPoint(1023.0, 256.0)
        val wrapped = projection.latLngForPixel(rightEdge)
        val unwrapped = projection.latLngForPixelUnwrapped(rightEdge)

        assertTrue(wrapped.longitude in -180.0..180.0)
        assertTrue(unwrapped.longitude > 180.0, "the right edge sits in a later world copy")
        val worldCopies = (unwrapped.longitude - wrapped.longitude) / 360.0
        assertEquals(kotlin.math.round(worldCopies), worldCopies, 1e-9)
        assertEquals(wrapped.latitude, unwrapped.latitude, 1e-9)
      } finally {
        projection.close()
      }
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun mapCoordinateConversionsRoundTrip(): Unit = runSuspendTest {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 128
            height = 128
            scaleFactor = 1.0
            mapMode = MapMode.STATIC
          },
        )
        .await()

    try {
      val coordinate = LatLng(0.0, 0.0)
      val point = map.pixelForLatLng(coordinate).await()
      val roundTrip = map.latLngForPixel(point).await()
      assertEquals(coordinate.latitude, roundTrip.latitude, 1e-6)
      assertEquals(coordinate.longitude, roundTrip.longitude, 1e-6)

      val coordinates = listOf(LatLng(0.0, 0.0), LatLng(10.0, 20.0))
      val points = map.pixelsForLatLngs(coordinates).await()
      assertEquals(coordinates.size, points.size)
      val batchRoundTrips = map.latLngsForPixels(points).await()
      assertEquals(coordinates.size, batchRoundTrips.size)
      coordinates.zip(batchRoundTrips).forEach { (expected, actual) ->
        assertEquals(expected.latitude, actual.latitude, 1e-6)
        assertEquals(expected.longitude, actual.longitude, 1e-6)
      }

      // An empty batch still queues one query and answers with an empty list.
      assertEquals(emptyList(), map.pixelsForLatLngs(emptyList()).await())
      assertEquals(emptyList(), map.latLngsForPixels(emptyList()).await())
      assertEquals(emptyList(), map.latLngsForPixelsUnwrapped(emptyList()).await())
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun mapUnwrappedConversionsPreserveVisibleWorldCopies(): Unit = runSuspendTest {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 1024
            height = 512
            scaleFactor = 1.0
            mapMode = MapMode.STATIC
          },
        )
        .await()

    try {
      map
        .updateCamera(
          CameraUpdate(
            camera =
              CameraOptions().apply {
                center = LatLng(0.0, 180.0)
                zoom = 0.0
              }
          )
        )
        .await()

      // The viewport is two world copies wide, so its edges name the same wrapped longitude
      // in different copies.
      val points = listOf(ScreenPoint(0.0, 256.0), ScreenPoint(1024.0, 256.0))
      val wrapped = map.latLngsForPixels(points).await()
      val unwrapped = map.latLngsForPixelsUnwrapped(points).await()

      assertTrue(wrapped.all { it.longitude in -180.0..180.0 })
      assertTrue(unwrapped[1].longitude - unwrapped[0].longitude > 360.0)
      assertTrue(map.latLngForPixel(points[1]).await().longitude in -180.0..180.0)
      assertEquals(
        unwrapped[1].longitude,
        map.latLngForPixelUnwrapped(points[1]).await().longitude,
        1e-10,
      )
    } finally {
      map.close()
      runtime.close()
    }
  }

  private fun imageCoordinates(): List<LatLng> =
    listOf(LatLng(0.0, 0.0), LatLng(0.0, 1.0), LatLng(1.0, 1.0), LatLng(1.0, 0.0))

  private fun assertCommandFailed(completion: CommandCompletion, status: MaplibreStatus) {
    assertEquals(CommandDisposition.FAILED, completion.disposition)
    assertEquals(status, completion.status)
    assertTrue(completion.diagnostic.isNotEmpty())
  }

  @Test
  fun loadedStyleDocumentAndUrlReadBackWhatWasLoaded(): Unit = runSuspendTest {
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
