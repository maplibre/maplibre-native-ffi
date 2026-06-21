package org.maplibre.nativeffi.map

import org.bytedeco.javacpp.Pointer
import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.status.Status

/** Owned Android JNI standalone projection snapshot. */
public actual class MapProjectionHandle internal constructor(private val handleAddress: Long) :
  AutoCloseable {
  private val core = HandleStateCore("MapProjectionHandle", handleAddress)

  public actual val camera: CameraOptions
    get() {
      NativeAccess.ensureLoaded()
      MaplibreNativeC.mln_camera_options_default().use { outCamera ->
        Status.check(
          MaplibreNativeC.mln_map_projection_get_camera(projection(requireLiveAddress()), outCamera)
        )
        return projectionCameraOptions(outCamera)
      }
    }

  public actual fun setCamera(camera: CameraOptions) {
    NativeAccess.ensureLoaded()
    ProjectionCameraOptionsScope(camera).use { nativeCamera ->
      Status.check(
        MaplibreNativeC.mln_map_projection_set_camera(
          projection(requireLiveAddress()),
          nativeCamera.options,
        )
      )
    }
  }

  public actual fun setVisibleCoordinates(coordinates: List<LatLng>, padding: EdgeInsets) {
    NativeAccess.ensureLoaded()
    ProjectionLatLngArrayScope(coordinates).use { nativeCoordinates ->
      MaplibreNativeC.mln_edge_insets()
        .top(padding.top)
        .left(padding.left)
        .bottom(padding.bottom)
        .right(padding.right)
        .use { nativePadding ->
          Status.check(
            MaplibreNativeC.mln_map_projection_set_visible_coordinates(
              projection(requireLiveAddress()),
              nativeCoordinates.coordinates,
              nativeCoordinates.count,
              nativePadding,
            )
          )
        }
    }
  }

  public actual fun setVisibleGeometry(geometry: Geometry, padding: EdgeInsets) {
    NativeAccess.ensureLoaded()
    GeometryScope(geometry).use { nativeGeometry ->
      MaplibreNativeC.mln_edge_insets()
        .top(padding.top)
        .left(padding.left)
        .bottom(padding.bottom)
        .right(padding.right)
        .use { nativePadding ->
          Status.check(
            MaplibreNativeC.mln_map_projection_set_visible_geometry(
              projection(requireLiveAddress()),
              nativeGeometry.value,
              nativePadding,
            )
          )
        }
    }
  }

  public actual fun pixelForLatLng(coordinate: LatLng): ScreenPoint {
    NativeAccess.ensureLoaded()
    MaplibreNativeC.mln_screen_point().use { outPoint ->
      Status.check(
        MaplibreNativeC.mln_map_projection_pixel_for_lat_lng(
          projection(requireLiveAddress()),
          MaplibreNativeC.mln_lat_lng()
            .latitude(coordinate.latitude)
            .longitude(coordinate.longitude),
          outPoint,
        )
      )
      return ScreenPoint(outPoint.x(), outPoint.y())
    }
  }

  public actual fun latLngForPixel(point: ScreenPoint): LatLng {
    NativeAccess.ensureLoaded()
    MaplibreNativeC.mln_lat_lng().use { outCoordinate ->
      Status.check(
        MaplibreNativeC.mln_map_projection_lat_lng_for_pixel(
          projection(requireLiveAddress()),
          MaplibreNativeC.mln_screen_point().x(point.x).y(point.y),
          outCoordinate,
        )
      )
      return LatLng(outCoordinate.latitude(), outCoordinate.longitude())
    }
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual override fun close() {
    core.closeOnce(
      destroy = { MaplibreNativeC.mln_map_projection_destroy(projection(handleAddress)) }
    )
  }

  private fun requireLiveAddress(): Long {
    core.requireLive()
    return handleAddress
  }
}

private fun projection(address: Long): MaplibreNativeC.mln_map_projection =
  MaplibreNativeC.mln_map_projection(ProjectionAddressPointer(address))

private class ProjectionAddressPointer(address: Long) : Pointer(null as Pointer?) {
  init {
    this.address = address
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
