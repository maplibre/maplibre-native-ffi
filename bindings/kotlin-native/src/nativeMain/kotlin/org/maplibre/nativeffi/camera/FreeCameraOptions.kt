package org.maplibre.nativeffi.camera

import org.maplibre.nativeffi.geo.Quaternion
import org.maplibre.nativeffi.geo.Vec3

/** Mutable free-camera descriptor. */
public class FreeCameraOptions {
  public var position: Vec3? = null
    private set

  public var orientation: Quaternion? = null
    private set

  public fun hasPosition(): Boolean = position != null

  public fun position(position: Vec3): FreeCameraOptions = apply { this.position = position }

  public fun clearPosition(): FreeCameraOptions = apply { position = null }

  public fun hasOrientation(): Boolean = orientation != null

  public fun orientation(orientation: Quaternion): FreeCameraOptions = apply {
    this.orientation = orientation
  }

  public fun clearOrientation(): FreeCameraOptions = apply { orientation = null }
}
