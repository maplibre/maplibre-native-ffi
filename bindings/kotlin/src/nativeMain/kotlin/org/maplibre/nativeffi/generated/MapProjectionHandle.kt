// Generated from handle ownership plans. Do not edit.
package org.maplibre.nativeffi.generated

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.createCleaner

@OptIn(ExperimentalNativeApi::class)
public actual class MapProjectionHandle
internal constructor(
  private val handle: ULong,
  dispose: (Long) -> Unit = GeneratedOwnerDisposal::mapProjection,
) : GeneratedMapProjectionOperations(), AutoCloseable {
  private val core =
    org.maplibre.nativeffi.internal.lifecycle.HandleStateCore(
      "MapProjectionHandle",
      handle.toLong(),
      dispose = dispose,
    )
  @Suppress("unused") private val cleaner = createCleaner(core.leakReport) { it.report() }

  internal override fun bindingMapProjectionHandle(): ULong {
    core.requireLive()
    return handle
  }

  internal override fun bindingCloseMapProjection(call: (ULong) -> Int) {
    core.closeOnce({ call(handle) })
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()
}
