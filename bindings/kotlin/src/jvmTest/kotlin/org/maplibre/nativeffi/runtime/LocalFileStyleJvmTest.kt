package org.maplibre.nativeffi.runtime

import java.nio.file.Files
import kotlin.test.Test
import kotlin.test.assertTrue
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions

class LocalFileStyleJvmTest {
  @Test
  fun localFileStyleLoadsFromCanonicalEncodedUri(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
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
              .await()
          try {
            map.setStyleUrl(styleFile.toUri().toASCIIString()).await()
            assertTrue(waitForStyleLoaded(runtime, map))
          } finally {
            map.close()
          }
        }
      } finally {
        tempDirectory.toFile().deleteRecursively()
      }
    }

  private suspend fun waitForStyleLoaded(runtime: RuntimeHandle, map: MapHandle): Boolean {
    repeat(10_000) {
      runtime.barrier().await()
      if (
        runtime.drainEvents().events.any {
          it.type == RuntimeEventType.MAP_STYLE_LOADED && it.mapSource == map
        }
      ) {
        return true
      }
    }
    return false
  }
}

private const val STYLE_JSON = """{"version":8,"sources":{},"layers":[]}"""
