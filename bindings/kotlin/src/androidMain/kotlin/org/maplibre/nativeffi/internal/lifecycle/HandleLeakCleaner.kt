package org.maplibre.nativeffi.internal.lifecycle

internal actual fun disposeWhenUnreachable(owner: Any, state: HandleState): Any {
  HandleLeakCleaner.register(owner, state)
  return Unit
}

/** Disposes unreachable owners on the native any-thread disposal path. */
internal object HandleLeakCleaner {
  private val leakReportActions = UnreachableActions.isolated("maplibre-leak-reports")

  /** Disposes and reports [state]'s handle when [owner] becomes unreachable unreleased. */
  fun register(owner: Any, state: HandleState) {
    leakReportActions.register(owner, Runnable { state.disposeLeaked() })
  }
}
