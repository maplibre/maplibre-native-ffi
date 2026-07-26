package org.maplibre.nativeffi.runtime

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi

/** Report-only cleanup state for an owner-thread offline operation. */
@OptIn(ExperimentalAtomicApi::class)
internal class OfflineOperationLeakReport(
  private val id: Long,
  private val kind: OfflineOperationKind,
  private val resultKind: OfflineOperationResultKind,
  private val writeLine: (String) -> Unit = { message -> println(message) },
) {
  private val closed = AtomicInt(0)

  fun markClosed() {
    closed.store(1)
  }

  fun report() {
    if (closed.load() == 0) {
      writeLine(
        "Leaked OfflineOperationHandle id=$id kind=$kind resultKind=$resultKind; " +
          "take or discard operations explicitly on the runtime owner thread."
      )
    }
  }
}
