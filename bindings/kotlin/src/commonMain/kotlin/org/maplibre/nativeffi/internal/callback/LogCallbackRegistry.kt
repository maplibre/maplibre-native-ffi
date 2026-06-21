package org.maplibre.nativeffi.internal.callback

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.maplibre.nativeffi.internal.status.Status

/** Process-global callback replacement registry shared by platform log bridges. */
@OptIn(ExperimentalAtomicApi::class)
internal class LogCallbackRegistry<T : AutoCloseable> {
  private val updateLock = AtomicInt(0)
  private val installed = AtomicInt(0)
  private val current = AtomicReference<T?>(null)

  fun set(replacement: T, install: () -> Int) {
    var previous: T? = null
    try {
      withUpdateLock {
        if (installed.load() == 0) {
          Status.check(install())
          installed.store(1)
        }
        previous = current.exchange(replacement)
      }
    } catch (error: Throwable) {
      replacement.close()
      throw error
    }
    previous?.close()
  }

  fun clear(clearNative: () -> Int) {
    var previous: T? = null
    withUpdateLock {
      if (installed.load() != 0) {
        Status.check(clearNative())
        installed.store(0)
      }
      previous = current.exchange(null)
    }
    previous?.close()
  }

  fun current(): T? = current.load()

  private inline fun <R> withUpdateLock(block: () -> R): R {
    while (!updateLock.compareAndSet(0, 1)) {
      // Spin briefly; log callback registration is process-global and infrequent.
    }
    try {
      return block()
    } finally {
      updateLock.store(0)
    }
  }
}
