package org.maplibre.nativeffi.runtime

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.browserTest
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.UnsupportedFeatureException
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.maplibreScope
import org.maplibre.nativeffi.nextPageTask
import org.maplibre.nativeffi.pageTimeMillis
import org.maplibre.nativeffi.runOnNextPageTask

/**
 * A runtime, and the owner thread the module runs it on.
 *
 * Every call here is placed on the module's owner thread and waited for by parking the page's
 * Kotlin stack on a promise. The thread that created the runtime is its owner thread as far as the
 * C API is concerned, so a call that reached native from anywhere else would report a wrong-thread
 * status rather than succeed.
 */
class RuntimeHandleBrowserTest {
  // Spec coverage: BND-023, BND-040, BND-042, BND-080, BND-088, BND-089, BND-192.

  @Test
  fun aRuntimeIsCreatedAndClosedOnTheOwnerThread(): Promise<JsAny?> = browserTest {
    maplibreScope {
      val runtime = RuntimeHandle.create(RuntimeOptions())
      // A second reference to the same handle, so release is observed through every alias rather
      // than only through the one that closed it.
      val alias = runtime

      assertFalse(runtime.isClosed)
      runtime.pump(0)
      assertNull(runtime.pollEvent())

      runtime.close()
      // The second release is a no-op rather than a second native destroy.
      runtime.close()

      assertTrue(runtime.isClosed)
      assertTrue(alias.isClosed)
      assertFailsWith<InvalidStateException> { runtime.pump(0) }
      assertFailsWith<InvalidStateException> { alias.pollEvent() }
    }
  }

  @Test
  fun aRuntimeWillNotCloseWhileOneOfItsMapsIsLive(): Promise<JsAny?> = browserTest {
    maplibreScope {
      val runtime = RuntimeHandle.create(RuntimeOptions())
      val map =
        MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 64
            height = 64
          },
        )
      try {
        val error = assertFailsWith<InvalidStateException> { runtime.close() }

        assertEquals(MaplibreStatus.INVALID_STATE, error.status)
        assertEquals("RuntimeHandle has 1 live child handle(s): MapHandle", error.diagnostic)
        assertFalse(runtime.isClosed)

        // The refused close left the runtime usable, so a host can close the child and retry.
        runtime.pump(0)
      } finally {
        map.close()
      }

      runtime.close()
      assertTrue(runtime.isClosed)
    }
  }

  @Test
  fun oneScopeConfinesCreatePumpPollAndCloseToTheOwnerThread(): Promise<JsAny?> = browserTest {
    // The browser binding's owner-thread adapter is the scope: the module owns one thread, every
    // owner-affine call is placed on it, and the page waits by parking. A runtime created in one
    // scope is therefore usable in the next, because it is the same thread underneath both — which
    // is what says the adapter confines rather than merely serializes.
    val runtime = maplibreScope { RuntimeHandle.create(RuntimeOptions()) }
    try {
      maplibreScope {
        runtime.pump(0)
        while (runtime.pollEvent() != null) {}
      }
      // Between the two scopes the page ran ordinary tasks of its own, and the owner thread stayed
      // where it was.
      nextPageTask()
      maplibreScope { runtime.pump(0) }
    } finally {
      maplibreScope { runtime.close() }
    }
    assertTrue(runtime.isClosed)
  }

  @Test
  fun aWakeSourceStaysUsableAfterTheRuntimeItCameFromIsGone(): Promise<JsAny?> = browserTest {
    maplibreScope {
      val runtime = RuntimeHandle.create(RuntimeOptions())
      val wake = runtime.acquireWakeSource()

      // A pump takes the flag a signal raised, so the next park is not released by it. The pump
      // below would otherwise return on a flag this test never cleared.
      wake.signal()
      runtime.pump(0)

      val idleStarted = pageTimeMillis()
      runtime.pump(IDLE_PUMP_MILLIS)
      assertTrue(
        pageTimeMillis() - idleStarted >= IDLE_PUMP_MILLIS / 2,
        "the pump returned early, so the wake flag outlived the pump that took it",
      )

      // Hosts tear the two down in either order, so a source outlives its runtime.
      runtime.close()
      wake.signal()
      wake.close()

      assertTrue(wake.isClosed)
      assertFailsWith<InvalidStateException> { wake.signal() }
    }
  }

  @Test
  fun aWakeSourceReleasesAPumpThePageServicedMeanwhile(): Promise<JsAny?> = browserTest {
    maplibreScope {
      RuntimeHandle.create(RuntimeOptions()).use { runtime ->
        runtime.acquireWakeSource().use { wake ->
          var pageTaskRan = false
          // Queued before the pump and serviced while it is outstanding, which is what the whole
          // design rests on: the owner thread parks, and the page does not.
          runOnNextPageTask {
            pageTaskRan = true
            wake.signal()
          }

          val started = pageTimeMillis()
          runtime.pump(PUMP_TIMEOUT_MILLIS)
          val waited = pageTimeMillis() - started

          assertTrue(pageTaskRan, "the page ran no task while a pump was outstanding")
          assertTrue(waited < PUMP_TIMEOUT_MILLIS, "the pump waited $waited ms for its timeout")
        }
      }
    }
  }

  @Test
  fun anOfflineOperationHoldsItsRuntimeOpen(): Promise<JsAny?> = browserTest {
    // Spec coverage: BND-042.
    maplibreScope {
      val runtime = RuntimeHandle.create(RuntimeOptions().apply { cachePath = ":memory:" })
      val operation = runtime.startAmbientCacheOperation(AmbientCacheOperation.INVALIDATE)

      assertEquals(OfflineOperationKind.AMBIENT_CACHE, operation.kind)
      assertFailsWith<InvalidStateException> { runtime.close() }

      operation.close()
      runtime.close()

      assertTrue(runtime.isClosed)
    }
  }

  @Test
  fun anOutgoingHeaderTransformIsRefusedWhileClearingIsServed(): Promise<JsAny?> = browserTest {
    // Spec coverage: BND-158, BND-159 recorded as inapplicable; see the note below.
    maplibreScope {
      RuntimeHandle.create(RuntimeOptions()).use { runtime ->
        // The browser's fetch transport follows redirects itself, so it cannot keep a transformed
        // header out of a cross-origin hop. The C API reports the same status for the same reason.
        assertFailsWith<UnsupportedFeatureException> {
          runtime.setHttpHeaderTransform { emptyList() }
        }
        // Clearing is served anyway, so a host that tears down unconditionally does not have to
        // know that installation was refused.
        runtime.clearHttpHeaderTransform()
      }
    }
  }

  private companion object {
    /** Long enough that a pump which ran to its timeout is unmistakable in the elapsed time. */
    const val PUMP_TIMEOUT_MILLIS = 5000L

    /** Short enough not to slow the suite, long enough to distinguish from an immediate return. */
    const val IDLE_PUMP_MILLIS = 200L
  }
}
