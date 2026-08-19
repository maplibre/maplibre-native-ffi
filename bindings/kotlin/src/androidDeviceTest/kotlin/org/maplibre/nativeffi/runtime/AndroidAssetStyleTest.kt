package org.maplibre.nativeffi.runtime

import androidx.test.platform.app.InstrumentationRegistry
import java.io.File
import java.util.concurrent.ConcurrentLinkedQueue
import kotlin.test.Test
import kotlin.test.assertTrue
import kotlin.test.fail
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.map.MapOptions

class AndroidAssetStyleTest {
  @Test
  fun styleLoadsFromAssetScheme() {
    assertTrue(loadStyle("asset://style.json"))
  }

  @Test
  fun styleLoadsFromAndroidAssetFileUri() {
    assertTrue(loadStyle("file:///android_asset/style.json"))
  }

  @Test
  fun styleLoadsFromFilesystemFileUri() {
    val instrumentation = InstrumentationRegistry.getInstrumentation()
    val downloaded = File(instrumentation.targetContext.filesDir, "downloaded-style.json")
    instrumentation.context.assets.open("style.json").use { input ->
      downloaded.outputStream().use { input.copyTo(it) }
    }
    assertTrue(loadStyle("file://${downloaded.absolutePath}"))
  }

  @Test
  fun pmtilesAssetSourceReadsRangedMetadata() {
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
            map.setStyleJson(PMTILES_STYLE.encodeToByteArray())
            assertTrue(waitForStyleLoaded(runtime, map))
            assertTrue(map.styleSourceExists("tiles"))
            // URL sources do not expose parsed TileJSON through styleSourceInfo.
            // A range miss returns the whole archive, which is not JSON, and
            // MapLibre logs a source load failure.
            repeat(2_000) {
              runtime.pump(0)
              val failed =
                runtime.drainEvents().events.filter {
                  it.type == RuntimeEventType.MAP_LOADING_FAILED
                }
              if (failed.isNotEmpty()) {
                fail(failed.joinToString { it.message })
              }
              sourceErrors.poll()?.let { fail(it) }
              runtime.pump(1)
              waitForAsyncTestWork()
            }
          }
      }
    } finally {
      Maplibre.clearLogCallback()
    }
  }

  private fun loadStyle(styleUrl: String): Boolean {
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
          map.setStyleUrl(styleUrl)
          loaded = waitForStyleLoaded(runtime, map)
        }
    }
    return loaded
  }

  private fun waitForStyleLoaded(runtime: RuntimeHandle, map: MapHandle): Boolean {
    repeat(10_000) {
      runtime.pump(0)
      if (
        runtime.drainEvents().events.any {
          it.type == RuntimeEventType.MAP_STYLE_LOADED && it.mapSource == map
        }
      ) {
        return true
      }
      runtime.pump(1)
      waitForAsyncTestWork()
    }
    return false
  }
}

private const val PMTILES_STYLE =
  """{"version":8,"sources":{"tiles":{"type":"vector","url":"pmtiles://asset://range-check.pmtiles"}},"layers":[]}"""
