package org.maplibre.nativeffi.internal.lifecycle

/** Disposes unreachable owners on the native any-thread disposal path. */
internal object HandleLeakCleaner {
  private val leakReportActions = UnreachableActions.isolated("maplibre-leak-reports")

  /** Runs native disposal when [handle] becomes unreachable before explicit release. */
  fun register(handle: Any, leakReport: HandleStateCore.LeakReport) {
    leakReportActions.register(handle, LeakReportAction(leakReport))
  }

  private class LeakReportAction(private val leakReport: HandleStateCore.LeakReport) : Runnable {
    override fun run() {
      leakReport.report()
    }
  }
}
