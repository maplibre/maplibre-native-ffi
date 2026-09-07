package org.maplibre.nativeffi.runtime

import androidx.test.platform.app.InstrumentationRegistry
import java.io.File
import java.util.concurrent.ConcurrentLinkedQueue
import kotlin.test.Test
import kotlin.test.assertNotNull
import kotlin.test.assertTrue
import kotlin.test.fail
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.sleepMillis

class AndroidAssetStyleTest {
  @Test
  fun styleLoadsFromAssetScheme(): Unit = runSuspendTest {
    assertTrue(loadStyle("asset://style.json"))
  }

  @Test
  fun styleLoadsFromAndroidAssetFileUri(): Unit = runSuspendTest {
    assertTrue(loadStyle("file:///android_asset/style.json"))
  }

  @Test
  fun styleLoadsFromFilesystemFileUri(): Unit = runSuspendTest {
    val instrumentation = InstrumentationRegistry.getInstrumentation()
    val downloaded = File(instrumentation.targetContext.filesDir, "downloaded-style.json")
    instrumentation.context.assets.open("style.json").use { input ->
      downloaded.outputStream().use { input.copyTo(it) }
    }
    assertTrue(loadStyle("file://${downloaded.absolutePath}"))
  }

  @Test
  fun pmtilesAssetSourceReadsRangedMetadata(): Unit = runSuspendTest {
    val sourceErrors = ConcurrentLinkedQueue<String>()
    Maplibre.setLogCallback(
      LogCallback { record ->
        if (record.message.contains("Failed to load source tiles")) {
          sourceErrors.add(record.message)
        }
        false
      }
    )
    try {
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
            map.setStyleJson(PMTILES_STYLE.encodeToByteArray()).await()
            assertTrue(waitForStyleLoaded(runtime, map))
            assertNotNull(map.styleSourceInfo("tiles").await())
            // URL sources do not expose parsed TileJSON through styleSourceInfo.
            // A range miss returns the whole archive, which is not JSON, and
            // MapLibre logs a source load failure.
            repeat(2_000) {
              runtime.barrier().await()
              val failed =
                runtime.drainEvents().filter { it.type == RuntimeEventType.MAP_LOADING_FAILED }
              if (failed.isNotEmpty()) {
                fail(failed.joinToString { it.message })
              }
              sourceErrors.poll()?.let { fail(it) }
              sleepMillis(1)
            }
          }
      }
    } finally {
      Maplibre.clearLogCallback()
    }
  }

  private suspend fun loadStyle(styleUrl: String): Boolean {
    var loaded = false
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
          map.setStyleUrl(styleUrl).await()
          loaded = waitForStyleLoaded(runtime, map)
        }
    }
    return loaded
  }

  private suspend fun waitForStyleLoaded(runtime: RuntimeHandle, map: MapHandle): Boolean {
    repeat(10_000) {
      runtime.barrier().await()
      if (
        runtime.drainEvents().any {
          it.type == RuntimeEventType.MAP_STYLE_LOADED && it.mapSource == map
        }
      ) {
        return true
      }
      sleepMillis(1)
    }
    return false
  }
}

private const val PMTILES_STYLE =
  """{"version":8,"sources":{"tiles":{"type":"vector","url":"pmtiles://asset://range-check.pmtiles"}},"layers":[]}"""
