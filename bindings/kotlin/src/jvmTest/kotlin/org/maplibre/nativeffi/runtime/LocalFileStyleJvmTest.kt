package org.maplibre.nativeffi.runtime

import java.nio.file.Files
import kotlin.test.Test
import kotlin.test.assertTrue
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions

class LocalFileStyleJvmTest {
  @Test
  fun localFileStyleLoadsFromCanonicalEncodedUri() {
    val tempDirectory = Files.createTempDirectory("maplibre resources ké地図")
    try {
      val styleFile =
        Files.createDirectories(tempDirectory.resolve("style sheets").resolve("スタイル"))
          .resolve("style.json")
      Files.writeString(styleFile, STYLE_JSON)

      RuntimeHandle.create(RuntimeOptions()).use { runtime ->
        val map =
          MapHandle.create(
            runtime,
            MapOptions().apply {
              width = 64
              height = 64
            },
          )
        try {
          map.setStyleUrl(styleFile.toUri().toASCIIString())
          assertTrue(waitForStyleLoaded(runtime, map))
        } finally {
          map.close()
        }
      }
    } finally {
      tempDirectory.toFile().deleteRecursively()
    }
  }

  private fun waitForStyleLoaded(runtime: RuntimeHandle, map: MapHandle): Boolean {
    repeat(10_000) {
      runtime.pump(1)
      while (true) {
        val event = runtime.pollEvent() ?: break
        if (event.type == RuntimeEventType.MAP_STYLE_LOADED && event.mapSource == map) return true
      }
    }
    return false
  }
}

private const val STYLE_JSON = """{"version":8,"sources":{},"layers":[]}"""
