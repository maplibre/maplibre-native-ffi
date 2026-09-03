package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint

/** Any-thread standalone projection snapshot created from a map. */
public expect class MapProjectionHandle : AutoCloseable {
  public val camera: CameraOptions

  public fun setCamera(camera: CameraOptions)

  public fun setVisibleCoordinates(coordinates: List<LatLng>, padding: EdgeInsets)

  public fun setVisibleGeometry(geometry: ByteArray, padding: EdgeInsets)

  public fun pixelForLatLng(coordinate: LatLng): ScreenPoint

  /** Converts a screen point to a coordinate with longitude wrapped to -180 through 180. */
  public fun latLngForPixel(point: ScreenPoint): LatLng

  /** Converts a screen point to an unwrapped coordinate that preserves its visible world copy. */
  public fun latLngForPixelUnwrapped(point: ScreenPoint): LatLng

  public val isClosed: Boolean

  override fun close()
}
