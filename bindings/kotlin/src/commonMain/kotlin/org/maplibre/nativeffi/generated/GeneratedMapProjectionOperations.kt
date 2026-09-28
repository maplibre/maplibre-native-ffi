// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.callback.*

public expect abstract class GeneratedMapProjectionOperations internal constructor() {
  public fun close(): Unit

  public fun getCamera(): CameraOptions

  public fun latLngForPixel(point: ScreenPoint): LatLng

  public fun latLngForPixelUnwrapped(point: ScreenPoint): LatLng

  public fun metersPerPixelAtLatitude(latitude: Double): Double

  public fun pixelForLatLng(coordinate: LatLng): ScreenPoint

  public fun setCamera(camera: CameraOptions): Unit

  public fun setVisibleCoordinates(coordinates: List<LatLng>, padding: EdgeInsets): Unit

  public fun setVisibleGeometry(geometry: ByteArray, padding: EdgeInsets): Unit
}
