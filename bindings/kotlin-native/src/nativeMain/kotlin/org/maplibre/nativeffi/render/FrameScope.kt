package org.maplibre.nativeffi.render

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi

/** Tracks callback-scoped access to borrowed native render frame handles. */
@OptIn(ExperimentalAtomicApi::class)
internal class FrameScope {
  private val active = AtomicInt(1)

  fun ensureActive() {
    check(active.load() != 0) { "render frame is no longer active" }
  }

  fun close() {
    active.store(0)
  }
}
