package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint

/** Owned standalone projection snapshot created from a map. */
public expect class MapProjectionHandle {
  public suspend fun camera(): CameraOptions

  public fun setCamera(camera: CameraOptions): Long

  public fun setVisibleCoordinates(coordinates: List<LatLng>, padding: EdgeInsets): Long

  public fun setVisibleGeometry(geometry: ByteArray, padding: EdgeInsets): Long

  public suspend fun pixelForLatLng(coordinate: LatLng): ScreenPoint

  public suspend fun latLngForPixel(point: ScreenPoint): LatLng

  public val isClosed: Boolean

  public suspend fun close()
}
