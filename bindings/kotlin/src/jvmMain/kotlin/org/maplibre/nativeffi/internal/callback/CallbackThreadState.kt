package org.maplibre.nativeffi.internal.callback

internal actual class CallbackThreadState actual constructor() {
  private val depth = ThreadLocal.withInitial { 0 }

  actual fun enter() {
    depth.set((depth.get() ?: 0) + 1)
  }

  actual fun exit() {
    val next = (depth.get() ?: 0) - 1
    if (next == 0) {
      depth.remove()
    } else {
      depth.set(next)
    }
  }

  actual fun isInCallback(): Boolean = (depth.get() ?: 0) > 0
}

internal actual fun yieldCallbackClose() {
  Thread.yield()
}
