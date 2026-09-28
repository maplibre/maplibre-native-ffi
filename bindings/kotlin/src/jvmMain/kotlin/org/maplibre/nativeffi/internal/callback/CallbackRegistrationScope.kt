package org.maplibre.nativeffi.internal.callback

import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.atomic.AtomicLong
import org.maplibre.nativeffi.internal.lifecycle.WeakBox

internal actual object CallbackRoots {
  private val next = AtomicLong(1)
  private val roots = ConcurrentHashMap<Long, WeakBox<CallbackRoot>>()

  actual fun retain(root: CallbackRoot): Long =
    next.getAndIncrement().also { roots[it] = WeakBox(root) }

  actual fun get(token: Long): CallbackRoot? = roots[token]?.get()

  actual fun release(token: Long) {
    roots.remove(token)?.get()?.release()
  }
}

internal actual object CallbackContext {
  private val local = ThreadLocal<CallbackScope?>()
  actual var current: CallbackScope?
    get() = local.get()
    set(value) {
      if (value == null) local.remove() else local.set(value)
    }
}
