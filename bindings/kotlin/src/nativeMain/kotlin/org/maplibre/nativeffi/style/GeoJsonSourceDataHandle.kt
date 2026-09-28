package org.maplibre.nativeffi.style

import kotlinx.cinterop.ExperimentalForeignApi
import org.maplibre.nativeffi.internal.lifecycle.HandleState
import org.maplibre.nativeffi.internal.lifecycle.NativeGeoJsonSourceData
import org.maplibre.nativeffi.internal.lifecycle.rawHandleValue

/** Owned prepared GeoJSON source data. */
@OptIn(ExperimentalForeignApi::class)
public actual class GeoJsonSourceDataHandle internal constructor(handle: NativeGeoJsonSourceData) :
  org.maplibre.nativeffi.generated.GeneratedGeojsonSourceDataOperations(), AutoCloseable {
  internal override fun bindingGeojsonSourceDataHandle(): ULong = state.requireLive().rawHandleValue

  private val state =
    HandleState(
      "GeoJsonSourceDataHandle",
      handle,
      dispose = org.maplibre.nativeffi.generated.GeneratedOwnerDisposal::geojsonSourceData,
    )

  public actual val isClosed: Boolean
    get() = state.isReleased()

  internal override fun bindingCloseGeojsonSourceData(call: (ULong) -> Int) {
    state.closeOnce { call(it.rawHandleValue) }
  }

  public actual override fun close() {
    destroy()
  }

  /** Runs [block] after checking that this wrapper still owns the native handle. */
  internal fun <R> withNativeHandle(block: (NativeGeoJsonSourceData) -> R): R =
    state.withLive(block)
}
