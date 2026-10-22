package org.maplibre.nativeffi.internal.lifecycle

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.maplibre.nativeffi.internal.status.invalidState

/** Holds off release while the binding copies outputs that native lends until release. */
@OptIn(ExperimentalAtomicApi::class)
internal class ReadHandleState(
  private val typeName: String,
  handle: Long,
  dispose: (Long) -> Unit,
) : HandleState(typeName, handle, dispose) {
  // The number of reads in progress, or minus the number of releases in progress.
  private val gate = AtomicInt(0)

  fun <T> read(block: () -> T): T {
    while (true) {
      val current = gate.load()
      if (current < 0) throw invalidState("$typeName is releasing")
      if (gate.compareAndSet(current, current + 1)) break
    }
    try {
      return block()
    } finally {
      gate.addAndFetch(-1)
    }
  }

  override fun <T> release(call: () -> T): T {
    while (true) {
      val current = gate.load()
      if (current > 0) throw invalidState("$typeName is in use")
      if (gate.compareAndSet(current, current - 1)) break
    }
    try {
      return super.release(call)
    } finally {
      gate.addAndFetch(1)
    }
  }
}
