package org.maplibre.nativeffi.camera

import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint

/** Mutable camera descriptor used for camera snapshots and commands. */
public class CameraOptions {
  public var center: LatLng? = null

  public var centerAltitude: Double? = null

  public var padding: EdgeInsets? = null

  public var anchor: ScreenPoint? = null

  public var zoom: Double? = null

  public var bearing: Double? = null

  public var pitch: Double? = null

  public var roll: Double? = null

  public var fieldOfView: Double? = null
}
