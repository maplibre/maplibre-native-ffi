// Generated from handle ownership plans. Do not edit.
package org.maplibre.nativeffi.generated

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.createCleaner

@OptIn(ExperimentalNativeApi::class)
public actual class MapHandle
internal constructor(
  private val handle: ULong,
  parent: RuntimeHandle,
  dispose: (Long) -> Unit = GeneratedOwnerDisposal::map,
) : GeneratedMapOperations(), org.maplibre.nativeffi.runtime.AsyncReleasable {
  private val core =
    org.maplibre.nativeffi.internal.lifecycle.HandleStateCore(
      "MapHandle",
      handle.toLong(),
      parent,
      dispose = dispose,
    )
  @Suppress("unused") private val cleaner = createCleaner(core.leakReport) { it.report() }

  internal override fun bindingMapHandle(): ULong {
    core.requireLive()
    return handle
  }

  internal override fun bindingCloseMap(call: (ULong) -> Unit) {
    core.closeOnce({ call(handle) })
  }

  internal override fun bindingRetireMap(
    call: (ULong) -> kotlinx.coroutines.Deferred<Unit>
  ): kotlinx.coroutines.Deferred<Unit> = core.retire({ call(handle) })

  public actual val isClosed: Boolean
    get() = core.isReleased()
}
