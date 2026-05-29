package org.maplibre.nativeffi.map

/** Mutable descriptor used when creating a [MapHandle]. */
public class MapOptions {
  public var width: Int? = null
    private set

  public var height: Int? = null
    private set

  public var scaleFactor: Double? = null
    private set

  public var mapMode: MapMode? = null
    private set

  public fun size(width: Int, height: Int): MapOptions {
    require(width >= 0) { "width must be non-negative" }
    require(height >= 0) { "height must be non-negative" }
    this.width = width
    this.height = height
    return this
  }

  public fun scaleFactor(scaleFactor: Double): MapOptions = apply { this.scaleFactor = scaleFactor }

  public fun clearScaleFactor(): MapOptions = apply { scaleFactor = null }

  public fun mapMode(mapMode: MapMode): MapOptions = apply { this.mapMode = mapMode }

  public fun clearMapMode(): MapOptions = apply { mapMode = null }
}
