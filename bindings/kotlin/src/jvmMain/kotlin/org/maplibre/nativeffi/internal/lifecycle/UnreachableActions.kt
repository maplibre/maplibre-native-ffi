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
internal object UnreachableActions {
  private val cleaner: Cleaner = Cleaner.create()

  /** Runs [action] once [referent] becomes unreachable. */
  fun register(referent: Any, action: Runnable) {
    cleaner.register(referent, action)
  }
}
