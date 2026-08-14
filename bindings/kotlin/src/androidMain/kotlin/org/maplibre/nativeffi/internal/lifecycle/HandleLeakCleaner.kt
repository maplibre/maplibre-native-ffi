package org.maplibre.nativeffi.internal.lifecycle

import java.util.concurrent.ConcurrentHashMap
import org.maplibre.nativeffi.render.OwnedTextureFrameHandleCore
import org.maplibre.nativeffi.runtime.OperationLeakReport

/**
 * Reports native handles that become unreachable before explicit release.
 *
 * Cleanup remains explicit so failures are observable. Render sessions also require their graphics
 * thread, which an unreachable-action thread cannot provide.
 *
 * Registered actions must capture leak-report state only. Capturing the wrapper would keep it
 * reachable and suppress every report.
 */
internal object HandleLeakCleaner {
  private val leakReportActions = UnreachableActions.isolated("maplibre-leak-reports")

  /** Keeps host callback state reachable for as long as native may invoke it. */
  private val nativeCallbackRoots = ConcurrentHashMap.newKeySet<Any>()

  /** Reports [leakReport] when [handle] becomes unreachable before explicit release. */
  fun register(handle: Any, leakReport: HandleStateCore.LeakReport) {
    leakReportActions.register(handle, LeakReportAction(leakReport))
  }

  /** Retains callback state independently of its public native-owner wrapper. */
  fun retainNativeCallbackRoot(root: Any) {
    nativeCallbackRoots.add(root)
  }

  /** Releases callback state after native can no longer invoke it. */
  fun releaseNativeCallbackRoot(root: Any?) {
    if (root != null) {
      nativeCallbackRoots.remove(root)
    }
  }

  /** Reports [frameCore] when [handle] becomes unreachable before explicit release. */
  fun registerFrame(handle: Any, frameCore: OwnedTextureFrameHandleCore) {
    leakReportActions.register(handle, FrameLeakAction(frameCore))
  }

  /** Reports [leakReport] when an operation becomes unreachable. */
  fun registerOperation(handle: Any, leakReport: OperationLeakReport) {
    leakReportActions.register(handle, OperationLeakAction(leakReport))
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

  private class OperationLeakAction(private val leakReport: OperationLeakReport) : Runnable {
    override fun run() {
      leakReport.report()
    }
  }
}
