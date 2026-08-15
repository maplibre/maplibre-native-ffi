package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeMapProjection
import org.maplibre.nativeffi.internal.loader.NativeAccess

/**
 * Owned JVM FFM standalone projection snapshot.
 *
 * Every call is synchronous, runs on the calling thread, is internally serialized, and may be made
 * from any thread. A projection copies the map's transform state at creation and never observes map
 * changes made after that; a live projection prevents its map from closing.
 */
public actual class MapProjectionHandle
internal constructor(private val handle: NativeMapProjection) {
  private val core = HandleStateCore("MapProjectionHandle", handle.raw)

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  public actual fun camera(): CameraOptions {
    NativeAccess.ensureLoaded()
    return NativeAccess.projectionCamera(requireLiveHandle())
  }

  public actual fun setCamera(camera: CameraOptions) {
    NativeAccess.ensureLoaded()
    NativeAccess.setProjectionCamera(requireLiveHandle(), camera)
  }

  public actual fun setVisibleCoordinates(coordinates: List<LatLng>, padding: EdgeInsets) {
    NativeAccess.ensureLoaded()
    NativeAccess.setProjectionVisibleCoordinates(requireLiveHandle(), coordinates, padding)
  }

  public actual fun setVisibleGeometry(geometry: ByteArray, padding: EdgeInsets) {
    NativeAccess.ensureLoaded()
    NativeAccess.setProjectionVisibleGeometry(requireLiveHandle(), geometry, padding)
  }

  public actual fun pixelForLatLng(coordinate: LatLng): ScreenPoint {
    NativeAccess.ensureLoaded()
    return NativeAccess.projectionPixelForLatLng(requireLiveHandle(), coordinate)
  }

  public actual fun latLngForPixel(point: ScreenPoint): LatLng {
    NativeAccess.ensureLoaded()
    return NativeAccess.projectionLatLngForPixel(requireLiveHandle(), point)
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun close() {
    if (!core.beginClose()) return
    try {
      NativeAccess.closeProjection(handle)
    } catch (error: Throwable) {
      core.abortClose()
      throw error
    }
    core.completeClose()
  }

  private fun requireLiveHandle(): NativeMapProjection {
    core.requireLive()
    return handle
  }
}
