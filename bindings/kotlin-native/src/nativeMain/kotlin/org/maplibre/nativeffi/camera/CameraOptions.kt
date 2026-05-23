package org.maplibre.nativeffi.camera

import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint

/** Mutable camera descriptor used for camera snapshots and commands. */
public class CameraOptions {
  public var center: LatLng? = null
    private set

  public var centerAltitude: Double? = null
    private set

  public var padding: EdgeInsets? = null
    private set

  public var anchor: ScreenPoint? = null
    private set

  public var zoom: Double? = null
    private set

  public var bearing: Double? = null
    private set

  public var pitch: Double? = null
    private set

  public var roll: Double? = null
    private set

  public var fieldOfView: Double? = null
    private set

  public fun hasCenter(): Boolean = center != null

  public fun center(center: LatLng): CameraOptions = apply { this.center = center }

  public fun center(latitude: Double, longitude: Double): CameraOptions =
    center(LatLng(latitude, longitude))

  public fun clearCenter(): CameraOptions = apply { center = null }

  public fun hasCenterAltitude(): Boolean = centerAltitude != null

  public fun centerAltitude(centerAltitude: Double): CameraOptions = apply {
    this.centerAltitude = centerAltitude
  }

  public fun clearCenterAltitude(): CameraOptions = apply { centerAltitude = null }

  public fun hasPadding(): Boolean = padding != null

  public fun padding(padding: EdgeInsets): CameraOptions = apply { this.padding = padding }

  public fun clearPadding(): CameraOptions = apply { padding = null }

  public fun hasAnchor(): Boolean = anchor != null

  public fun anchor(anchor: ScreenPoint): CameraOptions = apply { this.anchor = anchor }

  public fun clearAnchor(): CameraOptions = apply { anchor = null }

  public fun hasZoom(): Boolean = zoom != null

  public fun zoom(zoom: Double): CameraOptions = apply { this.zoom = zoom }

  public fun clearZoom(): CameraOptions = apply { zoom = null }

  public fun hasBearing(): Boolean = bearing != null

  public fun bearing(bearing: Double): CameraOptions = apply { this.bearing = bearing }

  public fun clearBearing(): CameraOptions = apply { bearing = null }

  public fun hasPitch(): Boolean = pitch != null

  public fun pitch(pitch: Double): CameraOptions = apply { this.pitch = pitch }

  public fun clearPitch(): CameraOptions = apply { pitch = null }

  public fun hasRoll(): Boolean = roll != null

  public fun roll(roll: Double): CameraOptions = apply { this.roll = roll }

  public fun clearRoll(): CameraOptions = apply { roll = null }

  public fun hasFieldOfView(): Boolean = fieldOfView != null

  public fun fieldOfView(fieldOfView: Double): CameraOptions = apply {
    this.fieldOfView = fieldOfView
  }

  public fun clearFieldOfView(): CameraOptions = apply { fieldOfView = null }
}
