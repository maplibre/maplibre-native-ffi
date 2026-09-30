package org.maplibre.nativeffi.internal.lifecycle

import java.util.concurrent.BlockingQueue
import java.util.concurrent.CountDownLatch
import java.util.concurrent.LinkedBlockingQueue
import java.util.concurrent.TimeUnit
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue

/** The JVM cleaner that reports and reclaims handles nobody released. */
class HandleLeakCleanerTest {
  @Test
  fun anUnreachableHandleReportsItsLeak() {
    val reports = LinkedBlockingQueue<String>()
    registerUnreachableHandle(reports)
    assertEquals(
      "Leaked RuntimeHandle native handle 0x1234; close it explicitly.",
      awaitReport(reports),
    )
  }

  @Test
  fun aBlockedLeakReportDoesNotBlockNativeReclamation() {
    val reportStarted = CountDownLatch(1)
    val unblockReport = CountDownLatch(1)
    val reclaimed = CountDownLatch(1)
    registerBlockingLeakReport(reportStarted, unblockReport)
    UnreachableActions.register(Any(), reclaimed::countDown)
    try {
      assertTrue(awaitLatch(reportStarted), "expected the leak-report worker to start")
      assertTrue(awaitLatch(reclaimed), "expected native reclamation to use another worker")
    } finally {
      unblockReport.countDown()
    }
  }

  /** Registers a handle that is unreachable once this call returns. */
  private fun registerUnreachableHandle(reports: MutableCollection<String>) {
    HandleLeakCleaner.register(
      Any(),
      HandleStateCore.LeakReport("RuntimeHandle", 0x1234L, { reports.add(it) }),
    )
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

  /** Requests collections until the cleaner delivers a report, blocking on the queue between. */
  private fun awaitReport(reports: BlockingQueue<String>): String? {
    val deadline = System.nanoTime() + WAIT_NANOS
    while (System.nanoTime() < deadline) {
      System.gc()
      reports.poll(ROUND_MILLIS, TimeUnit.MILLISECONDS)?.let {
        return it
      }
    }
    return null
  }

  private fun awaitLatch(latch: CountDownLatch): Boolean {
    val deadline = System.nanoTime() + WAIT_NANOS
    while (System.nanoTime() < deadline) {
      System.gc()
      if (latch.await(ROUND_MILLIS, TimeUnit.MILLISECONDS)) return true
    }
    return false
  }

  private companion object {
    const val WAIT_NANOS = 10_000_000_000L
    const val ROUND_MILLIS = 100L
  }
}
