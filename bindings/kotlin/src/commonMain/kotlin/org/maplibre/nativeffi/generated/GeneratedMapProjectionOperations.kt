// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

public abstract class GeneratedMapProjectionOperations internal constructor() {
  internal abstract val binding: HandleStateCore

  /**
   * Closes a standalone projection.
   *
   * See `mln_map_projection_close` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun close(): Unit =
    nativeClose(this, binding, "mln_map_projection_close") {
      check(C.mln_map_projection_close(handle, diagnostic))
    }

  /**
   * Copies the projection camera into out_camera.
   *
   * See `mln_map_projection_get_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun getCamera(): CameraOptions =
    nativeCall(this, binding, "mln_map_projection_get_camera") {
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
    nativeCall(this, binding, "mln_map_projection_lat_lng_for_pixel") {
      val out = allocate(16, 8)
      check(
        C.mln_map_projection_lat_lng_for_pixel(handle, writeScreenPoint(point), out, diagnostic)
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
    nativeCall(this, binding, "mln_map_projection_lat_lng_for_pixel_unwrapped") {
      val out = allocate(16, 8)
      check(
        C.mln_map_projection_lat_lng_for_pixel_unwrapped(
          handle,
          writeScreenPoint(point),
          out,
          diagnostic,
        )
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
    nativeCall(this, binding, "mln_map_projection_meters_per_pixel_at_latitude") {
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
    nativeCall(this, binding, "mln_map_projection_pixel_for_lat_lng") {
      val out = allocate(16, 8)
      check(
        C.mln_map_projection_pixel_for_lat_lng(handle, writeLatLng(coordinate), out, diagnostic)
      )
      readScreenPoint(out)
    }

  /**
   * Applies a camera update to a standalone projection.
   *
   * See `mln_map_projection_set_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun setCamera(camera: CameraOptions): Unit =
    nativeCall(this, binding, "mln_map_projection_set_camera") {
      check(C.mln_map_projection_set_camera(handle, writeCameraOptions(camera), diagnostic))
    }

  /**
   * Applies a camera fit for geographic coordinates.
   *
   * See `mln_map_projection_set_visible_coordinates` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun setVisibleCoordinates(coordinates: List<LatLng>, padding: EdgeInsets): Unit =
    nativeCall(this, binding, "mln_map_projection_set_visible_coordinates") {
      check(
        C.mln_map_projection_set_visible_coordinates(
          handle,
          array(coordinates, 16, 8) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          writeEdgeInsets(padding),
          diagnostic,
        )
      )
    }

  /**
   * Applies a camera fit for GeoJSON Geometry bytes.
   *
   * See `mln_map_projection_set_visible_geometry` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun setVisibleGeometry(geometry: ByteArray, padding: EdgeInsets): Unit =
    nativeCall(this, binding, "mln_map_projection_set_visible_geometry") {
      check(
        C.mln_map_projection_set_visible_geometry(
          handle,
          view(geometry),
          writeEdgeInsets(padding),
          diagnostic,
        )
      )
    }
}
