// Generated from handle ownership plans. Do not edit.
package org.maplibre.nativeffi.generated

public actual class MapProjectionHandle
internal constructor(
  private val handle: Long,
  dispose: (Long) -> Unit = GeneratedOwnerDisposal::mapProjection,
) : GeneratedMapProjectionOperations(), AutoCloseable {
  private val core =
    org.maplibre.nativeffi.internal.lifecycle.HandleStateCore(
      "MapProjectionHandle",
      handle,
      dispose = dispose,
    )

  init {
    org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner.register(this, core.leakReport)
  }

  internal override fun bindingMapProjectionHandle(): Long {
    core.requireLive()
    return handle
  }

  internal override fun bindingCloseMapProjection(call: (Long) -> Unit) {
    core.closeOnce({ call(handle) })
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()
}
