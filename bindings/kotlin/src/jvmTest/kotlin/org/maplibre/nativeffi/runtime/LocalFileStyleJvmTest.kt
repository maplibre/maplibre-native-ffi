package org.maplibre.nativeffi.runtime

import java.nio.file.Files
import kotlin.test.Test
import kotlin.test.assertTrue
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.RuntimeEventSourceType
import org.maplibre.nativeffi.generated.RuntimeEventType
import org.maplibre.nativeffi.generated.RuntimeHandle

class LocalFileStyleJvmTest {
  @Test
  fun localFileStyleLoadsFromCanonicalEncodedUri(): Unit = runSuspendTest {
    val tempDirectory = Files.createTempDirectory("maplibre resources ké地図")
    try {
      val styleFile =
        Files.createDirectories(tempDirectory.resolve("style sheets").resolve("スタイル"))
          .resolve("style.json")
      Files.writeString(styleFile, EMPTY_STYLE_JSON)

      GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
        val map =
          runtime
            .mapCreate(
              GeneratedApi.mapOptionsDefault()
                .copy(
                  initialExtent =
                    GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u)
                )
            )
            .await()
        try {
          map.setStyleUrl(styleFile.toUri().toASCIIString()).await()
          assertTrue(waitForStyleLoaded(runtime, map))
        } finally {
          map.release()
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
        runtime
          .drainEvents()
          .use { it.get().events }
          .any {
            it.type == RuntimeEventType.MAP_STYLE_LOADED &&
              it.sourceType == RuntimeEventSourceType.MAP
          }
      ) {
        return true
      }
    }
    return false
  }
}
