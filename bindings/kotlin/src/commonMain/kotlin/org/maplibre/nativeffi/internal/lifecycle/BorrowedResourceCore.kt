package org.maplibre.nativeffi.internal.lifecycle

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi

/** Platform-neutral close state for resources that allow temporary active borrows. */
@OptIn(ExperimentalAtomicApi::class)
internal class BorrowedResourceCore(
  private val typeName: String,
  private val releaseNative: () -> Unit,
) : AutoCloseable {
  private val state = AtomicInt(0)
  private val nativeReleased = AtomicInt(0)

  fun <T> withOpenResource(block: () -> T): T {
    retain()
    try {
      return block()
    } finally {
      releaseBorrow()
    }
  }

  override fun close() {
    while (true) {
      val current = state.load()
      if (current < 0) return
      if (state.compareAndSet(current, CLOSED_FLAG or current)) {
        if (current == 0) releaseNativeOnce()
        return
      }
    }
  }

  fun releaseNativeForCleaner() {
    releaseNativeOnce()
  }

  private fun retain() {
    while (true) {
      val current = state.load()
      check(current >= 0) { "$typeName is already closed" }
      check(current < ACTIVE_MASK) { "too many active $typeName borrows" }
      if (state.compareAndSet(current, current + 1)) return
    }
  }

  private fun releaseBorrow() {
    while (true) {
      val current = state.load()
      val active = current and ACTIVE_MASK
      check(active > 0) { "$typeName borrow count underflow" }
      val next = current - 1
      if (state.compareAndSet(current, next)) {
        if (next == CLOSED_FLAG) releaseNativeOnce()
        return
      }
    }
  }

  private fun releaseNativeOnce() {
    if (nativeReleased.compareAndSet(0, 1)) releaseNative()
  }

  private companion object {
    private const val CLOSED_FLAG = Int.MIN_VALUE
    private const val ACTIVE_MASK = Int.MAX_VALUE
  }
}
