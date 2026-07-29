package org.maplibre.nativeffi.camera

import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint

/**
 * Mutable camera descriptor used for camera snapshots and commands.
 *
 * Compares and hashes by field value; [copy] returns an independent instance. Keep an instance
 * unmodified while it is a key in a hash-based collection.
 */
public class CameraOptions {
  public var center: LatLng? = null

  public var centerAltitude: Double? = null

  public var padding: EdgeInsets? = null

  /**
   * The screen point that stays fixed while a camera command applies.
   *
   * This field is input-only. MapLibre Native honours it on camera commands and never reports it in
   * a camera snapshot, so a value read back from [org.maplibre.nativeffi.map.MapHandle.camera] is
   * always null and code branching on it there never runs.
   */
  public var anchor: ScreenPoint? = null

  public var zoom: Double? = null

  public var bearing: Double? = null

  public var pitch: Double? = null

  public var roll: Double? = null

  public var fieldOfView: Double? = null

  /** Returns an independent copy of this descriptor with [block] applied to the copy. */
  public fun copy(block: CameraOptions.() -> Unit = {}): CameraOptions =
    CameraOptions()
      .also {
        it.center = center
        it.centerAltitude = centerAltitude
        it.padding = padding
        it.anchor = anchor
        it.zoom = zoom
        it.bearing = bearing
        it.pitch = pitch
        it.roll = roll
        it.fieldOfView = fieldOfView
      }
      .apply(block)

  private val fields: List<Any?>
    get() = listOf(center, centerAltitude, padding, anchor, zoom, bearing, pitch, roll, fieldOfView)

  override fun equals(other: Any?): Boolean = other is CameraOptions && fields == other.fields

  override fun hashCode(): Int = fields.hashCode()
}
