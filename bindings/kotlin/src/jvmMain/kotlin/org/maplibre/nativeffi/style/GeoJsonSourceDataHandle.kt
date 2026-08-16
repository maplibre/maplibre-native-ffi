package org.maplibre.nativeffi.style

import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeGeoJsonSourceData
import org.maplibre.nativeffi.internal.loader.NativeAccess

/** Owned JVM FFM prepared GeoJSON source data. */
public actual class GeoJsonSourceDataHandle
internal constructor(private val handle: NativeGeoJsonSourceData) : AutoCloseable {
  private val core = HandleStateCore("GeoJsonSourceDataHandle", handle.raw)

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual override fun close() {
    core.closeOnce(
      destroy = {
        NativeAccess.destroyGeoJsonSourceData(handle)
        MaplibreStatus.OK.nativeCode
      }
    )
  }

  /** Runs [block] with the live native handle and release held off for the borrow window. */
  internal fun <R> withNativeHandle(block: (NativeGeoJsonSourceData) -> R): R = core.withLive {
    block(handle)
  }

  public actual companion object {
    public actual fun create(
      data: ByteArray,
      options: GeoJsonSourceOptions?,
    ): GeoJsonSourceDataHandle {
      NativeAccess.ensureLoaded()
      return GeoJsonSourceDataHandle(NativeAccess.createGeoJsonSourceData(data, options))
    }
  }
}
