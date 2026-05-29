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
  private var handle: CPointer<T>? = handle

  fun requireLive(): CPointer<T> = handle ?: throw Status.released(typeName)

  fun isReleased(): Boolean = handle == null

  fun address(): Long = address

  fun closeOnce(destroy: (CPointer<T>) -> Int) {
    closeOnce(destroy) {}
  }

  fun closeOnce(destroy: (CPointer<T>) -> Int, afterSuccess: () -> Unit) {
    val live = handle ?: return
    Status.check(destroy(live))
    handle = null
    leakReport.markReleased()
    afterSuccess()
  }

  private class LeakReport(private val typeName: String, private val address: Long) {
    private val released = AtomicInt(0)

    fun markReleased() {
      released.store(1)
    }

    fun report() {
      if (released.load() == 0) {
        println(
          "Leaked $typeName native handle 0x${address.toString(16)}; " +
            "close handles explicitly on their owner thread."
        )
      }
    }
  }
}
