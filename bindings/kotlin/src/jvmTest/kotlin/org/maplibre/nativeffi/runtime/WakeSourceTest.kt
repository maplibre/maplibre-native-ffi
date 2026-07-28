package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import kotlin.time.TimeSource
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions

class WakeSourceTest {
  // Leaves the runtime idle with no latched signal, so a following park can only be released by
  // the signal the test raises.
  private fun quiesce(runtime: RuntimeHandle) {
    repeat(100) {
      runtime.pump(0)
      var drained = false
      while (runtime.pollEvent() != null) {
        drained = true
      }
      if (!drained) {
        return
      }
    }
    error("the runtime kept producing events while idle")
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

    quiesce(runtime)

    // The style is malformed, so native reports the failure from its own threads. What matters
    // here is that the failure reaches a parked owner thread at all.
    map.setStyleUrl("unsupported://style.json")
    var loadingFailed = false
    val loadStarted = TimeSource.Monotonic.markNow()
    repeat(20) {
      if (!loadingFailed) {
        runtime.pump(10_000)
        assertTrue(
          loadStarted.elapsedNow().inWholeMilliseconds < 5_000,
          "parks sat out their timeouts while the style load was pending",
        )
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
    quiesce(runtime)
    val signaller = Thread {
      Thread.sleep(20)
      source.signal()
    }
    signaller.start()
    val parkStarted = TimeSource.Monotonic.markNow()
    runtime.pump(10_000)
    assertTrue(
      parkStarted.elapsedNow().inWholeMilliseconds < 5_000,
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
  fun pumpConsumesOneLatchedSignalAtATime() {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      runtime.acquireWakeSource().use { source ->
        quiesce(runtime)

        source.signal()
        val signalledStarted = TimeSource.Monotonic.markNow()
        runtime.pump(10_000)
        assertTrue(
          signalledStarted.elapsedNow().inWholeMilliseconds < 5_000,
          "a pump blocked despite a latched signal",
        )

        // The latch is spent, so an idle runtime now sits out its whole timeout.
        val idleStarted = TimeSource.Monotonic.markNow()
        runtime.pump(200)
        assertTrue(
          idleStarted.elapsedNow().inWholeMilliseconds >= 100,
          "a second pump consumed a latch the first should have spent",
        )
      }
    }
  }
}
