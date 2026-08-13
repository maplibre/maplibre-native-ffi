package org.maplibre.nativeffi

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertNotEquals
import kotlin.test.assertNotSame
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.BoundsConstraint
import org.maplibre.nativeffi.camera.CameraFitOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.camera.FreeCameraOptions
import org.maplibre.nativeffi.camera.UnitBezier
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.geo.Quaternion
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.geo.Vec3
import org.maplibre.nativeffi.map.ConstrainMode
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.map.NorthOrientation
import org.maplibre.nativeffi.map.ProjectionModeOptions
import org.maplibre.nativeffi.map.TileLodMode
import org.maplibre.nativeffi.map.TileOptions
import org.maplibre.nativeffi.map.ViewportMode
import org.maplibre.nativeffi.map.ViewportOptions
import org.maplibre.nativeffi.offline.OfflineRegionDefinition
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions
import org.maplibre.nativeffi.runtime.RuntimeEventMask
import org.maplibre.nativeffi.runtime.RuntimeOptions
import org.maplibre.nativeffi.style.GeoJsonSourceOptions
import org.maplibre.nativeffi.style.ImageStretch
import org.maplibre.nativeffi.style.RasterDemEncoding
import org.maplibre.nativeffi.style.StyleImageOptions
import org.maplibre.nativeffi.style.StyleTransitionOptions
import org.maplibre.nativeffi.style.TileScheme
import org.maplibre.nativeffi.style.TileSourceOptions
import org.maplibre.nativeffi.style.VectorTileEncoding

/**
 * BND-070. Each case lists one mutator per declared field, so adding a field without extending
 * equality fails the mutator assertion.
 */
class OptionsValueSemanticsTest {
  private fun <T : Any> assertValueSemantics(
    baseline: () -> T,
    copyOf: (T) -> T,
    mutators: List<T.() -> Unit>,
  ) {
    val left = baseline()
    val right = baseline()
    assertEquals(left, right)
    assertEquals(left.hashCode(), right.hashCode())

    val copy = copyOf(left)
    assertEquals(left, copy)
    assertNotSame(left, copy)

    mutators.forEachIndexed { index, mutate ->
      val mutated = baseline().apply(mutate)
      assertNotEquals(baseline(), mutated, "field $index is missing from equality")
      assertEquals(mutated, copyOf(mutated), "field $index is missing from copy")
    }
  }

  @Test
  fun cameraOptionsComparesByFieldValue() {
    assertValueSemantics(
      baseline = {
        CameraOptions().apply {
          center = LatLng(1.0, 2.0)
          centerAltitude = 3.0
          padding = EdgeInsets(4.0, 5.0, 6.0, 7.0)
          anchor = ScreenPoint(8.0, 9.0)
          zoom = 10.0
          bearing = 11.0
          pitch = 12.0
          roll = 13.0
          fieldOfView = 14.0
        }
      },
      copyOf = { it.copy() },
      mutators =
        listOf(
          { center = LatLng(90.0, 90.0) },
          { centerAltitude = 300.0 },
          { padding = EdgeInsets.ZERO },
          { anchor = ScreenPoint(80.0, 90.0) },
          { zoom = 100.0 },
          { bearing = 110.0 },
          { pitch = 120.0 },
          { roll = 130.0 },
          { fieldOfView = 140.0 },
        ),
    )
  }

  @Test
  fun animationOptionsComparesByFieldValue() {
    assertValueSemantics(
      baseline = {
        AnimationOptions().apply {
          durationMs = 1.0
          velocity = 2.0
          minZoom = 3.0
          easing = UnitBezier(0.1, 0.2, 0.3, 0.4)
          transitionId = 4L
        }
      },
      copyOf = { it.copy() },
      mutators =
        listOf(
          { durationMs = 10.0 },
          { velocity = 20.0 },
          { minZoom = 30.0 },
          { easing = UnitBezier(0.9, 0.8, 0.7, 0.6) },
          { transitionId = 40L },
        ),
    )
  }

  @Test
  fun boundOptionsComparesByFieldValue() {
    assertValueSemantics(
      baseline = {
        BoundOptions().apply {
          bounds = BoundsConstraint.Bounded(LatLngBounds(LatLng(0.0, 0.0), LatLng(1.0, 1.0)))
          minZoom = 2.0
          maxZoom = 3.0
          minPitch = 4.0
          maxPitch = 5.0
        }
      },
      copyOf = { it.copy() },
      mutators =
        listOf(
          { bounds = BoundsConstraint.Bounded(LatLngBounds(LatLng(-1.0, -1.0), LatLng(2.0, 2.0))) },
          { bounds = BoundsConstraint.Unbounded },
          { minZoom = 20.0 },
          { maxZoom = 30.0 },
          { minPitch = 40.0 },
          { maxPitch = 50.0 },
        ),
    )
  }

  @Test
  fun cameraFitOptionsComparesByFieldValue() {
    assertValueSemantics(
      baseline = {
        CameraFitOptions().apply {
          padding = EdgeInsets(1.0, 2.0, 3.0, 4.0)
          bearing = 5.0
          pitch = 6.0
        }
      },
      copyOf = { it.copy() },
      mutators = listOf({ padding = EdgeInsets.ZERO }, { bearing = 50.0 }, { pitch = 60.0 }),
    )
  }

  @Test
  fun freeCameraOptionsComparesByFieldValue() {
    assertValueSemantics(
      baseline = {
        FreeCameraOptions().apply {
          position = Vec3(1.0, 2.0, 3.0)
          orientation = Quaternion(0.0, 0.0, 0.0, 1.0)
        }
      },
      copyOf = { it.copy() },
      mutators =
        listOf({ position = Vec3(9.0, 9.0, 9.0) }, { orientation = Quaternion(1.0, 0.0, 0.0, 0.0) }),
    )
  }

  @Test
  fun viewportOptionsComparesByFieldValue() {
    assertValueSemantics(
      baseline = {
        ViewportOptions().apply {
          northOrientation = NorthOrientation(0)
          constrainMode = ConstrainMode(0)
          viewportMode = ViewportMode(0)
          frustumOffset = EdgeInsets(1.0, 2.0, 3.0, 4.0)
        }
      },
      copyOf = { it.copy() },
      mutators =
        listOf(
          { northOrientation = NorthOrientation(2) },
          { constrainMode = ConstrainMode(1) },
          { viewportMode = ViewportMode(1) },
          { frustumOffset = EdgeInsets.ZERO },
        ),
    )
  }

  @Test
  fun tileOptionsComparesByFieldValue() {
    assertValueSemantics(
      baseline = {
        TileOptions().apply {
          prefetchZoomDelta = 1
          lodMinRadius = 2.0
          lodScale = 3.0
          lodPitchThreshold = 4.0
          lodZoomShift = 5.0
          lodMode = TileLodMode(0)
        }
      },
      copyOf = { it.copy() },
      mutators =
        listOf(
          { prefetchZoomDelta = 7 },
          { lodMinRadius = 20.0 },
          { lodScale = 30.0 },
          { lodPitchThreshold = 40.0 },
          { lodZoomShift = 50.0 },
          { lodMode = TileLodMode(1) },
        ),
    )
  }

  @Test
  fun projectionModeOptionsComparesByFieldValue() {
    assertValueSemantics(
      baseline = {
        ProjectionModeOptions().apply {
          axonometric = true
          xSkew = 1.0
          ySkew = 2.0
        }
      },
      copyOf = { it.copy() },
      mutators = listOf({ axonometric = false }, { xSkew = 10.0 }, { ySkew = 20.0 }),
    )
  }

  @Test
  fun mapOptionsComparesByFieldValue() {
    assertValueSemantics(
      baseline = {
        MapOptions().apply {
          width = 100
          height = 200
          scaleFactor = 2.0
          mapMode = MapMode(0)
          fastPforEnabled = false
          eventMask = RuntimeEventMask.ALL
        }
      },
      copyOf = { it.copy() },
      mutators =
        listOf(
          { width = 300 },
          { height = 400 },
          { scaleFactor = 3.0 },
          { mapMode = MapMode(1) },
          { fastPforEnabled = true },
          { eventMask = RuntimeEventMask.ALL_MAP_EVENTS },
        ),
    )
  }

  @Test
  fun runtimeOptionsComparesByFieldValue() {
    assertValueSemantics(
      baseline = {
        RuntimeOptions().apply {
          assetPath = "assets"
          cachePath = "cache"
          eventMask = RuntimeEventMask.ALL
        }
      },
      copyOf = { it.copy() },
      mutators =
        listOf(
          { assetPath = "other-assets" },
          { cachePath = "other-cache" },
          { eventMask = RuntimeEventMask.NONE },
        ),
    )
  }

  @Test
  fun tileSourceOptionsComparesByFieldValue() {
    assertValueSemantics(
      baseline = {
        TileSourceOptions().apply {
          minZoom = 1.0
          maxZoom = 2.0
          attribution = "attribution"
          scheme = TileScheme.XYZ
          bounds = LatLngBounds(LatLng(0.0, 0.0), LatLng(1.0, 1.0))
          tileSize = 256
          vectorEncoding = VectorTileEncoding.MVT
          rasterDemEncoding = RasterDemEncoding.MAPBOX
        }
      },
      copyOf = { it.copy() },
      mutators =
        listOf(
          { minZoom = 10.0 },
          { maxZoom = 20.0 },
          { attribution = "other" },
          { scheme = TileScheme.TMS },
          { bounds = LatLngBounds(LatLng(-1.0, -1.0), LatLng(2.0, 2.0)) },
          { tileSize = 512 },
          { vectorEncoding = VectorTileEncoding.MLT },
          { rasterDemEncoding = RasterDemEncoding.TERRARIUM },
        ),
    )
  }

  @Test
  fun geoJsonSourceOptionsComparesByFieldValue() {
    assertValueSemantics(
      baseline = {
        GeoJsonSourceOptions().apply {
          minZoom = 1.0
          maxZoom = 2.0
          tolerance = 0.5
          clusterMaxZoom = 14.0
          clusterProperties = clusterProperties("sum")
          tileSize = 256
          buffer = 64
          clusterRadius = 40
          clusterMinPoints = 3
          lineMetrics = true
          cluster = true
          synchronousUpdate = true
        }
      },
      copyOf = { it.copy() },
      mutators =
        listOf(
          { minZoom = 10.0 },
          { maxZoom = 20.0 },
          { tolerance = 0.25 },
          { clusterMaxZoom = 12.0 },
          { clusterProperties = clusterProperties("max") },
          { tileSize = 512 },
          { buffer = 128 },
          { clusterRadius = 50 },
          { clusterMinPoints = 2 },
          { lineMetrics = false },
          { cluster = false },
          { synchronousUpdate = false },
        ),
    )
  }

  /** Builds a `clusterProperties` object whose aggregation operator is [operator]. */
  private fun clusterProperties(operator: String): ByteArray =
    "{\"weight\":[\"$operator\",[\"get\",\"weight\"]]}".encodeToByteArray()

  @Test
  fun styleImageOptionsComparesByFieldValue() {
    assertValueSemantics(
      baseline = {
        StyleImageOptions().apply {
          pixelRatio = 2.0f
          sdf = true
        }
      },
      copyOf = { it.copy() },
      mutators = listOf({ pixelRatio = 3.0f }, { sdf = false }),
    )
  }

  @Test
  fun styleTransitionOptionsComparesByFieldValue() {
    assertValueSemantics(
      baseline = {
        StyleTransitionOptions().apply {
          durationMs = 300.0
          delayMs = 0.0
          enablePlacementTransitions = false
        }
      },
      copyOf = { it.copy() },
      mutators =
        listOf(
          { durationMs = 500.0 },
          // A present zero stays distinguishable from an absent field.
          { delayMs = null },
          // A present false stays distinguishable from an absent field.
          { enablePlacementTransitions = null },
        ),
    )
  }

  @Test
  fun renderedFeatureQueryOptionsComparesByFieldValue() {
    assertValueSemantics(
      baseline = {
        RenderedFeatureQueryOptions().apply {
          layerIds = listOf("a", "b")
          filter = "true".encodeToByteArray()
        }
      },
      copyOf = { it.copy() },
      mutators = listOf({ layerIds = listOf("a") }, { filter = "\"filter\"".encodeToByteArray() }),
    )
  }

  @Test
  fun sourceFeatureQueryOptionsComparesByFieldValue() {
    assertValueSemantics(
      baseline = {
        SourceFeatureQueryOptions().apply {
          sourceLayerIds = listOf("a", "b")
          filter = "true".encodeToByteArray()
        }
      },
      copyOf = { it.copy() },
      mutators =
        listOf({ sourceLayerIds = listOf("a") }, { filter = "\"filter\"".encodeToByteArray() }),
    )
  }

  @Test
  fun absentLayerIdsDifferFromEmptyLayerIds() {
    // The native field mask distinguishes an absent layer filter from an empty one.
    val absent = RenderedFeatureQueryOptions()
    val empty = RenderedFeatureQueryOptions().apply { layerIds = emptyList() }

    assertNotEquals(absent, empty)
  }

  @Test
  fun absentJsonBytesDifferFromEmptyJsonBytes() {
    assertNotEquals(
      RenderedFeatureQueryOptions(),
      RenderedFeatureQueryOptions().apply { filter = byteArrayOf() },
    )
    assertNotEquals(
      SourceFeatureQueryOptions(),
      SourceFeatureQueryOptions().apply { filter = byteArrayOf() },
    )
    assertNotEquals(
      GeoJsonSourceOptions(),
      GeoJsonSourceOptions().apply { clusterProperties = byteArrayOf() },
    )
  }

  @Test
  fun copyBlockAppliesToTheCopyOnly() {
    val original = CameraOptions().apply { zoom = 1.0 }
    val derived = original.copy { zoom = 2.0 }

    assertEquals(1.0, original.zoom)
    assertEquals(2.0, derived.zoom)
  }

  @Test
  fun queryOptionsSnapshotCallerOwnedLayerIds() {
    // BND-069.
    val layerIds = mutableListOf("a")
    val options = RenderedFeatureQueryOptions().apply { this.layerIds = layerIds }
    val copy = options.copy()

    layerIds.add("b")

    assertEquals(listOf("a"), options.layerIds)
    assertEquals(listOf("a"), copy.layerIds)

    val sourceLayerIds = mutableListOf("a")
    val sourceOptions = SourceFeatureQueryOptions().apply { this.sourceLayerIds = sourceLayerIds }

    sourceLayerIds.add("b")

    assertEquals(listOf("a"), sourceOptions.sourceLayerIds)
  }

  @Test
  fun byteTransitDescriptorsSnapshotCallerOwnedArrays() {
    val filter = "true".encodeToByteArray()
    val query = RenderedFeatureQueryOptions().apply { this.filter = filter }
    filter[0] = 'f'.code.toByte()
    assertContentEquals("true".encodeToByteArray(), query.filter)

    val exposedFilter = query.filter ?: error("missing filter")
    exposedFilter[0] = 'f'.code.toByte()
    assertContentEquals("true".encodeToByteArray(), query.filter)

    val sourceFilter = "false".encodeToByteArray()
    val sourceQuery = SourceFeatureQueryOptions().apply { this.filter = sourceFilter }
    sourceFilter[0] = 't'.code.toByte()
    assertContentEquals("false".encodeToByteArray(), sourceQuery.filter)

    val properties = "{}".encodeToByteArray()
    val source = GeoJsonSourceOptions().apply { clusterProperties = properties }
    properties[0] = '['.code.toByte()
    assertContentEquals("{}".encodeToByteArray(), source.clusterProperties)

    val geometry = "{\"type\":\"Point\",\"coordinates\":[0,0]}".encodeToByteArray()
    val region = OfflineRegionDefinition.GeometryRegion("style", geometry, 0.0, 1.0, 1f, false)
    geometry[0] = '['.code.toByte()
    assertContentEquals(
      "{\"type\":\"Point\",\"coordinates\":[0,0]}".encodeToByteArray(),
      region.geometry,
    )
  }

  @Test
  fun styleImageOptionsSnapshotCallerOwnedStretches() {
    // BND-069.
    val stretchX = mutableListOf(ImageStretch(0.0f, 1.0f))
    val options =
      StyleImageOptions().apply {
        this.stretchX = stretchX
        stretchY = emptyList()
      }
    val copy = options.copy()

    stretchX.add(ImageStretch(1.0f, 2.0f))

    assertEquals(listOf(ImageStretch(0.0f, 1.0f)), options.stretchX)
    assertEquals(listOf(ImageStretch(0.0f, 1.0f)), copy.stretchX)
    // A present empty list stays distinguishable from an absent one.
    assertEquals(emptyList(), copy.stretchY)
  }
}
