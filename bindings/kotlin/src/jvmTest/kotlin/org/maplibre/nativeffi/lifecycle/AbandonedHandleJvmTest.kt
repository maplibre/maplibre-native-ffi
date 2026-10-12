package org.maplibre.nativeffi.lifecycle

import java.io.PrintStream
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import kotlin.system.exitProcess
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNotNull
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.libraryProperties
import org.maplibre.nativeffi.runChildJvm
import org.maplibre.nativeffi.runSuspendTest

/**
 * The leak report for an abandoned handle reaches standard output from the leak-report worker. The
 * probe runs in a child JVM, so the test reads the output the way a host sees it.
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
    val stdout = System.out
    val reported = CountDownLatch(1)
    // Tags each line with the thread that printed it.
    System.setOut(
      object : PrintStream(stdout, true) {
        override fun println(x: String?) {
          super.println("[${Thread.currentThread().name}] $x")
          if (x?.startsWith("Leaked MapHandle ") == true) reported.countDown()
        }

        override fun println(x: Any?) = println(x?.toString())
      }
    )
    Maplibre.loadNativeLibrary()
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
