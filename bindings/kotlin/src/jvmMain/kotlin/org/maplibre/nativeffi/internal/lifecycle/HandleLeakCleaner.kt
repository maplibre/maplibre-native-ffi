package org.maplibre.nativeffi.internal.lifecycle

import java.lang.ref.Cleaner
import org.maplibre.nativeffi.render.OwnedTextureFrameHandleCore
import org.maplibre.nativeffi.runtime.OfflineOperationLeakReport

/**
 * Reports owner-thread-affine native handles that become unreachable before explicit release.
 *
 * Registration runs a diagnostic line on a cleaner thread once the public wrapper is unreachable.
 * Runtime, map, projection, and render-session handles are bound to their owner thread, so this
 * hook MUST NOT destroy them: explicit release on the owner thread stays the only path that frees
 * native state. Reporting a leak keeps the failure visible while leaving native ownership
 * untouched.
 *
 * Registered actions capture leak-report state only. Capturing the wrapper would keep it reachable
 * and suppress every report.
 */
internal object HandleLeakCleaner {
  private val cleaner: Cleaner = Cleaner.create()

  /** Reports [leakReport] when [handle] becomes unreachable before explicit release. */
  fun register(handle: Any, leakReport: HandleStateCore.LeakReport) {
    cleaner.register(handle, LeakReportAction(leakReport))
  }

  /** Reports [frameCore] when [handle] becomes unreachable before explicit release. */
  fun registerFrame(handle: Any, frameCore: OwnedTextureFrameHandleCore) {
    cleaner.register(handle, FrameLeakAction(frameCore))
  }

  /** Reports [leakReport] when an offline operation becomes unreachable. */
  fun registerOfflineOperation(handle: Any, leakReport: OfflineOperationLeakReport) {
    cleaner.register(handle, OfflineOperationLeakAction(leakReport))
  }

  private class LeakReportAction(private val leakReport: HandleStateCore.LeakReport) : Runnable {
    override fun run() {
      leakReport.report()
    }
  }

  private class FrameLeakAction(private val frameCore: OwnedTextureFrameHandleCore) : Runnable {
    override fun run() {
      frameCore.reportLeak()
    }
  }

  private class OfflineOperationLeakAction(private val leakReport: OfflineOperationLeakReport) :
    Runnable {
    override fun run() {
      leakReport.report()
    }
  }
}
