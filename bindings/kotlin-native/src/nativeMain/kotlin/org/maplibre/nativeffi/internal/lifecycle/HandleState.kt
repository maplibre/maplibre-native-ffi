package org.maplibre.nativeffi.internal.lifecycle

import kotlinx.cinterop.CPointed
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.rawValue
import kotlinx.cinterop.toLong
import org.maplibre.nativeffi.internal.status.Status

/** Shared closed-state bookkeeping for native handles. */
@OptIn(ExperimentalForeignApi::class)
internal class HandleState<T : CPointed>(
  private val typeName: String,
  handle: CPointer<T>?,
  vararg parents: Any,
) {
  @Suppress("unused") private val parents: Array<out Any> = parents
  private val address =
    requireNotNull(handle) { "$typeName native handle is null" }.rawValue.toLong()
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
    afterSuccess()
  }
}
