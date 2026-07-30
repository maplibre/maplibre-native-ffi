package org.maplibre.nativeffi.internal.lifecycle

import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.Cleaner
import kotlin.native.ref.createCleaner

/** Shared closed-state bookkeeping for native handles. */
@OptIn(ExperimentalAtomicApi::class, ExperimentalNativeApi::class)
internal class HandleState<T : NativeHandle>(
  private val typeName: String,
  handle: T,
  vararg parents: Any,
) {
  private val liveHandle = handle.also { require(!it.isNull) { "$typeName native handle is null" } }
  private val core = HandleStateCore(typeName, liveHandle.raw, *parents)
  private val leakReport = core.leakReport
  @Suppress("unused") private val cleaner: Cleaner = createCleaner(leakReport) { it.report() }
  private var handle: T? = liveHandle

  fun requireLive(): T {
    core.requireLive()
    return handle ?: throw org.maplibre.nativeffi.internal.status.Status.released(typeName)
  }

  /** Runs [block] with the live handle and release held off. See [HandleStateCore.withLive]. */
  fun <R> withLive(block: (T) -> R): R = core.withLive {
    val live = handle ?: throw org.maplibre.nativeffi.internal.status.Status.released(typeName)
    block(live)
  }

  fun isReleased(): Boolean = core.isReleased()

  fun handleId(): Long = core.handleId()

  fun retainChild(childTypeName: String): HandleStateCore.ChildRetention =
    core.retainChild(childTypeName)

  fun closeOnce(destroy: (T) -> Int) {
    closeOnce(destroy) {}
  }

  fun closeOnce(destroy: (T) -> Int, afterSuccess: () -> Unit) {
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
    handleId: Long,
    writeLine: (String) -> Unit = { message -> println(message) },
  ) {
    private val delegate = HandleStateCore.LeakReport(typeName, handleId, writeLine)

    fun markReleased() {
      delegate.markReleased()
    }

    fun report() {
      delegate.report()
    }
  }
}
