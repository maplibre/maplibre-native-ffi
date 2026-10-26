package org.maplibre.nativeffi.lifecycle

import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import kotlin.system.exitProcess
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNotNull
import org.maplibre.nativeffi.libraryProperties
import org.maplibre.nativeffi.loadNativeLibrary
import org.maplibre.nativeffi.runChildJvm
import org.maplibre.nativeffi.runSuspendTest

/**
 * The leak report for an abandoned handle reaches the leak-report worker's uncaught exception
 * handler. The probe runs in a child JVM, so its default handler sees only the probe's reports.
 */
class AbandonedHandleJvmTest {
  @Test
  fun anAbandonedMapIsReportedOnTheLeakReportWorker() {
    val child = runChildJvm(AbandonedMapProbe::class, properties = libraryProperties())
    assertEquals(0, child.exitCode, "the probe failed:\n${child.output}")
    val report = child.output.lineSequence().firstNotNullOfOrNull { LEAK_LINE.matchEntire(it) }
    assertNotNull(report, "the probe printed no leak report:\n${child.output}")
    // The cleaner's own worker reports, not the thread that dropped the handle.
    assertEquals(LEAK_REPORT_THREAD, report.groupValues[1])
  }
}

/** Abandons a map, waits for its disposal, and then for its leak report. */
object AbandonedMapProbe {
  @JvmStatic
  fun main(args: Array<String>) {
    val reported = CountDownLatch(1)
    // Prints each report with the thread that reported it.
    Thread.setDefaultUncaughtExceptionHandler { thread, error ->
      println("[${thread.name}] ${error.message}")
      if (error.message?.startsWith("Leaked MapHandle ") == true) reported.countDown()
    }
    loadNativeLibrary()
    runSuspendTest { abandonAMapAndAwaitItsDisposal() }
    // The report follows the disposal that cancelled the request.
    check(reported.await(REPORT_WAIT_SECONDS, TimeUnit.SECONDS)) { "the map was never reported" }
    exitProcess(0)
  }

  private const val REPORT_WAIT_SECONDS = 10L
}

private const val LEAK_REPORT_THREAD = "maplibre-leak-reports"
private val LEAK_LINE =
  Regex("""\[(.+)] Leaked MapHandle native handle 0x[0-9a-f]+; close it explicitly\.""")
