package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeMapProjection
import org.maplibre.nativeffi.internal.loader.NativeAccess

/** Any-thread JVM FFM standalone projection snapshot. */
public actual class MapProjectionHandle
internal constructor(private val handle: NativeMapProjection) : AutoCloseable {
  private val core = HandleStateCore("MapProjectionHandle", handle.raw)

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  public actual val camera: CameraOptions
    get() {
      NativeAccess.ensureLoaded()
      return withLiveHandle(NativeAccess::projectionCamera)
    }

  public actual fun setCamera(camera: CameraOptions) {
    NativeAccess.ensureLoaded()
    withLiveHandle { handle -> NativeAccess.setProjectionCamera(handle, camera) }
  }

  public actual fun setVisibleCoordinates(coordinates: List<LatLng>, padding: EdgeInsets) {
    NativeAccess.ensureLoaded()
    withLiveHandle { handle ->
      NativeAccess.setProjectionVisibleCoordinates(handle, coordinates, padding)
    }
  }

  public actual fun setVisibleGeometry(geometry: ByteArray, padding: EdgeInsets) {
    NativeAccess.ensureLoaded()
    withLiveHandle { handle ->
      NativeAccess.setProjectionVisibleGeometry(handle, geometry, padding)
    }
  }

  public actual fun pixelForLatLng(coordinate: LatLng): ScreenPoint {
    NativeAccess.ensureLoaded()
    return withLiveHandle { handle -> NativeAccess.projectionPixelForLatLng(handle, coordinate) }
  }

  public actual fun latLngForPixel(point: ScreenPoint): LatLng {
    NativeAccess.ensureLoaded()
    return withLiveHandle { handle -> NativeAccess.projectionLatLngForPixel(handle, point) }
  }

  public actual fun latLngForPixelUnwrapped(point: ScreenPoint): LatLng {
    NativeAccess.ensureLoaded()
    return withLiveHandle { handle ->
      NativeAccess.projectionLatLngForPixelUnwrapped(handle, point)
    }
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual override fun close() {
    core.closeOnce(destroy = { NativeAccess.destroyMapProjection(handle) })
  }

  private fun <T> withLiveHandle(block: (NativeMapProjection) -> T): T = core.withLive {
    block(handle)
  }
}
