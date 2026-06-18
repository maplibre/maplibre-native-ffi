package org.maplibre.nativeffi.internal.lifecycle

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.Cleaner
import kotlin.native.ref.createCleaner
import kotlinx.cinterop.CPointed
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.rawValue
import kotlinx.cinterop.toLong
import org.maplibre.nativeffi.internal.status.Status

/** Shared closed-state bookkeeping for native handles. */
@OptIn(ExperimentalForeignApi::class, ExperimentalAtomicApi::class, ExperimentalNativeApi::class)
internal class HandleState<T : CPointed>(
  private val typeName: String,
  handle: CPointer<T>?,
  vararg parents: Any,
) {
  @Suppress("unused") private val parents: Array<out Any> = parents
  private val address =
    requireNotNull(handle) { "$typeName native handle is null" }.rawValue.toLong()
  private val leakReport = LeakReport(typeName, address)
  @Suppress("unused") private val cleaner: Cleaner = createCleaner(leakReport) { it.report() }
  private val releaseState = AtomicInt(STATE_LIVE)
  private var handle: CPointer<T>? = handle

  fun requireLive(): CPointer<T> =
    when (releaseState.load()) {
      STATE_LIVE -> handle ?: throw Status.released(typeName)
      STATE_RELEASING -> throw Status.invalidState("$typeName is currently releasing")
      else -> throw Status.released(typeName)
    }

  fun isReleased(): Boolean = releaseState.load() == STATE_CLOSED

  fun address(): Long = address

  fun closeOnce(destroy: (CPointer<T>) -> Int) {
    closeOnce(destroy) {}
  }

  fun closeOnce(destroy: (CPointer<T>) -> Int, afterSuccess: () -> Unit) {
    if (!releaseState.compareAndSet(STATE_LIVE, STATE_RELEASING)) {
      when (releaseState.load()) {
        STATE_CLOSED -> return
        STATE_RELEASING -> throw Status.invalidState("$typeName is currently releasing")
        else -> throw Status.released(typeName)
      }
    }
    val live =
      handle
        ?: run {
          releaseState.store(STATE_CLOSED)
          return
        }
    try {
      Status.check(destroy(live))
    } catch (error: Throwable) {
      releaseState.store(STATE_LIVE)
      throw error
    }
    handle = null
    leakReport.markReleased()
    releaseState.store(STATE_CLOSED)
    afterSuccess()
  }

  internal class LeakReport(
    private val typeName: String,
    private val address: Long,
    private val writeLine: (String) -> Unit = { message -> println(message) },
  ) {
    private val released = AtomicInt(0)

    fun markReleased() {
      released.store(1)
    }

    fun report() {
      if (released.load() == 0) {
        writeLine(
          "Leaked $typeName native handle 0x${address.toString(16)}; " +
            "close handles explicitly on their owner thread."
        )
      }
    }
  }

  private companion object {
    private const val STATE_LIVE = 0
    private const val STATE_RELEASING = 1
    private const val STATE_CLOSED = 2
  }
}
