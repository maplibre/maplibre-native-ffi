package org.maplibre.nativeffi.lifecycle

import java.util.Locale
import java.util.concurrent.TimeUnit
import kotlin.test.Test
import kotlin.test.assertTrue
import org.maplibre.nativeffi.internal.callback.LOG_TAG
import org.maplibre.nativeffi.runSuspendTest

/**
 * The leak report for an abandoned handle reaches logcat as a warning. The JVM reports from its
 * leak-report worker; see AbandonedHandleJvmTest.
 */
class AbandonedHandleAndroidTest {
  @Test
  fun anAbandonedMapIsReportedAsALogWarning() {
    // Earlier tests may have leaked maps too, so logcat reads only what the binding warns from now
    // on, and exits at the first leak report. An epoch time names that moment in any time zone.
    val since = String.format(Locale.US, "%.3f", System.currentTimeMillis() / 1000.0)
    val logcat =
      ProcessBuilder("logcat", "-T", since, "-s", "$LOG_TAG:W", "-e", LEAK_LINE, "-m", "1").start()
    try {
      runSuspendTest { abandonAMapAndAwaitItsDisposal() }
      // The report follows the disposal that cancelled the request.
      assertTrue(
        logcat.waitFor(REPORT_WAIT_SECONDS, TimeUnit.SECONDS),
        "the map was never reported; the binding warned since $since:\n${warningsSince(since)}",
      )
    } finally {
      logcat.destroy()
    }
  }

  private companion object {
    const val REPORT_WAIT_SECONDS = 10L
    const val LEAK_LINE = """^Leaked MapHandle native handle 0x[0-9a-f]+; close it explicitly\."""

    fun warningsSince(since: String): String =
      ProcessBuilder("logcat", "-d", "-T", since, "-s", "$LOG_TAG:W").start().inputStream.use {
        it.bufferedReader().readText()
      }
  }
}
