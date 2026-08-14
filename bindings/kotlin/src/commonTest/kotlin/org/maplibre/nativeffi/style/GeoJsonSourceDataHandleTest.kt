package org.maplibre.nativeffi.style

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions

class GeoJsonSourceDataHandleTest {
  @Test
  fun preparesFreeOfAnyRuntimeOrMap() {
    Maplibre.loadNativeLibrary()
    val data = GeoJsonSourceDataHandle.create(featureCollection())
    data.close()
    assertTrue(data.isClosed)
    // A second close is a no-op rather than a double release.
    data.close()
  }

  @Test
  fun createValidatesDataAndClusterConstraints() {
    Maplibre.loadNativeLibrary()
    assertFailsWith<InvalidArgumentException> {
      GeoJsonSourceDataHandle.create("not geojson".encodeToByteArray())
    }
    // Clustering rejects a feature that carries non-point geometry at preparation time.
    assertFailsWith<InvalidArgumentException> {
      GeoJsonSourceDataHandle.create(lineFeatureCollection(), clusterOptions())
    }
  }

  @Test
  fun onePreparedHandleInstallsOnManySources() {
    withMap { map ->
      GeoJsonSourceDataHandle.create(featureCollection(), baseOptions()).use { data ->
        map.addGeoJsonSourceData("places-a", data)
        map.addGeoJsonSourceData("places-b", data)
        assertEquals(SourceType.GEOJSON, map.styleSourceType("places-a"))
        assertEquals(SourceType.GEOJSON, map.styleSourceType("places-b"))
        map.setGeoJsonSourceData("places-a", data)
      }
    }
  }

  @Test
  fun releaseNeverInvalidatesAnInstalledSource() {
    withMap { map ->
      val data = GeoJsonSourceDataHandle.create(featureCollection())
      map.addGeoJsonSourceData("places", data)
      data.close()

      // The source keeps its own reference and remains usable after the handle is gone.
      assertEquals(SourceType.GEOJSON, map.styleSourceType("places"))
      GeoJsonSourceDataHandle.create(featureCollection()).use { replacement ->
        map.setGeoJsonSourceData("places", replacement)
      }

      // A released handle is no longer installable.
      assertFailsWith<InvalidStateException> { map.addGeoJsonSourceData("more-places", data) }
    }
  }

  @Test
  fun setRejectsDataPreparedWithMismatchedOptions() {
    withMap { map ->
      GeoJsonSourceDataHandle.create(featureCollection(), baseOptions()).use { data ->
        map.addGeoJsonSourceData("places", data)
      }

      assertFailsWith<InvalidArgumentException> {
        GeoJsonSourceDataHandle.create(featureCollection(), baseOptions().copy { buffer = 32 })
          .use { mismatched -> map.setGeoJsonSourceData("places", mismatched) }
      }

      // clusterProperties is excepted from the options comparison.
      GeoJsonSourceDataHandle.create(
          featureCollection(),
          baseOptions().copy {
            clusterProperties = "{\"total\":[\"+\",[\"get\",\"rank\"]]}".encodeToByteArray()
          },
        )
        .use { compatible -> map.setGeoJsonSourceData("places", compatible) }
    }
  }

  @Test
  fun synchronousTilingOverridesALiveSource() {
    withMap { map ->
      GeoJsonSourceDataHandle.create(featureCollection()).use { data ->
        map.addGeoJsonSourceData("places", data)
        map.setGeoJsonSourceSynchronousTiling("places", true)
        GeoJsonSourceDataHandle.create(featureCollection()).use { update ->
          map.setGeoJsonSourceData("places", update)
        }
        map.setGeoJsonSourceSynchronousTiling("places", false)
      }
      assertFailsWith<InvalidArgumentException> {
        map.setGeoJsonSourceSynchronousTiling("missing", true)
      }
    }
  }

  private fun withMap(block: (MapHandle) -> Unit) {
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
      block(map)
    } finally {
      map.close()
      runtime.close()
    }
  }

  private fun baseOptions(): GeoJsonSourceOptions =
    GeoJsonSourceOptions().apply {
      minZoom = 0.0
      maxZoom = 14.0
      buffer = 64
    }

  private fun clusterOptions(): GeoJsonSourceOptions =
    GeoJsonSourceOptions().apply {
      cluster = true
      clusterRadius = 50
    }

  private fun featureCollection(): ByteArray =
    ("{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\",\"id\":1," +
        "\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,0]}," +
        "\"properties\":{\"rank\":1}}]}")
      .encodeToByteArray()

  private fun lineFeatureCollection(): ByteArray =
    ("{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\"," +
        "\"geometry\":{\"type\":\"LineString\",\"coordinates\":[[0,0],[1,1]]}," +
        "\"properties\":{}},{\"type\":\"Feature\"," +
        "\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,0]},\"properties\":{}}]}")
      .encodeToByteArray()
}
