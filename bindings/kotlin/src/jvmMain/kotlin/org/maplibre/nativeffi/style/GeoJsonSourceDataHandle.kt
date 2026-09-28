package org.maplibre.nativeffi.style

import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeGeoJsonSourceData

/** Owned JVM FFM prepared GeoJSON source data. */
public actual class GeoJsonSourceDataHandle
internal constructor(private val handle: NativeGeoJsonSourceData) :
  org.maplibre.nativeffi.generated.GeneratedGeojsonSourceDataOperations(), AutoCloseable {
  internal override fun bindingGeojsonSourceDataHandle(): Long = withNativeHandle { it.raw }

  private val core =
    HandleStateCore(
      "GeoJsonSourceDataHandle",
      handle.raw,
      dispose = org.maplibre.nativeffi.generated.GeneratedOwnerDisposal::geojsonSourceData,
    )

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  internal override fun bindingCloseGeojsonSourceData(call: (Long) -> Int) {
    core.closeOnce(destroy = { call(handle.raw) })
  }

  public actual override fun close() {
    destroy()
  }

  /** Runs [block] after checking that this wrapper still owns the native handle. */
  internal fun <R> withNativeHandle(block: (NativeGeoJsonSourceData) -> R): R = core.withLive {
    block(handle)
  }
}
