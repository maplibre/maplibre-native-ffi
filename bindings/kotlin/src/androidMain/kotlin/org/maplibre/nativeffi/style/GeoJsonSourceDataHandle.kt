package org.maplibre.nativeffi.style

import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore

/** Owned Android JNI prepared GeoJSON source data. */
public actual class GeoJsonSourceDataHandle internal constructor(private val handleId: Long) :
  org.maplibre.nativeffi.generated.GeneratedGeojsonSourceDataOperations(), AutoCloseable {
  internal override fun bindingGeojsonSourceDataHandle(): Long = withNativeHandle { it }

  private val core =
    HandleStateCore(
      "GeoJsonSourceDataHandle",
      handleId,
      dispose = org.maplibre.nativeffi.generated.GeneratedOwnerDisposal::geojsonSourceData,
    )

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  internal override fun bindingCloseGeojsonSourceData(call: (Long) -> Int) {
    core.closeOnce(destroy = { call(handleId) })
  }

  public actual override fun close() {
    destroy()
  }

  /** Runs [block] after checking that this wrapper still owns the native handle. */
  internal fun <R> withNativeHandle(block: (Long) -> R): R = core.withLive { block(handleId) }
}
