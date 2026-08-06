package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.elapsedMillis
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.UnsupportedFeatureException
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.resource.ResourceProviderDecision
import org.maplibre.nativeffi.withMap

/**
 * A runtime, and the thread this binding runs it on.
 *
 * Kotlin/Wasm runs on the pthread the module gives `main()`, where blocking is legal, so a runtime
 * is created, pumped, polled, and closed from the thread the tests themselves run on. That thread
 * is the runtime's owner thread as far as the C API is concerned, and it is the only one this
 * binding has.
 *
 * The callback families MapLibre raises on its own worker threads are also here, because what this
 * target reports for them is part of the runtime's public shape: a worker is a separate JavaScript
 * agent and cannot enter this module, so a host callback that has to answer one is refused and a
 * native rule table takes its place.
 */
class RuntimeHandleBrowserTest {
  // Spec coverage: BND-023, BND-040, BND-042, BND-080, BND-088, BND-089, BND-192.

  @Test
  fun aRuntimeIsCreatedAndClosedOnTheOwnerThread() {
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

  @Test
  fun aRuntimeWillNotCloseWhileOneOfItsMapsIsLive() {
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

  @Test
  fun aWakeSourceStaysUsableAfterTheRuntimeItCameFromIsGone() {
    // Spec coverage: BND-089.
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val wake = runtime.acquireWakeSource()

    // A pump takes the flag a signal raised, so the next park is not released by it. The pump
    // below would otherwise return on a flag this test never cleared.
    wake.signal()
    runtime.pump(0)

    val idle = elapsedMillis { runtime.pump(IDLE_PUMP_MILLIS) }
    assertTrue(
      idle >= IDLE_PUMP_MILLIS / 2,
      "the pump returned after $idle ms, so the wake flag outlived the pump that took it",
    )

    // Hosts tear the two down in either order, so a source outlives its runtime.
    runtime.close()
    wake.signal()
    wake.close()

    assertTrue(wake.isClosed)
    assertFailsWith<InvalidStateException> { wake.signal() }
  }

  /**
   * A pump that parks is released by native work rather than running to its timeout.
   *
   * This is what makes a blocking pump usable at all on this target. Kotlin holds the only thread
   * it has while `mln_runtime_pump` waits, so a wait that ended only on the timeout would cost a
   * host the whole timeout on every idle frame. MapLibre's own threads are what end it: the style
   * parse below finishes on a worker and posts back, and the parked wait returns as soon as it
   * lands.
   *
   * The timeout is far longer than any style this suite loads takes, so a pump that spent it is
   * unmistakable in the elapsed time.
   */
  @Test
  fun aParkedPumpIsReleasedByNativeWorkRatherThanByItsTimeout() {
    // Spec coverage: BND-088.
    withMap { runtime, map ->
      map.setStyleJson(EMPTY_STYLE_JSON)

      val waited = elapsedMillis { runtime.pump(PUMP_TIMEOUT_MILLIS) }

      assertTrue(waited < PUMP_TIMEOUT_MILLIS, "the pump waited $waited ms for its timeout")
      assertNotNull(runtime.pollEvent(), "the pump returned early with nothing to report")
    }
  }

  @Test
  fun anOfflineOperationHoldsItsRuntimeOpen() {
    // Spec coverage: BND-042.
    val runtime = RuntimeHandle.create(RuntimeOptions().apply { cachePath = ":memory:" })
    val operation = runtime.startAmbientCacheOperation(AmbientCacheOperation.INVALIDATE)

    assertEquals(OfflineOperationKind.AMBIENT_CACHE, operation.kind)
    assertFailsWith<InvalidStateException> { runtime.close() }

    operation.close()
    runtime.close()

    assertTrue(runtime.isClosed)
  }

  @Test
  fun anOutgoingHeaderTransformIsRefusedWhileClearingIsServed() {
    // Spec coverage: BND-158, BND-159 recorded as inapplicable; see the note below.
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

  /**
   * The two callback families whose common form this target cannot answer.
   *
   * Both are raised on whichever MapLibre thread wants the resource, and each of those is a
   * separate JavaScript agent that cannot enter this module. A binding that accepted the callback
   * and answered it from somewhere else would be answering a question MapLibre has already moved
   * past, so the registration is refused and the queued provider and the rewrite rule table take
   * its place. `ResourceProviderBrowserTest` covers both of those.
   */
  @Test
  fun theSynchronousProviderAndTransformFormsAreRefusedWhileClearingIsServed() {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      assertFailsWith<UnsupportedFeatureException> {
        runtime.setResourceProvider { _, _ -> ResourceProviderDecision.PASS_THROUGH }
      }
      assertFailsWith<UnsupportedFeatureException> { runtime.setResourceTransform { null } }

      // Clearing serves whatever is installed, including nothing, so a host tearing down does not
      // have to know which form its provider took.
      runtime.clearResourceProvider()
      runtime.clearResourceTransform()
    }
  }

  private companion object {
    /** Long enough that a pump which ran to its timeout is unmistakable in the elapsed time. */
    const val PUMP_TIMEOUT_MILLIS = 5000L

    /** Short enough not to slow the suite, long enough to distinguish from an immediate return. */
    const val IDLE_PUMP_MILLIS = 200L
  }
}
