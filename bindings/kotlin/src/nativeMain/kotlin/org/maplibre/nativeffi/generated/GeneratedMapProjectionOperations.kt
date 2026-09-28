// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import kotlinx.cinterop.*
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.c.*
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.status.Status as BindingStatus

@OptIn(ExperimentalForeignApi::class)
public actual abstract class GeneratedMapProjectionOperations internal actual constructor() {
  internal abstract fun bindingMapProjectionHandle(): ULong

  internal abstract fun bindingCloseMapProjection(call: (ULong) -> Int)

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
        memScoped {
          val arena = this
          mln_map_projection_close(owner)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun getCamera(): CameraOptions {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapProjectionHandle().toLong(),
        "mln_map_projection_get_camera",
      )
      return memScoped {
        val arena = this
        val output = arena.alloc<mln_camera_options>()
        output.size = sizeOf<mln_camera_options>().toUInt()
        BindingStatus.check(mln_map_projection_get_camera(bindingMapProjectionHandle(), output.ptr))
        GeneratedValues.readCameraOptions(output)
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun latLngForPixel(point: ScreenPoint): LatLng {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapProjectionHandle().toLong(),
        "mln_map_projection_lat_lng_for_pixel",
      )
      return memScoped {
        val arena = this
        val output = arena.alloc<mln_lat_lng>()
        BindingStatus.check(
          mln_map_projection_lat_lng_for_pixel(
            bindingMapProjectionHandle(),
            GeneratedValues.writeScreenPoint(arena, point).pointed.readValue(),
            output.ptr,
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
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapProjectionHandle().toLong(),
        "mln_map_projection_lat_lng_for_pixel_unwrapped",
      )
      return memScoped {
        val arena = this
        val output = arena.alloc<mln_lat_lng>()
        BindingStatus.check(
          mln_map_projection_lat_lng_for_pixel_unwrapped(
            bindingMapProjectionHandle(),
            GeneratedValues.writeScreenPoint(arena, point).pointed.readValue(),
            output.ptr,
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
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapProjectionHandle().toLong(),
        "mln_map_projection_meters_per_pixel_at_latitude",
      )
      return memScoped {
        val arena = this
        val output = arena.alloc<DoubleVar>()
        BindingStatus.check(
          mln_map_projection_meters_per_pixel_at_latitude(
            bindingMapProjectionHandle(),
            latitude,
            output.ptr,
          )
        )
        output.value
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun pixelForLatLng(coordinate: LatLng): ScreenPoint {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapProjectionHandle().toLong(),
        "mln_map_projection_pixel_for_lat_lng",
      )
      return memScoped {
        val arena = this
        val output = arena.alloc<mln_screen_point>()
        BindingStatus.check(
          mln_map_projection_pixel_for_lat_lng(
            bindingMapProjectionHandle(),
            GeneratedValues.writeLatLng(arena, coordinate).pointed.readValue(),
            output.ptr,
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
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapProjectionHandle().toLong(),
        "mln_map_projection_set_camera",
      )
      return memScoped {
        val arena = this
        BindingStatus.check(
          mln_map_projection_set_camera(
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
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapProjectionHandle().toLong(),
        "mln_map_projection_set_visible_coordinates",
      )
      return memScoped {
        val arena = this
        BindingStatus.check(
          mln_map_projection_set_visible_coordinates(
            bindingMapProjectionHandle(),
            GeneratedValues.writeLatLngArray(arena, coordinates),
            coordinates.size.convert(),
            GeneratedValues.writeEdgeInsets(arena, padding).pointed.readValue(),
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
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingMapProjectionHandle().toLong(),
        "mln_map_projection_set_visible_geometry",
      )
      return memScoped {
        val arena = this
        BindingStatus.check(
          mln_map_projection_set_visible_geometry(
            bindingMapProjectionHandle(),
            GeneratedValues.byteView(arena, geometry).pointed.readValue(),
            GeneratedValues.writeEdgeInsets(arena, padding).pointed.readValue(),
          )
        )
        Unit
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }
}
