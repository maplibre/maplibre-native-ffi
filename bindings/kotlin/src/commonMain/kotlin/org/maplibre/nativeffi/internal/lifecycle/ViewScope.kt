package org.maplibre.nativeffi.internal.lifecycle

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi

/** Tracks callback-scoped access to a borrowed native view and the values read from it. */
@OptIn(ExperimentalAtomicApi::class)
internal class ViewScope {
  private val active = AtomicInt(1)

  fun ensureActive() {
    check(active.load() != 0) { "borrowed view is no longer active" }
  }

  fun close() {
    active.store(0)
  }
}
