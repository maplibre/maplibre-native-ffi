package org.maplibre.nativeffi.internal.lifecycle

import java.lang.ref.Cleaner

/**
 * Runs registered actions once their referents become unreachable.
 *
 * A registered action must hold cleanup state only. An action that captures its referent keeps the
 * referent strongly reachable from this registry, so the action never runs.
 */
internal class UnreachableActions private constructor(threadName: String) {
  // The worker lives for the process, so it drops inherited thread locals and pins
  // the binding's class loader instead of retaining the context of whichever thread
  // registered first.
  private val cleaner: Cleaner = Cleaner.create { action ->
    Thread(null, action, threadName, 0, false).apply {
      isDaemon = true
      contextClassLoader = UnreachableActions::class.java.classLoader
    }
  }

  /** Runs [action] once [referent] becomes unreachable. */
  fun register(referent: Any, action: Runnable) {
    cleaner.register(referent, action)
  }

  internal companion object {
    private val shared = UnreachableActions("maplibre-unreachable-actions")

    /** Runs [action] on the shared reclamation worker once [referent] becomes unreachable. */
    fun register(referent: Any, action: Runnable) {
      shared.register(referent, action)
    }

    /** Creates an independent worker for actions that may block. */
    fun isolated(threadName: String): UnreachableActions = UnreachableActions(threadName)
  }
}
