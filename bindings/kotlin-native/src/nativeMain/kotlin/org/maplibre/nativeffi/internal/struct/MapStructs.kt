package org.maplibre.nativeffi.internal.struct

import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.MemScope
import kotlinx.cinterop.alloc
import kotlinx.cinterop.ptr
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.internal.c.MLN_CAMERA_OPTION_ANCHOR
import org.maplibre.nativeffi.internal.c.MLN_CAMERA_OPTION_BEARING
import org.maplibre.nativeffi.internal.c.MLN_CAMERA_OPTION_CENTER
import org.maplibre.nativeffi.internal.c.MLN_CAMERA_OPTION_CENTER_ALTITUDE
import org.maplibre.nativeffi.internal.c.MLN_CAMERA_OPTION_FOV
import org.maplibre.nativeffi.internal.c.MLN_CAMERA_OPTION_PADDING
import org.maplibre.nativeffi.internal.c.MLN_CAMERA_OPTION_PITCH
import org.maplibre.nativeffi.internal.c.MLN_CAMERA_OPTION_ROLL
import org.maplibre.nativeffi.internal.c.MLN_CAMERA_OPTION_ZOOM
import org.maplibre.nativeffi.internal.c.mln_camera_options
import org.maplibre.nativeffi.internal.c.mln_camera_options_default

/** Materializes map and camera descriptors at the C boundary. */
@OptIn(ExperimentalForeignApi::class)
internal object MapStructs {
  fun cameraOptions(value: CameraOptions, scope: MemScope): CPointer<mln_camera_options> {
    val native = scope.alloc<mln_camera_options>()
    mln_camera_options_default().place(native.ptr)
    value.center?.let {
      native.fields = native.fields or MLN_CAMERA_OPTION_CENTER
      native.latitude = it.latitude
      native.longitude = it.longitude
    }
    value.centerAltitude?.let {
      native.fields = native.fields or MLN_CAMERA_OPTION_CENTER_ALTITUDE
      native.center_altitude = it
    }
    value.padding?.let {
      native.fields = native.fields or MLN_CAMERA_OPTION_PADDING
      native.padding.top = it.top
      native.padding.left = it.left
      native.padding.bottom = it.bottom
      native.padding.right = it.right
    }
    value.anchor?.let {
      native.fields = native.fields or MLN_CAMERA_OPTION_ANCHOR
      native.anchor.x = it.x
      native.anchor.y = it.y
    }
    value.zoom?.let {
      native.fields = native.fields or MLN_CAMERA_OPTION_ZOOM
      native.zoom = it
    }
    value.bearing?.let {
      native.fields = native.fields or MLN_CAMERA_OPTION_BEARING
      native.bearing = it
    }
    value.pitch?.let {
      native.fields = native.fields or MLN_CAMERA_OPTION_PITCH
      native.pitch = it
    }
    value.roll?.let {
      native.fields = native.fields or MLN_CAMERA_OPTION_ROLL
      native.roll = it
    }
    value.fieldOfView?.let {
      native.fields = native.fields or MLN_CAMERA_OPTION_FOV
      native.field_of_view = it
    }
    return native.ptr
  }

  fun cameraOptions(value: mln_camera_options): CameraOptions {
    val camera = CameraOptions()
    if ((value.fields and MLN_CAMERA_OPTION_CENTER) != 0U) {
      camera.center(value.latitude, value.longitude)
    }
    if ((value.fields and MLN_CAMERA_OPTION_CENTER_ALTITUDE) != 0U) {
      camera.centerAltitude(value.center_altitude)
    }
    if ((value.fields and MLN_CAMERA_OPTION_PADDING) != 0U) {
      camera.padding(CoreStructs.edgeInsets(value.padding))
    }
    if ((value.fields and MLN_CAMERA_OPTION_ANCHOR) != 0U) {
      camera.anchor(CoreStructs.screenPoint(value.anchor))
    }
    if ((value.fields and MLN_CAMERA_OPTION_ZOOM) != 0U) {
      camera.zoom(value.zoom)
    }
    if ((value.fields and MLN_CAMERA_OPTION_BEARING) != 0U) {
      camera.bearing(value.bearing)
    }
    if ((value.fields and MLN_CAMERA_OPTION_PITCH) != 0U) {
      camera.pitch(value.pitch)
    }
    if ((value.fields and MLN_CAMERA_OPTION_ROLL) != 0U) {
      camera.roll(value.roll)
    }
    if ((value.fields and MLN_CAMERA_OPTION_FOV) != 0U) {
      camera.fieldOfView(value.field_of_view)
    }
    return camera
  }
}
