package org.maplibre.nativeffi.internal.lifecycle

import java.util.concurrent.CopyOnWriteArrayList
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.runtime.OfflineOperationKind
import org.maplibre.nativeffi.runtime.OfflineOperationLeakReport
import org.maplibre.nativeffi.runtime.OfflineOperationResultKind

class HandleLeakCleanerTest {
  // BND-044: non-deterministic cleanup hooks report leaked thread-affine handles rather than
  // destroying them. Destruction stays owner-thread-bound, so the cleaner thread only reports.

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
    // A second unreleased registration proves the cleaner ran, so silence above is a real result
    // rather than a collection that never happened.
    registerUnreachableHandle(liveReports)

    assertTrue(awaitReport(liveReports), "expected the cleaner to run")
    assertEquals(emptyList(), reports)
  }

  @Test
  fun unreachableOfflineOperationReportsLeak() {
    val reports = CopyOnWriteArrayList<String>()
    val kind = OfflineOperationKind.AMBIENT_CACHE
    val resultKind = OfflineOperationResultKind.NONE

    HandleLeakCleaner.registerOfflineOperation(
      Any(),
      OfflineOperationLeakReport(42L, kind, resultKind, reports::add),
    )

    assertTrue(awaitReport(reports), "expected the cleaner to report the offline operation")
    assertEquals(
      "Leaked OfflineOperationHandle id=42 kind=$kind resultKind=$resultKind; " +
        "take or discard operations explicitly on the runtime owner thread.",
      reports.single(),
    )
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
