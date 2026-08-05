package org.maplibre.nativeffi.internal.callback

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.maplibre.nativeffi.internal.lifecycle.yieldWhileClosing
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
        yieldWhileClosing()
      }
    }
  }

  /**
   * Stops admitting callbacks and lets whatever is already inside finish on its own.
   *
   * For a callback family whose bodies may suspend. [close] waits for the last body to leave, and
   * the two ways it can do that both assume a body that runs to completion once it has been
   * entered: either the closer is the thread inside the callback, and says so, or it is a thread
   * that can usefully wait for the one that is. A body that has parked is neither. Its frame is not
   * on the closing stack, so the closer cannot be it, and it is not on another thread either, so
   * waiting cannot bring it any closer to finishing -- only the event loop can resume it, and a
   * close that spins never returns to one. Such a close does not take longer; it never completes.
   *
   * So this does not wait. What a caller gives up is the guarantee that no body is running when
   * this returns, which is only worth having for a gate that holds something native a running body
   * would be left without. [closeNative] still runs after the last body leaves, on whichever stack
   * that turns out to be, so a gate that does hold something still releases it in the right order.
   */
  fun closeWithoutDraining() {
    while (true) {
      val current = state.load()
      if (current and CLOSED_FLAG != 0) return
      if (current and ACTIVE_MASK == 0) {
        if (state.compareAndSet(current, CLOSED_FLAG)) {
          closeNativeOnce()
          return
        }
      } else if (state.compareAndSet(current, current or CLOSING_FLAG)) {
        return
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
