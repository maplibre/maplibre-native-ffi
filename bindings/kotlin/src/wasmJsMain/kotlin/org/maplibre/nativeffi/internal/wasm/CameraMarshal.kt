package org.maplibre.nativeffi.internal.wasm

import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.wasm.generated.MlnCameraOptionField
import org.maplibre.nativeffi.internal.wasm.generated.MlnCameraOptions
import org.maplibre.nativeffi.internal.wasm.generated.MlnEdgeInsets
import org.maplibre.nativeffi.internal.wasm.generated.MlnLatLng
import org.maplibre.nativeffi.internal.wasm.generated.MlnScreenPoint

/**
 * Places a [CameraOptions] into the Emscripten heap, and reads one back.
 *
 * The C descriptor pairs its values with a bit per field, so an absent Kotlin value is a bit left
 * clear rather than a sentinel written into the value. Reading works the same way: a bit that is
 * clear produces null rather than whatever the field happened to hold.
 *
 * Every offset and width here comes from the generated accessors, so this code names fields.
 */
internal object CameraMarshal {
  /** Bytes one camera descriptor occupies, including its nested padding and anchor. */
  val SIZEOF: Int = MlnCameraOptions.SIZEOF

  /**
   * Writes the descriptor header alone, for a buffer native fills.
   *
   * An output descriptor still states its size: native reads it to decide which fields it may
   * write, and a zeroed block would ask for a zero-sized camera.
   */
  fun writeHeader(base: HeapPointer) {
    MlnCameraOptions.setSize(base, MlnCameraOptions.SIZEOF)
  }

  /** Writes [camera] at [base], setting a field's bit only where the value is present. */
  fun write(base: HeapPointer, camera: CameraOptions) {
    // The leading size field is how the C API versions a descriptor: it carries the size this
    // binding was generated against so native can tell which fields it may read.
    MlnCameraOptions.setSize(base, MlnCameraOptions.SIZEOF)
    var fields = 0
    camera.center?.let {
      fields = fields or MlnCameraOptionField.MLN_CAMERA_OPTION_CENTER
      MlnCameraOptions.setLatitude(base, it.latitude)
      MlnCameraOptions.setLongitude(base, it.longitude)
    }
    camera.centerAltitude?.let {
      fields = fields or MlnCameraOptionField.MLN_CAMERA_OPTION_CENTER_ALTITUDE
      MlnCameraOptions.setCenterAltitude(base, it)
    }
    camera.padding?.let {
      fields = fields or MlnCameraOptionField.MLN_CAMERA_OPTION_PADDING
      writeEdgeInsets(base + MlnCameraOptions.OFFSET_PADDING, it)
    }
    camera.anchor?.let {
      fields = fields or MlnCameraOptionField.MLN_CAMERA_OPTION_ANCHOR
      val anchor = base + MlnCameraOptions.OFFSET_ANCHOR
      MlnScreenPoint.setX(anchor, it.x)
      MlnScreenPoint.setY(anchor, it.y)
    }
    camera.zoom?.let {
      fields = fields or MlnCameraOptionField.MLN_CAMERA_OPTION_ZOOM
      MlnCameraOptions.setZoom(base, it)
    }
    camera.bearing?.let {
      fields = fields or MlnCameraOptionField.MLN_CAMERA_OPTION_BEARING
      MlnCameraOptions.setBearing(base, it)
    }
    camera.pitch?.let {
      fields = fields or MlnCameraOptionField.MLN_CAMERA_OPTION_PITCH
      MlnCameraOptions.setPitch(base, it)
    }
    camera.roll?.let {
      fields = fields or MlnCameraOptionField.MLN_CAMERA_OPTION_ROLL
      MlnCameraOptions.setRoll(base, it)
    }
    camera.fieldOfView?.let {
      fields = fields or MlnCameraOptionField.MLN_CAMERA_OPTION_FOV
      MlnCameraOptions.setFieldOfView(base, it)
    }
    MlnCameraOptions.setFields(base, fields)
  }

  /** Reads the camera at [base], producing null for every field whose bit is clear. */
  fun read(base: HeapPointer): CameraOptions {
    val fields = MlnCameraOptions.fields(base)
    fun has(bit: Int) = (fields and bit) != 0
    return CameraOptions().also {
      if (has(MlnCameraOptionField.MLN_CAMERA_OPTION_CENTER)) {
        it.center = LatLng(MlnCameraOptions.latitude(base), MlnCameraOptions.longitude(base))
      }
      if (has(MlnCameraOptionField.MLN_CAMERA_OPTION_CENTER_ALTITUDE)) {
        it.centerAltitude = MlnCameraOptions.centerAltitude(base)
      }
      if (has(MlnCameraOptionField.MLN_CAMERA_OPTION_PADDING)) {
        it.padding = readEdgeInsets(base + MlnCameraOptions.OFFSET_PADDING)
      }
      if (has(MlnCameraOptionField.MLN_CAMERA_OPTION_ANCHOR)) {
        val anchor = base + MlnCameraOptions.OFFSET_ANCHOR
        it.anchor = ScreenPoint(MlnScreenPoint.x(anchor), MlnScreenPoint.y(anchor))
      }
      if (has(MlnCameraOptionField.MLN_CAMERA_OPTION_ZOOM)) it.zoom = MlnCameraOptions.zoom(base)
      if (has(MlnCameraOptionField.MLN_CAMERA_OPTION_BEARING)) {
        it.bearing = MlnCameraOptions.bearing(base)
      }
      if (has(MlnCameraOptionField.MLN_CAMERA_OPTION_PITCH)) it.pitch = MlnCameraOptions.pitch(base)
      if (has(MlnCameraOptionField.MLN_CAMERA_OPTION_ROLL)) it.roll = MlnCameraOptions.roll(base)
      if (has(MlnCameraOptionField.MLN_CAMERA_OPTION_FOV)) {
        it.fieldOfView = MlnCameraOptions.fieldOfView(base)
      }
    }
  }

  /** Writes an edge-inset descriptor, which carries no field mask of its own. */
  fun writeEdgeInsets(base: HeapPointer, insets: EdgeInsets) {
    MlnEdgeInsets.setTop(base, insets.top)
    MlnEdgeInsets.setLeft(base, insets.left)
    MlnEdgeInsets.setBottom(base, insets.bottom)
    MlnEdgeInsets.setRight(base, insets.right)
  }

  fun readEdgeInsets(base: HeapPointer): EdgeInsets =
    EdgeInsets(
      MlnEdgeInsets.top(base),
      MlnEdgeInsets.left(base),
      MlnEdgeInsets.bottom(base),
      MlnEdgeInsets.right(base),
    )

  fun writeLatLng(base: HeapPointer, coordinate: LatLng) {
    MlnLatLng.setLatitude(base, coordinate.latitude)
    MlnLatLng.setLongitude(base, coordinate.longitude)
  }

  fun readLatLng(base: HeapPointer): LatLng =
    LatLng(MlnLatLng.latitude(base), MlnLatLng.longitude(base))
}
