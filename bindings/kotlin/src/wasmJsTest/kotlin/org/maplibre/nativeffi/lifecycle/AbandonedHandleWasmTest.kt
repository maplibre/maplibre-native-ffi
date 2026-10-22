package org.maplibre.nativeffi.lifecycle

import kotlin.test.Test
import kotlin.test.assertNotNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.withConsoleLog

/**
 * The leak report for an abandoned handle reaches console.log from the FinalizationRegistry task
 * that disposes it. The JVM reports from its leak-report worker; see AbandonedHandleJvmTest.
 */
class AbandonedHandleWasmTest {
  @Test
  fun anAbandonedMapIsReportedOnConsoleLog() = runSuspendTest {
    val reports = mutableListOf<String>()
    withConsoleLog({ line -> if (line.startsWith("Leaked MapHandle ")) reports += line }) {
      // The task that disposes the map writes the report before the cancel reaches the page.
      abandonAMapAndAwaitItsDisposal()
    }
    val report = reports.singleOrNull()
    assertNotNull(report, "expected one leak report, got $reports")
    assertTrue(LEAK_LINE.matches(report), report)
  }
}

private val LEAK_LINE =
  Regex("""Leaked MapHandle native handle 0x[0-9a-f]+; close it explicitly\.""")
