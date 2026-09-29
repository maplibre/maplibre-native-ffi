// Generated from handle ownership plans. Do not edit.
package org.maplibre.nativeffi.generated

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.createCleaner

@OptIn(ExperimentalNativeApi::class)
public actual class AcquiredFrameHandle
internal constructor(
  private val handle: ULong,
  parent: RenderSessionHandle,
  dispose: (Long) -> Unit = GeneratedOwnerDisposal::acquiredFrame,
) : GeneratedAcquiredFrameOperations(), AutoCloseable {
  private val core =
    org.maplibre.nativeffi.internal.lifecycle.HandleStateCore(
      "AcquiredFrameHandle",
      handle.toLong(),
      parent,
      dispose = dispose,
    )
  @Suppress("unused") private val cleaner = createCleaner(core.leakReport) { it.report() }

  internal override fun bindingAcquiredFrameHandle(): ULong {
    core.requireLive()
    return handle
  }

  internal override fun bindingCloseAcquiredFrame(call: (ULong) -> Unit) {
    core.closeOnce({ call(handle) })
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual override fun close() {
    release()
  }
}
