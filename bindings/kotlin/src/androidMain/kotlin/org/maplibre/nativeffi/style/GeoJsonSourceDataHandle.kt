package org.maplibre.nativeffi.style

import org.bytedeco.javacpp.LongPointer
import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.javacpp.ByteArrayViewScope
import org.maplibre.nativeffi.internal.javacpp.GeoJsonSourceOptionsScope
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.status.Status

/** Owned Android JNI prepared GeoJSON source data. */
public actual class GeoJsonSourceDataHandle internal constructor(private val handleId: Long) :
  AutoCloseable {
  private val core = HandleStateCore("GeoJsonSourceDataHandle", handleId)

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual override fun close() {
    core.closeOnce(
      destroy = {
        MaplibreNativeC.mln_geojson_source_data_destroy(handleId)
        MaplibreStatus.OK.nativeCode
      }
    )
  }

  /** Runs [block] with the live native handle and release held off for the borrow window. */
  internal fun <R> withNativeHandle(block: (Long) -> R): R = core.withLive { block(handleId) }

  public actual companion object {
    public actual fun create(
      data: ByteArray,
      options: GeoJsonSourceOptions?,
    ): GeoJsonSourceDataHandle {
      NativeAccess.ensureLoaded()
      ByteArrayViewScope(data).use { nativeData ->
        GeoJsonSourceOptionsScope(options).use { nativeOptions ->
          LongPointer(1).use { outData ->
            outData.put(0, 0L)
            Status.check(
              MaplibreNativeC.mln_geojson_source_data_create(
                nativeData.view,
                nativeOptions.options,
                outData,
              )
            )
            val handleId = outData.get()
            require(handleId != 0L) { "mln_geojson_source_data_create returned the null handle" }
            return GeoJsonSourceDataHandle(handleId)
          }
        }
      }
    }
  }
}
