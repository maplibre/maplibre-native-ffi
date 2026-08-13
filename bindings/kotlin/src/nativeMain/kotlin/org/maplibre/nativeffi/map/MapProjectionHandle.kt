package org.maplibre.nativeffi.map

import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.ULongVar
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import kotlinx.cinterop.ptr
import kotlinx.cinterop.value
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.c.mln_camera_options_default
import org.maplibre.nativeffi.internal.c.mln_lat_lng
import org.maplibre.nativeffi.internal.c.mln_map_projection_close_start
import org.maplibre.nativeffi.internal.c.mln_map_projection_get_camera_start
import org.maplibre.nativeffi.internal.c.mln_map_projection_get_camera_take_result
import org.maplibre.nativeffi.internal.c.mln_map_projection_lat_lng_for_pixel_start
import org.maplibre.nativeffi.internal.c.mln_map_projection_lat_lng_for_pixel_take_result
import org.maplibre.nativeffi.internal.c.mln_map_projection_pixel_for_lat_lng_start
import org.maplibre.nativeffi.internal.c.mln_map_projection_pixel_for_lat_lng_take_result
import org.maplibre.nativeffi.internal.c.mln_map_projection_set_camera
import org.maplibre.nativeffi.internal.c.mln_map_projection_set_visible_coordinates
import org.maplibre.nativeffi.internal.c.mln_map_projection_set_visible_geometry
import org.maplibre.nativeffi.internal.c.mln_operation_release
import org.maplibre.nativeffi.internal.c.mln_screen_point
import org.maplibre.nativeffi.internal.lifecycle.HandleState
import org.maplibre.nativeffi.internal.lifecycle.NativeMapProjection
import org.maplibre.nativeffi.internal.lifecycle.rawHandleValue
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.struct.ByteStructs
import org.maplibre.nativeffi.internal.struct.CoreStructs
import org.maplibre.nativeffi.internal.struct.MapStructs
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.startOperation

/** Owned standalone projection snapshot created from a map. */
@OptIn(ExperimentalForeignApi::class)
public actual class MapProjectionHandle
internal constructor(private val runtime: RuntimeHandle, handle: NativeMapProjection) {
  private val state = HandleState("MapProjectionHandle", handle)

  public actual suspend fun camera(): CameraOptions {
    val operation = startOperation { outOperation ->
      mln_map_projection_get_camera_start(state.requireLive().rawHandleValue, outOperation)
    }
    try {
      runtime.awaitOperation(operation)
      return memScoped {
        val outCamera = mln_camera_options_default().getPointer(this)
        Status.check(mln_map_projection_get_camera_take_result(operation, outCamera))
        MapStructs.cameraOptions(outCamera.pointed)
      }
    } finally {
      mln_operation_release(operation)
    }
  }

  public actual fun setCamera(camera: CameraOptions): Long = memScoped {
    val commandId = alloc<ULongVar>()
    commandId.value = 0uL
    Status.check(
      mln_map_projection_set_camera(
        state.requireLive().rawHandleValue,
        MapStructs.cameraOptions(camera, this),
        commandId.ptr,
      )
    )
    commandId.value.toLong()
  }

  public actual fun setVisibleCoordinates(coordinates: List<LatLng>, padding: EdgeInsets): Long {
    val coordinateSnapshot = coordinates.toList()
    return memScoped {
      val commandId = alloc<ULongVar>()
      commandId.value = 0uL
      Status.check(
        mln_map_projection_set_visible_coordinates(
          state.requireLive().rawHandleValue,
          CoreStructs.latLngArray(coordinateSnapshot, this),
          coordinateSnapshot.size.toULong(),
          CoreStructs.edgeInsets(padding),
          commandId.ptr,
        )
      )
      commandId.value.toLong()
    }
  }

  public actual fun setVisibleGeometry(geometry: ByteArray, padding: EdgeInsets): Long = memScoped {
    val commandId = alloc<ULongVar>()
    commandId.value = 0uL
    Status.check(
      mln_map_projection_set_visible_geometry(
        state.requireLive().rawHandleValue,
        ByteStructs.bufferView(geometry, this),
        CoreStructs.edgeInsets(padding),
        commandId.ptr,
      )
    )
    commandId.value.toLong()
  }

  public actual suspend fun pixelForLatLng(coordinate: LatLng): ScreenPoint {
    val operation = startOperation { outOperation ->
      mln_map_projection_pixel_for_lat_lng_start(
        state.requireLive().rawHandleValue,
        CoreStructs.latLng(coordinate),
        outOperation,
      )
    }
    try {
      runtime.awaitOperation(operation)
      return memScoped {
        val outPoint = alloc<mln_screen_point>()
        Status.check(mln_map_projection_pixel_for_lat_lng_take_result(operation, outPoint.ptr))
        CoreStructs.screenPoint(outPoint)
      }
    } finally {
      mln_operation_release(operation)
    }
  }

  public actual suspend fun latLngForPixel(point: ScreenPoint): LatLng {
    val operation = startOperation { outOperation ->
      mln_map_projection_lat_lng_for_pixel_start(
        state.requireLive().rawHandleValue,
        CoreStructs.screenPoint(point),
        outOperation,
      )
    }
    try {
      runtime.awaitOperation(operation)
      return memScoped {
        val outCoordinate = alloc<mln_lat_lng>()
        Status.check(mln_map_projection_lat_lng_for_pixel_take_result(operation, outCoordinate.ptr))
        CoreStructs.latLng(outCoordinate)
      }
    } finally {
      mln_operation_release(operation)
    }
  }

  public actual suspend fun close() {
    if (!state.beginClose()) return
    val operation =
      try {
        startOperation { outOperation ->
          mln_map_projection_close_start(state.handleForClose().rawHandleValue, outOperation)
        }
      } catch (error: Throwable) {
        state.abortClose()
        throw error
      }
    try {
      runtime.awaitOperation(operation)
    } catch (error: Throwable) {
      state.abortClose()
      throw error
    } finally {
      mln_operation_release(operation)
    }
    state.completeClose()
  }

  public actual val isClosed: Boolean
    get() = state.isReleased()

  internal fun nativeHandle(): NativeMapProjection = state.requireLive()

  internal fun nativeHandleId(): Long = state.handleId()
}
