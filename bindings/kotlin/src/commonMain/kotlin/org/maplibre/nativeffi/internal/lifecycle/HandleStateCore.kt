package org.maplibre.nativeffi.internal.lifecycle

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.maplibre.nativeffi.internal.status.Status

/** Platform-neutral release-state bookkeeping for native handles. */
@OptIn(ExperimentalAtomicApi::class)
internal class HandleStateCore(
  private val typeName: String,
  private val handleId: Long,
  vararg parents: Any,
) {
  @Suppress("unused") private val parents: Array<out Any> = parents
  val leakReport: LeakReport = LeakReport(typeName, handleId)
  private val releaseState = AtomicInt(STATE_LIVE)

  fun requireLive() {
    when (releaseState.load()) {
      STATE_LIVE -> return
      STATE_RELEASING -> throw Status.invalidState("$typeName is currently releasing")
      else -> throw Status.released(typeName)
    }
  }

  /** Runs [block] after checking that this wrapper still owns its native handle. */
  fun <T> withLive(block: () -> T): T {
    requireLive()
    return block()
  }

  fun isReleased(): Boolean = releaseState.load() == STATE_CLOSED

  /** The C API handle id this wrapper owns. */
  fun handleId(): Long = handleId

  /**
   * Acquires the exclusive close lease before an asynchronous native close starts.
   *
   * Returns false when the handle is already closed. The caller must pair a true result with
   * [completeClose] or [abortClose].
   */
  fun beginClose(): Boolean {
    if (!releaseState.compareAndSet(STATE_LIVE, STATE_RELEASING)) {
      when (releaseState.load()) {
        STATE_CLOSED -> return false
        STATE_RELEASING -> throw Status.invalidState("$typeName is currently releasing")
        else -> throw Status.released(typeName)
      }
    }
    return true
  }

  fun completeClose(afterSuccess: () -> Unit = {}) {
    check(releaseState.load() == STATE_RELEASING)
    leakReport.markReleased()
    releaseState.store(STATE_CLOSED)
    afterSuccess()
  }

  fun abortClose() {
    check(releaseState.compareAndSet(STATE_RELEASING, STATE_LIVE))
  }

  fun closeOnce(destroy: () -> Int, afterSuccess: () -> Unit = {}) {
    if (!beginClose()) return
    try {
      Status.check(destroy())
    } catch (error: Throwable) {
      abortClose()
      throw error
    }
    completeClose(afterSuccess)
  }

  @OptIn(ExperimentalAtomicApi::class)
  internal class LeakReport(
    private val typeName: String,
    private val handleId: Long,
    private val writeLine: (String) -> Unit = { message -> println(message) },
  ) {
    private val released = AtomicInt(0)

    fun markReleased() {
      released.store(1)
    }

    fun report() {
      if (released.load() == 0) {
        writeLine("Leaked $typeName native handle 0x${handleId.toString(16)}; close it explicitly.")
      }
    }
  }

  private companion object {
    private const val STATE_LIVE = 0
    private const val STATE_RELEASING = 1
    private const val STATE_CLOSED = 2
  }
}
