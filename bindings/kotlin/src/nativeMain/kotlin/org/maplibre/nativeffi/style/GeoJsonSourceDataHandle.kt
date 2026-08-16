package org.maplibre.nativeffi.style

import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.ULongVar
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.value
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.c.mln_geojson_source_data_create
import org.maplibre.nativeffi.internal.c.mln_geojson_source_data_destroy
import org.maplibre.nativeffi.internal.lifecycle.HandleState
import org.maplibre.nativeffi.internal.lifecycle.NativeGeoJsonSourceData
import org.maplibre.nativeffi.internal.lifecycle.asHandle
import org.maplibre.nativeffi.internal.lifecycle.geoJsonSourceDataHandle
import org.maplibre.nativeffi.internal.lifecycle.rawHandleValue
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.struct.ByteStructs
import org.maplibre.nativeffi.internal.struct.StyleStructs

/** Owned prepared GeoJSON source data. */
@OptIn(ExperimentalForeignApi::class)
public actual class GeoJsonSourceDataHandle internal constructor(handle: NativeGeoJsonSourceData) :
  AutoCloseable {
  private val state = HandleState("GeoJsonSourceDataHandle", handle)

  public actual val isClosed: Boolean
    get() = state.isReleased()

  public actual override fun close() {
    state.closeOnce { handle ->
      mln_geojson_source_data_destroy(handle.rawHandleValue)
      MaplibreStatus.OK.nativeCode
    }
  }

  /** Runs [block] with the live native handle and release held off for the borrow window. */
  internal fun <R> withNativeHandle(block: (NativeGeoJsonSourceData) -> R): R =
    state.withLive(block)

  public actual companion object {
    public actual fun create(
      data: ByteArray,
      options: GeoJsonSourceOptions?,
    ): GeoJsonSourceDataHandle = memScoped {
      val outData = alloc<ULongVar>()
      outData.value = 0uL
      Status.check(
        mln_geojson_source_data_create(
          ByteStructs.bufferView(data, this),
          StyleStructs.geoJsonSourceOptions(options, this),
          outData.ptr,
        )
      )
      GeoJsonSourceDataHandle(
        outData.value.asHandle("mln_geojson_source_data_create", ::geoJsonSourceDataHandle)
      )
    }
  }
}
