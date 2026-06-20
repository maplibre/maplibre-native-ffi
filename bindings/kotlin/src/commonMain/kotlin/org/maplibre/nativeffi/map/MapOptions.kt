package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.internal.status.Status

/** Mutable descriptor used when creating a [MapHandle]. */
public class MapOptions {
  public var width: Int? = null
    set(value) {
      value?.let { Status.requireArgument(it >= 0) { "width must be non-negative" } }
      field = value
    }

  public var height: Int? = null
    set(value) {
      value?.let { Status.requireArgument(it >= 0) { "height must be non-negative" } }
      field = value
    }

  public var scaleFactor: Double? = null

  public var mapMode: MapMode? = null
}
