package org.maplibre.nativeffi.internal.lifecycle

import java.lang.ref.Cleaner

/**
 * Runs registered actions once their referents become unreachable.
 *
 * A registered action holds cleanup state only. An action that captures its referent keeps the
 * referent strongly reachable from this registry, so the registration stays pending and the action
 * never runs.
 *
 * The Android source set mirrors this contract with `PhantomReference` and `ReferenceQueue`,
 * because `java.lang.ref.Cleaner` arrives at API 33 while the binding supports API 24.
 */
internal class UnreachableActions private constructor(threadName: String) {
  private val cleaner: Cleaner = Cleaner.create { action ->
    Thread(action, threadName).apply { isDaemon = true }
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
