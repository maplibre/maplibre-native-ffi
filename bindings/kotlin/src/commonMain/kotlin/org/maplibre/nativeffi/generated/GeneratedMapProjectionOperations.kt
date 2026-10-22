// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

public abstract class GeneratedMapProjectionOperations internal constructor() {
  internal abstract val binding: HandleState

  /**
   * Copies the projection camera into out_camera.
   *
   * See `mln_map_projection_get_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun getCamera(): CameraOptions =
    nativeCall(this, binding) {
      val out = sized(120, 8)
      check(C.mln_map_projection_get_camera(handle, out, diagnostic))
      readCameraOptions(out)
    }

  /**
   * Converts a screen point to a geographic coordinate.
   *
   * See `mln_map_projection_lat_lng_for_pixel` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun latLngForPixel(point: ScreenPoint): LatLng =
    nativeCall(this, binding) {
      val out = allocate(16, 8)
      check(
        C.mln_map_projection_lat_lng_for_pixel(handle, writeScreenPoint(point), out, diagnostic),
      )
      readLatLng(out)
    }

  /**
   * Converts a screen point to an unwrapped geographic coordinate.
   *
   * See `mln_map_projection_lat_lng_for_pixel_unwrapped` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun latLngForPixelUnwrapped(point: ScreenPoint): LatLng =
    nativeCall(this, binding) {
      val out = allocate(16, 8)
      check(
        C.mln_map_projection_lat_lng_for_pixel_unwrapped(
          handle,
          writeScreenPoint(point),
          out,
          diagnostic,
        ),
      )
      readLatLng(out)
    }

  /**
   * Converts screen points to geographic coordinates.
   *
   * See `mln_map_projection_lat_lngs_for_pixels` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun latLngsForPixels(points: List<ScreenPoint>): List<LatLng> =
    nativeCall(this, binding) {
      val out = allocate(points.size.toLong() * 16, 8)
      check(
        C.mln_map_projection_lat_lngs_for_pixels(
          handle,
          array(points, 16, 8) { at, item -> putScreenPoint(at, item) },
          points.size.toLong(),
          out,
          diagnostic,
        ),
      )
      readArray(out, points.size.toULong(), 16.toLong()) { item -> readLatLng(item) }
    }

  /**
   * Converts screen points to unwrapped geographic coordinates.
   *
   * See `mln_map_projection_lat_lngs_for_pixels_unwrapped` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun latLngsForPixelsUnwrapped(points: List<ScreenPoint>): List<LatLng> =
    nativeCall(this, binding) {
      val out = allocate(points.size.toLong() * 16, 8)
      check(
        C.mln_map_projection_lat_lngs_for_pixels_unwrapped(
          handle,
          array(points, 16, 8) { at, item -> putScreenPoint(at, item) },
          points.size.toLong(),
          out,
          diagnostic,
        ),
      )
      readArray(out, points.size.toULong(), 16.toLong()) { item -> readLatLng(item) }
    }

  /**
   * Reads the ground distance covered by one logical map pixel at a latitude for the helper camera
   * zoom.
   *
   * See `mln_map_projection_meters_per_pixel_at_latitude` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun metersPerPixelAtLatitude(latitude: Double): Double =
    nativeCall(this, binding) {
      val out = allocate(8, 8)
      check(C.mln_map_projection_meters_per_pixel_at_latitude(handle, latitude, out, diagnostic))
      readF64(out)
    }

  /**
   * Converts a geographic coordinate to a screen point.
   *
   * See `mln_map_projection_pixel_for_lat_lng` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun pixelForLatLng(coordinate: LatLng): ScreenPoint =
    nativeCall(this, binding) {
      val out = allocate(16, 8)
      check(
        C.mln_map_projection_pixel_for_lat_lng(handle, writeLatLng(coordinate), out, diagnostic),
      )
      readScreenPoint(out)
    }

  /**
   * Converts geographic coordinates to screen points.
   *
   * See `mln_map_projection_pixels_for_lat_lngs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun pixelsForLatLngs(coordinates: List<LatLng>): List<ScreenPoint> =
    nativeCall(this, binding) {
      val out = allocate(coordinates.size.toLong() * 16, 8)
      check(
        C.mln_map_projection_pixels_for_lat_lngs(
          handle,
          array(coordinates, 16, 8) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          out,
          diagnostic,
        ),
      )
      readArray(out, coordinates.size.toULong(), 16.toLong()) { item -> readScreenPoint(item) }
    }

  /**
   * Releases a standalone projection.
   *
   * See `mln_map_projection_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun release(): Unit =
    nativeClose(this, binding) { check(C.mln_map_projection_release(handle, diagnostic)) }

  /**
   * Applies a camera update to a standalone projection.
   *
   * See `mln_map_projection_set_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun setCamera(camera: CameraOptions): Unit =
    nativeCall(this, binding) {
      check(C.mln_map_projection_set_camera(handle, writeCameraOptions(camera), diagnostic))
    }
}
