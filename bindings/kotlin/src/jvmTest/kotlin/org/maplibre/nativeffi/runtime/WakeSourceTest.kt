package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions

class WakeSourceTest {
  // Leaves the runtime idle with no latched signal, so a following park can only be released by
  // the signal the test raises.
  private fun drainLatchedWakes(runtime: RuntimeHandle) {
    repeat(100) {
      if (!runtime.waitForWork(0)) {
        return
      }
      runtime.runOnce()
      while (runtime.pollEvent() != null) {
        // Drain.
      }
    }
    error("the runtime kept latching wakes while idle")
  }

  @Test
  fun parkedOwnerThreadWakesForNativeWorkAndForAWakeSource() {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 512
          height = 512
        },
      )

    drainLatchedWakes(runtime)

    // The style is malformed, so native reports the failure from its own threads. What matters
    // here is that the failure reaches a parked owner thread at all.
    map.setStyleUrl("unsupported://style.json")
    var loadingFailed = false
    repeat(20) {
      if (!loadingFailed) {
        assertTrue(runtime.waitForWork(10_000), "a park timed out while the style load was pending")
        runtime.runOnce()
        while (true) {
          val event = runtime.pollEvent() ?: break
          if (event.type == RuntimeEventType.MAP_LOADING_FAILED) {
            loadingFailed = true
          }
        }
      }
    }
    assertTrue(loadingFailed)

    // A source signalled from another thread is what a host's submission path holds, and the park
    // it releases has no other work to end it.
    val source = runtime.acquireWakeSource()
    drainLatchedWakes(runtime)
    val signaller = Thread {
      Thread.sleep(20)
      source.signal()
    }
    signaller.start()
    assertTrue(
      runtime.waitForWork(10_000),
      "the parked owner thread timed out instead of taking the signal",
    )
    signaller.join()

    // A wake source stays usable once its runtime is gone, so host teardown ordering is free.
    map.close()
    runtime.close()
    source.signal()
    source.close()
    assertTrue(source.isClosed)
    assertFailsWith<InvalidStateException> { source.signal() }
  }

  @Test
  fun waitConsumesOneLatchedSignalAtATime() {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      runtime.acquireWakeSource().use { source ->
        drainLatchedWakes(runtime)

        source.signal()
        assertTrue(runtime.waitForWork(0))
        // The latch is consumed, so an idle runtime reports the timeout instead.
        assertFalse(runtime.waitForWork(0))
      }
    }
  }
}
