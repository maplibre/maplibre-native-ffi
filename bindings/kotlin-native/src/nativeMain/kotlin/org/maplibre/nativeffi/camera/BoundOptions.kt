package org.maplibre.nativeffi.camera

import org.maplibre.nativeffi.geo.LatLngBounds

/** Mutable map bounds descriptor. */
public class BoundOptions {
  public var bounds: LatLngBounds? = null

  public var minZoom: Double? = null

  public var maxZoom: Double? = null

  public var minPitch: Double? = null

  public var maxPitch: Double? = null
}
