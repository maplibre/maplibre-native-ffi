package org.maplibre.nativeffi.map

import java.lang.foreign.MemorySegment
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.loader.NativeAccess

/** Owned JVM FFM standalone projection snapshot. */
public actual class MapProjectionHandle internal constructor(private val handle: MemorySegment) :
  AutoCloseable {
  private val core = HandleStateCore("MapProjectionHandle", handle.address())

  public actual val camera: CameraOptions
    get() {
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

  public actual fun setVisibleGeometry(geometry: Geometry, padding: EdgeInsets) {
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

  public actual override fun close() {
    core.closeOnce(destroy = { NativeAccess.destroyMapProjection(handle) })
  }

  private fun requireLiveHandle(): MemorySegment {
    core.requireLive()
    return handle
  }
}
