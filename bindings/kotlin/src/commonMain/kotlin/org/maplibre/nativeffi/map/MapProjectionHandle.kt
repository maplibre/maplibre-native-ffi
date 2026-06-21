package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint

/** Owned standalone projection snapshot created from a map. */
public expect class MapProjectionHandle : AutoCloseable {
  public val camera: CameraOptions

  public fun setCamera(camera: CameraOptions)

  public fun setVisibleCoordinates(coordinates: List<LatLng>, padding: EdgeInsets)

  public fun setVisibleGeometry(geometry: Geometry, padding: EdgeInsets)

  public fun pixelForLatLng(coordinate: LatLng): ScreenPoint

  public fun latLngForPixel(point: ScreenPoint): LatLng

  public val isClosed: Boolean

  override fun close()
}
