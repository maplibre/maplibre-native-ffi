// Generated from handle ownership plans. Do not edit.
package org.maplibre.nativeffi.generated

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.createCleaner

@OptIn(ExperimentalNativeApi::class)
public actual class GeojsonSourceDataHandle
internal constructor(
  private val handle: ULong,
  dispose: (Long) -> Unit = GeneratedOwnerDisposal::geojsonSourceData,
) : GeneratedGeojsonSourceDataOperations(), AutoCloseable {
  private val core =
    org.maplibre.nativeffi.internal.lifecycle.HandleStateCore(
      "GeojsonSourceDataHandle",
      handle.toLong(),
      dispose = dispose,
    )
  @Suppress("unused") private val cleaner = createCleaner(core.leakReport) { it.report() }

  internal override fun bindingGeojsonSourceDataHandle(): ULong {
    core.requireLive()
    return handle
  }

  internal override fun bindingCloseGeojsonSourceData(call: (ULong) -> Int) {
    core.closeOnce({ call(handle) })
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual override fun close() {
    destroy()
  }
}
