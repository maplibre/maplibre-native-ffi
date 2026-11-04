// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * A camera view for converting between screen points and coordinates.
 *
 * See `mln_map_projection` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class MapProjectionHandle
internal constructor(
  handle: Long,
) : AutoCloseable {
  internal val binding =
    HandleState("MapProjectionHandle", handle, GeneratedOwnerDisposal::mapProjection)
  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)
  /** The native handle value. It stays readable after release. */
  public val id: ULong
    get() = binding.handle.toULong()

  /** Whether a release, dispose, or discard has ended the handle. */
  public val isClosed: Boolean
    get() = binding.isReleased

  /** Calls [release], for the standard library's `use`. */
  public override fun close() {
    release()
  }

  /**
   * Reads the projection's camera.
   *
   * See `mln_map_projection_get_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun getCamera(): MapProjectionCameraResult =
    nativeCall(this, binding) {
      val out = sized(w(104, 104, 100), w(8, 8, 4))
      check(C.mln_map_projection_get_camera(handle, out, diagnostic))
      readMapProjectionCameraResult(out)
    }

  /**
   * Finds the coordinate at a screen point.
   *
   * The longitude stays between -180 and 180.
   *
   * See `mln_map_projection_lat_lng_for_pixel` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun latLngForPixel(point: ScreenPoint): LatLng =
    nativeCall(this, binding) {
      val out = allocate(16, w(8, 8, 4))
      check(
        C.mln_map_projection_lat_lng_for_pixel(handle, writeScreenPoint(point), out, diagnostic),
      )
      readLatLng(out)
    }

  /**
   * Finds the coordinate at a screen point, without wrapping its longitude.
   *
   * The longitude can fall outside -180 to 180, to show which copy of the world the point is in.
   *
   * See `mln_map_projection_lat_lng_for_pixel_unwrapped` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun latLngForPixelUnwrapped(point: ScreenPoint): LatLng =
    nativeCall(this, binding) {
      val out = allocate(16, w(8, 8, 4))
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
   * Finds the coordinates at several screen points, as `MapProjectionHandle.latLngForPixel()` does.
   *
   * See `mln_map_projection_lat_lngs_for_pixels` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun latLngsForPixels(points: List<ScreenPoint>): List<LatLng> =
    nativeCall(this, binding) {
      val out = allocate(points.size.toLong() * 16, w(8, 8, 4))
      check(
        C.mln_map_projection_lat_lngs_for_pixels(
          handle,
          array(points, 16, w(8, 8, 4)) { at, item -> putScreenPoint(at, item) },
          points.size.toLong(),
          out,
          diagnostic,
        ),
      )
      readArray(out, points.size.toULong(), 16.toLong()) { item -> readLatLng(item) }
    }

  /**
   * Finds the coordinates at several screen points, as
   * `MapProjectionHandle.latLngForPixelUnwrapped()` does.
   *
   * See `mln_map_projection_lat_lngs_for_pixels_unwrapped` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun latLngsForPixelsUnwrapped(points: List<ScreenPoint>): List<LatLng> =
    nativeCall(this, binding) {
      val out = allocate(points.size.toLong() * 16, w(8, 8, 4))
      check(
        C.mln_map_projection_lat_lngs_for_pixels_unwrapped(
          handle,
          array(points, 16, w(8, 8, 4)) { at, item -> putScreenPoint(at, item) },
          points.size.toLong(),
          out,
          diagnostic,
        ),
      )
      readArray(out, points.size.toULong(), 16.toLong()) { item -> readLatLng(item) }
    }

  /**
   * Computes how many meters one screen pixel covers at `latitude`, at the projection's zoom.
   *
   * The result ignores the camera's pitch.
   *
   * See `mln_map_projection_meters_per_pixel_at_latitude` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun metersPerPixelAtLatitude(latitude: Double): Double =
    nativeCall(this, binding) {
      val out = allocate(8, w(8, 8, 4))
      check(C.mln_map_projection_meters_per_pixel_at_latitude(handle, latitude, out, diagnostic))
      readF64(out)
    }

  /**
   * Finds the screen point of a coordinate.
   *
   * The point is in the copy of the world nearest the camera's center.
   *
   * See `mln_map_projection_pixel_for_lat_lng` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun pixelForLatLng(coordinate: LatLng): ScreenPoint =
    nativeCall(this, binding) {
      val out = allocate(16, w(8, 8, 4))
      check(
        C.mln_map_projection_pixel_for_lat_lng(handle, writeLatLng(coordinate), out, diagnostic),
      )
      readScreenPoint(out)
    }

  /**
   * Finds the screen points of several coordinates, as `MapProjectionHandle.pixelForLatLng()` does.
   *
   * See `mln_map_projection_pixels_for_lat_lngs` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun pixelsForLatLngs(coordinates: List<LatLng>): List<ScreenPoint> =
    nativeCall(this, binding) {
      val out = allocate(coordinates.size.toLong() * 16, w(8, 8, 4))
      check(
        C.mln_map_projection_pixels_for_lat_lngs(
          handle,
          array(coordinates, 16, w(8, 8, 4)) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          out,
          diagnostic,
        ),
      )
      readArray(out, coordinates.size.toULong(), 16.toLong()) { item -> readScreenPoint(item) }
    }

  /**
   * Releases a projection.
   *
   * See `mln_map_projection_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun release(): Unit = nativeClose(this, binding) { C.mln_map_projection_release(handle) }

  /**
   * Moves the projection's camera, without changing the map.
   *
   * Camera fields that `camera` doesn't set keep their values.
   *
   * See `mln_map_projection_set_camera` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun setCamera(camera: CameraOptions): Unit =
    nativeCall(this, binding) {
      check(C.mln_map_projection_set_camera(handle, writeCameraOptions(camera), diagnostic))
    }

  override fun toString(): String =
    "MapProjectionHandle(id=0x${binding.handle.toString(16)}, closed=$isClosed)"

  public companion object {
    /**
     * Creates a projection that converts coordinates for one view of a map.
     *
     * Create it from the `view` of an `MapSnapshot` or an `RenderFrameResult`. The projection
     * doesn't follow the map, so create a new one after the map's camera or size changes.
     *
     * See `mln_map_projection_create` in the
     * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
     */
    public fun create(view: MapView): MapProjectionHandle =
      nativeCall(null, null) {
        val out = allocate(8, w(8, 8, 4))
        check(C.mln_map_projection_create(writeMapView(view), out, diagnostic))
        adopt(out, GeneratedOwnerDisposal::mapProjection) { MapProjectionHandle(it) }
      }
  }
}
