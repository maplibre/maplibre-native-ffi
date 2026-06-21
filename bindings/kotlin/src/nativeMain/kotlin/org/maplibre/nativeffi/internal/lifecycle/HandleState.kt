package org.maplibre.nativeffi.internal.lifecycle

import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.Cleaner
import kotlin.native.ref.createCleaner
import kotlinx.cinterop.CPointed
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.rawValue
import kotlinx.cinterop.toLong

/** Shared closed-state bookkeeping for native handles. */
@OptIn(ExperimentalForeignApi::class, ExperimentalAtomicApi::class, ExperimentalNativeApi::class)
internal class HandleState<T : CPointed>(
  private val typeName: String,
  handle: CPointer<T>?,
  vararg parents: Any,
) {
  private val liveHandle = requireNotNull(handle) { "$typeName native handle is null" }
  private val core = HandleStateCore(typeName, liveHandle.rawValue.toLong(), *parents)
  private val leakReport = core.leakReport
  @Suppress("unused") private val cleaner: Cleaner = createCleaner(leakReport) { it.report() }
  private var handle: CPointer<T>? = liveHandle

  fun requireLive(): CPointer<T> {
    core.requireLive()
    return handle ?: throw org.maplibre.nativeffi.internal.status.Status.released(typeName)
  }

  fun isReleased(): Boolean = core.isReleased()

  fun address(): Long = core.address()

  fun retainChild(): HandleStateCore.ChildRetention = core.retainChild()

  fun closeOnce(destroy: (CPointer<T>) -> Int) {
    closeOnce(destroy) {}
  }

  fun closeOnce(destroy: (CPointer<T>) -> Int, afterSuccess: () -> Unit) {
    val live =
      handle
        ?: run {
          core.closeOnce(
            { org.maplibre.nativeffi.error.MaplibreStatus.OK.nativeCode },
            afterSuccess,
          )
          return
        }
    core.closeOnce({ destroy(live) }) {
      handle = null
      afterSuccess()
    }
  }

  internal class LeakReport(
    typeName: String,
    address: Long,
    writeLine: (String) -> Unit = { message -> println(message) },
  ) {
    private val delegate = HandleStateCore.LeakReport(typeName, address, writeLine)

    fun markReleased() {
      delegate.markReleased()
    }

    fun report() {
      delegate.report()
    }
  }
}
