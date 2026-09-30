package org.maplibre.nativeffi.lifecycle

import java.io.PrintStream
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.runSuspendTest

/**
 * The leak report for an abandoned handle reaches standard output, which Android sends to logcat,
 * from the leak-report worker rather than the thread that dropped the handle.
 */
class AbandonedHandleAndroidTest {
  @Test
  fun anAbandonedMapIsReportedOnTheLeakReportWorker() {
    val stdout = System.out
    val reported = CountDownLatch(1)
    var reportThread: String? = null
    System.setOut(
      object : PrintStream(stdout, true) {
        override fun println(x: String?) {
          super.println(x)
          if (x != null && LEAK_LINE.matches(x) && reportThread == null) {
            reportThread = Thread.currentThread().name
            reported.countDown()
          }
        }

        override fun println(x: Any?) = println(x?.toString())
      }
    )
    try {
      runSuspendTest { abandonAMapAndAwaitItsDisposal() }
      // The report follows the disposal that cancelled the request.
      assertTrue(
        reported.await(REPORT_WAIT_SECONDS, TimeUnit.SECONDS),
        "the map was never reported",
      )
      assertEquals(LEAK_REPORT_THREAD, reportThread)
    } finally {
      System.setOut(stdout)
    }
  }

  private companion object {
    const val REPORT_WAIT_SECONDS = 10L
    const val LEAK_REPORT_THREAD = "maplibre-leak-reports"
    val LEAK_LINE = Regex("""Leaked MapHandle native handle 0x[0-9a-f]+; close it explicitly\.""")
  }
}
