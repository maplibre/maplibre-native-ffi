package org.maplibre.nativeffi.map

/** Mutable descriptor used when creating a [MapHandle]. */
public class MapOptions {
  public var width: Int? = null
    set(value) {
      value?.let { require(it >= 0) { "width must be non-negative" } }
      field = value
    }

  public var height: Int? = null
    set(value) {
      value?.let { require(it >= 0) { "height must be non-negative" } }
      field = value
    }

  public var scaleFactor: Double? = null

  public var mapMode: MapMode? = null
}
