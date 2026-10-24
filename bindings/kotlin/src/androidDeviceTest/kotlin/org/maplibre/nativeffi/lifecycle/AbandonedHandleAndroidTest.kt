package org.maplibre.nativeffi.lifecycle

import java.util.Locale
import kotlin.test.Test
import kotlin.test.assertTrue
import kotlinx.coroutines.CompletableDeferred
import org.maplibre.nativeffi.internal.callback.LOG_TAG
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleState
import org.maplibre.nativeffi.runSuspendTest

/**
 * The leak report for an abandoned handle reaches logcat as a warning. The JVM reports from its
 * leak-report worker; see AbandonedHandleJvmTest.
 */
class AbandonedHandleAndroidTest {
  @Test
  fun anAbandonedMapIsReportedAsALogWarning() = runSuspendTest {
    // Earlier tests may have leaked maps too, so the test reads only what the binding warns from
    // now on. An epoch time names that moment in any time zone.
    val since = String.format(Locale.US, "%.3f", System.currentTimeMillis() / 1000.0)
    abandonAMapAndAwaitItsDisposal()
    awaitLeakReportsSoFar()
    // A dump, because Android 13 and later hold back an app's streaming log reads until the user
    // grants log access.
    val warnings =
      ProcessBuilder("logcat", "-d", "-T", since, "-s", "$LOG_TAG:W").start().inputStream.use {
        it.bufferedReader().readText()
      }
    assertTrue(LEAK_LINE.containsMatchIn(warnings), "the map was never reported:\n$warnings")
  }

  /**
   * Returns once the leak-report worker has finished every report queued so far: it runs one action
   * at a time, so a marker registered after them runs after they finish.
   */
  private suspend fun awaitLeakReportsSoFar() {
    val reached = CompletableDeferred<Unit>()
    registerMarker(reached)
    awaitWhileCollecting("the leak-report worker to reach a marker", reached)
  }

  // A function of its own, so nothing keeps the marker reachable once it returns.
  private fun registerMarker(reached: CompletableDeferred<Unit>) {
    HandleLeakCleaner.register(
      Any(),
      HandleState("LeakReportMarker", 0L) { reached.complete(Unit) },
    )
  }

  private companion object {
    val LEAK_LINE = Regex("""Leaked MapHandle native handle 0x[0-9a-f]+; close it explicitly\.""")
  }
}
