package org.maplibre.nativeffi.map

import cnames.structs.mln_map_projection
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import kotlinx.cinterop.ptr
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.c.mln_camera_options_default
import org.maplibre.nativeffi.internal.c.mln_lat_lng
import org.maplibre.nativeffi.internal.c.mln_map_projection_destroy
import org.maplibre.nativeffi.internal.c.mln_map_projection_get_camera
import org.maplibre.nativeffi.internal.c.mln_map_projection_lat_lng_for_pixel
import org.maplibre.nativeffi.internal.c.mln_map_projection_pixel_for_lat_lng
import org.maplibre.nativeffi.internal.c.mln_map_projection_set_camera
import org.maplibre.nativeffi.internal.c.mln_map_projection_set_visible_coordinates
import org.maplibre.nativeffi.internal.c.mln_map_projection_set_visible_geometry
import org.maplibre.nativeffi.internal.c.mln_screen_point
import org.maplibre.nativeffi.internal.lifecycle.HandleState
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.struct.CoreStructs
import org.maplibre.nativeffi.internal.struct.MapStructs
import org.maplibre.nativeffi.internal.struct.ValueStructs

/** Owned standalone projection snapshot created from a map. */
@OptIn(ExperimentalForeignApi::class)
public actual class MapProjectionHandle internal constructor(handle: CPointer<mln_map_projection>) :
  AutoCloseable {
  private val state = HandleState("MapProjectionHandle", handle)

  public actual val camera: CameraOptions
    get() = memScoped {
      val outCamera = mln_camera_options_default().getPointer(this)
      Status.check(mln_map_projection_get_camera(state.requireLive(), outCamera))
      MapStructs.cameraOptions(outCamera.pointed)
    }

  public actual fun setCamera(camera: CameraOptions) {
    memScoped {
      Status.check(
        mln_map_projection_set_camera(state.requireLive(), MapStructs.cameraOptions(camera, this))
      )
    }
  }

  public actual fun setVisibleCoordinates(coordinates: List<LatLng>, padding: EdgeInsets) {
    val coordinateSnapshot = coordinates.toList()
    memScoped {
      Status.check(
        mln_map_projection_set_visible_coordinates(
          state.requireLive(),
          CoreStructs.latLngArray(coordinateSnapshot, this),
          coordinateSnapshot.size.toULong(),
          CoreStructs.edgeInsets(padding),
        )
      )
    }
  }

  public actual fun setVisibleGeometry(geometry: Geometry, padding: EdgeInsets) {
    memScoped {
      Status.check(
        mln_map_projection_set_visible_geometry(
          state.requireLive(),
          ValueStructs.geometry(geometry, this),
          CoreStructs.edgeInsets(padding),
        )
      )
    }
  }

  public actual fun pixelForLatLng(coordinate: LatLng): ScreenPoint = memScoped {
    val outPoint = alloc<mln_screen_point>()
    Status.check(
      mln_map_projection_pixel_for_lat_lng(
        state.requireLive(),
        CoreStructs.latLng(coordinate),
        outPoint.ptr,
      )
    )
    CoreStructs.screenPoint(outPoint)
  }

  public actual fun latLngForPixel(point: ScreenPoint): LatLng = memScoped {
    val outCoordinate = alloc<mln_lat_lng>()
    Status.check(
      mln_map_projection_lat_lng_for_pixel(
        state.requireLive(),
        CoreStructs.screenPoint(point),
        outCoordinate.ptr,
      )
    )
    CoreStructs.latLng(outCoordinate)
  }

  public actual override fun close() {
    state.closeOnce(::mln_map_projection_destroy)
  }

  public actual val isClosed: Boolean
    get() = state.isReleased()

  internal fun nativeHandle(): CPointer<mln_map_projection> = state.requireLive()

  internal fun nativeAddress(): Long = state.address()
}
