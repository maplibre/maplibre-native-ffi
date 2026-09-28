package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertTrue
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.RuntimeEventMask
import org.maplibre.nativeffi.runOnBackgroundThread

class RuntimeExecutorTest {
  @Test
  fun aCommandCommittedFromAnotherThreadIsVisibleToTheCallingThread(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u)
            )
        )
        .await()
        .use { map ->
          var committed = 0uL
          var failure: Throwable? = null
          runOnBackgroundThread {
            try {
              committed =
                runSuspendTest { map.setEventMask(RuntimeEventMask.ALL).awaitCommitted() }
                  .generation
            } catch (error: Throwable) {
              failure = error
            }
          }
          failure?.let { throw it }
          assertTrue(committed > 0uL, "a committed command publishes a generation")

          // An ordered query behind that command observes the generation it published.
          assertTrue(map.cameraQuery().await().generation >= committed)
        }
    }
  }
}
