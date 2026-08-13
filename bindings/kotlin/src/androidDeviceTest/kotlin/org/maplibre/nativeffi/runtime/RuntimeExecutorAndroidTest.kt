package org.maplibre.nativeffi.runtime

import java.util.concurrent.CompletableFuture
import kotlin.test.Test
import kotlin.test.assertTrue
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions

class RuntimeExecutorAndroidTest {
  @Test
  fun runtimeAndMapProgressAcrossCoroutineResumptionAndHostThreads(): Unit = runSuspendTest {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 64
          height = 64
        },
      )
    val generation = CompletableFuture.supplyAsync { map.snapshot().generation }.get()
    runtime.barrier()
    assertTrue(map.queryCamera().generation >= generation)
    map.close()
    runtime.close()
  }
}
