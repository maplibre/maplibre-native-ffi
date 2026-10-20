package org.maplibre.nativeffi.internal.lifecycle

import kotlin.concurrent.atomics.AtomicBoolean
import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.maplibre.nativeffi.internal.status.invalidState

/**
 * What a generated owner tracks about its native [handle].
 *
 * Native validates every handle and leases it for each call, so a call on a released handle reaches
 * native, which reports it. The binding keeps only what native cannot: whether a release ended the
 * handle, which tells unreachable-owner cleanup whether to dispose it, and the [read]s in progress,
 * which copy outputs that native lends until the release. A state holds neither its owner nor the
 * owner's parent, so cleanup can hold the state without keeping either reachable.
 */
@OptIn(ExperimentalAtomicApi::class)
internal class HandleState(
  private val typeName: String,
  val handle: Long,
  private val dispose: (Long) -> Unit,
) {
  private val released = AtomicBoolean(false)

  /** The number of reads in progress, or minus the number of releases in progress. */
  private val gate = AtomicInt(0)

  val isReleased: Boolean
    get() = released.load()

  /**
   * Runs [block] while a release of this handle fails, so outputs that native lends until the
   * release stay valid while the binding copies them.
   */
  fun <T> read(block: () -> T): T {
    while (true) {
      val current = gate.load()
      if (current < 0) throw invalidState("$typeName is releasing")
      if (gate.compareAndSet(current, current + 1)) break
    }
    try {
      return block()
    } finally {
      gate.addAndFetch(-1)
    }
  }

  /**
   * Runs the release [call] and marks the handle released once native accepts it. A release that
   * native refuses leaves the handle as it was, so a later one can retry. Releases never wait for
   * each other: native decides which one ends the handle. A release during a [read] fails.
   */
  fun <T> release(call: () -> T): T {
    while (true) {
      val current = gate.load()
      if (current > 0) throw invalidState("$typeName is in use")
      if (gate.compareAndSet(current, current - 1)) break
    }
    try {
      return call().also { released.store(true) }
    } finally {
      gate.addAndFetch(1)
    }
  }

  /** Marks the handle released without a call, for a handle that native took back. */
  fun handOff() {
    released.store(true)
  }

  /**
   * Disposes a handle that its owner never released, then reports the leak through [writeLine],
   * with the disposal's failure when it had one. Does nothing for a released handle.
   */
  fun disposeLeaked(writeLine: (String) -> Unit = { message -> println(message) }) {
    if (!released.compareAndSet(false, true)) return
    val failure =
      try {
        dispose(handle)
        null
      } catch (error: Throwable) {
        error
      }
    val leak = "Leaked $typeName native handle 0x${handle.toString(16)}; close it explicitly."
    writeLine(if (failure == null) leak else "$leak Disposing it failed: ${failure.message}")
  }
}
