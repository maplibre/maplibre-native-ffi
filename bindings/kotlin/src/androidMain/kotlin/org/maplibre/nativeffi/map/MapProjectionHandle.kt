package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.javacpp.AndroidNativeBridge
import org.maplibre.nativeffi.internal.javacpp.ByteArrayViewScope
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.javacpp.readCameraSnapshot
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.status.Status

/** Any-thread Android JNI standalone projection snapshot. */
public actual class MapProjectionHandle internal constructor(private val handleId: Long) :
  AutoCloseable {
  private val core = HandleStateCore("MapProjectionHandle", handleId)

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  public actual val camera: CameraOptions
    get() {
      NativeAccess.ensureLoaded()
      return withLiveHandle { handle ->
        readCameraSnapshot { outCamera ->
          AndroidNativeBridge.projectionGetCamera(handle, outCamera)
        }
      }
    }

  public actual fun setCamera(camera: CameraOptions) {
    NativeAccess.ensureLoaded()
    ProjectionCameraOptionsScope(camera).use { nativeCamera ->
      withLiveHandle { handle ->
        Status.check(MaplibreNativeC.mln_map_projection_set_camera(handle, nativeCamera.options))
      }
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
          withLiveHandle { handle ->
            Status.check(
              MaplibreNativeC.mln_map_projection_set_visible_coordinates(
                handle,
                nativeCoordinates.coordinates,
                nativeCoordinates.count,
                nativePadding,
              )
            )
          }
        }
    }
  }

  public actual fun setVisibleGeometry(geometry: ByteArray, padding: EdgeInsets) {
    NativeAccess.ensureLoaded()
    ByteArrayViewScope(geometry).use { nativeGeometry ->
      MaplibreNativeC.mln_edge_insets()
        .top(padding.top)
        .left(padding.left)
        .bottom(padding.bottom)
        .right(padding.right)
        .use { nativePadding ->
          withLiveHandle { handle ->
            Status.check(
              MaplibreNativeC.mln_map_projection_set_visible_geometry(
                handle,
                nativeGeometry.view,
                nativePadding,
              )
            )
          }
        }
    }
  }

  public actual fun pixelForLatLng(coordinate: LatLng): ScreenPoint {
    NativeAccess.ensureLoaded()
    val out = DoubleArray(2)
    withLiveHandle { handle ->
      Status.check(
        AndroidNativeBridge.projectionPixelForLatLng(
          handle,
          coordinate.latitude,
          coordinate.longitude,
          out,
        )
      )
    }
    return ScreenPoint(out[0], out[1])
  }

  public actual fun latLngForPixel(point: ScreenPoint): LatLng {
    NativeAccess.ensureLoaded()
    val out = DoubleArray(2)
    withLiveHandle { handle ->
      Status.check(
        AndroidNativeBridge.projectionLatLngForPixel(handle, point.x, point.y, false, out)
      )
    }
    return LatLng(out[0], out[1])
  }

  public actual fun latLngForPixelUnwrapped(point: ScreenPoint): LatLng {
    NativeAccess.ensureLoaded()
    val out = DoubleArray(2)
    withLiveHandle { handle ->
      Status.check(
        AndroidNativeBridge.projectionLatLngForPixel(handle, point.x, point.y, true, out)
      )
    }
    return LatLng(out[0], out[1])
  }

  public actual fun metersPerPixelAtLatitude(latitude: Double): Double {
    NativeAccess.ensureLoaded()
    val outMetersPerPixel = doubleArrayOf(0.0)
    withLiveHandle { handle ->
      Status.check(
        MaplibreNativeC.mln_map_projection_meters_per_pixel_at_latitude(
          handle,
          latitude,
          outMetersPerPixel,
        )
      )
    }
    return outMetersPerPixel[0]
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual override fun close() {
    core.closeOnce(destroy = { MaplibreNativeC.mln_map_projection_destroy(handleId) })
  }

  private fun <T> withLiveHandle(block: (Long) -> T): T = core.withLive { block(handleId) }
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
