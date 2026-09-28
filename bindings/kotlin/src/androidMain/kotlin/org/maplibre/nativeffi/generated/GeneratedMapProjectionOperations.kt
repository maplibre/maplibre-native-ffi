// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.bytedeco.javacpp.*
import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.status.Status as BindingStatus

public actual abstract class GeneratedMapProjectionOperations internal actual constructor() {
  internal abstract fun bindingMapProjectionHandle(): Long

  internal abstract fun bindingCloseMapProjection(call: (Long) -> Int)

  public actual fun close(): Unit {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.checkOperation(
        "mln_map_projection_close"
      )
      return bindingCloseMapProjection { owner ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          owner.toLong(),
          "mln_map_projection_close",
        )
        PointerScope().use { arena -> MaplibreNativeC.mln_map_projection_close(owner) }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun getCamera(): CameraOptions {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapProjectionHandle().toLong(),
        "mln_map_projection_get_camera",
      )
      return PointerScope().use { arena ->
        val output = MaplibreNativeC.mln_camera_options()
        output.size(output.sizeof())
        BindingStatus.check(
          MaplibreNativeC.mln_map_projection_get_camera(bindingMapProjectionHandle(), output)
        )
        GeneratedValues.readCameraOptions(output)
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun latLngForPixel(point: ScreenPoint): LatLng {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapProjectionHandle().toLong(),
        "mln_map_projection_lat_lng_for_pixel",
      )
      return PointerScope().use { arena ->
        val output = MaplibreNativeC.mln_lat_lng()
        BindingStatus.check(
          MaplibreNativeC.mln_map_projection_lat_lng_for_pixel(
            bindingMapProjectionHandle(),
            GeneratedValues.writeScreenPoint(arena, point),
            output,
          )
        )
        GeneratedValues.readLatLng(output)
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun latLngForPixelUnwrapped(point: ScreenPoint): LatLng {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapProjectionHandle().toLong(),
        "mln_map_projection_lat_lng_for_pixel_unwrapped",
      )
      return PointerScope().use { arena ->
        val output = MaplibreNativeC.mln_lat_lng()
        BindingStatus.check(
          MaplibreNativeC.mln_map_projection_lat_lng_for_pixel_unwrapped(
            bindingMapProjectionHandle(),
            GeneratedValues.writeScreenPoint(arena, point),
            output,
          )
        )
        GeneratedValues.readLatLng(output)
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun metersPerPixelAtLatitude(latitude: Double): Double {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapProjectionHandle().toLong(),
        "mln_map_projection_meters_per_pixel_at_latitude",
      )
      return PointerScope().use { arena ->
        val output = DoublePointer(1L)
        BindingStatus.check(
          MaplibreNativeC.mln_map_projection_meters_per_pixel_at_latitude(
            bindingMapProjectionHandle(),
            latitude,
            output,
          )
        )
        output.get()
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun pixelForLatLng(coordinate: LatLng): ScreenPoint {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapProjectionHandle().toLong(),
        "mln_map_projection_pixel_for_lat_lng",
      )
      return PointerScope().use { arena ->
        val output = MaplibreNativeC.mln_screen_point()
        BindingStatus.check(
          MaplibreNativeC.mln_map_projection_pixel_for_lat_lng(
            bindingMapProjectionHandle(),
            GeneratedValues.writeLatLng(arena, coordinate),
            output,
          )
        )
        GeneratedValues.readScreenPoint(output)
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun setCamera(camera: CameraOptions): Unit {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapProjectionHandle().toLong(),
        "mln_map_projection_set_camera",
      )
      return PointerScope().use { arena ->
        BindingStatus.check(
          MaplibreNativeC.mln_map_projection_set_camera(
            bindingMapProjectionHandle(),
            GeneratedValues.writeCameraOptions(arena, camera),
          )
        )
        Unit
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun setVisibleCoordinates(coordinates: List<LatLng>, padding: EdgeInsets): Unit {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapProjectionHandle().toLong(),
        "mln_map_projection_set_visible_coordinates",
      )
      return PointerScope().use { arena ->
        BindingStatus.check(
          MaplibreNativeC.mln_map_projection_set_visible_coordinates(
            bindingMapProjectionHandle(),
            GeneratedValues.writeLatLngArray(arena, coordinates),
            coordinates.size.toLong(),
            GeneratedValues.writeEdgeInsets(arena, padding),
          )
        )
        Unit
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun setVisibleGeometry(geometry: ByteArray, padding: EdgeInsets): Unit {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapProjectionHandle().toLong(),
        "mln_map_projection_set_visible_geometry",
      )
      return PointerScope().use { arena ->
        BindingStatus.check(
          MaplibreNativeC.mln_map_projection_set_visible_geometry(
            bindingMapProjectionHandle(),
            GeneratedValues.byteView(arena, geometry),
            GeneratedValues.writeEdgeInsets(arena, padding),
          )
        )
        Unit
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }
}
