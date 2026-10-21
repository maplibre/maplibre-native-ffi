package org.maplibre.nativeffi.internal.lifecycle

import kotlin.concurrent.atomics.AtomicBoolean
import kotlin.concurrent.atomics.ExperimentalAtomicApi

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
) {
  private val released = AtomicBoolean(false)

  val isReleased: Boolean
    get() = released.load()

  /**
   * Runs the release [call] and marks the handle released once native accepts it. A release that
   * native refuses leaves the flag clear, so a later one can retry. Native decides which concurrent
   * release ends the handle.
   */
  open fun <T> release(call: () -> T): T = call().also { released.store(true) }

  /** Marks the handle released without a call, for a handle that native took back. */
  fun handOff() {
    released.store(true)
  }

  /**
   * Disposes a handle that its owner never released, then prints the leak and any disposal failure.
   * Does nothing for a released handle.
   */
  fun disposeLeaked() {
    if (!released.compareAndSet(false, true)) return
    val failure =
      try {
        dispose(handle)
        null
      } catch (error: Throwable) {
        error
      }
    val leak = "Leaked $typeName native handle 0x${handle.toString(16)}; close it explicitly."
    println(if (failure == null) leak else "$leak Disposing it failed: ${failure.message}")
  }
}
