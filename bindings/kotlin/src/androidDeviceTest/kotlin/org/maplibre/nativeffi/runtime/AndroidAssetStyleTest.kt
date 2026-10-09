package org.maplibre.nativeffi.runtime

import androidx.test.platform.app.InstrumentationRegistry
import java.io.File
import kotlin.test.Test
import kotlin.test.assertEquals
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.denyingProvider
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.MapMode
import org.maplibre.nativeffi.generated.ResourceProviderDecision
import org.maplibre.nativeffi.generated.RuntimeEventSourceType
import org.maplibre.nativeffi.generated.RuntimeEventType
import org.maplibre.nativeffi.generated.RuntimeOptions
import org.maplibre.nativeffi.render.withOwnedTexture
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.withMap

/** The Android asset and file sources, and the runtime asset root, which only run on a device. */
class AndroidAssetStyleTest {
  @Test
  fun styleLoadsFromAssetScheme(): Unit = runSuspendTest {
    // A null or empty asset path both select the APK's asset root.
    loadStyle("asset://style.json")
    loadStyle("asset://style.json", assetPath = "")
  }

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
  fun styleLoadsFromApkAssetRoot(): Unit = runSuspendTest {
    for (assetPath in listOf("/android_asset/maps", "/android_asset/maps/")) {
      loadStyle(
        "asset://style%20with%20spaces.json?revision=1#style",
        assetPath = assetPath,
        expectedLayer = "custom-root",
      )
    }
  }

  @Test
  fun styleLoadsFromFilesystemAssetRoot(): Unit = runSuspendTest {
    loadStyle("asset://style.json", downloadedAssets().absolutePath, "custom-root")
  }

  @Test
  fun explicitFileUrisIgnoreAssetRoot(): Unit = runSuspendTest {
    val directory = downloadedAssets()
    loadStyle("file:///android_asset/style.json", directory.absolutePath)
    loadStyle(
      "file://${File(directory, "style.json").toURI().rawPath}",
      "/android_asset",
      "custom-root",
    )
  }

  @Test
  fun pmtilesAssetSourceReadsRangedMetadata(): Unit = runSuspendTest {
    renderPmtiles("asset://range-check.pmtiles")
  }

  @Test
  fun pmtilesAssetFileUriReadsRangedMetadata(): Unit = runSuspendTest {
    renderPmtiles("file:///android_asset/range-check.pmtiles")
  }

  @Test
  fun pmtilesFilesystemAssetRootReadsRangedMetadata(): Unit = runSuspendTest {
    renderPmtiles("asset://downloaded.pmtiles", downloadedAssets().absolutePath)
  }

  /** Writes a style and a PMTiles archive to app storage, for use as a filesystem asset root. */
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

  private suspend fun renderPmtiles(archiveUrl: String, assetPath: String? = null) {
    // A static map renders its still image only once every source has loaded, and a PMTiles
    // source loads only when its ranged reads of the archive's header and metadata succeed. A range
    // miss returns the whole archive, which fails the source and the still image with it.
    withOwnedTexture(
      width = 64,
      height = 64,
      provider = localFilesProvider(),
      runtimeOptions = runtimeOptions(assetPath),
    ) {
      setStyle(
        """{"version":8,"sources":{"tiles":{"type":"vector","url":"pmtiles://$archiveUrl"}},"layers":[]}"""
      )
      renderStill()
    }
  }

  /** Loads [styleUrl] under [assetPath], and asserts that the style holds [expectedLayer] only. */
  private suspend fun loadStyle(
    styleUrl: String,
    assetPath: String? = null,
    expectedLayer: String? = null,
  ) {
    withMap(
      mapMode = MapMode.STATIC,
      provider = localFilesProvider(),
      runtimeOptions = runtimeOptions(assetPath),
    ) {
      assertCommitted(map.setStyleUrl(styleUrl).awaitWithin("the style command"))
      // A failed load reports MAP_LOADING_FAILED instead, so fail fast on it with its message.
      val loaded =
        runtimeFixture.awaitEvent("the style $styleUrl to load") {
          it.sourceType == RuntimeEventSourceType.MAP &&
            (it.type == RuntimeEventType.MAP_STYLE_LOADED ||
              it.type == RuntimeEventType.MAP_LOADING_FAILED)
        }
      assertEquals(RuntimeEventType.MAP_STYLE_LOADED, loaded.type, loaded.message)
      assertEquals(
        listOfNotNull(expectedLayer),
        map.listStyleLayerIds().awaitWithin("the style layer ids"),
      )
    }
  }

  private fun runtimeOptions(assetPath: String?): RuntimeOptions =
    GeneratedApi.runtimeOptionsDefault().copy(assetPath = assetPath)
}

/** Passes asset and file URLs through to the platform file sources, and denies the rest. */
private fun localFilesProvider() = denyingProvider { request, _ ->
  val url = request.requestedUrl.orEmpty()
  if (url.startsWith("asset://") || url.startsWith("file://") || url.startsWith("pmtiles://")) {
    ResourceProviderDecision.PASS_THROUGH
  } else null
}

private const val CUSTOM_ROOT_STYLE =
  """{"version":8,"sources":{},"layers":[{"id":"custom-root","type":"background"}]}"""
