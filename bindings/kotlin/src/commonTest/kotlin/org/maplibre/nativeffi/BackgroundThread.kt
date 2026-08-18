package org.maplibre.nativeffi

import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi

/** Runs [block] on a second native thread and waits for it to finish. */
internal expect fun runOnBackgroundThread(block: () -> Unit)

@OptIn(ExperimentalAtomicApi::class)
internal fun failureFromBackgroundThread(block: () -> Unit): Throwable {
  val result = AtomicReference<Throwable?>(null)
  runOnBackgroundThread { result.store(runCatching(block).exceptionOrNull()) }
  return result.load() ?: error("background-thread operation succeeded")
}
