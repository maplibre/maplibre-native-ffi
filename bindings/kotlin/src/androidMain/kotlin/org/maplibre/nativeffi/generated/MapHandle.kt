// Generated from handle ownership plans. Do not edit.
package org.maplibre.nativeffi.generated

public actual class MapHandle
internal constructor(
  private val handle: Long,
  parent: RuntimeHandle,
  dispose: (Long) -> Unit = GeneratedOwnerDisposal::map,
) : GeneratedMapOperations(), org.maplibre.nativeffi.runtime.AsyncReleasable {
  private val core =
    org.maplibre.nativeffi.internal.lifecycle.HandleStateCore(
      "MapHandle",
      handle,
      parent,
      dispose = dispose,
    )

  init {
    org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner.register(this, core.leakReport)
  }

  internal override fun bindingMapHandle(): Long {
    core.requireLive()
    return handle
  }

  internal override fun bindingCloseMap(call: (Long) -> Int) {
    core.closeOnce({ call(handle) })
  }

  internal override fun bindingRetireMap(
    call: (Long) -> kotlinx.coroutines.Deferred<Unit>
  ): kotlinx.coroutines.Deferred<Unit> = core.retire({ call(handle) })

  public actual val isClosed: Boolean
    get() = core.isReleased()
}
