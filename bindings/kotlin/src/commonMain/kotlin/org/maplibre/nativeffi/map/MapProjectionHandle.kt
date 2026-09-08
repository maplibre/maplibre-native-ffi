package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint

/**
 * Owned standalone projection snapshot created from a map.
 *
 * Every call is synchronous, runs on the calling thread, is internally serialized, and may be made
 * from any thread. A projection copies the map's transform state at creation, never observes later
 * map changes, and stays usable after its source map and runtime close.
 */
public expect class MapProjectionHandle : AutoCloseable {
  /** Copies the projection camera, observing every earlier projection setter. */
  public fun camera(): CameraOptions

  /** Applies a camera update; only fields present on [camera] affect the projection. */
  public fun setCamera(camera: CameraOptions)

  /** Applies a camera fit for [coordinates]. */
  public fun setVisibleCoordinates(coordinates: List<LatLng>, padding: EdgeInsets)

  /** Applies a camera fit for GeoJSON Geometry bytes. */
  public fun setVisibleGeometry(geometry: ByteArray, padding: EdgeInsets)

  /** Converts a geographic coordinate to a logical-pixel screen point. */
  public fun pixelForLatLng(coordinate: LatLng): ScreenPoint

  /** Converts a logical-pixel screen point to a geographic coordinate. */
  public fun latLngForPixel(point: ScreenPoint): LatLng

  /** Converts a screen point to an unwrapped coordinate that preserves its visible world copy. */
  public fun latLngForPixelUnwrapped(point: ScreenPoint): LatLng

  public val isClosed: Boolean

  /** Closes the projection after waiting for projection calls already running on other threads. */
  override fun close()
}
