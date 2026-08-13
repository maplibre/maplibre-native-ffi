package org.maplibre.nativeffi.internal.lifecycle

import java.util.concurrent.CopyOnWriteArrayList
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.runtime.OperationLeakReport

class HandleLeakCleanerTest {
  // BND-044. Destruction is owner-thread-bound, so the cleaner thread only reports.

  @Test
  fun unreachableHandleReportsLeakWithoutExplicitRelease() {
    val reports = CopyOnWriteArrayList<String>()

    registerUnreachableHandle(reports)

    assertTrue(awaitReport(reports), "expected the cleaner to report the unreachable handle")
    assertEquals(
      "Leaked RuntimeHandle native handle 0x1234; close handles explicitly on their owner thread.",
      reports.single(),
    )
  }

  @Test
  fun releasedHandleStaysSilentWhenCollected() {
    val reports = CopyOnWriteArrayList<String>()
    val liveReports = CopyOnWriteArrayList<String>()

    registerReleasedHandle(reports)
    // A second unreleased registration proves the cleaner ran, so the silence above is a
    // real result.
    registerUnreachableHandle(liveReports)

    assertTrue(awaitReport(liveReports), "expected the cleaner to run")
    assertEquals(emptyList(), reports)
  }

  @Test
  fun unreachableOperationReportsLeak() {
    val reports = CopyOnWriteArrayList<String>()

    HandleLeakCleaner.registerOperation(Any(), OperationLeakReport(reports::add))

    assertTrue(awaitReport(reports), "expected the cleaner to report the operation")
    assertEquals("Leaked OperationHandle; close the operation explicitly.", reports.single())
  }

  @Test
  fun blockedLeakReportDoesNotBlockNativeReclamationWorker() {
    val reportStarted = CountDownLatch(1)
    val unblockReport = CountDownLatch(1)
    val reclaimed = CountDownLatch(1)

    registerBlockingLeakReport(reportStarted, unblockReport)
    UnreachableActions.register(Any(), reclaimed::countDown)

    try {
      assertTrue(awaitAction(reportStarted), "expected the leak-report worker to start")
      assertTrue(awaitAction(reclaimed), "expected native reclamation to use another worker")
    } finally {
      unblockReport.countDown()
    }
  }

  /** Registers a handle that is unreachable once this call returns. */
  private fun registerUnreachableHandle(reports: MutableList<String>) {
    HandleLeakCleaner.register(
      Any(),
      HandleStateCore.LeakReport("RuntimeHandle", 0x1234L, reports::add),
    )
  }

  /** Registers an explicitly released handle that is unreachable once this call returns. */
  private fun registerReleasedHandle(reports: MutableList<String>) {
    val leakReport = HandleStateCore.LeakReport("MapHandle", 0x5678L, reports::add)
    leakReport.markReleased()
    HandleLeakCleaner.register(Any(), leakReport)
  }

  /** Registers a diagnostic that blocks after its handle becomes unreachable. */
  private fun registerBlockingLeakReport(started: CountDownLatch, unblock: CountDownLatch) {
    HandleLeakCleaner.register(
      Any(),
      HandleStateCore.LeakReport("RuntimeHandle", 0x1234L) {
        started.countDown()
        unblock.await()
      },
    )
  }

  private fun awaitAction(action: CountDownLatch): Boolean {
    repeat(ATTEMPTS) {
      if (action.await(POLL_MILLIS, TimeUnit.MILLISECONDS)) return true
      System.gc()
    }
    return action.count == 0L
  }

  private fun awaitReport(reports: List<String>): Boolean {
    repeat(ATTEMPTS) {
      if (reports.isNotEmpty()) return true
      System.gc()
      Thread.sleep(POLL_MILLIS)
    }
    return reports.isNotEmpty()
  }

  private companion object {
    private const val ATTEMPTS = 100
    private const val POLL_MILLIS = 20L
  }
}
