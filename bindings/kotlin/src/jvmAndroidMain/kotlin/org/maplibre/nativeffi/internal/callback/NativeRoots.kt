package org.maplibre.nativeffi.internal.callback

import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.atomic.AtomicLong

// Completions and upcalls from MapLibre worker threads look roots up concurrently, so
// they take no shared lock.
internal actual object NativeRoots {
  private val next = AtomicLong(1)
  private val roots = ConcurrentHashMap<Long, Any>()

  actual fun retain(value: Any): Long = next.getAndIncrement().also { roots[it] = value }

  actual fun get(token: Long): Any? = roots[token]

  actual fun release(token: Long): Any? = roots.remove(token)
}
