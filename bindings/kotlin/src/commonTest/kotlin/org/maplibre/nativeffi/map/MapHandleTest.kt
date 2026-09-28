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
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.generated.BoundOptions
import org.maplibre.nativeffi.generated.CameraFitOptions
import org.maplibre.nativeffi.generated.CameraOptions
import org.maplibre.nativeffi.generated.CameraUpdate
import org.maplibre.nativeffi.generated.CanonicalTileId
import org.maplibre.nativeffi.generated.CommandDisposition
import org.maplibre.nativeffi.generated.CustomGeometrySourceOptions
import org.maplibre.nativeffi.generated.CustomMvtVectorSourceOptions
import org.maplibre.nativeffi.generated.EdgeInsets
import org.maplibre.nativeffi.generated.FeatureStateSelector
import org.maplibre.nativeffi.generated.FreeCameraOptions
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.GeojsonSourceOptions
import org.maplibre.nativeffi.generated.LatLng
import org.maplibre.nativeffi.generated.LatLngBounds
import org.maplibre.nativeffi.generated.LogicalExtent
import org.maplibre.nativeffi.generated.MapDebugOption
import org.maplibre.nativeffi.generated.MapMode
import org.maplibre.nativeffi.generated.MapTileOptions
import org.maplibre.nativeffi.generated.MapViewportOptions
import org.maplibre.nativeffi.generated.NorthOrientation
import org.maplibre.nativeffi.generated.PremultipliedRgba8Image
import org.maplibre.nativeffi.generated.ProjectionMode
import org.maplibre.nativeffi.generated.Quaternion
import org.maplibre.nativeffi.generated.ScreenPoint
import org.maplibre.nativeffi.generated.StyleImageOptions
import org.maplibre.nativeffi.generated.StyleLayerEntry
import org.maplibre.nativeffi.generated.StyleLayerVisibility
import org.maplibre.nativeffi.generated.StyleRasterDemEncoding
import org.maplibre.nativeffi.generated.StyleSourceResult
import org.maplibre.nativeffi.generated.StyleSourceType
import org.maplibre.nativeffi.generated.StyleTileScheme
import org.maplibre.nativeffi.generated.StyleTileSourceOptions
import org.maplibre.nativeffi.generated.StyleVectorTileEncoding
import org.maplibre.nativeffi.generated.Vec3
import org.maplibre.nativeffi.runtime.CommandCompletion
import org.maplibre.nativeffi.runtime.awaitCommitted
import org.maplibre.nativeffi.runtime.runSuspendTest
import org.maplibre.nativeffi.runtime.use

class MapHandleTest {

  // global-state lifetime and copied JSON values.
  @Test
  fun globalStateUsesStyleDefaultsAndResetsOnStyleReplacement(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      runtime.mapCreate(GeneratedApi.mapOptionsDefault()).await().use { map ->
        assertEquals("{}", map.getGlobalState().await().decodeToString())
        assertCommandFailed(
          map.setGlobalStateProperty("theme", "true".encodeToByteArray()).await(),
          MaplibreStatus.INVALID_STATE,
        )
        val style =
          """{"version":8,"sources":{},"layers":[],"state":{"theme":{"default":"light"}}}"""
            .encodeToByteArray()
        map.setStyleJson(style).awaitCommitted()
        assertEquals("""{"theme":"light"}""", map.getGlobalState().await().decodeToString())
        val input = """["dark",{"enabled":true}]""".encodeToByteArray()
        val submitted = map.setGlobalStateProperty("theme", input)
        input.fill(0)
        submitted.awaitCommitted()
        val snapshot = map.getGlobalState().await()
        assertEquals("""{"theme":["dark",{"enabled":true}]}""", snapshot.decodeToString())
        assertCommandFailed(
          map.setGlobalStateProperty("theme", "[".encodeToByteArray()).await(),
          MaplibreStatus.INVALID_ARGUMENT,
        )
        map.setGlobalStateProperty("theme", "null".encodeToByteArray()).awaitCommitted()
        assertEquals("""{"theme":"light"}""", map.getGlobalState().await().decodeToString())
        assertEquals("""{"theme":["dark",{"enabled":true}]}""", snapshot.decodeToString())
        map
          .setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray())
          .awaitCommitted()
        assertEquals("{}", map.getGlobalState().await().decodeToString())
        map.setGlobalStateProperty("theme", "false".encodeToByteArray()).awaitCommitted()
        map.setGlobalStateProperty("theme", "null".encodeToByteArray()).awaitCommitted()
        assertEquals("""{"theme":null}""", map.getGlobalState().await().decodeToString())
      }
    }
  }

  @Test
  fun layerBaseAccessorsReachNativeThroughDowncalls(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u)
            )
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

          assertNull(map.copyLayerSourceLayer("fill").await())
          // An unset source layer reads as absent through the copied layer info.
          assertNull(assertNotNull(map.getStyleLayerInfo("fill").await()).sourceLayer)
          map.setLayerSourceLayer("fill", "roads").awaitCommitted()
          assertEquals("roads", map.copyLayerSourceLayer("fill").await())
          assertEquals("geo", map.copyLayerSourceId("fill").await())

          // A background layer takes no source.
          assertCommandFailed(
            map.setLayerSourceLayer("bg", "roads").await(),
            MaplibreStatus.INVALID_ARGUMENT,
          )
          assertNull(map.copyLayerSourceLayer("bg").await())

          // An unset zoom range crosses the boundary as infinities.
          val unbounded = assertNotNull(map.getStyleLayerInfo("fill").await())
          assertEquals("fill", unbounded.info.type)
          assertEquals(Double.NEGATIVE_INFINITY, unbounded.info.minZoom)
          assertEquals(Double.POSITIVE_INFINITY, unbounded.info.maxZoom)
          assertEquals(StyleLayerVisibility.VISIBLE, unbounded.info.visibility)
          // The info's source flags feed the copy operations.
          assertEquals("geo", unbounded.sourceId)
          assertEquals("roads", unbounded.sourceLayer)

          map.setLayerMinZoom("fill", 4.0).awaitCommitted()
          map.setLayerMaxZoom("fill", 12.5).awaitCommitted()
          map.setLayerVisibility("fill", StyleLayerVisibility.NONE).awaitCommitted()
          val bounded = assertNotNull(map.getStyleLayerInfo("fill").await())
          assertEquals(4.0, bounded.info.minZoom)
          assertEquals(12.5, bounded.info.maxZoom)
          assertEquals(StyleLayerVisibility.NONE, bounded.info.visibility)

          // A sourceless layer reports absent source fields.
          val background = assertNotNull(map.getStyleLayerInfo("bg").await())
          assertEquals("background", background.info.type)
          assertNull(background.sourceId)
          assertNull(background.sourceLayer)

          // No layer carries this ID.
          assertNull(map.getStyleLayerInfo("missing").await())

          assertCommandFailed(
            map.setLayerVisibility("fill", StyleLayerVisibility(900u)).await(),
            MaplibreStatus.INVALID_ARGUMENT,
          )
          assertEquals(
            StyleLayerVisibility.NONE,
            map.getStyleLayerInfo("fill").await()?.info?.visibility,
          )
        }
    }
  }

  @Test
  fun styleTransitionOptionsRoundTripThroughDowncalls(): Unit = runSuspendTest {
    val transitionStyleJson =
      "{\"version\":8,\"transition\":{\"duration\":750,\"delay\":100}," +
        "\"sources\":{},\"layers\":[]}"
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u)
            )
        )
        .use { map ->
          // Duration and delay are absent until a style loads; the placement flag always
          // holds a value.
          val empty = map.getStyleTransitionOptions().await()
          assertNull(empty.durationMs)
          assertNull(empty.delayMs)
          assertEquals(true, empty.enablePlacementTransitions)

          // The style parser supplies a 300ms default duration.
          map
            .setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}".encodeToByteArray())
            .awaitCommitted()
          val parsed = map.getStyleTransitionOptions().await()
          assertEquals(300.0, parsed.durationMs)
          assertNull(parsed.delayMs)

          map.setStyleJson(transitionStyleJson.encodeToByteArray()).awaitCommitted()
          val declared = map.getStyleTransitionOptions().await()
          assertEquals(750.0, declared.durationMs)
          assertEquals(100.0, declared.delayMs)
          assertEquals(true, declared.enablePlacementTransitions)

          // A present zero stays distinguishable from an absent field, and an absent
          // field
          // clears what the style declared.
          val options =
            GeneratedApi.styleTransitionOptionsDefault()
              .copy(durationMs = 0.0, enablePlacementTransitions = false)
          map.setStyleTransitionOptions(options).awaitCommitted()
          assertEquals(options, map.getStyleTransitionOptions().await())

          // Omitting the flag leaves the cross-fade on.
          map
            .setStyleTransitionOptions(
              GeneratedApi.styleTransitionOptionsDefault().copy(durationMs = 250.0)
            )
            .awaitCommitted()
          assertEquals(true, map.getStyleTransitionOptions().await().enablePlacementTransitions)

          // Loading a style replaces the override with what that style declares.
          map.setStyleJson(transitionStyleJson.encodeToByteArray()).awaitCommitted()
          assertEquals(declared, map.getStyleTransitionOptions().await())

          assertCommandFailed(
            map
              .setStyleTransitionOptions(
                GeneratedApi.styleTransitionOptionsDefault().copy(delayMs = -1.0)
              )
              .await(),
            MaplibreStatus.INVALID_ARGUMENT,
          )
          assertEquals(declared, map.getStyleTransitionOptions().await())
        }
    }
  }

  @Test
  fun mapCreateStyleAndCloseRetainsRuntime(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map =
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault()
                  .initialExtent
                  .copy(width = 64u, height = 64u, scaleFactor = 1.0),
              mapMode = MapMode.STATIC,
            )
        )
        .await()

    assertFalse(map.isClosed)
    assertSame(runtime, map.runtime())
    assertFailsWith<InvalidStateException> { runtime.close().await() }

    map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()).await()
    map.setStyleUrl("https://example.com/style.json").await()
    map.close().await()
    map.close().await()

    assertTrue(map.isClosed)
    assertFailsWith<InvalidStateException> {
      map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()).await()
    }
    runtime.close().await()
    assertTrue(runtime.isClosed)
  }

  @Test
  fun mapSizeReportsCreationExtentAndPixelRatio(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map =
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault()
                  .initialExtent
                  .copy(width = 512u, height = 256u, scaleFactor = 2.0)
            )
        )
        .await()

    val snapshot = map.snapshotGet()
    val size = snapshot.logicalExtent
    assertEquals(512u, size.width)
    assertEquals(256u, size.height)
    assertEquals(2.0, size.scaleFactor)
    assertEquals(LogicalExtent(512u, 256u, 2.0), size)
    assertEquals(LogicalExtent(512u, 256u, 2.0).hashCode(), size.hashCode())

    // A resize that keeps the creation scale factor publishes the new extent.
    map.resize(LogicalExtent(320u, 200u, 2.0)).awaitCommitted()
    assertEquals(LogicalExtent(320u, 200u, 2.0), map.snapshotGet().logicalExtent)

    // A different scale factor is rejected, and the published extent does not move.
    assertFailsWith<InvalidArgumentException> { map.resize(LogicalExtent(320u, 200u, 1.0)).await() }
    assertEquals(LogicalExtent(320u, 200u, 2.0), map.snapshotGet().logicalExtent)

    map.close().await()
    runtime.close().await()
  }

  @Test
  fun styleSourceJsonCanBeAddedInspectedListedAndRemoved(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map =
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u),
              mapMode = MapMode.STATIC,
            )
        )
        .await()

    try {
      map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()).await()
      map.addStyleSourceJson("places", geoJsonSource()).await()

      assertEquals(StyleSourceType.GEOJSON, map.getStyleSourceInfo("places").await()?.info?.type)
      assertTrue(map.listStyleSourceIds().await().contains("places"))
      map.removeStyleSource("places").awaitCommitted()
      assertNull(map.getStyleSourceInfo("places").await())
      assertCommandFailed(map.removeStyleSource("places").await(), MaplibreStatus.NOT_FOUND)
    } finally {
      map.close().await()
      runtime.close().await()
    }
  }

  @Test
  fun styleSourceVolatilityCanBeToggled(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map =
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u),
              mapMode = MapMode.STATIC,
            )
        )
        .await()

    try {
      map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()).await()
      map.addStyleSourceJson("places", geoJsonSource()).await()

      assertFalse(assertNotNull(map.getStyleSourceInfo("places").await()).info.isVolatile)
      map.setStyleSourceVolatile("places", true).awaitCommitted()
      assertTrue(assertNotNull(map.getStyleSourceInfo("places").await()).info.isVolatile)
      map.setStyleSourceVolatile("places", false).awaitCommitted()
      assertFalse(assertNotNull(map.getStyleSourceInfo("places").await()).info.isVolatile)
      assertCommandFailed(
        map.setStyleSourceVolatile("missing", true).await(),
        MaplibreStatus.NOT_FOUND,
      )
    } finally {
      map.close().await()
      runtime.close().await()
    }
  }

  @Test
  fun styleSourceInfoCopiesUrlAndInlineTileMetadataPastNativeLifetime(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map =
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u),
              mapMode = MapMode.STATIC,
            )
        )
        .await()
    lateinit var retainedInfo: StyleSourceResult
    val tileUrls =
      listOf("https://a.example.com/{z}/{x}/{y}.pbf", "https://b.example.com/{z}/{x}/{y}.pbf")
    val bounds = LatLngBounds(LatLng(-5.0, -10.0), LatLng(15.0, 20.0))
    try {
      map
        .setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray())
        .awaitCommitted()
      map.addVectorSourceUrl("remote", "https://example.com/vector.json").awaitCommitted()
      val remote = assertNotNull(map.getStyleSourceInfo("remote").await())
      assertEquals("https://example.com/vector.json", remote.url)
      assertNull(remote.info.tilejson)

      map
        .addVectorSourceTiles(
          "inline",
          tileUrls,
          StyleTileSourceOptions(
            minZoom = 0.0,
            maxZoom = 12.0,
            attribution = "inline attribution",
            scheme = StyleTileScheme.TMS,
            bounds = bounds,
            tileSize = 256u,
            vectorEncoding = StyleVectorTileEncoding.MLT,
          ),
        )
        .awaitCommitted()

      retainedInfo = assertNotNull(map.getStyleSourceInfo("inline").await())
      assertNull(retainedInfo.url)
      assertEquals("inline attribution", retainedInfo.attribution)
      assertEquals(tileUrls, retainedInfo.tileUrls)
      assertEquals(0.0, retainedInfo.info.tilejson?.minZoom)
      assertEquals(12.0, retainedInfo.info.tilejson?.maxZoom)
      assertEquals(StyleTileScheme.TMS, retainedInfo.info.tilejson?.scheme)
      assertEquals(bounds, retainedInfo.info.bounds)
      assertEquals(512u, retainedInfo.info.tileSize)
      assertEquals(StyleVectorTileEncoding.MLT, retainedInfo.info.vectorEncoding)
      assertNull(retainedInfo.info.rasterEncoding)
      map.removeStyleSource("inline").awaitCommitted()
    } finally {
      map.close().await()
      runtime.close().await()
    }

    assertEquals(tileUrls, retainedInfo.tileUrls)
    assertEquals(bounds, retainedInfo.info.bounds)
  }

  @Test
  fun styleSourceUrlAttributionAndTileUrlsCopyIndependently(): Unit = runSuspendTest {
    val tileUrls =
      listOf("https://a.example.com/{z}/{x}/{y}.pbf", "https://b.example.com/{z}/{x}/{y}.pbf")
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u),
              mapMode = MapMode.STATIC,
            )
        )
        .use { map ->
          map.setStyleJson(EMPTY_STYLE_JSON.encodeToByteArray()).awaitCommitted()
          map
            .addVectorSourceTiles(
              "inline",
              tileUrls,
              StyleTileSourceOptions(attribution = "inline attribution"),
            )
            .awaitCommitted()
          map.addVectorSourceUrl("remote", "https://example.com/vector.json").awaitCommitted()

          // An inline tile source carries its tile URLs and no source URL.
          assertEquals("inline attribution", map.copyStyleSourceAttribution("inline").await())
          assertNull(map.copyStyleSourceUrl("inline").await())
          assertEquals(tileUrls, map.getStyleSourceTileUrls("inline").await()?.tileUrls)

          // A URL-backed tile source carries the reverse. Its attribution arrives with
          // the
          // TileJSON the URL resolves to, so it reads as absent until that load finishes.
          assertEquals("https://example.com/vector.json", map.copyStyleSourceUrl("remote").await())
          assertEquals(emptyList(), map.getStyleSourceTileUrls("remote").await()?.tileUrls)
          assertNull(map.copyStyleSourceAttribution("remote").await())

          // No source carries this ID.
          assertNull(map.copyStyleSourceAttribution("missing").await())
          assertNull(map.copyStyleSourceUrl("missing").await())
          assertNull(map.getStyleSourceTileUrls("missing").await())
        }
    }
  }

  @Test
  fun geoJsonSourcesCanBeAddedAndUpdated(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map =
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u),
              mapMode = MapMode.STATIC,
            )
        )
        .await()

    try {
      map
        .setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray())
        .awaitCommitted()
      map
        .addGeojsonSourceUrl("remote-places", "https://example.com/places.geojson", null)
        .awaitCommitted()
      assertEquals(
        StyleSourceType.GEOJSON,
        map.getStyleSourceInfo("remote-places").await()?.info?.type,
      )
      map
        .setGeojsonSourceUrl("remote-places", "https://example.com/updated.geojson")
        .awaitCommitted()

      val inlineOptions =
        GeojsonSourceOptions(
          minZoom = 0.0,
          maxZoom = 14.0,
          tolerance = 0.5,
          tileSize = 256u,
          buffer = 64u,
          lineMetrics = true,
        )
      GeneratedApi.geojsonSourceDataCreate(geoJsonData(), inlineOptions).use { data ->
        map.addGeojsonSourceData("inline-places", data).awaitCommitted()
      }
      assertEquals(
        StyleSourceType.GEOJSON,
        map.getStyleSourceInfo("inline-places").await()?.info?.type,
      )
      GeneratedApi.geojsonSourceDataCreate(
          ("{\"type\":\"Feature\",\"geometry\":{\"type\":\"LineString\"," +
              "\"coordinates\":[[0,0],[1,1]]},\"properties\":{}}")
            .encodeToByteArray(),
          inlineOptions,
        )
        .use { update -> map.setGeojsonSourceData("inline-places", update).awaitCommitted() }

      GeneratedApi.geojsonSourceDataCreate(nearbyPoints(), clusterOptions()).use { clustered ->
        map.addGeojsonSourceData("clustered-places", clustered).awaitCommitted()
      }
      assertEquals(
        StyleSourceType.GEOJSON,
        map.getStyleSourceInfo("clustered-places").await()?.info?.type,
      )

      assertFailsWith<InvalidArgumentException> {
        map
          .addGeojsonSourceUrl(
            "invalid-zooms",
            "https://example.com/places.geojson",
            GeojsonSourceOptions(minZoom = 12.0, maxZoom = 4.0),
          )
          .await()
      }
      assertNull(map.getStyleSourceInfo("invalid-zooms").await())
      assertCommandFailed(
        map
          .addGeojsonSourceUrl(
            "invalid-cluster-properties",
            "https://example.com/places.geojson",
            GeojsonSourceOptions(clusterProperties = "\"not an object\"".encodeToByteArray()),
          )
          .await(),
        MaplibreStatus.INVALID_ARGUMENT,
      )
      assertNull(map.getStyleSourceInfo("invalid-cluster-properties").await())
    } finally {
      map.close().await()
      runtime.close().await()
    }
  }

  @Test
  fun customGeometrySourcesCanBeManaged(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map =
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u),
              mapMode = MapMode.STATIC,
            )
        )
        .await()

    try {
      map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()).await()
      map
        .addCustomGeometrySource(
          "custom-places",
          CustomGeometrySourceOptions(
            fetchTile = {},
            minZoom = 0.0,
            maxZoom = 14.0,
            tolerance = 0.375,
            tileSize = 512u,
            buffer = 64u,
            clip = true,
            wrap = false,
          ),
        )
        .await()

      assertEquals(
        StyleSourceType.CUSTOM_VECTOR,
        map.getStyleSourceInfo("custom-places").await()?.info?.type,
      )

      val tileId = CanonicalTileId(0u, 0u, 0u)
      map.setCustomGeometrySourceTileData("custom-places", tileId, geoJsonData())
      map.invalidateCustomGeometrySourceTile("custom-places", tileId).await()
      map
        .invalidateCustomGeometrySourceRegion(
          "custom-places",
          LatLngBounds(LatLng(-1.0, -1.0), LatLng(1.0, 1.0)),
        )
        .await()

      map.removeStyleSource("custom-places").awaitCommitted()
      assertNull(map.getStyleSourceInfo("custom-places").await())
    } finally {
      map.close().await()
      runtime.close().await()
    }
  }

  @Test
  fun customMvtVectorSourcesCanBeManaged(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map =
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u),
              mapMode = MapMode.STATIC,
            )
        )
        .await()

    try {
      map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()).await()
      map
        .addCustomMvtVectorSource(
          "custom-mvt",
          CustomMvtVectorSourceOptions(fetchTile = {}, minZoom = 0.0, maxZoom = 14.0),
        )
        .await()

      assertEquals(
        StyleSourceType.CUSTOM_MVT_VECTOR,
        map.getStyleSourceInfo("custom-mvt").await()?.info?.type,
      )

      val tileId = CanonicalTileId(0u, 0u, 0u)
      map.setCustomMvtVectorSourceTileData("custom-mvt", tileId, ByteArray(0)).await()
      map.setCustomMvtVectorSourceTileError("custom-mvt", tileId, "tile missing").await()
      map.invalidateCustomMvtVectorSourceTile("custom-mvt", tileId).await()

      // A second source under the same ID is rejected, and the first one stays installed.
      assertCommandFailed(
        map
          .addCustomMvtVectorSource("custom-mvt", CustomMvtVectorSourceOptions(fetchTile = {}))
          .await(),
        MaplibreStatus.INVALID_ARGUMENT,
      )

      map.removeStyleSource("custom-mvt").awaitCommitted()
      assertNull(map.getStyleSourceInfo("custom-mvt").await())
    } finally {
      map.close().await()
      runtime.close().await()
    }
  }

  // Every live custom source keeps its own callback state, past the ten-slot JavaCPP
  // function-pointer pool that per-source thunks would exhaust on Android.

  @Test
  fun elevenLiveCustomSourcesStayRegistered(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map =
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u),
              mapMode = MapMode.STATIC,
            )
        )
        .await()

    try {
      map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()).await()
      val geometryOptions = CustomGeometrySourceOptions(fetchTile = {})
      val mvtOptions = CustomMvtVectorSourceOptions(fetchTile = {})
      val geometryIds = (1..11).map { "custom-$it" }
      val mvtIds = (1..11).map { "custom-mvt-$it" }
      geometryIds.forEach { map.addCustomGeometrySource(it, geometryOptions).await() }
      mvtIds.forEach { map.addCustomMvtVectorSource(it, mvtOptions).await() }
      (geometryIds + mvtIds).forEach { id -> assertNotNull(map.getStyleSourceInfo(id).await(), id) }

      map.removeStyleSource("custom-1").awaitCommitted()
      map.removeStyleSource("custom-mvt-1").awaitCommitted()
      map.addCustomGeometrySource("custom-12", geometryOptions).await()
      map.addCustomMvtVectorSource("custom-mvt-12", mvtOptions).await()
      assertNotNull(map.getStyleSourceInfo("custom-12").await())
      assertNotNull(map.getStyleSourceInfo("custom-mvt-12").await())
      assertNull(map.getStyleSourceInfo("custom-1").await())
      assertNull(map.getStyleSourceInfo("custom-mvt-1").await())
    } finally {
      map.close().await()
      runtime.close().await()
    }
  }

  @Test
  fun featureStateRoundTripsThroughTheMapStore(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map =
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u),
              mapMode = MapMode.STATIC,
            )
        )
        .await()

    try {
      val selector = FeatureStateSelector("point", featureId = "feature-1")

      // The store answers before any source loads, and missing state reads as an empty
      // object.
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
          FeatureStateSelector("point", featureId = "feature-1", stateKey = "hover")
        )
        .awaitCommitted()
      val afterRemove = map.getFeatureState(selector).await().decodeToString()
      assertFalse(afterRemove.contains("hover"), afterRemove)
      assertTrue(afterRemove.contains("\"radius\":20"), afterRemove)

      map.removeFeatureState(selector).awaitCommitted()
      assertEquals("{}", map.getFeatureState(selector).await().decodeToString())
    } finally {
      map.close().await()
      runtime.close().await()
    }
  }

  @Test
  fun tileSourcesCanBeAddedAndInspected(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map =
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u),
              mapMode = MapMode.STATIC,
            )
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
          StyleTileSourceOptions(
            minZoom = 1.0,
            maxZoom = 12.0,
            attribution = "vector attribution",
            scheme = StyleTileScheme.XYZ,
            vectorEncoding = StyleVectorTileEncoding.MVT,
          ),
        )
        .awaitCommitted()
      map
        .addRasterSourceTiles(
          "satellite",
          listOf("https://example.com/raster/{z}/{x}/{y}.png"),
          StyleTileSourceOptions(tileSize = 256u),
        )
        .awaitCommitted()
      map
        .addRasterDemSourceTiles(
          "terrain",
          listOf("https://example.com/terrain/{z}/{x}/{y}.png"),
          StyleTileSourceOptions(tileSize = 512u, rasterEncoding = StyleRasterDemEncoding.TERRARIUM),
        )
        .awaitCommitted()

      assertEquals(StyleSourceType.VECTOR, map.getStyleSourceInfo("roads").await()?.info?.type)
      val rasterInfo = assertNotNull(map.getStyleSourceInfo("satellite").await())
      assertEquals(StyleSourceType.RASTER, rasterInfo.info.type)
      assertEquals(256u, rasterInfo.info.tileSize)
      assertEquals(
        StyleSourceType.RASTER_DEM,
        map.getStyleSourceInfo("terrain").await()?.info?.type,
      )
      assertEquals(
        StyleRasterDemEncoding.TERRARIUM,
        map.getStyleSourceInfo("terrain").await()?.info?.rasterEncoding,
      )
      assertTrue(
        map.listStyleSourceIds().await().containsAll(listOf("roads", "satellite", "terrain"))
      )
    } finally {
      map.close().await()
      runtime.close().await()
    }
  }

  @Test
  fun styleLayerJsonCanBeAddedInspectedListedAndRemoved(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map =
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u),
              mapMode = MapMode.STATIC,
            )
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

      assertEquals("background", map.getStyleLayerInfo("background").await()?.info?.type)
      assertNotNull(map.getStyleLayerInfo("puck").await())
      assertTrue(map.listStyleLayerIds().await().contains("background"))
      assertTrue(map.listStyleLayerIds().await().contains("puck"))
      assertTrue(
        map
          .getStyleLayerJson("background")
          .await()!!
          .decodeToString()
          .contains("\"type\":\"background\"")
      )
      map
        .setLayerProperty("background", "background-opacity", "0.5".encodeToByteArray())
        .awaitCommitted()
      assertEquals(
        "0.5",
        map.getLayerProperty("background", "background-opacity").await()?.decodeToString(),
      )
      map.setStyleLightProperty("anchor", "\"viewport\"".encodeToByteArray()).awaitCommitted()
      assertEquals("\"viewport\"", map.getStyleLightProperty("anchor").await()?.decodeToString())
      map.removeStyleLayer("background").awaitCommitted()
      map.removeStyleLayer("puck").awaitCommitted()
      assertNull(map.getStyleLayerInfo("background").await())
      assertCommandFailed(map.removeStyleLayer("background").await(), MaplibreStatus.NOT_FOUND)
    } finally {
      map.close().await()
      runtime.close().await()
    }
  }

  @Test
  fun metersPerPixelQueryObservesCameraCommands(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map = runtime.mapCreate(GeneratedApi.mapOptionsDefault()).await()
    try {
      map.updateCamera(CameraUpdate(camera = CameraOptions(zoom = 3.0))).awaitCommitted()
      val projection = map.projectionCreate().await()
      try {
        val meters = map.metersPerPixelAtLatitude(45.0).await()
        assertEquals(meters, projection.metersPerPixelAtLatitude(45.0), 1e-10)
        map.updateCamera(CameraUpdate(camera = CameraOptions(zoom = 4.0))).awaitCommitted()
        assertEquals(meters / 2, map.metersPerPixelAtLatitude(45.0).await(), 1e-10)
        assertEquals(meters, projection.metersPerPixelAtLatitude(45.0), 1e-10)
      } finally {
        projection.close()
      }
    } finally {
      map.close().await()
      runtime.close().await()
    }
  }

  // the layer stack lists in style order with optional source fields.
  @Test
  fun styleLayersListTheLayerStackInStyleOrder(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map =
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u),
              mapMode = MapMode.STATIC,
            )
        )
        .await()

    try {
      map.setStyleJson(
        """
        {
          "version": 8,
          "sources": {
            "tiles": {"type": "vector", "tiles": ["https://example.invalid/{z}/{x}/{y}.pbf"]}
          },
          "layers": [
            {"id": "roads", "type": "line", "source": "tiles", "source-layer": "transportation"},
            {"id": "sky", "type": "background"}
          ]
        }
        """
          .trimIndent()
          .encodeToByteArray()
      )

      assertEquals(
        listOf(
          StyleLayerEntry("roads", "line", "tiles", "transportation"),
          StyleLayerEntry("sky", "background", null, null),
        ),
        map.listStyleLayers().await(),
      )
    } finally {
      map.close().await()
      runtime.close().await()
    }
  }

  @Test
  fun styleImageCanBeSetCopiedInspectedAndRemoved(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map =
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u),
              mapMode = MapMode.STATIC,
            )
        )
        .await()

    try {
      val image = PremultipliedRgba8Image(1u, 1u, 4u, byteArrayOf(1, 2, 3, 4))
      val options = StyleImageOptions(pixelRatio = 2.0f, sdf = true)

      map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()).await()
      map.setStyleImage("dot", image, options)

      val info = map.getStyleImageInfo("dot").await()
      assertEquals(1u, info?.info?.width)
      assertEquals(1u, info?.info?.height)
      assertEquals(4u, info?.info?.stride)
      assertEquals(4uL, info?.info?.byteLength)
      assertEquals(2.0f, info?.info?.pixelRatio)
      assertEquals(true, info?.info?.sdf)
      // The pixel copy carries the bytes alone; the metadata stays in the info query.
      assertContentEquals(image.pixels, map.copyStyleImagePremultipliedRgba8("dot").await())
      assertNull(map.copyStyleImagePremultipliedRgba8("missing").await())
      map.removeStyleImage("dot").awaitCommitted()
      assertNull(map.getStyleImageInfo("dot").await())
      assertNull(map.copyStyleImagePremultipliedRgba8("dot").await())
      assertCommandFailed(map.removeStyleImage("dot").await(), MaplibreStatus.NOT_FOUND)
    } finally {
      map.close().await()
      runtime.close().await()
    }
  }

  @Test
  fun imageSourcesCanBeAddedUpdatedAndInspected(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map =
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u),
              mapMode = MapMode.STATIC,
            )
        )
        .await()

    try {
      val image = PremultipliedRgba8Image(1u, 1u, 4u, byteArrayOf(1, 2, 3, 4))
      val coordinates = imageCoordinates()
      val moved = listOf(LatLng(1.0, 0.0), LatLng(1.0, 1.0), LatLng(0.0, 1.0), LatLng(0.0, 0.0))

      map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()).await()
      map.addImageSourceUrl("overlay", coordinates, "https://example.com/image.png").await()

      assertEquals(StyleSourceType.IMAGE, map.getStyleSourceInfo("overlay").await()?.info?.type)
      assertEquals(coordinates, map.getImageSourceCoordinates("overlay").await())
      map.setImageSourceUrl("overlay", "https://example.com/updated-image.png").await()
      map.setImageSourceImage("overlay", image).await()
      map.setImageSourceCoordinates("overlay", moved).await()
      assertEquals(moved, map.getImageSourceCoordinates("overlay").await())
      assertEquals(null, map.getImageSourceCoordinates("missing-overlay").await())

      map.addImageSourceImage("inline-overlay", coordinates, image)
      assertEquals(
        StyleSourceType.IMAGE,
        map.getStyleSourceInfo("inline-overlay").await()?.info?.type,
      )
    } finally {
      map.close().await()
      runtime.close().await()
    }
  }

  // The command completion generation fences a later snapshot: a snapshot at or past it
  // observes the commit.
  @Test
  fun committedCommandGenerationFencesTheSnapshot(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u)
            )
        )
        .await()
        .use { map ->
          val debug =
            MapDebugOption(MapDebugOption.TILE_BORDERS.rawValue or MapDebugOption.OVERDRAW.rawValue)
          val generation = map.setDebugOptions(debug).awaitCommitted().generation
          assertTrue(generation > 0uL, "committed command must publish a generation")
          val snapshot = map.snapshotGet()
          assertTrue(snapshot.generation >= generation)
          assertEquals(debug, snapshot.debugOptions)
        }
    }
  }

  @Test
  fun snapshotFieldsRoundTripThroughTheirSetCommands(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u)
            )
        )
        .await()
        .use { map ->
          assertEquals(MapDebugOption(0u), map.snapshotGet().debugOptions)
          assertFalse(map.snapshotGet().renderingStatsViewEnabled)

          map.setRenderingStatsViewEnabled(true).awaitCommitted()
          assertTrue(map.snapshotGet().renderingStatsViewEnabled)

          val viewport = MapViewportOptions(northOrientation = NorthOrientation.DOWN)
          map.setViewportOptions(viewport).awaitCommitted()
          assertEquals(NorthOrientation.DOWN, map.snapshotGet().viewport.northOrientation)

          val tile = MapTileOptions(prefetchZoomDelta = 3u, lodScale = 1.5)
          map.setTileOptions(tile).awaitCommitted()
          val tileSnapshot = map.snapshotGet().tile
          assertEquals(3u, tileSnapshot.prefetchZoomDelta)
          assertEquals(1.5, tileSnapshot.lodScale)

          val bounds =
            BoundOptions(
              minZoom = 2.0,
              maxZoom = 15.0,
              bounds = LatLngBounds(LatLng(-10.0, -10.0), LatLng(10.0, 10.0)),
            )
          map.setBounds(bounds).awaitCommitted()
          val boundsSnapshot = map.snapshotGet().bounds
          assertEquals(2.0, boundsSnapshot.minZoom)
          assertEquals(15.0, boundsSnapshot.maxZoom)
          assertEquals(bounds.bounds, boundsSnapshot.bounds)

          val freeCamera =
            FreeCameraOptions(
              position = Vec3(0.5, 0.5, 0.5),
              orientation = Quaternion(0.0, 0.0, 0.0, 1.0),
            )
          map.setFreeCameraOptions(freeCamera).awaitCommitted()
          val freeCameraSnapshot = map.snapshotGet().freeCamera
          kotlin.test.assertNotNull(freeCameraSnapshot.position)
          kotlin.test.assertNotNull(freeCameraSnapshot.orientation)

          val projectionMode = ProjectionMode(axonometric = true, xSkew = 0.5, ySkew = 0.25)
          map.setProjectionMode(projectionMode).awaitCommitted()
          val projectionModeSnapshot = map.snapshotGet().projectionMode
          assertEquals(true, projectionModeSnapshot.axonometric)
          assertEquals(0.5, projectionModeSnapshot.xSkew)
          assertEquals(0.25, projectionModeSnapshot.ySkew)

          // The debug dump writes to the log; the map keeps serving commands after it.
          map.dumpDebugLogs().awaitCommitted()
          assertTrue(map.snapshotGet().renderingStatsViewEnabled)
        }
    }
  }

  @Test
  fun repaintIsAcceptedOnlyByAContinuousMap(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u)
            )
        )
        .await()
        .use { map -> map.requestRepaint().awaitCommitted() }

      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u),
              mapMode = MapMode.STATIC,
            )
        )
        .await()
        .use { map -> assertFailsWith<InvalidStateException> { map.requestRepaint().await() } }
    }
  }

  @Test
  fun cameraFitAndBoundsQueriesResolveBehindEarlierCommands(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault()
                  .initialExtent
                  .copy(width = 1024u, height = 512u, scaleFactor = 1.0),
              mapMode = MapMode.STATIC,
            )
        )
        .await()
        .use { map ->
          val camera = CameraOptions(center = LatLng(10.0, 20.0), zoom = 3.0)
          map.updateCamera(CameraUpdate(camera = camera)).awaitCommitted()

          val fit = CameraFitOptions(padding = EdgeInsets(), bearing = 0.0, pitch = 0.0)
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
          val antimeridian = CameraOptions(center = LatLng(0.0, 179.0), zoom = 3.0)
          val wrappedAcrossSeam = map.latLngBoundsForCamera(antimeridian).await()
          val unwrappedAcrossSeam = map.latLngBoundsForCameraUnwrapped(antimeridian).await()
          assertTrue(wrappedAcrossSeam.southwest.longitude in -180.0..180.0)
          assertTrue(wrappedAcrossSeam.northeast.longitude in -180.0..180.0)
          // The unwrapped east edge stays in the world copy it was read from, so the span
          // is the 90 degrees the viewport covers rather than the 270 the wrapped hull
          // spans.
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

  private fun clusterOptions(): GeojsonSourceOptions =
    GeojsonSourceOptions(
      cluster = true,
      clusterRadius = 50u,
      clusterMaxZoom = 14.0,
      clusterMinPoints = 2u,
      clusterProperties = "{\"total\":[\"+\",[\"get\",\"weight\"]]}".encodeToByteArray(),
    )

  private fun geoJsonData(): ByteArray =
    ("{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\",\"id\":1," +
        "\"geometry\":{\"type\":\"GeometryCollection\",\"geometries\":[" +
        "{\"type\":\"Point\",\"coordinates\":[0,0]}," +
        "{\"type\":\"MultiLineString\",\"coordinates\":[[[0,0],[1,1]]]}]}," +
        "\"properties\":{\"name\":\"Null Island\",\"rank\":1}}]}")
      .encodeToByteArray()

  private fun backgroundLayer(): ByteArray =
    "{\"id\":\"background\",\"type\":\"background\"}".encodeToByteArray()

  @Test
  fun projectionUnwrappedConversionPreservesVisibleWorldCopy(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map =
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault()
                  .initialExtent
                  .copy(width = 1024u, height = 512u, scaleFactor = 1.0),
              mapMode = MapMode.STATIC,
            )
        )
        .await()

    try {
      map
        .updateCamera(CameraUpdate(camera = CameraOptions(center = LatLng(0.0, 179.0), zoom = 0.0)))
        .await()

      val projection = map.projectionCreate().await()
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
      map.close().await()
      runtime.close().await()
    }
  }

  @Test
  fun mapCoordinateConversionsRoundTrip(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map =
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault()
                  .initialExtent
                  .copy(width = 128u, height = 128u, scaleFactor = 1.0),
              mapMode = MapMode.STATIC,
            )
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
      map.close().await()
      runtime.close().await()
    }
  }

  @Test
  fun mapUnwrappedConversionsPreserveVisibleWorldCopies(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map =
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault()
                  .initialExtent
                  .copy(width = 1024u, height = 512u, scaleFactor = 1.0),
              mapMode = MapMode.STATIC,
            )
        )
        .await()

    try {
      map
        .updateCamera(CameraUpdate(camera = CameraOptions(center = LatLng(0.0, 180.0), zoom = 0.0)))
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
      map.close().await()
      runtime.close().await()
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
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u)
            )
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
