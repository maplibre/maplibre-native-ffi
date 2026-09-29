// Generated from handle ownership plans. Do not edit.
package org.maplibre.nativeffi.generated

public actual class GeojsonSourceDataHandle
internal constructor(
  private val handle: Long,
  dispose: (Long) -> Unit = GeneratedOwnerDisposal::geojsonSourceData,
) : GeneratedGeojsonSourceDataOperations(), AutoCloseable {
  private val core =
    org.maplibre.nativeffi.internal.lifecycle.HandleStateCore(
      "GeojsonSourceDataHandle",
      handle,
      dispose = dispose,
    )

  init {
    org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner.register(this, core.leakReport)
  }

  internal override fun bindingGeojsonSourceDataHandle(): Long {
    core.requireLive()
    return handle
  }

  internal override fun bindingCloseGeojsonSourceData(call: (Long) -> Unit) {
    core.closeOnce({ call(handle) })
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual override fun close() {
    destroy()
  }
}
