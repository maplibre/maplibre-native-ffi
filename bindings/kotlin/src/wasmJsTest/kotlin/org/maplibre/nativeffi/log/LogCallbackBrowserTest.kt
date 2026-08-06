package org.maplibre.nativeffi.log

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertNotNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.callback.CallbackRing
import org.maplibre.nativeffi.internal.wasm.InjectedFaults
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.pumpTurns
import org.maplibre.nativeffi.pumpUntil
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.withMap

/**
 * The process-global log callback, which reaches host code through the module's record ring.
 *
 * MapLibre logs from whichever thread reaches the condition, and none of them may enter this
 * WebAssembly instance. So the C shim copies each record into a bounded ring and signals the
 * runtime's wake source, and the binding drains that ring inside `pump`. Two things follow, and
 * both shape every test here.
 *
 * A record arrives on a pump rather than on the call that provoked it, so a test pumps before it
 * asserts on what a callback received.
 *
 * Retirement travels in the same ring, behind the records it retires. Clearing or replacing a
 * callback pushes a marker, and the drain stops delivering to that callback when the marker comes
 * out — not at the moment the host asked. Which is why every claim below about a callback going
 * quiet is made about records produced *after* the clear.
 */
class LogCallbackBrowserTest {
  // Spec coverage: BND-120, BND-121, BND-122, BND-123.

  @Test
  fun aRecordNativeProducesReachesTheInstalledCallback() {
    val records = mutableListOf<LogRecord>()
    withFailingStyleLoads { runtime, map ->
      Maplibre.setLogCallback({ records += it }, consume = false)

      map.setStyleUrl(UNSERVED_STYLE_URL)
      assertTrue(pumpUntil(runtime) { records.isNotEmpty() }, "no log record reached the callback")

      val record = assertNotNull(records.firstOrNull())
      assertTrue(record.message.isNotBlank())
      assertTrue(record.severity.nativeValue > 0)
      // Copied out of the record before the drain released it, so it still reads afterwards.
      assertEquals(record.message, records.first().message)
    }
  }

  @Test
  fun clearingStopsDeliveryAndAReplacementIsTheOnlyOneCalled() {
    val first = mutableListOf<LogRecord>()
    val second = mutableListOf<LogRecord>()
    withFailingStyleLoads { runtime, map ->
      Maplibre.setLogCallback({ first += it }, consume = false)
      map.setStyleUrl(UNSERVED_STYLE_URL)
      assertTrue(pumpUntil(runtime) { first.isNotEmpty() }, "the first callback heard nothing")

      // Installed with `consume` set, which is the other half of the registration: the decision is
      // fixed here because MapLibre needs it on the logging thread, long before the record is
      // drained.
      Maplibre.setLogCallback({ second += it }, consume = true)
      val firstAfterReplace = quiesce(runtime, first)

      map.setStyleUrl(UNSERVED_STYLE_URL + "?replaced")
      assertTrue(pumpUntil(runtime) { second.isNotEmpty() }, "the replacement heard nothing")
      assertEquals(
        firstAfterReplace,
        first.size,
        "the replaced callback was still called for records produced after it was replaced",
      )

      Maplibre.clearLogCallback()
      val secondAfterClear = quiesce(runtime, second)

      map.setStyleUrl(UNSERVED_STYLE_URL + "?cleared")
      pumpTurns(runtime, QUIET_PUMPS)
      assertEquals(secondAfterClear, second.size, "a cleared callback was called again")

      // Clearing one that is already cleared stays a successful no-op.
      Maplibre.clearLogCallback()
    }
  }

  @Test
  fun aFailingCallbackIsContainedAndLaterRecordsStillArrive() {
    val seen = mutableListOf<LogRecord>()
    withFailingStyleLoads { runtime, map ->
      Maplibre.setLogCallback(
        {
          seen += it
          // The drain is what runs this, and nothing above it is a native frame to unwind into. A
          // failure here must not stop the records behind this one.
          throw IllegalStateException("contained")
        },
        consume = false,
      )

      map.setStyleUrl(UNSERVED_STYLE_URL)
      assertTrue(pumpUntil(runtime) { seen.size >= 2 }, "the drain stopped at the failed callback")

      // And the runtime is unharmed: it still pumps, and still delivers.
      map.setStyleUrl(UNSERVED_STYLE_URL + "?after-failure")
      val before = seen.size
      assertTrue(pumpUntil(runtime) { seen.size > before }, "delivery stopped after a failure")
    }
  }

  /**
   * Replacing or clearing from inside the callback being replaced.
   *
   * The body runs inside the drain, inside the pump that is delivering to it, so retiring its
   * registration would be a close waiting on the frame below it. There is one thread and one stack
   * here, so that wait can never finish and is refused instead.
   */
  @Test
  fun aCallbackCannotBeReplacedOrClearedFromInsideItself() {
    var replaceError: Throwable? = null
    var clearError: Throwable? = null
    withFailingStyleLoads { runtime, map ->
      Maplibre.setLogCallback(
        {
          replaceError =
            runCatching { Maplibre.setLogCallback({}, consume = false) }.exceptionOrNull()
          clearError = runCatching { Maplibre.clearLogCallback() }.exceptionOrNull()
        },
        consume = false,
      )

      map.setStyleUrl(UNSERVED_STYLE_URL)
      assertTrue(pumpUntil(runtime) { replaceError != null }, "the callback was never called")

      assertTrue(replaceError is InvalidStateException, "replace reported $replaceError")
      assertTrue(clearError is InvalidStateException, "clear reported $clearError")
    }
  }

  /**
   * A replacement native refuses leaves the callback that was already there receiving records.
   *
   * The binding installs the replacement's registration state before it tells native, because the
   * shim reads the listener through the state it was given. So at the moment native refuses, the
   * binding holds a registration native has never heard of, and it has to give that one back.
   *
   * Native has no refusal of its own to offer — the arguments the binding passes are always valid
   * by then — so it is injected.
   */
  // Spec coverage: BND-122.
  @Test
  fun aLogCallbackReplacementNativeRefusesKeepsThePreviousCallback() {
    val installed = mutableListOf<LogRecord>()
    val refused = mutableListOf<LogRecord>()
    withFailingStyleLoads { runtime, map ->
      Maplibre.setLogCallback({ installed += it }, consume = false)
      map.setStyleUrl(UNSERVED_STYLE_URL)
      assertTrue(pumpUntil(runtime) { installed.isNotEmpty() }, "the callback heard nothing")

      try {
        InjectedFaults.failNextCall(
          "mln_kotlin_log_install",
          MaplibreStatus.INVALID_ARGUMENT,
          "log callback must not be null",
        )
        val error =
          assertFailsWith<InvalidArgumentException> {
            Maplibre.setLogCallback({ refused += it }, consume = false)
          }
        assertEquals("log callback must not be null", error.diagnostic)
      } finally {
        InjectedFaults.reset()
      }

      // The one native already had is still the one it reaches.
      val before = installed.size
      map.setStyleUrl(UNSERVED_STYLE_URL + "?refused")
      assertTrue(
        pumpUntil(runtime) { installed.size > before },
        "the previous callback stopped receiving records",
      )
      assertTrue(refused.isEmpty(), "the callback native refused received records anyway")

      // A later replacement is accepted, so the refusal left the registration able to take one.
      Maplibre.setLogCallback({ refused += it }, consume = false)
      map.setStyleUrl(UNSERVED_STYLE_URL + "?accepted")
      assertTrue(pumpUntil(runtime) { refused.isNotEmpty() }, "the replacement heard nothing")
    }
  }

  /**
   * The ring does not overflow while the host keeps pumping.
   *
   * The ring is bounded and drops the oldest record when it is full, which is a delivery loss the
   * host would otherwise never learn of — so the shim counts what it dropped and the binding
   * reports the count. This is what says the bound and the drain cadence go together: a style load
   * that fails is one of the noisiest things MapLibre does, and one drain per pump keeps up with
   * it.
   */
  @Test
  fun aPumpedHostLosesNoRecordToTheRing() {
    val records = mutableListOf<LogRecord>()
    withFailingStyleLoads { runtime, map ->
      Maplibre.setLogCallback({ records += it }, consume = false)
      val droppedBefore = CallbackRing.droppedRecords

      repeat(NOISY_LOADS) { attempt ->
        map.setStyleUrl("$UNSERVED_STYLE_URL?noisy=$attempt")
        pumpTurns(runtime, NOISY_PUMPS)
      }

      assertTrue(records.isNotEmpty(), "the noisy loads produced no records at all")
      assertEquals(
        droppedBefore,
        CallbackRing.droppedRecords,
        "the ring dropped records while the host was pumping",
      )
    }
  }

  @Test
  fun aSeverityMaskRefusesAValueTheModuleHasNoBitFor() {
    assertEquals(1 shl 1, LogSeverity.INFO.nativeMask)
    assertFailsWith<InvalidArgumentException> {
      Maplibre.setAsyncLogSeverities(setOf(LogSeverity(900)))
    }
  }

  /**
   * Pumps until the drain has caught up with everything already in the ring.
   *
   * A retirement marker travels behind the records it retires, so a callback that has just been
   * replaced may still have records of its own coming. This spends the turns those need and reports
   * the count they reached, which is the baseline every "went quiet" assertion is made against.
   */
  private fun quiesce(runtime: RuntimeHandle, records: List<LogRecord>): Int {
    var settled = records.size
    repeat(QUIET_PUMPS) {
      pumpTurns(runtime, 1)
      if (records.size == settled) return settled
      settled = records.size
    }
    return records.size
  }

  /** Runs [body] with a map, with async logging on and the log callback cleared afterwards. */
  private fun withFailingStyleLoads(body: (RuntimeHandle, MapHandle) -> Unit) {
    try {
      Maplibre.setAsyncLogSeverities(
        setOf(LogSeverity.INFO, LogSeverity.WARNING, LogSeverity.ERROR)
      )
      withMap { runtime, map -> body(runtime, map) }
    } finally {
      runCatching { Maplibre.clearLogCallback() }
      Maplibre.restoreDefaultAsyncLogSeverities()
    }
  }

  private companion object {
    /** Long enough that a callback still installed would have been called at least once. */
    const val QUIET_PUMPS = 200

    const val NOISY_LOADS = 16
    const val NOISY_PUMPS = 8

    /**
     * A scheme no file source serves, which MapLibre reports through the log as well as an event.
     */
    const val UNSERVED_STYLE_URL = "jar:file:/packaged/style.json"
  }
}
