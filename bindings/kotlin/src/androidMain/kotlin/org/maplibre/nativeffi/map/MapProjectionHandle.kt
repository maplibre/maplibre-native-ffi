package org.maplibre.nativeffi.map

import org.bytedeco.javacpp.LongPointer
import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.javacpp.ByteArrayViewScope
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.startOperation

/** Owned Android JNI standalone projection snapshot. */
public actual class MapProjectionHandle
internal constructor(private val runtime: RuntimeHandle, private val handleId: Long) {
  private val core = HandleStateCore("MapProjectionHandle", handleId)

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  public actual suspend fun camera(): CameraOptions {
    NativeAccess.ensureLoaded()
    val operation = startOperation { outOperation ->
      MaplibreNativeC.mln_map_projection_get_camera_start(requireLiveHandle(), outOperation)
    }
    try {
      runtime.awaitOperation(operation)
      MaplibreNativeC.mln_camera_options_default().use { outCamera ->
        Status.check(
          MaplibreNativeC.mln_map_projection_get_camera_take_result(operation, outCamera)
        )
        return projectionCameraOptions(outCamera)
      }
    } finally {
      MaplibreNativeC.mln_operation_release(operation)
    }
  }

  public actual fun setCamera(camera: CameraOptions): Long {
    NativeAccess.ensureLoaded()
    ProjectionCameraOptionsScope(camera).use { nativeCamera ->
      LongPointer(1).use { outCommandId ->
        outCommandId.put(0, 0L)
        Status.check(
          MaplibreNativeC.mln_map_projection_set_camera(
            requireLiveHandle(),
            nativeCamera.options,
            outCommandId,
          )
        )
        return outCommandId.get()
      }
    }
  }

  public actual fun setVisibleCoordinates(coordinates: List<LatLng>, padding: EdgeInsets): Long {
    NativeAccess.ensureLoaded()
    ProjectionLatLngArrayScope(coordinates).use { nativeCoordinates ->
      MaplibreNativeC.mln_edge_insets()
        .top(padding.top)
        .left(padding.left)
        .bottom(padding.bottom)
        .right(padding.right)
        .use { nativePadding ->
          LongPointer(1).use { outCommandId ->
            outCommandId.put(0, 0L)
            Status.check(
              MaplibreNativeC.mln_map_projection_set_visible_coordinates(
                requireLiveHandle(),
                nativeCoordinates.coordinates,
                nativeCoordinates.count,
                nativePadding,
                outCommandId,
              )
            )
            return outCommandId.get()
          }
        }
    }
  }

  public actual fun setVisibleGeometry(geometry: ByteArray, padding: EdgeInsets): Long {
    NativeAccess.ensureLoaded()
    ByteArrayViewScope(geometry).use { nativeGeometry ->
      MaplibreNativeC.mln_edge_insets()
        .top(padding.top)
        .left(padding.left)
        .bottom(padding.bottom)
        .right(padding.right)
        .use { nativePadding ->
          LongPointer(1).use { outCommandId ->
            outCommandId.put(0, 0L)
            Status.check(
              MaplibreNativeC.mln_map_projection_set_visible_geometry(
                requireLiveHandle(),
                nativeGeometry.view,
                nativePadding,
                outCommandId,
              )
            )
            return outCommandId.get()
          }
        }
    }
  }

  public actual suspend fun pixelForLatLng(coordinate: LatLng): ScreenPoint {
    NativeAccess.ensureLoaded()
    val operation = startOperation { outOperation ->
      MaplibreNativeC.mln_map_projection_pixel_for_lat_lng_start(
        requireLiveHandle(),
        MaplibreNativeC.mln_lat_lng().latitude(coordinate.latitude).longitude(coordinate.longitude),
        outOperation,
      )
    }
    try {
      runtime.awaitOperation(operation)
      MaplibreNativeC.mln_screen_point().use { outPoint ->
        Status.check(
          MaplibreNativeC.mln_map_projection_pixel_for_lat_lng_take_result(operation, outPoint)
        )
        return ScreenPoint(outPoint.x(), outPoint.y())
      }
    } finally {
      MaplibreNativeC.mln_operation_release(operation)
    }
  }

  public actual suspend fun latLngForPixel(point: ScreenPoint): LatLng {
    NativeAccess.ensureLoaded()
    val operation = startOperation { outOperation ->
      MaplibreNativeC.mln_map_projection_lat_lng_for_pixel_start(
        requireLiveHandle(),
        MaplibreNativeC.mln_screen_point().x(point.x).y(point.y),
        outOperation,
      )
    }
    try {
      runtime.awaitOperation(operation)
      MaplibreNativeC.mln_lat_lng().use { outCoordinate ->
        Status.check(
          MaplibreNativeC.mln_map_projection_lat_lng_for_pixel_take_result(operation, outCoordinate)
        )
        return LatLng(outCoordinate.latitude(), outCoordinate.longitude())
      }
    } finally {
      MaplibreNativeC.mln_operation_release(operation)
    }
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual suspend fun close() {
    if (!core.beginClose()) return
    val operation =
      try {
        startOperation { outOperation ->
          MaplibreNativeC.mln_map_projection_close_start(handleId, outOperation)
        }
      } catch (error: Throwable) {
        core.abortClose()
        throw error
      }
    try {
      runtime.awaitOperation(operation)
    } catch (error: Throwable) {
      core.abortClose()
      throw error
    } finally {
      MaplibreNativeC.mln_operation_release(operation)
    }
    core.completeClose()
  }

  private fun requireLiveHandle(): Long {
    core.requireLive()
    return handleId
  }
}

private fun projectionCameraOptions(value: MaplibreNativeC.mln_camera_options): CameraOptions {
  val fields = value.fields()
  return CameraOptions().apply {
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_CENTER) != 0) {
      center = LatLng(value.latitude(), value.longitude())
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_CENTER_ALTITUDE) != 0) {
      centerAltitude = value.center_altitude()
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_PADDING) != 0) {
      val padding = value.padding()
      this.padding = EdgeInsets(padding.top(), padding.left(), padding.bottom(), padding.right())
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_ANCHOR) != 0) {
      anchor = ScreenPoint(value.anchor().x(), value.anchor().y())
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_ZOOM) != 0) {
      zoom = value.zoom()
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_BEARING) != 0) {
      bearing = value.bearing()
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_PITCH) != 0) {
      pitch = value.pitch()
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_ROLL) != 0) {
      roll = value.roll()
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_FOV) != 0) {
      fieldOfView = value.field_of_view()
    }
  }
}

private class ProjectionCameraOptionsScope(value: CameraOptions) : AutoCloseable {
  val options: MaplibreNativeC.mln_camera_options = MaplibreNativeC.mln_camera_options_default()

  init {
    var fields = 0
    value.center?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_CENTER
      options.latitude(it.latitude).longitude(it.longitude)
    }
    value.centerAltitude?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_CENTER_ALTITUDE
      options.center_altitude(it)
    }
    value.padding?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_PADDING
      options.padding(
        MaplibreNativeC.mln_edge_insets()
          .top(it.top)
          .left(it.left)
          .bottom(it.bottom)
          .right(it.right)
      )
    }
    value.anchor?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_ANCHOR
      options.anchor(MaplibreNativeC.mln_screen_point().x(it.x).y(it.y))
    }
    value.zoom?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_ZOOM
      options.zoom(it)
    }
    value.bearing?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_BEARING
      options.bearing(it)
    }
    value.pitch?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_PITCH
      options.pitch(it)
    }
    value.roll?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_ROLL
      options.roll(it)
    }
    value.fieldOfView?.let {
      fields = fields or MaplibreNativeC.MLN_CAMERA_OPTION_FOV
      options.field_of_view(it)
    }
    options.fields(fields)
  }

  override fun close() {
    options.close()
  }
}

private class ProjectionLatLngArrayScope(values: List<LatLng>) : AutoCloseable {
  private val coordinateSnapshot = values.toList()
  val coordinates: MaplibreNativeC.mln_lat_lng =
    MaplibreNativeC.mln_lat_lng(coordinateSnapshot.size.toLong())
  val count: Long = coordinateSnapshot.size.toLong()

  init {
    coordinateSnapshot.forEachIndexed { index, coordinate ->
      coordinates
        .position(index.toLong())
        .latitude(coordinate.latitude)
        .longitude(coordinate.longitude)
    }
    coordinates.position(0)
  }

  override fun close() {
    coordinates.close()
  }
}
