package org.maplibre.nativeffi.runtime

import androidx.test.platform.app.InstrumentationRegistry
import java.io.File
import java.util.concurrent.ConcurrentLinkedQueue
import kotlin.test.Test
import kotlin.test.assertEquals
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
    assertTrue(loadStyle("asset://style.json", assetPath = ""))
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
  fun styleLoadsFromApkAssetRoot() {
    for (assetPath in listOf("/android_asset/maps", "/android_asset/maps/")) {
      assertTrue(
        loadStyle(
          "asset://style%20with%20spaces.json?revision=1#style",
          assetPath = assetPath,
          expectedLayer = "custom-root",
        )
      )
    }
  }

  @Test
  fun styleLoadsFromFilesystemAssetRoot() {
    val directory = downloadedAssets()
    assertTrue(loadStyle("asset://style.json", directory.absolutePath, "custom-root"))
  }

  @Test
  fun explicitFileUrisIgnoreAssetRoot() {
    val directory = downloadedAssets()
    assertTrue(loadStyle("file:///android_asset/style.json", directory.absolutePath))
    assertTrue(
      loadStyle(
        "file://${File(directory, "style.json").toURI().rawPath}",
        "/android_asset",
        "custom-root",
      )
    )
  }

  @Test
  fun pmtilesAssetSourceReadsRangedMetadata() {
    checkPmtiles("asset://range-check.pmtiles")
  }

  @Test
  fun pmtilesAssetFileUriReadsRangedMetadata() {
    checkPmtiles("file:///android_asset/range-check.pmtiles")
  }

  @Test
  fun pmtilesFilesystemAssetRootReadsRangedMetadata() {
    checkPmtiles("asset://downloaded.pmtiles", downloadedAssets().absolutePath)
  }

  private fun downloadedAssets(): File {
    val instrumentation = InstrumentationRegistry.getInstrumentation()
    // A directory path is literal; only the requested URL is percent-decoded.
    val directory = File(instrumentation.targetContext.filesDir, "asset root %20")
    directory.mkdirs()
    File(directory, "style.json").writeText(CUSTOM_ROOT_STYLE)
    instrumentation.context.assets.open("range-check.pmtiles").use { input ->
      File(directory, "downloaded.pmtiles").outputStream().use { input.copyTo(it) }
    }
    return directory
  }

  private fun checkPmtiles(archiveUrl: String, assetPath: String? = null) {
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
      RuntimeHandle.create(RuntimeOptions().apply { this.assetPath = assetPath }).use { runtime ->
        MapHandle.create(
            runtime,
            MapOptions().apply {
              width = 64
              height = 64
              mapMode = MapMode.STATIC
            },
          )
          .use { map ->
            map.setStyleJson(
              """{"version":8,"sources":{"tiles":{"type":"vector","url":"pmtiles://$archiveUrl"}},"layers":[]}"""
                .encodeToByteArray()
            )
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

  private fun loadStyle(
    styleUrl: String,
    assetPath: String? = null,
    expectedLayer: String? = null,
  ): Boolean {
    var loaded = false
    RuntimeHandle.create(RuntimeOptions().apply { this.assetPath = assetPath }).use { runtime ->
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
          if (loaded) {
            assertEquals(listOfNotNull(expectedLayer), map.styleLayerIds())
          }
        }
    }
    return loaded
  }

  private fun waitForStyleLoaded(runtime: RuntimeHandle, map: MapHandle): Boolean {
    repeat(10_000) {
      runtime.pump(0)
      val events = runtime.drainEvents().events
      events
        .firstOrNull { it.type == RuntimeEventType.MAP_LOADING_FAILED }
        ?.let { fail(it.message) }
      if (events.any { it.type == RuntimeEventType.MAP_STYLE_LOADED && it.mapSource == map }) {
        return true
      }
      runtime.pump(1)
      waitForAsyncTestWork()
    }
    return false
  }
}

private const val CUSTOM_ROOT_STYLE =
  """{"version":8,"sources":{},"layers":[{"id":"custom-root","type":"background"}]}"""
