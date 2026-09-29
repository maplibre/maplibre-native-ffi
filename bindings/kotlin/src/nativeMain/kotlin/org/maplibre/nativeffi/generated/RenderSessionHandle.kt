// Generated from handle ownership plans. Do not edit.
package org.maplibre.nativeffi.generated

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.createCleaner

@OptIn(ExperimentalNativeApi::class)
public actual class RenderSessionHandle
internal constructor(
  private val handle: ULong,
  parent: MapHandle,
  dispose: (Long) -> Unit = GeneratedOwnerDisposal::renderSession,
) : GeneratedRenderSessionOperations(), AutoCloseable {
  private val core =
    org.maplibre.nativeffi.internal.lifecycle.HandleStateCore(
      "RenderSessionHandle",
      handle.toLong(),
      parent,
      dispose = dispose,
    )
  @Suppress("unused") private val cleaner = createCleaner(core.leakReport) { it.report() }

  internal override fun bindingRenderSessionHandle(): ULong {
    core.requireLive()
    return handle
  }

  internal override fun bindingCloseRenderSession(call: (ULong) -> Unit) {
    core.closeOnce({ call(handle) })
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual override fun close() {
    destroy()
  }
}
