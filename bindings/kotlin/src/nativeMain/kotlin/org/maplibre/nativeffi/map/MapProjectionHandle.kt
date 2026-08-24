package org.maplibre.nativeffi.map

import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import kotlinx.cinterop.ptr
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
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
import org.maplibre.nativeffi.internal.lifecycle.NativeMapProjection
import org.maplibre.nativeffi.internal.lifecycle.rawHandleValue
import org.maplibre.nativeffi.internal.memory.toCSize
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.struct.ByteStructs
import org.maplibre.nativeffi.internal.struct.CoreStructs
import org.maplibre.nativeffi.internal.struct.MapStructs

/** Any-thread standalone projection snapshot created from a map. */
@OptIn(ExperimentalForeignApi::class)
public actual class MapProjectionHandle internal constructor(handle: NativeMapProjection) :
  AutoCloseable {
  private val state = HandleState("MapProjectionHandle", handle)

  public actual val camera: CameraOptions
    get() = memScoped {
      val outCamera = mln_camera_options_default().getPointer(this)
      state.withLive { handle ->
        Status.check(mln_map_projection_get_camera(handle.rawHandleValue, outCamera))
      }
      MapStructs.cameraOptions(outCamera.pointed)
    }

  public actual fun setCamera(camera: CameraOptions) {
    memScoped {
      state.withLive { handle ->
        Status.check(
          mln_map_projection_set_camera(
            handle.rawHandleValue,
            MapStructs.cameraOptions(camera, this),
          )
        )
      }
    }
  }

  public actual fun setVisibleCoordinates(coordinates: List<LatLng>, padding: EdgeInsets) {
    val coordinateSnapshot = coordinates.toList()
    memScoped {
      state.withLive { handle ->
        Status.check(
          mln_map_projection_set_visible_coordinates(
            handle.rawHandleValue,
            CoreStructs.latLngArray(coordinateSnapshot, this),
            coordinateSnapshot.size.toCSize(),
            CoreStructs.edgeInsets(padding),
          )
        )
      }
    }
  }

  public actual fun setVisibleGeometry(geometry: ByteArray, padding: EdgeInsets) {
    memScoped {
      state.withLive { handle ->
        Status.check(
          mln_map_projection_set_visible_geometry(
            handle.rawHandleValue,
            ByteStructs.bufferView(geometry, this),
            CoreStructs.edgeInsets(padding),
          )
        )
      }
    }
  }

  public actual fun pixelForLatLng(coordinate: LatLng): ScreenPoint = memScoped {
    val outPoint = alloc<mln_screen_point>()
    state.withLive { handle ->
      Status.check(
        mln_map_projection_pixel_for_lat_lng(
          handle.rawHandleValue,
          CoreStructs.latLng(coordinate),
          outPoint.ptr,
        )
      )
    }
    CoreStructs.screenPoint(outPoint)
  }

  public actual fun latLngForPixel(point: ScreenPoint): LatLng = memScoped {
    val outCoordinate = alloc<mln_lat_lng>()
    state.withLive { handle ->
      Status.check(
        mln_map_projection_lat_lng_for_pixel(
          handle.rawHandleValue,
          CoreStructs.screenPoint(point),
          outCoordinate.ptr,
        )
      )
    }
    CoreStructs.latLng(outCoordinate)
  }

  public actual override fun close() {
    state.closeOnce { handle -> mln_map_projection_destroy(handle.rawHandleValue) }
  }

  public actual val isClosed: Boolean
    get() = state.isReleased()

  internal fun nativeHandle(): NativeMapProjection = state.requireLive()

  internal fun nativeHandleId(): Long = state.handleId()
}
