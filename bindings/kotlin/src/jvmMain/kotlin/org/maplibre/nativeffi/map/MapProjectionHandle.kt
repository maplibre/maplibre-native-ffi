package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeMapProjection
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.runtime.RuntimeHandle

/** Owned JVM FFM standalone projection snapshot. */
public actual class MapProjectionHandle
internal constructor(private val runtime: RuntimeHandle, private val handle: NativeMapProjection) {
  private val core = HandleStateCore("MapProjectionHandle", handle.raw)

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  public actual suspend fun camera(): CameraOptions {
    NativeAccess.ensureLoaded()
    val operation = NativeAccess.startProjectionCamera(requireLiveHandle())
    try {
      runtime.awaitOperation(operation)
      return NativeAccess.takeProjectionCamera(operation)
    } finally {
      NativeAccess.releaseOperation(operation)
    }
  }

  public actual fun setCamera(camera: CameraOptions): Long {
    NativeAccess.ensureLoaded()
    return NativeAccess.setProjectionCamera(requireLiveHandle(), camera)
  }

  public actual fun setVisibleCoordinates(coordinates: List<LatLng>, padding: EdgeInsets): Long {
    NativeAccess.ensureLoaded()
    return NativeAccess.setProjectionVisibleCoordinates(requireLiveHandle(), coordinates, padding)
  }

  public actual fun setVisibleGeometry(geometry: ByteArray, padding: EdgeInsets): Long {
    NativeAccess.ensureLoaded()
    return NativeAccess.setProjectionVisibleGeometry(requireLiveHandle(), geometry, padding)
  }

  public actual suspend fun pixelForLatLng(coordinate: LatLng): ScreenPoint {
    NativeAccess.ensureLoaded()
    val operation = NativeAccess.startProjectionPixelForLatLng(requireLiveHandle(), coordinate)
    try {
      runtime.awaitOperation(operation)
      return NativeAccess.takeProjectionPixelForLatLng(operation)
    } finally {
      NativeAccess.releaseOperation(operation)
    }
  }

  public actual suspend fun latLngForPixel(point: ScreenPoint): LatLng {
    NativeAccess.ensureLoaded()
    val operation = NativeAccess.startProjectionLatLngForPixel(requireLiveHandle(), point)
    try {
      runtime.awaitOperation(operation)
      return NativeAccess.takeProjectionLatLngForPixel(operation)
    } finally {
      NativeAccess.releaseOperation(operation)
    }
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual suspend fun close() {
    if (!core.beginClose()) return
    val operation =
      try {
        NativeAccess.startProjectionClose(handle)
      } catch (error: Throwable) {
        core.abortClose()
        throw error
      }
    try {
      runtime.awaitOperation(operation)
    } catch (error: Throwable) {
      core.abortClose()
      throw error
    } finally {
      NativeAccess.releaseOperation(operation)
    }
    core.completeClose()
  }

  private fun requireLiveHandle(): NativeMapProjection {
    core.requireLive()
    return handle
  }
}
