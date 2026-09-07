package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertTrue
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.runOnBackgroundThread

class RuntimeExecutorTest {
  @Test
  fun aCommandCommittedFromAnotherThreadIsVisibleToTheCallingThread(): Unit = runSuspendTest {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 64
            height = 64
          },
        )
        .await()
        .use { map ->
          var committed = 0L
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
          assertTrue(committed > 0L, "a committed command publishes a generation")

          // An ordered query behind that command observes the generation it published.
          assertTrue(map.queryCamera().await().generation >= committed)
        }
    }
  }
}
