package org.maplibre.nativeffi.camera

import org.maplibre.nativeffi.geo.Quaternion
import org.maplibre.nativeffi.geo.Vec3

/** Mutable free-camera descriptor. */
public class FreeCameraOptions {
  public var position: Vec3? = null

  public var orientation: Quaternion? = null
}
