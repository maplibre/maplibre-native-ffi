package org.maplibre.nativeffi.runtime

import androidx.test.platform.app.InstrumentationRegistry
import java.io.File
import kotlin.test.Test
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.denyingProvider
import org.maplibre.nativeffi.generated.MapMode
import org.maplibre.nativeffi.generated.ResourceProviderDecision
import org.maplibre.nativeffi.generated.RuntimeEventType
import org.maplibre.nativeffi.render.withOwnedTexture
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.withMap

/** The Android asset and file sources, which only run on a device. */
class AndroidAssetStyleTest {
  @Test fun styleLoadsFromAssetScheme(): Unit = runSuspendTest { loadStyle("asset://style.json") }

  @Test
  fun styleLoadsFromAndroidAssetFileUri(): Unit = runSuspendTest {
    loadStyle("file:///android_asset/style.json")
  }

  @Test
  fun styleLoadsFromFilesystemFileUri(): Unit = runSuspendTest {
    val instrumentation = InstrumentationRegistry.getInstrumentation()
    val downloaded = File(instrumentation.targetContext.filesDir, "downloaded-style.json")
    instrumentation.context.assets.open("style.json").use { input ->
      downloaded.outputStream().use { input.copyTo(it) }
    }
    loadStyle("file://${downloaded.absolutePath}")
  }

  @Test
  fun pmtilesAssetSourceReadsRangedMetadata(): Unit = runSuspendTest {
    // A static map renders its still image only once every source has loaded, and a PMTiles
    // source loads only when its ranged reads of the archive's header and metadata succeed. A range
    // miss returns the whole archive, which fails the source and the still image with it.
    withOwnedTexture(width = 64, height = 64, provider = localFilesProvider()) {
      setStyle(PMTILES_STYLE)
      renderStill()
    }
  }

  private suspend fun loadStyle(styleUrl: String) {
    withMap(mapMode = MapMode.STATIC, provider = localFilesProvider()) {
      map.setStyleUrl(styleUrl).awaitWithin("the style command")
      awaitMapEvent(RuntimeEventType.MAP_STYLE_LOADED)
    }
  }
}

/** Passes asset and file URLs through to the platform file sources, and denies the rest. */
private fun localFilesProvider() = denyingProvider { request, _ ->
  val url = request.requestedUrl.orEmpty()
  if (url.startsWith("asset://") || url.startsWith("file://") || url.startsWith("pmtiles://")) {
    ResourceProviderDecision.PASS_THROUGH
  } else null
}

private const val PMTILES_STYLE =
  """{"version":8,"sources":{"tiles":{"type":"vector","url":"pmtiles://asset://range-check.pmtiles"}},"layers":[]}"""
