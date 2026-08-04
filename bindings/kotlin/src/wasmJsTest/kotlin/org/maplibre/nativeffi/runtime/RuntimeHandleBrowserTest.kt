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
import org.maplibre.nativeffi.error.UnsupportedFeatureException
import org.maplibre.nativeffi.maplibreScope
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
  @Test
  fun aRuntimeIsCreatedAndClosedOnTheOwnerThread(): Promise<JsAny?> = browserTest {
    maplibreScope {
      val runtime = RuntimeHandle.create(RuntimeOptions())

      assertFalse(runtime.isClosed)
      runtime.pump(0)
      assertNull(runtime.pollEvent())

      runtime.close()
      runtime.close()

      assertTrue(runtime.isClosed)
      assertFailsWith<InvalidStateException> { runtime.pump(0) }
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
  }
}
