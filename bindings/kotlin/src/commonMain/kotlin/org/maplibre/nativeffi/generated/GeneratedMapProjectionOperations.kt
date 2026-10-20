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
