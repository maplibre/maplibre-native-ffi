package org.maplibre.nativeffi.style

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.runtime.CommandDisposition
import org.maplibre.nativeffi.runtime.RuntimeEventPayload
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions
import org.maplibre.nativeffi.runtime.runSuspendTest

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
  fun onePreparedHandleInstallsOnManySources(): Unit = runSuspendTest {
    withMap { _, map ->
      GeoJsonSourceDataHandle.create(featureCollection(), baseOptions()).use { data ->
        map.addGeoJsonSourceData("places-a", data)
        map.addGeoJsonSourceData("places-b", data)
        assertEquals(SourceType.GEOJSON, map.styleSourceInfo("places-a")?.type)
        assertEquals(SourceType.GEOJSON, map.styleSourceInfo("places-b")?.type)
        map.setGeoJsonSourceData("places-a", data)
      }
    }
  }

  @Test
  fun releaseNeverInvalidatesAnInstalledSource(): Unit = runSuspendTest {
    withMap { _, map ->
      val data = GeoJsonSourceDataHandle.create(featureCollection())
      map.addGeoJsonSourceData("places", data)
      data.close()

      // The source keeps its own reference and remains usable after the handle is gone.
      assertEquals(SourceType.GEOJSON, map.styleSourceInfo("places")?.type)
      GeoJsonSourceDataHandle.create(featureCollection()).use { replacement ->
        map.setGeoJsonSourceData("places", replacement)
      }

      // A released handle is no longer installable.
      assertFailsWith<InvalidStateException> { map.addGeoJsonSourceData("more-places", data) }
    }
  }

  @Test
  fun setRejectsDataPreparedWithMismatchedOptions(): Unit = runSuspendTest {
    withMap { runtime, map ->
      GeoJsonSourceDataHandle.create(featureCollection(), baseOptions()).use { data ->
        map.addGeoJsonSourceData("places", data)
      }

      GeoJsonSourceDataHandle.create(featureCollection(), baseOptions().copy { buffer = 32 }).use {
        mismatched ->
        assertCommandRejected(runtime, map.setGeoJsonSourceData("places", mismatched))
      }

      // Cluster aggregations are part of the options comparison, so data
      // prepared with different clusterProperties is rejected too.
      GeoJsonSourceDataHandle.create(
          featureCollection(),
          baseOptions().copy {
            clusterProperties = "{\"total\":[\"+\",[\"get\",\"rank\"]]}".encodeToByteArray()
          },
        )
        .use { mismatched ->
          assertCommandRejected(runtime, map.setGeoJsonSourceData("places", mismatched))
        }
    }
  }

  @Test
  fun synchronousTilingOverridesALiveSource(): Unit = runSuspendTest {
    withMap { runtime, map ->
      GeoJsonSourceDataHandle.create(featureCollection()).use { data ->
        map.addGeoJsonSourceData("places", data)
        map.setGeoJsonSourceSynchronousTiling("places", true)
        GeoJsonSourceDataHandle.create(featureCollection()).use { update ->
          map.setGeoJsonSourceData("places", update)
        }
        map.setGeoJsonSourceSynchronousTiling("places", false)
      }
      assertCommandRejected(runtime, map.setGeoJsonSourceSynchronousTiling("missing", true))
    }
  }

  /** Asserts the command's terminal outcome is FAILED with INVALID_ARGUMENT detail. */
  private suspend fun assertCommandRejected(runtime: RuntimeHandle, commandId: ULong) {
    runtime.barrier()
    val matches =
      runtime.drainEvents().events.filter {
        (it.payload as? RuntimeEventPayload.CommandFinished)?.commandId == commandId
      }
    assertEquals(1, matches.size, "terminal outcome count for command $commandId")
    val payload = matches.single().payload as RuntimeEventPayload.CommandFinished
    assertEquals(CommandDisposition.FAILED, payload.disposition)
    assertEquals(MaplibreStatus.INVALID_ARGUMENT.nativeCode, matches.single().code)
  }

  private suspend fun withMap(block: suspend (RuntimeHandle, MapHandle) -> Unit) {
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
      block(runtime, map)
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
