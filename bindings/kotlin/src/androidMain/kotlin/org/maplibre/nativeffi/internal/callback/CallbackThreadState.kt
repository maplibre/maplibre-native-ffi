package org.maplibre.nativeffi.internal.callback

internal actual class CallbackThreadState actual constructor() {
  // `ThreadLocal.withInitial` arrives at API 26 while this binding supports API 24, so the initial
  // value comes from the override that covers the whole supported range.
  private val depth =
    object : ThreadLocal<Int>() {
      override fun initialValue(): Int = 0
    }

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
