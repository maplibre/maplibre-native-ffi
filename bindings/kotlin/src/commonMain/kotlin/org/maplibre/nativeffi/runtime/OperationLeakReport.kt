package org.maplibre.nativeffi.runtime

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi

/** Report-only cleanup state for an operation observer. */
@OptIn(ExperimentalAtomicApi::class)
internal class OperationLeakReport(
  private val writeLine: (String) -> Unit = { message -> println(message) }
) {
  private val closed = AtomicInt(0)

  fun markClosed() {
    closed.store(1)
  }

  fun report() {
    if (closed.load() == 0) {
      writeLine("Leaked OperationHandle; close the operation explicitly.")
    }
  }
}
