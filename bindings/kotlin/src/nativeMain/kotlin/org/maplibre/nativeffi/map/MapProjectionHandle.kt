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
import org.maplibre.nativeffi.internal.c.mln_map_projection_close
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
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.struct.ByteStructs
import org.maplibre.nativeffi.internal.struct.CoreStructs
import org.maplibre.nativeffi.internal.struct.MapStructs

/**
 * Owned standalone projection snapshot created from a map.
 *
 * Every call is synchronous, runs on the calling thread, is internally serialized, and may be made
 * from any thread. A projection copies the map's transform state at creation and never observes map
 * changes made after that and remains usable after its source map and runtime close.
 */
@OptIn(ExperimentalForeignApi::class)
public actual class MapProjectionHandle internal constructor(handle: NativeMapProjection) {
  private val state = HandleState("MapProjectionHandle", handle)

  public actual fun camera(): CameraOptions = memScoped {
    val outCamera = mln_camera_options_default().getPointer(this)
    Status.check(mln_map_projection_get_camera(state.requireLive().rawHandleValue, outCamera))
    MapStructs.cameraOptions(outCamera.pointed)
  }

  public actual fun setCamera(camera: CameraOptions): Unit = memScoped {
    Status.check(
      mln_map_projection_set_camera(
        state.requireLive().rawHandleValue,
        MapStructs.cameraOptions(camera, this),
      )
    )
  }

  public actual fun setVisibleCoordinates(coordinates: List<LatLng>, padding: EdgeInsets) {
    val coordinateSnapshot = coordinates.toList()
    memScoped {
      state.withLive { handle ->
        Status.check(
          mln_map_projection_set_visible_coordinates(
            handle.rawHandleValue,
            CoreStructs.latLngArray(coordinateSnapshot, this),
            coordinateSnapshot.size.toULong(),
            CoreStructs.edgeInsets(padding),
          )
        )
      }
    }
  }

  public actual fun setVisibleGeometry(geometry: ByteArray, padding: EdgeInsets): Unit = memScoped {
    Status.check(
      mln_map_projection_set_visible_geometry(
        state.requireLive().rawHandleValue,
        ByteStructs.bufferView(geometry, this),
        CoreStructs.edgeInsets(padding),
      )
    )
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

  public actual fun close() {
    if (!state.beginClose()) return
    try {
      Status.check(mln_map_projection_close(state.handleForClose().rawHandleValue))
    } catch (error: Throwable) {
      state.abortClose()
      throw error
    }
    state.completeClose()
  }

  public actual val isClosed: Boolean
    get() = state.isReleased()
}
