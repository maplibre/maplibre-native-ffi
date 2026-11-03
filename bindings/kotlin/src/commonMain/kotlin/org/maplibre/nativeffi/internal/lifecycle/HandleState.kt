package org.maplibre.nativeffi.internal.lifecycle

import kotlin.concurrent.atomics.AtomicBoolean
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.maplibre.nativeffi.MaplibreException
import org.maplibre.nativeffi.Status
import org.maplibre.nativeffi.internal.callback.reportUncaught

/**
 * What a generated owner tracks about its native [handle].
 *
 * Native validates every handle and leases it for each call, so a call on a released handle reaches
 * native, which reports it. The binding keeps only what native cannot: whether a release ended the
 * handle, which tells unreachable-owner cleanup whether to dispose it. A state holds neither its
 * owner nor the owner's parent, so cleanup can hold the state without keeping either reachable.
 */
@OptIn(ExperimentalAtomicApi::class)
internal open class HandleState(
  private val typeName: String,
  val handle: Long,
  private val dispose: (Long) -> Unit,
  /** Whether a callback borrows the handle, which the binding then never releases or disposes. */
  val borrowed: Boolean = false,
) {
  private val released = AtomicBoolean(false)

  val isReleased: Boolean
    get() = released.load()

  /**
   * Runs the release [call] and marks the handle released once native accepts it. A release that
   * native refuses leaves the flag clear, so a later one can retry. Native decides which concurrent
   * release ends the handle.
   */
  open fun <T> release(call: () -> T): T {
    checkReleasable()
    return call().also { released.store(true) }
  }

  /**
   * Runs the answer [call], which ends the handle whatever it returns. Native ends a live handle on
   * every outcome, and a failure disposes the handle in case it came before native saw the answer.
   */
  fun answer(call: () -> Unit) {
    checkReleasable()
    try {
      call()
    } catch (error: Throwable) {
      if (released.compareAndSet(false, true)) {
        try {
          dispose(handle)
        } catch (cleanup: Throwable) {
          error.addSuppressed(cleanup)
        }
      }
      throw error
    }
    released.store(true)
  }

  /** Refuses to release a handle that a callback borrows. */
  protected fun checkReleasable() {
    if (borrowed) {
      throw MaplibreException(
        Status.INVALID_STATE,
        "$typeName is borrowed by a callback, which can't release it",
      )
    }
  }

  /**
   * Disposes a handle that its owner never released, then reports the leak, with any disposal
   * failure as its cause, where the binding reports callback failures. Does nothing for a released
   * handle.
   */
  fun disposeLeaked() {
    if (borrowed || !released.compareAndSet(false, true)) return
    val failure =
      try {
        dispose(handle)
        null
      } catch (error: Throwable) {
        error
      }
    reportUncaught(
      IllegalStateException(
        "Leaked $typeName native handle 0x${handle.toString(16)}; release it explicitly.",
        failure,
      ),
    )
  }
}
