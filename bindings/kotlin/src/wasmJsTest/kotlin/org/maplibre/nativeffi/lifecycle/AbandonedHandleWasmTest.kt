package org.maplibre.nativeffi.lifecycle

import kotlin.test.Test
import kotlin.test.assertNotNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.interceptUncaught
import org.maplibre.nativeffi.runSuspendTest

/**
 * The leak report for an abandoned handle reaches the page's reportError from the
 * FinalizationRegistry task that disposes it. The JVM reports from its leak-report worker; see
 * AbandonedHandleJvmTest.
 */
class AbandonedHandleWasmTest {
  @Test
  fun anAbandonedMapIsReportedOnConsoleLog() = runSuspendTest {
    val reports = mutableListOf<String>()
    val restore =
      interceptUncaught { failure ->
        val message = failure.message.orEmpty()
        message.startsWith("Leaked MapHandle ").also { if (it) reports += message }
      }!!
    try {
      // The task that disposes the map reports before the cancel reaches the page.
      abandonAMapAndAwaitItsDisposal()
    } finally {
      restore()
    }
    val report = reports.singleOrNull()
    assertNotNull(report, "expected one leak report, got $reports")
    assertTrue(LEAK_LINE.matches(report), report)
  }
}

private val LEAK_LINE =
  Regex("""Leaked MapHandle native handle 0x[0-9a-f]+; close it explicitly\.""")
