package org.maplibre.nativeffi.style

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.generated.CommandDisposition
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.GeojsonSourceOptions
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.MapMode
import org.maplibre.nativeffi.generated.RuntimeHandle
import org.maplibre.nativeffi.generated.StyleSourceType
import org.maplibre.nativeffi.runtime.runSuspendTest

class GeojsonSourceDataHandleTest {
  @Test
  fun preparesFreeOfAnyRuntimeOrMap() {
    Maplibre.loadNativeLibrary()
    val data = GeneratedApi.geojsonSourceDataCreate(featureCollection())
    data.close()
    assertTrue(data.isClosed)
    // A second close is a no-op rather than a double release.
    data.close()
  }

  @Test
  fun createValidatesDataAndClusterConstraints() {
    Maplibre.loadNativeLibrary()
    assertFailsWith<InvalidArgumentException> {
      GeneratedApi.geojsonSourceDataCreate("not geojson".encodeToByteArray())
    }
    // Clustering rejects a feature that carries non-point geometry at preparation time.
    assertFailsWith<InvalidArgumentException> {
      GeneratedApi.geojsonSourceDataCreate(lineFeatureCollection(), clusterOptions())
    }
  }

  @Test
  fun onePreparedHandleInstallsOnManySources(): Unit = runSuspendTest {
    withMap { _, map ->
      GeneratedApi.geojsonSourceDataCreate(featureCollection(), baseOptions()).use { data ->
        map.addGeojsonSourceData("places-a", data).await()
        map.addGeojsonSourceData("places-b", data).await()
        assertEquals(
          StyleSourceType.GEOJSON,
          map.getStyleSourceInfo("places-a").await()?.info?.type,
        )
        assertEquals(
          StyleSourceType.GEOJSON,
          map.getStyleSourceInfo("places-b").await()?.info?.type,
        )
        map.setGeojsonSourceData("places-a", data).await()
      }
    }
  }

  @Test
  fun releaseNeverInvalidatesAnInstalledSource(): Unit = runSuspendTest {
    withMap { _, map ->
      val data = GeneratedApi.geojsonSourceDataCreate(featureCollection())
      map.addGeojsonSourceData("places", data).await()
      data.close()

      // The source keeps its own reference and remains usable after the handle is gone.
      assertEquals(StyleSourceType.GEOJSON, map.getStyleSourceInfo("places").await()?.info?.type)
      GeneratedApi.geojsonSourceDataCreate(featureCollection()).use { replacement ->
        map.setGeojsonSourceData("places", replacement).await()
      }

      // A released handle is no longer installable.
      assertFailsWith<InvalidStateException> {
        map.addGeojsonSourceData("more-places", data).await()
      }
    }
  }

  @Test
  fun setRejectsDataPreparedWithMismatchedOptions(): Unit = runSuspendTest {
    withMap { _, map ->
      GeneratedApi.geojsonSourceDataCreate(featureCollection(), baseOptions()).use { data ->
        map.addGeojsonSourceData("places", data).await()
      }

      GeneratedApi.geojsonSourceDataCreate(featureCollection(), baseOptions().copy(buffer = 32u))
        .use { mismatched ->
          assertCommandFailed(
            map.setGeojsonSourceData("places", mismatched).await(),
            MaplibreStatus.INVALID_ARGUMENT,
          )
        }

      // Cluster aggregations are part of the options comparison, so data
      // prepared with different clusterProperties is rejected too.
      GeneratedApi.geojsonSourceDataCreate(
          featureCollection(),
          baseOptions()
            .copy(clusterProperties = "{\"total\":[\"+\",[\"get\",\"rank\"]]}".encodeToByteArray()),
        )
        .use { mismatched ->
          assertCommandFailed(
            map.setGeojsonSourceData("places", mismatched).await(),
            MaplibreStatus.INVALID_ARGUMENT,
          )
        }
    }
  }

  @Test
  fun synchronousTilingOverridesALiveSource(): Unit = runSuspendTest {
    withMap { _, map ->
      GeneratedApi.geojsonSourceDataCreate(featureCollection()).use { data ->
        map.addGeojsonSourceData("places", data).await()
        map.setGeojsonSourceSynchronousTiling("places", true).await()
        GeneratedApi.geojsonSourceDataCreate(featureCollection()).use { update ->
          map.setGeojsonSourceData("places", update).await()
        }
        map.setGeojsonSourceSynchronousTiling("places", false).await()
      }
      assertCommandFailed(
        map.setGeojsonSourceSynchronousTiling("missing", true).await(),
        MaplibreStatus.NOT_FOUND,
      )
    }
  }

  private fun assertCommandFailed(
    completion: org.maplibre.nativeffi.runtime.CommandCompletion,
    status: MaplibreStatus,
  ) {
    assertEquals(CommandDisposition.FAILED, completion.disposition)
    assertEquals(status, completion.status)
    assertTrue(completion.diagnostic.isNotEmpty())
  }

  private suspend fun withMap(block: suspend (RuntimeHandle, MapHandle) -> Unit) {
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
      block(runtime, map)
    } finally {
      map.release().await()
      runtime.release().await()
    }
  }

  private fun baseOptions(): GeojsonSourceOptions =
    GeojsonSourceOptions(minZoom = 0.0, maxZoom = 14.0, buffer = 64u)

  private fun clusterOptions(): GeojsonSourceOptions =
    GeojsonSourceOptions(cluster = true, clusterRadius = 50u)

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
