package org.maplibre.nativeffi.internal.callback

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.maplibre.nativeffi.internal.lifecycle.yieldThread

// Kotlin/Native has no concurrent map, so one lock guards the table. Each section is a
// single map operation.
@OptIn(ExperimentalAtomicApi::class)
internal actual object NativeRoots {
  private val locked = AtomicInt(0)
  private var next = 1L
  private val roots = HashMap<Long, Any>()

  private inline fun <T> locked(block: () -> T): T {
    while (!locked.compareAndSet(0, 1)) yieldThread()
    try {
      return block()
    } finally {
      locked.store(0)
    }
  }

  actual fun retain(value: Any): Long = locked { next++.also { roots[it] = value } }

  actual fun get(token: Long): Any? = locked { roots[token] }

  actual fun release(token: Long): Any? = locked { roots.remove(token) }
}
