package org.maplibre.nativeffi.internal.javacpp

import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.status.Status

/** Copies the private camera_bridge.h array into Kotlin values. */
internal inline fun readCameraSnapshot(read: (DoubleArray) -> Int): CameraOptions {
  val out = DoubleArray(15)
  Status.check(read(out))
  val fields = out[0].toInt()
  return CameraOptions().apply {
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_CENTER) != 0) {
      center = LatLng(out[1], out[2])
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_CENTER_ALTITUDE) != 0) {
      centerAltitude = out[3]
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_PADDING) != 0) {
      padding = EdgeInsets(out[4], out[5], out[6], out[7])
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_ANCHOR) != 0) {
      anchor = ScreenPoint(out[8], out[9])
    }
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_ZOOM) != 0) zoom = out[10]
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_BEARING) != 0) bearing = out[11]
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_PITCH) != 0) pitch = out[12]
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_ROLL) != 0) roll = out[13]
    if ((fields and MaplibreNativeC.MLN_CAMERA_OPTION_FOV) != 0) fieldOfView = out[14]
  }
}
