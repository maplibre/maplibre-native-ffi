// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

public abstract class GeneratedMapProjectionOperations internal constructor() {
  internal abstract val binding: HandleStateCore

  public fun close(): Unit =
    nativeClose(this, binding, "mln_map_projection_close") {
      check(C.mln_map_projection_close(handle, diagnostic))
    }

  public fun getCamera(): CameraOptions =
    nativeCall(this, binding, "mln_map_projection_get_camera") {
      val out = allocate(120, 8).also { writeU32(it, 120.toUInt()) }
      check(C.mln_map_projection_get_camera(handle, out, diagnostic))
      readCameraOptions(out)
    }

  public fun latLngForPixel(point: ScreenPoint): LatLng =
    nativeCall(this, binding, "mln_map_projection_lat_lng_for_pixel") {
      val out = allocate(16, 8)
      check(
        C.mln_map_projection_lat_lng_for_pixel(handle, writeScreenPoint(point), out, diagnostic)
      )
      readLatLng(out)
    }

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

  public fun metersPerPixelAtLatitude(latitude: Double): Double =
    nativeCall(this, binding, "mln_map_projection_meters_per_pixel_at_latitude") {
      val out = allocate(8)
      check(C.mln_map_projection_meters_per_pixel_at_latitude(handle, latitude, out, diagnostic))
      readF64(out)
    }

  public fun pixelForLatLng(coordinate: LatLng): ScreenPoint =
    nativeCall(this, binding, "mln_map_projection_pixel_for_lat_lng") {
      val out = allocate(16, 8)
      check(
        C.mln_map_projection_pixel_for_lat_lng(handle, writeLatLng(coordinate), out, diagnostic)
      )
      readScreenPoint(out)
    }

  public fun setCamera(camera: CameraOptions): Unit =
    nativeCall(this, binding, "mln_map_projection_set_camera") {
      check(C.mln_map_projection_set_camera(handle, writeCameraOptions(camera), diagnostic))
    }

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
