package org.maplibre.nativeffi.internal.callback

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.maplibre.nativeffi.internal.status.Status

/** Platform-neutral callback entry gate with blocking close-after-last-callback cleanup. */
@OptIn(ExperimentalAtomicApi::class)
internal class CallbackGate(private val name: String, private val closeNative: () -> Unit = {}) :
  AutoCloseable {
  private val state = AtomicInt(0)
  private val nativeClosed = AtomicInt(0)
  private val threadState = CallbackThreadState()

  fun enter(): Lease? {
    while (true) {
      val current = state.load()
      if (current and (CLOSING_FLAG or CLOSED_FLAG) != 0) return null
      val activeCallbacks = current and ACTIVE_MASK
      check(activeCallbacks < ACTIVE_MASK) { "too many active $name" }
      if (state.compareAndSet(current, current + 1)) {
        threadState.enter()
        return Lease(this)
      }
    }
  }

  fun checkCanClose() {
    if (threadState.isInCallback()) {
      throw Status.callbackReentry(name)
    }
  }

  override fun close() {
    val closingFromCallback = threadState.isInCallback()
    while (true) {
      val current = state.load()
      if (current and CLOSED_FLAG != 0) return
      if (state.compareAndSet(current, current or CLOSING_FLAG)) {
        break
      }
    }
    if (closingFromCallback) return
    while (true) {
      val current = state.load()
      if (current and CLOSED_FLAG != 0) return
      if (current and ACTIVE_MASK == 0) {
        if (state.compareAndSet(current, CLOSED_FLAG)) {
          closeNativeOnce()
          return
        }
      } else {
        yieldCallbackClose()
      }
    }
  }

  fun isClosedForTesting(): Boolean = state.load() and CLOSED_FLAG != 0

  private fun exit() {
    while (true) {
      val current = state.load()
      val activeCallbacks = current and ACTIVE_MASK
      check(activeCallbacks > 0) { "$name count underflow" }
      val next =
        if (activeCallbacks == 1 && current and CLOSING_FLAG != 0) {
          CLOSED_FLAG
        } else {
          current - 1
        }
      if (state.compareAndSet(current, next)) {
        threadState.exit()
        if (next == CLOSED_FLAG) closeNativeOnce()
        return
      }
    }
  }

  private fun closeNativeOnce() {
    if (nativeClosed.compareAndSet(0, 1)) closeNative()
  }

  internal class Lease(private val gate: CallbackGate) : AutoCloseable {
    private val closed = AtomicInt(0)

    override fun close() {
      if (closed.compareAndSet(0, 1)) gate.exit()
    }
  }

  private companion object {
    private const val CLOSED_FLAG = Int.MIN_VALUE
    private const val CLOSING_FLAG = 1 shl 30
    private const val ACTIVE_MASK = CLOSING_FLAG - 1
  }
}

internal expect class CallbackThreadState() {
  fun enter()

  fun exit()

  fun isInCallback(): Boolean
}

internal expect fun yieldCallbackClose()
